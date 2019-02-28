/* vim:set noexpandtab tabstop=4 wrap */

#include "LoadWCSim.h"

LoadWCSim::LoadWCSim():Tool(){}

bool LoadWCSim::Initialise(std::string configfile, DataModel &data){
	
	/////////////////// Useful header ///////////////////////
	
	if(verbose) cout<<"Initializing Tool LoadWCSim"<<endl;
	
	if(configfile!="") m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	
	// Get the Tool configuration variables
	// ====================================
	m_variables.Get("verbose", verbose);
	m_variables.Get("InputFile", MCFile);
	m_variables.Get("HistoricTriggeroffset", HistoricTriggeroffset); // [ns]
	m_variables.Get("WCSimVersion", FILE_VERSION);
	m_variables.Get("UseDigitSmearedTime", use_smeared_digit_time);
	m_variables.Get("LappdNumStrips", LappdNumStrips);
	m_variables.Get("LappdStripLength", LappdStripLength);           // [mm]
	m_variables.Get("LappdStripSeparation", LappdStripSeparation);   // [mm]
	// put version in the CStore for downstream tools
	m_data->CStore.Set("WCSimVersion", FILE_VERSION);
	
	// Short Stores README
	//////////////////////
	// n.b. m_data->vars is a Store (of ben's Store type) that is not saved to disk?
	//      m_data->CStore is a single entry binary BoostStore that is not saved to disk.
	//      m_data->Stores["StoreName"] is a map of binary BoostStores that are saved to disk.
	// If using Stores->BoostStore->Set("MyVariable") it will always be saved to disk
	// Using Stores->BoostStore.Set("MyVariable",&myvar) if myvar is a pointer (to an object on
	// the heap) puts myvar in the Store and it's deletion will be handled by the Store.
	// (provided your class has a suitable destructor.)
	// Is 'BoostStore::Save' needed for single-entry stores?
	// ----------------
	// create a new BoostStore with key "ANNIEEvent" in the Stores std::map
	// BoostStore constructor args: typechecking (bool), m_format (0=binary, 1=ASCII, 2=multievent)
	// A BoostStore has a header where useful constants may be saved. The header is a BoostStore itself,
	// and can be accessed via: Store.Header->Get() and Store.Header->Set().
	// The method 'Store::Save()' writes everything 'Set' since the last 'Save' to the current entry.
	// Store::Clear clears the map of the current entry, to start building a new one.
	// Use 'Store::GetEntry(int entrynum)' to load an entry to then be able to 'Get' it's contents.
	// 'Store->Header->Get("TotalEntries",NumEvents)' will load the num entries into NumEvents
	// ------------------
	// When adding a BoostStore (or class object in general) to a BoostStore (such as ANNIEEvent)
	// you call BoostStore::Set("key",ObjectPointer) - BUT be aware that serialization happens when the
	// 'Set' method is called - although you pass it a pointer, any subsequent changes to the object
	// will NOT get saved! You must call 'Set' AFTER making ALL changes to your object!
	/////////////////////////////////////////////////////////////////
	
	// Make class private members; e.g. the WCSimT and WCSimRootGeom
	// =============================================================
	file= new TFile(MCFile.c_str(),"READ");
	wcsimtree= (TTree*) file->Get("wcsimT");
	NumEvents=wcsimtree->GetEntries();
	WCSimEntry= new wcsimT(wcsimtree);
	gROOT->cd();
	wcsimrootgeom = WCSimEntry->wcsimrootgeom;
	wcsimrootopts = WCSimEntry->wcsimrootopts;
	int pretriggerwindow=wcsimrootopts->GetNDigitsPreTriggerWindow();
	int posttriggerwindow=wcsimrootopts->GetNDigitsPostTriggerWindow();
	
	// put useful constants into the CStore
	// ====================================
	//m_data->CStore.Set("WCSimEntry",WCSimEntry,false); // pass on the WCSim entry - not possible
	// pass on root options. TODO should these be saved somewhere?
	m_data->CStore.Set("WCSimPreTriggerWindow",pretriggerwindow);
	m_data->CStore.Set("WCSimPostTriggerWindow",posttriggerwindow);
	// store the WCSimRootGeom for LAPPD reader calculation of global coords
	intptr_t geomptr = reinterpret_cast<intptr_t>(wcsimrootgeom);
	if(verbose>1) cout<<"wcsimrootgeom at "<<wcsimrootgeom<<", geomptr="<<geomptr<<endl;
	m_data->CStore.Set("WCSimRootGeom",geomptr);
//	int wcsimgeomexists = m_data->Stores.count("WCSimRootGeomStore");
//	if(wcsimgeomexists==0){
//		m_data->Stores["WCSimRootGeomStore"] = new BoostStore(false,0);
//		//m_data->Stores.at("WCSimRootGeomStore")->Header->Set("WCSimRootGeom",wcsimrootgeom);
//		m_data->Stores.at("WCSimRootGeomStore")->Set("WCSimRootGeom",&wcsimrootgeom);
//	}
//	
//	// Make a WCSimStore to store additional WCSim info passed between tools
//	// =====================================================================
//	int wcsimstoreexists = m_data->Stores.count("WCSimStore");
//	cout<<"wcsimstoreexists="<<wcsimstoreexists<<endl;
//	if(wcsimstoreexists==0){
//		m_data->Stores["WCSimStore"] = new BoostStore(false,0);
//	}
//	m_data->Stores.at("WCSimStore")->Set("WCSimRootGeom",geomptr);
//	m_data->Stores.at("WCSimStore")->Set("WCSimPreTriggerWindow",pretriggerwindow);
//	m_data->Stores.at("WCSimStore")->Set("WCSimPostTriggerWindow",posttriggerwindow);
	
	// Make the ANNIEEvent Store if it doesn't exist
	// =============================================
	int annieeventexists = m_data->Stores.count("ANNIEEvent");
	if(annieeventexists==0) m_data->Stores["ANNIEEvent"] = new BoostStore(false,2);
	
	// Convert WCSimRootGeom into ToolChain Geometry class
	ConstructToolChainGeometry();
	
	// Set run-level information in the ANNIEEvent
	// ===========================================
	/*
	At time of writing ANNIEEvent has the following that need to be Set before each Save:
		RunNumber
		SubrunNumber
		EventNumber
		MCParticles
		RecoParticles
		MCHits
		TDCData
		RawADCData
		CalibratedADCData
		RawLAPPDData
		CalibratedLAPPDData
		TriggerData
		MCFlag
		EventTime
		MCEventNum
		MCFile
		BeamStatus
	*/
	
	EventNumber=0;
	MCEventNum=-1;
	MCTriggernum=0;
	// pull the first entry with a trigger and use it's Date for the BeamStatus last recorded time. TODO
	if(verbose>1) cout<<"getting Run start time"<<endl;
	do{
		MCEventNum++;
		WCSimEntry->GetEntry(MCEventNum);
	} while(WCSimEntry->wcsimrootevent->GetNumberOfEvents()==0);
	atrigt = WCSimEntry->wcsimrootevent->GetTrigger(0);
	TimeClass RunStartTime(atrigt->GetHeader()->GetDate());
	MCEventNum=0;
	MCFile = wcsimtree->GetCurrentFile()->GetName();
	m_data->Stores.at("ANNIEEvent")->Set("MCFile",MCFile);
	
	// use nominal beam values TODO
	double beaminten=4.777e+12;
	double beampow=3.2545e+16;
	BeamStatus = new BeamStatusClass(RunStartTime, beaminten, beampow, "stable");
	
	// Construct the other objects we'll be setting at event level,
	// pass managed pointers to the ANNIEEvent Store
	MCParticles = new std::vector<MCParticle>;
	MCHits = new std::map<unsigned long,std::vector<Hit>>;
	TDCData = new std::map<unsigned long,std::vector<Hit>>;
	EventTime = new TimeClass();
	TriggerClass beamtrigger("beam",true,0);
	TriggerData = new std::vector<TriggerClass>{beamtrigger}; // FIXME ? one trigger and resetting time is ok?
	
	return true;
}


