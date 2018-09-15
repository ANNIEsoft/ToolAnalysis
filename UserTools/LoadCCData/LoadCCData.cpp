/* vim:set noexpandtab tabstop=4 wrap */

#include "LoadCCData.h"
#include "PMTData.h"
#include "MRDTree.h"
#include <limits>
//#include <unordered_map>
//#include <algorithm>

LoadCCData::LoadCCData():Tool(){}

// The MRDTimeStamp is a timestamp of when the TDC readout for that trigger
// ended, which is pretty inaccurate. We need to match it with a more accurate version
// from the ADC data.
// This Tool performs both loading of TDC data, and timestamp alignment.
// See Execute for more information.

namespace {
	constexpr uint64_t MRD_NS_PER_SAMPLE=4;
	constexpr uint64_t ms_to_ns = 1000000;         // needed for TDC to convert timestamps to ns
}

bool LoadCCData::Initialise(std::string configfile, DataModel &data){
	
	/////////////////// Usefull header ///////////////////////
	if(configfile!="")  m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	// XXX Cannot run Log before we've retrieved m_data!! XXX 
	m_variables.Get("verbose",verbosity);
	Log("LoadCCData Tool: Initializing Tool LoadCCData",v_message,verbosity);
	
	std::string filelist, alignmentfiles;
	m_variables.Get("FileList",filelist);
	m_variables.Get("AlignmentFileList",alignmentfiles);
	m_variables.Get("UseHeftyTimes",useHeftyTimes);
	
	std::string filetype;
	m_variables.Get("FileType",filetype); // should be "raw" or "postprocessed"
	/////////////////////////////////////////////////////////////////
	
	// Make the ANNIEEvent Store if it doesn't exist
	// =============================================
	int annieeventexists = m_data->Stores.count("ANNIEEvent");
	logmessage="LoadCCData Tool: ";
	logmessage += (annieeventexists) ? "Loading ANNIEEvent Store" : "Creating ANNIEEvent Store";
	Log(logmessage,v_debug,verbosity);
	if(annieeventexists==0) m_data->Stores["ANNIEEvent"] = new BoostStore(false,2);
	
	// ANNIEEvent TDC hit storage
	// ==========================
	TDCData = new std::map<ChannelKey, std::vector<std::vector<Hit>>>;
	
//	// fill the map with some empty containers - why? Need to remove TDCData->clear() if using this
//	for(std::pair<uint16_t,std::string>& apmtit : slotchantopmtid){  // iterate over the map of PMTs
//		std::string stringPMTID = apmtit.second;
//		uint32_t tubeid = stoi(stringPMTID);
//		ChannelKey key(subdetector::TDC,tubeid);
//		TDCData.emplace_back(key,std::vector<std::vector<Hit>> avec{});
//	}
	
	// Prepare the CC Data files for Reading
	// =====================================
	// 1. Load files from the file list
	std::string line;
	std::vector<std::string> filevector;
	ifstream myfile (filelist.c_str());
	if (myfile.is_open()){
		while ( getline (myfile,line) ){
			filevector.push_back(line);
		}
		myfile.close();
	} else {
		Log("LoadCCData Tool: Error: Bad CCData filename!",v_error,verbosity);
		return false;
	}
	
	// 2 : Construct the TChain
	// tree name is "CCData" in RAW files (e.g. DataR900S4p0.root),
	// but "MRDData" in post-processed files (e.g. V6DataR889S0p0T5Sep_25_1650.root).
	// In either case the tree formats are the same, so the Tool should work on both.
	// Use the first file in the list to determine the file type:
	TFile* tempfile = TFile::Open(filevector.front().c_str());
	bool hasccdata = tempfile->GetListOfKeys()->Contains("CCData");
	bool hasmrddata = tempfile->GetListOfKeys()->Contains("MRDData");
	tempfile->Close();
	
	// construct the appropriate TChain
	     if(hasccdata)  { MRDChain = new TChain("CCData");  }
	else if(hasmrddata) { MRDChain = new TChain("MRDData"); }
	else {
		Log("LoadCCData Tool: ERROR: First TDC file contains neither CCData nor MRDData Tree!",v_error,verbosity);
		return false;
	}
	// add the files to the TChain
	for(std::string afile : filevector){
		Log("LoadCCData Tool: Loading file "+line,v_message,verbosity);
		int filesadded = MRDChain->Add(afile.c_str());
		Log(to_string(filesadded)+" files added to Chain",v_debug,verbosity);
	}
	
	// 3. Construct a Reader class from the TChain
	Log("LoadCCData Tool: MRDChain has "+to_string(MRDChain->GetEntries())+" entries",v_debug,verbosity);
	TTree* testMRD=MRDChain->CloneTree();
	if(testMRD){
		MRDData=new MRDTree(MRDChain->CloneTree());
		MRDData->fChain->SetName("MRDData");
		//m_data->AddTTree("MRDData",MRDData->fChain);
	} else {
		Log("LoadCCData Tool: MRDChain CloneTree failure!",v_error,verbosity);
		return false;
	}
	NumCCDataEntries = MRDChain->GetEntries();
	TDCChainEntry=0;
	
	// Prepare the ADC Data files for reading
	// ======================================
	// 1. Make the TChain
	// we may either use the raw PMTData files, or the heftydb timestamp files
	if(not useHeftyTimes) { ADCTimestampChain = new TChain("PMTData"); }
	else                  { ADCTimestampChain = new TChain("heftydb"); }
	
	// 2. Load files from the file list into the TChain
	Log("LoadCCData Tool: Loading files for timestamp alignment",v_message,verbosity);
	myfile.open(alignmentfiles.c_str());
	if (myfile.is_open()){
		while ( getline (myfile,line) ){
			Log("LoadCCData Tool: Loading file "+line+" for PMTData",v_debug,verbosity);
			int filesadded = ADCTimestampChain->Add(line.c_str());
			Log(to_string(filesadded)+" files added to alignment Chain",v_debug,verbosity);
		}
		myfile.close();
	} else {
		Log("LoadCCData Tool: Error: Bad alignment filename "+line+"!",v_error,verbosity);
		return false;
	}
	
	// 3. Construct a Reader class from the TChain
	if(not useHeftyTimes){ thePMTData   = new                PMTData(ADCTimestampChain); }
	else                 { theHeftyData = new annie::HeftyTreeReader(ADCTimestampChain); }
	NumADCEntries = ADCTimestampChain->GetEntries();
	ADCChainEntry=0;
	
	// Pre-load the first TDC entry
	///////////////////////////////
	Log("LoadCCData Tool: Loading entry "+to_string(TDCChainEntry),v_debug,verbosity);
	Long64_t mentry = MRDData->LoadTree(TDCChainEntry);
	if(mentry < 0){
		Log("LoadCCData Tool: Ran off end of TChain! Stopping ToolChain",v_warning,verbosity);
		if(TDCChainEntry==NumCCDataEntries) m_data->vars.Set("StopLoop",1);
		return false;
	} else {
		Log("LoadCCData Tool: Getting TChain localentry" + to_string(mentry),v_debug,verbosity);
		MRDData->GetEntry(mentry);
		TDCChainEntry++;
		if(TDCChainEntry==NumCCDataEntries){
			Log("LoadCCData Tool: Last entry in CCData TChain, stopping chain",v_message,verbosity);
			m_data->vars.Set("StopLoop",1);
		}
	}
	
	return true;
}