bool LoadWCSim::Execute(){
	
	// probably not necessary, clears the map for this entry. We're going to re-Set the event entry anyway...
	//m_data->Stores.at("ANNIEEvent")->Clear();
	
	if(verbose) cout<<"Executing tool LoadWCSim with MC entry "<<MCEventNum<<", trigger "<<MCTriggernum<<endl;
	WCSimEntry->GetEntry(MCEventNum);
	MCFile = wcsimtree->GetCurrentFile()->GetName();
	
	MCParticles->clear();
	MCHits->clear();
	TDCData->clear();
	
	//for(int MCTriggernum=0; MCTriggernum<WCSimEntry->wcsimrootevent->GetNumberOfEvents(); MCTriggernum++){
		if(verbose>1) cout<<"getting triggers"<<endl;
		atrigt = WCSimEntry->wcsimrootevent->GetTrigger(MCTriggernum);
		WCSimRootTrigger* firsttrig=WCSimEntry->wcsimrootevent->GetTrigger(0);  // photons are all in first trig
		if(MCTriggernum<(WCSimEntry->wcsimrootevent_mrd->GetNumberOfEvents())){
			atrigm = WCSimEntry->wcsimrootevent_mrd->GetTrigger(MCTriggernum);
		} else { atrigm=nullptr; }
		if(MCTriggernum<(WCSimEntry->wcsimrootevent_facc->GetNumberOfEvents())){
			atrigv = WCSimEntry->wcsimrootevent_facc->GetTrigger(MCTriggernum);
		} else { atrigv=nullptr; }
		if(verbose>2) cout<<"wcsimrootevent="<<WCSimEntry->wcsimrootevent<<endl;
		if(verbose>2) cout<<"wcsimrootevent_mrd="<<WCSimEntry->wcsimrootevent_mrd<<endl;
		if(verbose>2) cout<<"wcsimrootevent_facc="<<WCSimEntry->wcsimrootevent_facc<<endl;
		if(verbose>2) cout<<"atrigt="<<atrigt<<", atrigm="<<atrigm<<", atrigv="<<atrigv<<endl;
		
		if(verbose>1) cout<<"getting event date"<<endl;
		RunNumber = atrigt->GetHeader()->GetRun();
		SubrunNumber = 0;
		EventTimeNs = atrigt->GetHeader()->GetDate();
		EventTime->SetNs(EventTimeNs);
		if(verbose>2) cout<<"EventTime is "<<EventTimeNs<<"ns"<<endl;
		std::string geniefilename = firsttrig->GetHeader()->GetGenieFileName().Data();
		int genieentry = firsttrig->GetHeader()->GetGenieEntryNum();
		if(verbose>3) cout<<"Genie file is "<<geniefilename<<", genie event num was "<<genieentry<<endl;
		m_data->CStore.Set("GenieFile",geniefilename);
		m_data->CStore.Set("GenieEntry",genieentry);
		if(verbose>1) cout<<"getting "<<atrigt->GetNtrack()<<" tracks"<<endl;
		for(int tracki=0; tracki<atrigt->GetNtrack(); tracki++){
			if(verbose>2) cout<<"getting track "<<tracki<<endl;
			WCSimRootTrack* nextrack = (WCSimRootTrack*)atrigt->GetTracks()->At(tracki);
			/* a WCSimRootTrack has methods:
			Int_t     GetIpnu()             pdg
			Int_t     GetFlag()             -1: neutrino primary, -2: neutrino target, 0: other
			Float_t   GetM()                mass
			Float_t   GetP()                momentum magnitude
			Float_t   GetE()                energy (inc rest mass^2)
			Float_t   GetEndE()             energy on stopping of particle tracking
			Float_t   GetEndP()             momentum on stopping of particle tracking
			Int_t     GetStartvol()         starting volume: 10 is tank, 20 is facc, 30 is mrd
			Int_t     GetStopvol()          stopping volume: but these may not be set.
			Float_t   GetDir(Int_t i=0)     momentum unit vector
			Float_t   GetPdir(Int_t i=0)    momentum vector
			Float_t   GetPdirEnd(Int_t i=0) direction vector on stop tracking
			Float_t   GetStop(Int_t i=0)    stopping vertex x,y,z for i=0-2, in cm
			Float_t   GetStart(Int_t i=0)   starting vertex x,y,z for i=0-2, in cm
			Int_t     GetParenttype()       parent pdg, 0 for primary.
			Float_t   GetTime()             trj->GetGlobalTime(); starting time of particle
			Float_t   GetStopTime()
			Int_t     GetId()               wcsim trackid
			*/
			
			tracktype startstoptype = tracktype::UNDEFINED;
			//nextrack->GetFlag()!=-1 ????? do we need to skip/override anything for these?
			//MC particle times are relative to the trigger time
			MCParticle thisparticle(
				nextrack->GetIpnu(), nextrack->GetE(), nextrack->GetEndE(),
				Position(nextrack->GetStart(0) / 100.,
						 nextrack->GetStart(1) / 100.,
						 nextrack->GetStart(2) / 100.),
				Position(nextrack->GetStop(0) / 100.,
						 nextrack->GetStop(1) / 100.,
						 nextrack->GetStop(2) / 100.),
				//MC particle times now stored relative to the trigger time
				(static_cast<double>(nextrack->GetTime()-EventTimeNs)),
				(static_cast<double>(nextrack->GetStopTime()-EventTimeNs)),
				Direction(nextrack->GetDir(0), nextrack->GetDir(1), nextrack->GetDir(2)),
				(sqrt(pow(nextrack->GetStop(0)-nextrack->GetStart(0),2.)+
					 pow(nextrack->GetStop(1)-nextrack->GetStart(1),2.)+
					 pow(nextrack->GetStop(2)-nextrack->GetStart(2),2.))) / 100.,
				startstoptype,
				tracki,
				nextrack->GetParenttype());
			
			MCParticles->push_back(thisparticle);
		}
		if(verbose>2) cout<<"MCParticles has "<<MCParticles->size()<<" entries"<<endl;
		
		int numtankdigits = atrigt ? atrigt->GetCherenkovDigiHits()->GetEntries() : 0;
		if(verbose>1) cout<<"looping over "<<numtankdigits<<" tank digits"<<endl;
		for(int digiti=0; digiti<numtankdigits; digiti++){
			if(verbose>2) cout<<"getting digit "<<digiti<<endl;
			WCSimRootCherenkovDigiHit* digihit =
				(WCSimRootCherenkovDigiHit*)atrigt->GetCherenkovDigiHits()->At(digiti);
			//WCSimRootChernkovDigiHit has methods GetTubeId(), GetT(), GetQ(), GetPhotonIds()
			if(verbose>2) cout<<"next digihit at "<<digihit<<endl;
			int tubeid = digihit->GetTubeId();  // geometry TubeID->channelkey map is made INCLUDING offset of 1
			if(verbose>2) cout<<"tubeid="<<tubeid<<endl;
			if(pmt_tubeid_to_channelkey.count(tubeid)==0){
				cerr<<"LoadWCSim ERROR: tank PMT with no associated ChannelKey!"<<endl;
				return false;
			}
			unsigned long key = pmt_tubeid_to_channelkey.at(tubeid);
			if(verbose>2) cout<<"ChannelKey="<<key<<endl;
			double digittime;
			if(use_smeared_digit_time){
				digittime = static_cast<double>(digihit->GetT()-HistoricTriggeroffset); // relative to trigger
			} else {
				// instead take the true time of the first photon
				std::vector<int> photonids = digihit->GetPhotonIds();   // indices of the digit's photons
				double earliestphotontruetime=999999999999;
				for(int& aphotonindex : photonids){
					WCSimRootCherenkovHitTime* thehittimeobject =
						 (WCSimRootCherenkovHitTime*)firsttrig->GetCherenkovHitTimes()->At(aphotonindex);
					if(thehittimeobject==nullptr){
						cerr<<"LoadWCSim Tool: ERROR! Retrieval of photon from digit returned nullptr!"<<endl;
						continue;
					}
					double aphotontime = static_cast<double>(thehittimeobject->GetTruetime());
					if(aphotontime<earliestphotontruetime){ earliestphotontruetime = aphotontime; }
				}
				digittime = earliestphotontruetime;
			}
			if(verbose>2){ cout<<"digittime is "<<digittime<<" [ns] from Trigger"<<endl; }
			float digiq = digihit->GetQ();
			if(verbose>2) cout<<"digit Q is "<<digiq<<endl;
			
			Hit nexthit(key, digittime, digiq);
			if(MCHits->count(key)==0) MCHits->emplace(key, std::vector<Hit>{nexthit});
			else MCHits->at(key).push_back(nexthit);
			if(verbose>2) cout<<"digit added"<<endl;
		}
		if(verbose>2) cout<<"done with tank digits"<<endl;
		
		//MRD Hits
		int nummrddigits = atrigm ? atrigm->GetCherenkovDigiHits()->GetEntries() : 0;
		if(verbose>1) cout<<"adding "<<nummrddigits<<" mrd digits"<<endl;
		for(int digiti=0; digiti<nummrddigits; digiti++){
			if(verbose>2) cout<<"getting digit "<<digiti<<endl;
			WCSimRootCherenkovDigiHit* digihit =
				(WCSimRootCherenkovDigiHit*)atrigm->GetCherenkovDigiHits()->At(digiti);
			if(verbose>2) cout<<"next digihit at "<<digihit<<endl;
			int tubeid = digihit->GetTubeId();
			if(verbose>2) cout<<"tubeid="<<tubeid<<endl;
			if(mrd_tubeid_to_channelkey.count(tubeid)==0){
				cerr<<"LoadWCSim ERROR: MRD PMT with no associated ChannelKey!"<<endl;
				return false;
			}
			unsigned long key = mrd_tubeid_to_channelkey.at(tubeid);
			double digittime(static_cast<double>(digihit->GetT()-HistoricTriggeroffset)); // rel. to trigger
			if(verbose>2){ cout<<"digittime is "<<digittime<<" [ns] from Trigger"<<endl; }
			float digiq = digihit->GetQ();
			if(verbose>2) cout<<"digit Q is "<<digiq<<endl;
			
			Hit nexthit(key, digittime, digiq);
			if(TDCData->count(key)==0) TDCData->emplace(key, std::vector<Hit>{nexthit});
			else TDCData->at(key).push_back(nexthit);
			if(verbose>2) cout<<"digit added"<<endl;
		}
		if(verbose>2) cout<<"done with mrd digits"<<endl;
		
		// Veto Hits
		int numvetodigits = atrigv ? atrigv->GetCherenkovDigiHits()->GetEntries() : 0;
		if(verbose>1) cout<<"adding "<<numvetodigits<<" veto digits"<<endl;
		for(int digiti=0; digiti<numvetodigits; digiti++){
			if(verbose>2) cout<<"getting digit "<<digiti<<endl;
			WCSimRootCherenkovDigiHit* digihit =
				(WCSimRootCherenkovDigiHit*)atrigv->GetCherenkovDigiHits()->At(digiti);
			if(verbose>2) cout<<"next digihit at "<<digihit<<endl;
			int tubeid = digihit->GetTubeId();
			if(verbose>2) cout<<"tubeid="<<tubeid<<endl;
			if(facc_tubeid_to_channelkey.count(tubeid)==0){
				cerr<<"LoadWCSim ERROR: FACC PMT with no associated ChannelKey!"<<endl;
				return false;
			}
			unsigned int key = facc_tubeid_to_channelkey.at(tubeid);
			double digittime(static_cast<double>(digihit->GetT()-HistoricTriggeroffset)); // rel. to trigger
			if(verbose>2){ cout<<"digittime is "<<digittime<<" [ns] from Trigger"<<endl; }
			float digiq = digihit->GetQ();
			if(verbose>2) cout<<"digit Q is "<<digiq<<endl;
			
			Hit nexthit(key, digittime, digiq);
			if(TDCData->count(key)==0) TDCData->emplace(key, std::vector<Hit>{nexthit});
			else TDCData->at(key).push_back(nexthit);
			if(verbose>2) cout<<"digit added"<<endl;
		}
		if(verbose>2) cout<<"done with veto digits"<<endl;
		
		if(verbose>2) cout<<"setting triggerdata time to "<<EventTimeNs<<"ns"<<endl;
		TriggerData->front().SetTime(EventTimeNs);
		
	//}
	
	//int mrdentries;
	//m_data->Stores.at("TDCData")->Get("TotalEntries",mrdentries); // ??
//	m_data->Stores("WCSimEntries")->Set("wcsimrootevent",WCSimEntry->wcsimrootevent);
//	m_data->Stores("WCSimEntries")->Set("wcsimrootevent_mrd",WCSimEntry->wcsimrootevent_mrd);
//	m_data->Stores("WCSimEntries")->Set("wcsimrootevent_facc",*(WCSimEntry->wcsimrootevent_facc));
	
	
	// set event level variables
	if(verbose>1) cout<<"setting the store variables"<<endl;
	m_data->Stores.at("ANNIEEvent")->Set("RunNumber",RunNumber);
	m_data->Stores.at("ANNIEEvent")->Set("SubrunNumber",SubrunNumber);
	m_data->Stores.at("ANNIEEvent")->Set("EventNumber",EventNumber);
	if(verbose>2) cout<<"particles"<<endl;
	m_data->Stores.at("ANNIEEvent")->Set("MCParticles",MCParticles,true);
	if(verbose>2) cout<<"hits"<<endl;
	m_data->Stores.at("ANNIEEvent")->Set("MCHits",MCHits,true);
	if(verbose>2) cout<<"tdcdata"<<endl;
	m_data->Stores.at("ANNIEEvent")->Set("TDCData",TDCData,true);
	if(verbose>2) cout<<"triggerdata"<<endl;
	m_data->Stores.at("ANNIEEvent")->Set("TriggerData",TriggerData,true);  // FIXME
	if(verbose>2) cout<<"eventtime"<<endl;
	m_data->Stores.at("ANNIEEvent")->Set("EventTime",EventTime,true);
	m_data->Stores.at("ANNIEEvent")->Set("MCEventNum",MCEventNum);
	m_data->Stores.at("ANNIEEvent")->Set("MCTriggernum",MCTriggernum);
	m_data->Stores.at("ANNIEEvent")->Set("MCFile",MCFile);
	m_data->Stores.at("ANNIEEvent")->Set("MCFlag",true);                   // constant
	m_data->Stores.at("ANNIEEvent")->Set("BeamStatus",BeamStatus,true);
	m_data->CStore.Set("NumTriggersThisMCEvt",WCSimEntry->wcsimrootevent->GetNumberOfEvents());
	//Things that need to be set by later tools:
	//RawADCData
	//CalibratedADCData
	//RawLAPPDData
	//CalibratedLAPPDData
	//RecoParticles
	
	EventNumber++;
	MCTriggernum++;
	if(verbose>2) cout<<"checking if we're done on trigs in this event"<<endl;
	if(MCTriggernum==WCSimEntry->wcsimrootevent->GetNumberOfEvents()){
		MCTriggernum=0;
		MCEventNum++;
		if(verbose>2) cout<<"this is the last trigger in the event: next loop will process a new event"<<endl;
	} else {
		if(verbose>2) cout<<"there are further triggers in this event: next loop will process the trigger "<<MCTriggernum<<"/"<<WCSimEntry->wcsimrootevent->GetNumberOfEvents()<<endl;
	}
	if(MCEventNum==NumEvents) m_data->vars.Set("StopLoop",1);
	if(verbose>1) cout<<"done loading event"<<endl;
	return true;
}

bool LoadWCSim::Finalise(){
	file->Close();
	delete WCSimEntry;
	//delete file;  // Done by WCSimEntry destructor
	
	return true;
}

void LoadWCSim::ConstructToolChainGeometry(){
	// Pull details from the WCSimRootGeom
	// ===================================
	double WCSimGeometryVer = 1;                      // TODO: pull this from some suitable variable
	// some variables are available from wcsimrootgeom.  TODO: put everything into wcsimrootgeom
	numtankpmts = wcsimrootgeom->GetWCNumPMT();
	numlappds = wcsimrootgeom->GetWCNumLAPPD();
	nummrdpmts = wcsimrootgeom->GetWCNumMRDPMT();
	numvetopmts = wcsimrootgeom->GetWCNumFACCPMT();
	double tank_xcentre = (wcsimrootgeom->GetWCOffset(0)) / 100.;  // convert [cm] to [m]
	double tank_ycentre = (wcsimrootgeom->GetWCOffset(1)) / 100.;
	double tank_zcentre = (wcsimrootgeom->GetWCOffset(2)) / 100.;
	Position tank_centre(tank_xcentre, tank_ycentre, tank_zcentre);
	double tank_radius = (wcsimrootgeom->GetWCCylRadius()) / 100.;
	double tank_halfheight = (wcsimrootgeom->GetWCCylLength()) / 100.;
	// geometry variables not yet in wcsimrootgeom are in MRDSpecs.hh
	double mrd_width  =  (MRDSpecs::MRD_width)  / 100.;
	double mrd_height =  (MRDSpecs::MRD_height) / 100.;
	double mrd_depth  =  (MRDSpecs::MRD_depth)  / 100.;
	double mrd_start  =  (MRDSpecs::MRD_start)  / 100.;
	if(verbose>1) cout<<"we have "<<numtankpmts<<" tank pmts, "<<nummrdpmts
					  <<" mrd pmts and "<<numlappds<<" lappds"<<endl;
	
	// construct the ToolChain Goemetry
	// ================================
	Geometry* anniegeom = new Geometry(WCSimGeometryVer,
									   tank_centre,
									   tank_radius,
									   tank_halfheight,
									   mrd_width,
									   mrd_height,
									   mrd_depth,
									   mrd_start,
									   numtankpmts,
									   nummrdpmts,
									   numvetopmts,
									   numlappds,
									   geostatus::FULLY_OPERATIONAL);
	if(verbose>1){
		cout<<"constructed anniegeom at "<<anniegeom<<" with tank origin "; tank_centre.Print();
	}
	m_data->Stores.at("ANNIEEvent")->Header->Set("AnnieGeometry",anniegeom,true);
	
	// Construct the Detectors and Channels
	// ====================================
	// PMTs
	unsigned int ADC_Crate_Num = 0;
	unsigned int ADC_Card_Num  = 0;
	unsigned int ADC_Chan_Num = 0;
	unsigned int MT_Crate_Num = 0;
	unsigned int MT_Card_Num = 0;
	unsigned int MT_Chan_Num = 0;
	// LAPPDs
	unsigned int ACDC_Crate_Num = 0;
	unsigned int ACDC_Card_Num = 0;
	unsigned int ACDC_Chan_Num = 0;
	unsigned int ACC_Crate_Num = 0;
	unsigned int ACC_Card_Num = 0;
	unsigned int ACC_Chan_Num = 0;
	// TDCs
	unsigned int TDC_Crate_Num = 0;
	unsigned int TDC_Card_Num = 0;
	unsigned int TDC_Chan_Num = 0;
	// HV
	unsigned int CAEN_HV_Crate_Num = 0;
	unsigned int CAEN_HV_Card_Num = 0;
	unsigned int CAEN_HV_Chan_Num = 0;
	unsigned int LeCroy_HV_Crate_Num = 0;
	unsigned int LeCroy_HV_Card_Num = 0;
	unsigned int LeCroy_HV_Chan_Num = 0;
	unsigned int LAPPD_HV_Crate_Num = 0;
	unsigned int LAPPD_HV_Card_Num = 0;
	unsigned int LAPPD_HV_Chan_Num = 0;
	
	// lappds
	for(int lappdi=0; lappdi<numlappds; lappdi++){
		WCSimRootPMT anlappd = wcsimrootgeom->GetLAPPD(lappdi);
		
		// Construct the detector associated with this tile
		unsigned long uniquedetectorkey = anniegeom->ConsumeNextFreeDetectorKey();
		lappd_tubeid_to_detectorkey.emplace(anlappd.GetTubeNo(),uniquedetectorkey);
		detectorkey_to_lappdid.emplace(uniquedetectorkey,anlappd.GetTubeNo());
		Detector adet(uniquedetectorkey,
					  "LAPPD",
					  Position( anlappd.GetPosition(0)/100.,
					            anlappd.GetPosition(1)/100.,
					            anlappd.GetPosition(2)/100.),
					  Direction(anlappd.GetOrientation(0),
					            anlappd.GetOrientation(1),
					            anlappd.GetOrientation(2)),
					  anlappd.GetName(),
					  detectorstatus::ON,
					  0.);
		
		// construct all the channels associated with this LAPPD
		for(int stripi=0; stripi<LappdNumStrips; stripi++){
			unsigned long uniquechannelkey = anniegeom->ConsumeNextFreeChannelKey();
			
			int stripside = ((stripi%2)==0);   // StripSide=0 for LHS (x<0), StripSide=1 for RHS (x>0)
			int stripnum = (int)(stripi/2);    // Strip number: add 2 channels per strip as we go
			double xpos = (stripside) ? -LappdStripLength : LappdStripLength;
			double ypos = (stripnum*LappdStripSeparation) - ((LappdNumStrips*LappdStripSeparation)/2.);
			
			// fill up ADC cards and channels monotonically, they're arbitrary for simulation
			ACDC_Chan_Num++;
			if(ACDC_Chan_Num>=ACDC_CHANNELS_PER_CARD)  { ACDC_Chan_Num=0; ACDC_Card_Num++; ACC_Chan_Num++; }
			if(ACDC_Card_Num>=ACDC_CARDS_PER_CRATE)    { ACDC_Card_Num=0; ACDC_Crate_Num++; }
			if(ACC_Chan_Num>=ACC_CHANNELS_PER_CARD)    { ACC_Chan_Num=0; ACC_Card_Num++;    }
			if(ACC_Card_Num>=ACC_CARDS_PER_CRATE)      { ACC_Card_Num=0; ACC_Crate_Num++;   }
			// same for HV
			LAPPD_HV_Chan_Num++;
			if(LAPPD_HV_Chan_Num>=LAPPD_HV_CHANNELS_PER_CARD)    { LAPPD_HV_Chan_Num=0; LAPPD_HV_Card_Num++;  }
			if(LAPPD_HV_Card_Num>=LAPPD_HV_CARDS_PER_CRATE) { LAPPD_HV_Card_Num=0; LAPPD_HV_Crate_Num++; }
			
			Channel lappdchannel(uniquechannelkey,
								 Position(xpos,ypos,0.),
								 stripside,
								 stripnum,
								 ACDC_Crate_Num,
								 ACDC_Card_Num,
								 ACDC_Chan_Num,
								 ACC_Crate_Num,
								 ACC_Card_Num,
								 ACC_Chan_Num,
								 LAPPD_HV_Crate_Num,
								 LAPPD_HV_Card_Num,
								 LAPPD_HV_Chan_Num,
								 channelstatus::ON);
			
			// Add this channel to the geometry
			if(verbose>4) cout<<"Adding channel "<<uniquechannelkey<<" to detector "<<uniquedetectorkey<<endl;
			adet.AddChannel(lappdchannel);
		}
		if(verbose>4) cout<<"Adding detector "<<uniquedetectorkey<<" to geometry"<<endl;
		// Add this detector to the geometry
		anniegeom->AddDetector(adet);
		if(verbose>4) cout<<"printing geometry"<<endl;
		if(verbose>4) anniegeom->PrintChannels();
	}
	
	// tank PMTs
	for(int pmti=0; pmti<numtankpmts; pmti++){
		WCSimRootPMT apmt = wcsimrootgeom->GetPMT(pmti);
		
		// Construct the detector associated with this PMT
		unsigned long uniquedetectorkey = anniegeom->ConsumeNextFreeDetectorKey();
		Detector adet(uniquedetectorkey,
					  "Tank",
					  Position( apmt.GetPosition(0)/100.,
					            apmt.GetPosition(1)/100.,
					            apmt.GetPosition(2)/100.),
					  Direction(apmt.GetOrientation(0),
					            apmt.GetOrientation(1),
					            apmt.GetOrientation(2)),
					  apmt.GetName(),
					  detectorstatus::ON,
					  0.);
		
		// construct the channel associated with this PMT
		unsigned long uniquechannelkey = anniegeom->ConsumeNextFreeChannelKey();
		pmt_tubeid_to_channelkey.emplace(apmt.GetTubeNo(), uniquechannelkey);
		channelkey_to_pmtid.emplace(uniquechannelkey,apmt.GetTubeNo());
		
		// fill up ADC cards and channels monotonically, they're arbitrary for simulation
		ADC_Chan_Num++;
		if(ADC_Chan_Num>=ADC_CHANNELS_PER_CARD)  { ADC_Chan_Num=0; ADC_Card_Num++; MT_Chan_Num++; }
		if(ADC_Card_Num>=ADC_CARDS_PER_CRATE)    { ADC_Card_Num=0; ADC_Crate_Num++; }
		if(MT_Chan_Num>=MT_CHANNELS_PER_CARD)    { MT_Chan_Num=0; MT_Card_Num++; }
		if(MT_Card_Num>=MT_CARDS_PER_CRATE)      { MT_Card_Num=0; MT_Crate_Num++; }
		// same for HV
		CAEN_HV_Chan_Num++;
		if(CAEN_HV_Chan_Num>=CAEN_HV_CHANNELS_PER_CARD)    { CAEN_HV_Chan_Num=0; CAEN_HV_Card_Num++;  }
		if(CAEN_HV_Card_Num>=CAEN_HV_CARDS_PER_CRATE) { CAEN_HV_Card_Num=0; CAEN_HV_Crate_Num++; }
		
		Channel pmtchannel( uniquechannelkey,
							Position(0,0,0.),
							0, // stripside
							0, // stripnum
							ADC_Crate_Num,
							ADC_Card_Num,
							ADC_Chan_Num,
							MT_Crate_Num,
							MT_Card_Num,
							MT_Chan_Num,
							CAEN_HV_Crate_Num,
							CAEN_HV_Card_Num,
							CAEN_HV_Chan_Num,
							channelstatus::ON);
		
		// Add this channel to the geometry
		if(verbose>4) cout<<"Adding channel "<<uniquechannelkey<<" to detector "<<uniquedetectorkey<<endl;
		adet.AddChannel(pmtchannel);
		
		// Add this detector to the geometry
		if(verbose>4) cout<<"Adding detector "<<uniquedetectorkey<<" to geometry"<<endl;
		anniegeom->AddDetector(adet);
		if(verbose>4) cout<<"printing geometry"<<endl;
		if(verbose>4) anniegeom->PrintChannels();
	}
	
	// mrd PMTs
	for(int mrdpmti=0; mrdpmti<nummrdpmts; mrdpmti++){
		WCSimRootPMT apmt = wcsimrootgeom->GetMRDPMT(mrdpmti);
		
		// Construct the detector associated with this PMT
		unsigned long uniquedetectorkey = anniegeom->ConsumeNextFreeDetectorKey();
		Detector adet(uniquedetectorkey,
					  "MRD",
					  Position( apmt.GetPosition(0)/100.,
					            apmt.GetPosition(1)/100.,
					            apmt.GetPosition(2)/100.),
					  Direction(apmt.GetOrientation(0),
					            apmt.GetOrientation(1),
					            apmt.GetOrientation(2)),
					  apmt.GetName(),
					  detectorstatus::ON,
					  0.);
		
		// construct the channel associated with this PMT
		unsigned long uniquechannelkey = anniegeom->ConsumeNextFreeChannelKey();
		mrd_tubeid_to_channelkey.emplace(apmt.GetTubeNo(), uniquechannelkey);
		channelkey_to_mrdpmtid.emplace(uniquechannelkey, apmt.GetTubeNo());
		
		// fill up TDC cards and channels monotonically, they're arbitrary for simulation
		TDC_Chan_Num++;
		if(TDC_Chan_Num>=TDC_CHANNELS_PER_CARD)  { TDC_Chan_Num=0; TDC_Card_Num++;  }
		if(TDC_Card_Num>=TDC_CARDS_PER_CRATE)    { TDC_Card_Num=0; TDC_Crate_Num++; }
		// same for HV
		LeCroy_HV_Chan_Num++;
		if(LeCroy_HV_Chan_Num>=LECROY_HV_CHANNELS_PER_CARD)    { LeCroy_HV_Chan_Num=0; LeCroy_HV_Card_Num++;  }
		if(LeCroy_HV_Card_Num>=LECROY_HV_CARDS_PER_CRATE) { LeCroy_HV_Card_Num=0; LeCroy_HV_Crate_Num++; }
		
		Channel pmtchannel( uniquechannelkey,
							Position(0,0,0.),
							0, // stripside
							0, // stripnum
							TDC_Crate_Num,
							TDC_Card_Num,
							TDC_Chan_Num,
							-1,                 // TDC has no level 2 signal handling
							-1,
							-1,
							LeCroy_HV_Crate_Num,
							LeCroy_HV_Card_Num,
							LeCroy_HV_Chan_Num,
							channelstatus::ON);
		
		// Add this channel to the geometry
		if(verbose>4) cout<<"Adding channel "<<uniquechannelkey<<" to detector "<<uniquedetectorkey<<endl;
		adet.AddChannel(pmtchannel);
		
		// Add this detector to the geometry
		if(verbose>4) cout<<"Adding detector "<<uniquedetectorkey<<" to geometry"<<endl;
		anniegeom->AddDetector(adet);
		if(verbose>4) cout<<"printing geometry"<<endl;
		if(verbose>4) anniegeom->PrintChannels();
	}
	
	// veto PMTs
	for(int faccpmti=0; faccpmti<numvetopmts; faccpmti++){
		WCSimRootPMT apmt = wcsimrootgeom->GetMRDPMT(faccpmti);
		
		// Construct the detector associated with this PMT
		unsigned long uniquedetectorkey = anniegeom->ConsumeNextFreeDetectorKey();
		Detector adet(uniquedetectorkey,
					  "Veto",
					  Position( apmt.GetPosition(0)/100.,
					            apmt.GetPosition(1)/100.,
					            apmt.GetPosition(2)/100.),
					  Direction(apmt.GetOrientation(0),
					            apmt.GetOrientation(1),
					            apmt.GetOrientation(2)),
					  apmt.GetName(),
					  detectorstatus::ON,
					  0.);
		
		// construct the channel associated with this PMT
		unsigned long uniquechannelkey = anniegeom->ConsumeNextFreeChannelKey();
		facc_tubeid_to_channelkey.emplace(apmt.GetTubeNo(), uniquechannelkey);
		channelkey_to_faccpmtid.emplace(uniquechannelkey, apmt.GetTubeNo());
		
		// fill up TDC cards and channels monotonically, they're arbitrary for simulation
		TDC_Chan_Num++;
		if(TDC_Chan_Num>=TDC_CHANNELS_PER_CARD)  { TDC_Chan_Num=0; TDC_Card_Num++;  }
		if(TDC_Card_Num>=TDC_CARDS_PER_CRATE)    { TDC_Card_Num=0; TDC_Crate_Num++; }
		// same for HV
		LeCroy_HV_Chan_Num++;
		if(LeCroy_HV_Chan_Num>=LECROY_HV_CHANNELS_PER_CARD)    { LeCroy_HV_Chan_Num=0; LeCroy_HV_Card_Num++;  }
		if(LeCroy_HV_Card_Num>=LECROY_HV_CARDS_PER_CRATE) { LeCroy_HV_Card_Num=0; LeCroy_HV_Crate_Num++; }
		
		Channel pmtchannel( uniquechannelkey,
							Position(0,0,0.),
							0, // stripside
							0, // stripnum
							TDC_Crate_Num,
							TDC_Card_Num,
							TDC_Chan_Num,
							-1,                 // TDC has no level 2 signal handling
							-1,
							-1,
							LeCroy_HV_Crate_Num,
							LeCroy_HV_Card_Num,
							LeCroy_HV_Chan_Num,
							channelstatus::ON);
		
		// Add this channel to the geometry
		if(verbose>4) cout<<"Adding channel "<<uniquechannelkey<<" to detector "<<uniquedetectorkey<<endl;
		adet.AddChannel(pmtchannel);
		
		// Add this detector to the geometry
		if(verbose>4) cout<<"Adding detector "<<uniquedetectorkey<<" to geometry"<<endl;
		anniegeom->AddDetector(adet);
		if(verbose>4) cout<<"printing geometry"<<endl;
		if(verbose>4) anniegeom->PrintChannels();
	}
	
	// for other WCSim tools that may need the WCSim Tube IDs
	m_data->CStore.Set("lappd_tubeid_to_detectorkey",lappd_tubeid_to_detectorkey);
	m_data->CStore.Set("pmt_tubeid_to_channelkey",pmt_tubeid_to_channelkey);
	m_data->CStore.Set("mrd_tubeid_to_channelkey",mrd_tubeid_to_channelkey);
	m_data->CStore.Set("facc_tubeid_to_channelkey",facc_tubeid_to_channelkey);
	// inverse
	m_data->CStore.Set("detectorkey_to_lappdid",detectorkey_to_lappdid);
	m_data->CStore.Set("channelkey_to_pmtid",channelkey_to_pmtid);
	m_data->CStore.Set("channelkey_to_mrdpmtid",channelkey_to_mrdpmtid);
	m_data->CStore.Set("channelkey_to_faccpmtid",channelkey_to_faccpmtid);
	
}