bool LoadCCData::Execute(){
	
	Log("LoadCCData Tool: Executing",v_debug,verbosity);
	
	/* 
	On each Execute() call, we should load the ANNIEEvent with any TDC hits matching each of the 
	40 ADC minibuffers currently being processed.
	We assume both streams of timestamps increase monotonically, and that we start processing from
	the start of the chain. We then check if a TDC timestamp better matches this ADC timestamp or the next.
	If it better matches the next, it won't better match any previous ones, and we put it off for the next loop.
	If it better matches this timestamp than the next, it won't better match any further future timestamps.
	So we add it to the set of TDC hits for this event.
	*/
	
	// The ADC timestamps of the current event are available from the RawLoader toolchain.
	// For hefty mode, it's in the 'HeftyInfo' saved in the ANNIEEvent,
	// for non-hefty mode they're in the 'MinibufferTimestamps' saved in the ANNIEEvent.
	// Use the presence/absence of HeftyInfo to check if we're reading Hefty data
	
	// Load the ADC info
	std::vector<unsigned long long> currentminibufts;
	HeftyInfo* eventheftyinfo=nullptr;
	get_ok = m_data->Stores["ANNIEEvent"]->Get("HeftyInfo",eventheftyinfo);
	if(not get_ok){
		// Non-Hefty mode
		Log("LoadCCData Tool: ANNIEEvent had no HeftyInfo - assuming non-Hefty data",v_warning,verbosity);
		//m_data->Stores["ANNIEEvent"]->Print(false);
		eventheftyinfo=nullptr;
		std::vector<TimeClass> mb_timestamps; // 1-element vector
		get_ok = m_data->Stores["ANNIEEvent"]->Get("MinibufferTimestamps", mb_timestamps);
		if(not get_ok){
			logmessage = "LoadCCData Tool: ANNIEEvent had no HeftyInfo, nor MinibufferTimestamps! ";
			logmessage += "Cannot load trigger timestamps needed for alignment!";
			Log(logmessage,v_error,verbosity);
			//m_data->Stores["ANNIEEvent"]->Print(false);
			return false;
		}
		currentminibufts.emplace_back(mb_timestamps.front().GetNs());
	} else {
		// Hefty Mode
		// CCData tree is 'flattened' relative to Hefty data - 
		// each readout corresponds to a single minibuffer, but we process multiple minibuffers
		// simultaneously. We'll need an internal loop to read as many TDCData entries 
		// as there are minibuffers in each ADC Readout.
		currentminibufts = eventheftyinfo->all_times();
		int numminibuffers = eventheftyinfo->num_minibuffers();
		if(currentminibufts.size()!=numminibuffers){
			Log("LoadCCData Tool: HeftyInfo mismatch between num minibuffers "
				+ to_string(numminibuffers) + "and size of returned times vector " 
				+ to_string(currentminibufts.size()),v_error,verbosity);
		}
	}
	
	logmessage = "LoadCCData Tool: first minibuffer time = " + to_string(currentminibufts.front());
	if(currentminibufts.size()>1){
		logmessage += ", last minibuffer [" + to_string(currentminibufts.size()) + "] at time " + 
						to_string(currentminibufts.back());
	}
	Log(logmessage,v_debug,verbosity);
	
	// Also load the next ADCData entry, so we have access to the first timestamp from the upcoming event
	LoadPMTDataEntries();
	
	// Load all matching TDC hits into the ANNIEEvent
	PerformMatching(currentminibufts);
	
	return true;
}

bool LoadCCData::Finalise(){
	Log("LoadCCData Tool: Finalizing Tool",v_message,verbosity);
	
	if(MRDData) delete MRDData;
	if(thePMTData) delete thePMTData;
	if(theHeftyData) delete theHeftyData;
	
	return true;
}

bool LoadCCData::PerformMatching(std::vector<unsigned long long> currentminibufts){
	// scan the TDC data and return all hits matching the current minibuffers
	
	// loop over minibuffers in this Execute() iteration
	for(int minibufi=0; minibufi<currentminibufts.size(); minibufi++){
		// get this minibuffer's timestamp
		unsigned long long theminibufferTS = currentminibufts.at(minibufi);
		
		// get the next minibuffer's timestamp. In the event that this is the last
		// minibuffer of the current readout, we need to use the timestamp of the 
		// first minibuffer from the next readout
		unsigned long long thenextminibufferTS;
		if((minibufi+1)<currentminibufts.size()){
			thenextminibufferTS = currentminibufts.at(minibufi+1);
		} else {
			thenextminibufferTS = nextreadoutfirstminibufstart;
		}
		
		// scan forward from last matched TDC sample,
		// looking for all TDC samples that match this minibuffer better than the next
		// (we assume both time series increase monotonically)
		do{
			// the last TDC entry we loaded didn't match the previous minibuffer
			// so we have a TDC entry pre-loaded
			
			// 1. Get the TDC readout start time
			ULong64_t MRDTimeStamp = MRDData->TimeStamp; // in unix ms - need to convert ms to ns
			TimeClass MRDEventTime = TimeClass(static_cast<uint64_t>(MRDTimeStamp*ms_to_ns));
			logmessage = "LoadCCData Tool: TDC readout [" + to_string(MRDData->Trigger) 
						 + "] at time " + to_string(MRDEventTime.GetNs());
			Log(logmessage,v_debug,verbosity);                     // XXX
			
			// Compare this TDC readout time with the current and next minibuffer time.
			// if it better fits this minibuffer time, we'll call it a match
			if(abs(theminibufferTS - MRDEventTime.GetNs()) < abs(thenextminibufferTS - MRDEventTime.GetNs())){
				// it's a match!
				
				// Add all the hits in this readout to the TDC hits matching the minibuffer
				UInt_t numhitsthisevent = MRDData->OutNumber;
				Log("LoadCCData Tool: Looping over "+to_string(numhitsthisevent)+" TDC hits",v_debug,verbosity);
				
				for(int hiti=0; hiti<numhitsthisevent; hiti++){
					
					//convert time since TDC Timestamp from ticks to ns
					double hit_time_ns = static_cast<double>(MRDData->Value->at(hiti)) / MRD_NS_PER_SAMPLE;
					
					// Get the MRD PMT ID plugged into this TDC Card + Channel
					uint32_t tubeid = TubeIdFromSlotChannel(MRDData->Slot->at(hiti),MRDData->Channel->at(hiti));
					if(tubeid==std::numeric_limits<uint32_t>::max()){
						// no matching PMT. This was not an MRD / FACC paddle hit.
						Log("LoadCCData Tool: Ignoring hit on Slot " + to_string(MRDData->Slot->at(hiti))
							+ ", Channel " + to_string(MRDData->Channel->at(hiti)) + ", no matching MRD / FACC PMT",
							v_debug,verbosity);
						continue;
					}
					ChannelKey key(subdetector::TDC,tubeid);
					
					// construct the hit in the ANNIEEvent TDCData
					// Hit nexthit(tubeid, hit_time_ns, -1.); // charge = -1, not recorded
					if(TDCData->count(key)==0) TDCData->emplace(key, std::vector<std::vector<Hit>>{});
					TDCData->at(key).at(minibufi).emplace_back(tubeid, hit_time_ns, -1.);
					logmessage  = "LoadCCData Tool: Hit on tube " + to_string(tubeid);
					logmessage += " at " + to_string(hit_time_ns) + " ns relative to TDC readout time";
					Log(logmessage,v_debug,verbosity);
					
				} // end loop over hits this TDC readout
				
				// Load the TDC readout
				///////////////////////
				Log("LoadCCData Tool: Loading entry "+to_string(TDCChainEntry),v_debug,verbosity);
				Long64_t mentry = MRDData->LoadTree(TDCChainEntry);
				if(mentry < 0){
					Log("LoadCCData Tool: Ran off end of TChain! Stopping ToolChain",v_warning,verbosity);
					if(TDCChainEntry==NumCCDataEntries) m_data->vars.Set("StopLoop",1);
					return false;
				} else {
					Log("LoadCCData Tool: Getting TChain localentry" + to_string(mentry),v_debug,verbosity);
					MRDData->GetEntry(mentry);
					TDCChainEntry++;
					if(TDCChainEntry==NumCCDataEntries){
						Log("LoadCCData Tool: Last entry in CCData TChain, stopping chain",v_message,verbosity);
						m_data->vars.Set("StopLoop",1);
					}
				} // next entry loaded, loop back round to compare it to this minibuffer timestamp
				
			} else {
				// This TDC readout better matches the next minibuffer. End of matching TDC readouts
				break;
			}
			
		} while (true); // end of do loop looking for matching TDC readouts
		
	} // end loop over minibuffers in this Execute()
	
	return true;
}

bool LoadCCData::LoadPMTDataEntries(){
	
	if(ADCChainEntry!=NumADCEntries){
		
		// Load more ADC Data
		if(useHeftyTimes){
			// if using Raw PMTData tree, we'll use the annie:RawCard class
			// to convert the timestamps to a useable format
			Long64_t pentry = thePMTData->LoadTree(ADCChainEntry);
			thePMTData->GetEntry(pentry);
			// n.b. PMTData::TriggerCounts is an array of size PMTData::TriggerNumber
			// but annie::RawCard takes TriggerCounts as a vector
			std::vector<unsigned long long> TriggerCountsAsVector(thePMTData->TriggerCounts, 
				thePMTData->TriggerCounts + thePMTData->TriggerNumber);
			std::vector<uint64_t> minibufTS = ConvertTimeStamps(
												thePMTData->LastSync,
												thePMTData->StartTimeSec,
												thePMTData->StartTimeNSec,
												thePMTData->StartCount,
												TriggerCountsAsVector );
			nextreadoutfirstminibufstart = minibufTS.front();
		} else {
			// if using the heftydb tree, we can just get the timestamps
			// in a usable format directly
			std::unique_ptr<HeftyInfo> entryinfo = theHeftyData->next();
			nextreadoutfirstminibufstart = entryinfo->time(0);
		}
		
		// note that we've read this set of timestamps
		ADCChainEntry++;
		
	}  // else this is the last entry in the TChain. 
	
	return true;
}

std::vector<uint64_t> LoadCCData::ConvertTimeStamps(unsigned long long LastSync, int StartTimeSec, int StartTimeNSec, unsigned long long StartCount, std::vector<unsigned long long> TriggerCounts){
	// unused members
	const std::vector<unsigned short> Data{};
	const std::vector<unsigned int> Rates{};
	int Channels = 0;                         // must have Channels == Data.size() / BufferSize
	int BufferSize = TriggerCounts.size();    // must have TriggerCounts.size() == (BufferSize / MiniBufferSize)
	int MiniBufferSize = 1;
	int CardID = 0;
	
	// construct a RawCard so we can access the function to obtain the minibuffer TimeStamps
	annie::RawCard tempcard = annie::RawCard(CardID, LastSync, StartTimeSec,
					StartTimeNSec, StartCount, Channels, BufferSize, MiniBufferSize,
					Data, TriggerCounts, Rates);
	std::vector<uint64_t> alltimestamps;
	// loop over all the minibuffers in the readout and retrieve their timestamps
	for(int minibufi=0; minibufi<TriggerCounts.size(); minibufi++){
		unsigned long long thetimestamp = tempcard.trigger_time(minibufi);
		alltimestamps.push_back(thetimestamp);
	}
	return alltimestamps;
}

uint32_t LoadCCData::TubeIdFromSlotChannel(unsigned int slot, unsigned int channel){
	// map TDC Slot + Channel into a MRD PMT Tube ID
	uint16_t slotchan = static_cast<uint16_t>(slot*100)+static_cast<uint16_t>(channel);
	if(slotchantopmtid.count(slotchan)){
		std::string stringPMTID = slotchantopmtid.at(slotchan);
		int iPMTID = stoi(stringPMTID);
		return static_cast<uint32_t>(iPMTID);
	} else if(channel==31){
		// this is the Trigger card output to force the TDCs to always fire
		return std::numeric_limits<uint32_t>::max();
	} else {
		std::cerr<<"Unknown TDC Slot + Channel combination: "<<slotchan
				<<", no matching MRD PMT ID!"<<endl;
		return std::numeric_limits<uint32_t>::max();
	}
}

// PMT ID is constructed from X, Y, Z position:
// e.g. x=0, y=13, z=5 would become '00'+'13'+'05' = 001305.
// since c++ insists on handling integer literals with leading zeros as octal,
// map them as string, then convert the string to int
// phase 1 map:
std::map<uint16_t,std::string> LoadCCData::slotchantopmtid{
	std::pair<uint16_t,std::string>{1701u,"000002"},
	std::pair<uint16_t,std::string>{1702u,"000102"},
	std::pair<uint16_t,std::string>{1703u,"000202"},
	std::pair<uint16_t,std::string>{1704u,"000302"},
	std::pair<uint16_t,std::string>{1705u,"000402"},
	std::pair<uint16_t,std::string>{1706u,"000502"},
	std::pair<uint16_t,std::string>{1707u,"000602"},
	std::pair<uint16_t,std::string>{1708u,"000702"},
	std::pair<uint16_t,std::string>{1709u,"000802"},
	std::pair<uint16_t,std::string>{1710u,"000902"},
	std::pair<uint16_t,std::string>{1711u,"001002"},
	std::pair<uint16_t,std::string>{1712u,"001102"},
	std::pair<uint16_t,std::string>{1713u,"001202"},
	std::pair<uint16_t,std::string>{1714u,"010002"},
	std::pair<uint16_t,std::string>{1715u,"010102"},
	std::pair<uint16_t,std::string>{1716u,"010202"},
	std::pair<uint16_t,std::string>{1717u,"010302"},
	std::pair<uint16_t,std::string>{1718u,"010402"},
	std::pair<uint16_t,std::string>{1719u,"010502"},
	std::pair<uint16_t,std::string>{1720u,"010602"},
	std::pair<uint16_t,std::string>{1721u,"010702"},
	std::pair<uint16_t,std::string>{1722u,"010802"},
	std::pair<uint16_t,std::string>{1723u,"010902"},
	std::pair<uint16_t,std::string>{1724u,"011002"},
	std::pair<uint16_t,std::string>{1725u,"011102"},
	std::pair<uint16_t,std::string>{1726u,"011202"},
	std::pair<uint16_t,std::string>{1801u,"000003"},
	std::pair<uint16_t,std::string>{1802u,"010003"},
	std::pair<uint16_t,std::string>{1803u,"020003"},
	std::pair<uint16_t,std::string>{1804u,"030003"},
	std::pair<uint16_t,std::string>{1805u,"040003"},
	std::pair<uint16_t,std::string>{1806u,"050003"},
	std::pair<uint16_t,std::string>{1807u,"060003"},
	std::pair<uint16_t,std::string>{1808u,"070003"},
	std::pair<uint16_t,std::string>{1809u,"080003"},
	std::pair<uint16_t,std::string>{1810u,"090003"},
	std::pair<uint16_t,std::string>{1811u,"100003"},
	std::pair<uint16_t,std::string>{1812u,"110003"},
	std::pair<uint16_t,std::string>{1813u,"120003"},
	std::pair<uint16_t,std::string>{1814u,"130003"},
	std::pair<uint16_t,std::string>{1815u,"000103"},
	std::pair<uint16_t,std::string>{1816u,"010103"},
	std::pair<uint16_t,std::string>{1817u,"020103"},
	std::pair<uint16_t,std::string>{1818u,"030103"},
	std::pair<uint16_t,std::string>{1819u,"040103"},
	std::pair<uint16_t,std::string>{1820u,"050103"},
	std::pair<uint16_t,std::string>{1821u,"060103"},
	std::pair<uint16_t,std::string>{1822u,"070103"},
	std::pair<uint16_t,std::string>{1823u,"080103"},
	std::pair<uint16_t,std::string>{1824u,"090103"},
	std::pair<uint16_t,std::string>{1825u,"100103"},
	std::pair<uint16_t,std::string>{1826u,"110103"},
	std::pair<uint16_t,std::string>{1827u,"120103"},
	std::pair<uint16_t,std::string>{1828u,"130103"},
	std::pair<uint16_t,std::string>{1829u,"140103"},
	std::pair<uint16_t,std::string>{1401u,"000000"},
	std::pair<uint16_t,std::string>{1402u,"000100"},
	std::pair<uint16_t,std::string>{1403u,"000200"},
	std::pair<uint16_t,std::string>{1404u,"000300"},
	std::pair<uint16_t,std::string>{1405u,"000400"},
	std::pair<uint16_t,std::string>{1406u,"000500"},
	std::pair<uint16_t,std::string>{1407u,"000600"},
	std::pair<uint16_t,std::string>{1408u,"000700"},
	std::pair<uint16_t,std::string>{1409u,"000800"},
	std::pair<uint16_t,std::string>{1410u,"000900"},
	std::pair<uint16_t,std::string>{1411u,"001000"},
	std::pair<uint16_t,std::string>{1412u,"001100"},
	std::pair<uint16_t,std::string>{1413u,"001200"},
	std::pair<uint16_t,std::string>{1414u,"010000"},
	std::pair<uint16_t,std::string>{1415u,"010100"},
	std::pair<uint16_t,std::string>{1416u,"010200"},
	std::pair<uint16_t,std::string>{1417u,"010300"},
	std::pair<uint16_t,std::string>{1418u,"010400"},
	std::pair<uint16_t,std::string>{1419u,"010500"},
	std::pair<uint16_t,std::string>{1420u,"010600"},
	std::pair<uint16_t,std::string>{1421u,"010700"},
	std::pair<uint16_t,std::string>{1422u,"010800"},
	std::pair<uint16_t,std::string>{1423u,"010900"},
	std::pair<uint16_t,std::string>{1424u,"011000"},
	std::pair<uint16_t,std::string>{1425u,"011100"},
	std::pair<uint16_t,std::string>{1426u,"011200"}
};


///////////// 
// MRDTesting TDC setup, part 1:
//	std::map<uint16_t,std::string> slotchantopmtid{
//		std::pair<uint16_t,std::string>{1423u,"000002"},
//		std::pair<uint16_t,std::string>{1416u,"000102"},
//		std::pair<uint16_t,std::string>{1417u,"000202"},
//		std::pair<uint16_t,std::string>{1418u,"000302"},
//		std::pair<uint16_t,std::string>{1419u,"000402"},
//		std::pair<uint16_t,std::string>{1420u,"000502"},
//		std::pair<uint16_t,std::string>{1421u,"000602"},
//		std::pair<uint16_t,std::string>{1422u,"000702"},
//		std::pair<uint16_t,std::string>{1424u,"000802"},
//		std::pair<uint16_t,std::string>{1425u,"000902"},
//		std::pair<uint16_t,std::string>{1426u,"001002"},
//		std::pair<uint16_t,std::string>{1427u,"001102"},
//		std::pair<uint16_t,std::string>{1328u,"001202"},
//		std::pair<uint16_t,std::string>{1823u,"000004"},
//		std::pair<uint16_t,std::string>{1816u,"000104"},
//		std::pair<uint16_t,std::string>{1817u,"000204"},
//		std::pair<uint16_t,std::string>{1818u,"000304"},
//		std::pair<uint16_t,std::string>{1819u,"000404"},
//		std::pair<uint16_t,std::string>{1820u,"000504"},
//		std::pair<uint16_t,std::string>{1821u,"000604"},
//		std::pair<uint16_t,std::string>{1822u,"000704"},
//		std::pair<uint16_t,std::string>{1824u,"000804"},
//		std::pair<uint16_t,std::string>{1825u,"000904"},
//		std::pair<uint16_t,std::string>{1826u,"001004"},
//		std::pair<uint16_t,std::string>{1827u,"001104"},
//		std::pair<uint16_t,std::string>{1828u,"001204"},
//		std::pair<uint16_t,std::string>{1800u,"000006"},
//		std::pair<uint16_t,std::string>{1801u,"000106"},
//		std::pair<uint16_t,std::string>{1802u,"000206"},
//		std::pair<uint16_t,std::string>{1803u,"000306"},
//		std::pair<uint16_t,std::string>{1804u,"000406"},
//		std::pair<uint16_t,std::string>{1805u,"000506"},
//		std::pair<uint16_t,std::string>{1806u,"000606"},
//		std::pair<uint16_t,std::string>{1807u,"000706"},
//		std::pair<uint16_t,std::string>{1808u,"000806"},
//		std::pair<uint16_t,std::string>{1809u,"000906"},
//		std::pair<uint16_t,std::string>{1810u,"001006"},
//		std::pair<uint16_t,std::string>{1811u,"001106"},
//		std::pair<uint16_t,std::string>{1812u,"001206"},
//		std::pair<uint16_t,std::string>{1407u,"000008"},
//		std::pair<uint16_t,std::string>{1400u,"000108"},
//		std::pair<uint16_t,std::string>{1401u,"000208"},
//		std::pair<uint16_t,std::string>{1402u,"000308"},
//		std::pair<uint16_t,std::string>{1403u,"000408"},
//		std::pair<uint16_t,std::string>{1404u,"000508"},
//		std::pair<uint16_t,std::string>{1405u,"000608"},
//		std::pair<uint16_t,std::string>{1406u,"000708"},
//		std::pair<uint16_t,std::string>{1408u,"000808"},
//		std::pair<uint16_t,std::string>{1409u,"000908"},
//		std::pair<uint16_t,std::string>{1410u,"001008"},
//		std::pair<uint16_t,std::string>{1411u,"001108"},
//		std::pair<uint16_t,std::string>{1412u,"001208"},
//		std::pair<uint16_t,std::string>{1700u,"000010"},
//		std::pair<uint16_t,std::string>{1701u,"000110"},
//		std::pair<uint16_t,std::string>{1702u,"000210"},
//		std::pair<uint16_t,std::string>{1703u,"000310"},
//		std::pair<uint16_t,std::string>{1704u,"000410"},
//		std::pair<uint16_t,std::string>{1705u,"000510"},
//		std::pair<uint16_t,std::string>{1706u,"000610"},
//		std::pair<uint16_t,std::string>{1707u,"000710"},
//		std::pair<uint16_t,std::string>{1708u,"000810"},
//		std::pair<uint16_t,std::string>{1709u,"000910"},
//		std::pair<uint16_t,std::string>{1710u,"001010"},
//		std::pair<uint16_t,std::string>{1711u,"001110"},
//		std::pair<uint16_t,std::string>{1712u,"001210"},
//		std::pair<uint16_t,std::string>{1716u,"000012"},
//		std::pair<uint16_t,std::string>{1717u,"000112"},
//		std::pair<uint16_t,std::string>{1718u,"000212"},
//		std::pair<uint16_t,std::string>{1719u,"000312"},
//		std::pair<uint16_t,std::string>{1720u,"000412"},
//		std::pair<uint16_t,std::string>{1721u,"000512"},
//		std::pair<uint16_t,std::string>{1722u,"000612"},
//		std::pair<uint16_t,std::string>{1723u,"000712"},
//		std::pair<uint16_t,std::string>{1724u,"000812"},
//		std::pair<uint16_t,std::string>{1725u,"000912"},
//		std::pair<uint16_t,std::string>{1726u,"001012"},
//		std::pair<uint16_t,std::string>{1727u,"001112"},
//		std::pair<uint16_t,std::string>{1728u,"001212"}
//	};


//		MRDData has the following members:
//		UInt_t                     Trigger;         // TDC readout counter
//		UInt_t                     OutNumber;       // number of hits in this event - size of vectors
//		ULong64_t                  TimeStamp;       // UTC MILLISECONDS since unix epoch
//		std::vector<std::string>*  Type;            // card type - all cards are "TDC"
//		std::vector<unsigned int>* Value;           // time in TDC ticks from TimeStamp
//		std::vector<unsigned int>* Slot;            // slot of hit
//		std::vector<unsigned int>* Channel;         // channel of hit
//		
//		The MRD process is:
//			1. Trigger card sends common start to MRD cards
//			2. A timer is started on all channels.
//			3. When a channel receives a pulse, the timer stops. XXX: only the first pulse is recorded!XXX
//			4. After all channels either record hits or time out (currently 4.2us) everything is read out.
//			   A timestamp is created at time of readout. Note this is CLOSE TO, BUT NOT EQUAL TO the trigger
//			   time. (Probably ~ trigger time + timeout...)
//			5. Channels with a hit will have an entry created with 'Value' = clock ticks between the common
//			   start and when the hit arrived. Channels that timed out have no entry.
//		
//		Timestamp is a UTC [MILLISECONDS] timestamp of when the readout ended. 
//		To correctly map Value to an actual time one would need to match the MRD timestamp 
//		to the trigger card timestamp (which will be more accurate)
//		then add Value * MRD_NS_PER_SAMPLE to the Trigger time. 
//		
//		Only the first pulse will be recorded .... IS THIS OK? XXX
//		What about pre-trigger? - common start issued by trigger is from Beam not on NDigits,
//		so always pre-beam...?
//		TDC records with resolution 4ns = 1 sample per ns? or round times to nearest/round down to 4ns?
//		TDCRes https://github.com/ANNIEDAQ/ANNIEDAQ/blob/master/configfiles/TDCreg
