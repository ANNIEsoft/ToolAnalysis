#include "DigitBuilder.h"


static DigitBuilder* fgDigitBuilder = 0;
DigitBuilder* DigitBuilder::Instance()
{
  if( !fgDigitBuilder ){
    fgDigitBuilder = new DigitBuilder();
  }

  return fgDigitBuilder;
}

DigitBuilder::DigitBuilder():Tool(){}
DigitBuilder::~DigitBuilder() {
	
}

bool DigitBuilder::Initialise(std::string configfile, DataModel &data){
	// Reset
	//this->Reset();

  /////////////////// Usefull header ///////////////////////
  if(verbosity) cout<<"Initializing Tool DigitBuilder"<<endl;
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  
  /// Get the Tool configuration variables
	m_variables.Get("verbosity",verbosity);
	Log("Initializing Tool DigitBuilder",v_message,verbosity);
	
	/// Construct the other objects we'll be setting at event level,
	fDigitList = new std::vector<RecoDigit>;
		
  fMRDTrackLengthMax = 0.;
		
	// Make the RecoDigit Store if it doesn't exist
	int recoeventexists = m_data->Stores.count("RecoEvent");
	if(recoeventexists==0) m_data->Stores["RecoEvent"] = new BoostStore(false,2);
	
  return true;
}

bool DigitBuilder::Execute(){
	
	if(verbosity) cout<<"Executing tool DigitBuilder with MC entry "<<fMCEventNum<<", trigger "<<fMCTriggernum<<endl;
	get_ok = m_data->Stores.count("ANNIEEvent");
	if(!get_ok){
		Log("DigitBuilder Tool: No ANNIEEvent store!",v_error,verbosity);
		return false;
	};
	
	/// First, see if this is a delayed trigger in the event
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum",fMCTriggernum);
	if(not get_ok){ Log("DigitBuilder Tool: Error retrieving MCTriggernum from ANNIEEvent!",v_error,verbosity); return false; }
	
	/// if so, truth analysis is probably not interested in this trigger. Primary muon will not be in the listed tracks.
	if(fMCTriggernum>0){ Log("DigitBuilder Tool: Skipping delayed trigger",v_debug,verbosity); return true;}
	
	/// Retrieve the hit info from ANNIEEvent
	get_ok = m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",fGeometry);
	if(not get_ok){
		Log("DigitBuilder Tool: Error retrieving Geometry,true from ANNIEEvent!",v_error,verbosity); \
		return false; 
	}
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCHits",fMCHits);
	if(not get_ok){ 
		Log("DigitBuilder Tool: Error retrieving MCHits,true from ANNIEEvent!",v_error,verbosity); 
		return false; 
	}
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCLAPPDHits",fMCLAPPDHits);
	if(not get_ok){
		Log("DigitBuilder Tool: Error retrieving MCLAPPDHits,true from ANNIEEvent!",v_error,verbosity); 
		return false;
	}
	
	/// Retrieve the reconstructed information from the MRD
	
	int mrdtrackexists = m_data->Stores.count("MRDTracks");
	if(mrdtrackexists == 0) {
	  Log("DigitBuilder Tool: MRDTracks store doesn't exist!",v_message,verbosity);
	  Log("DigitBuilder Tool: Continue without MRD event selection!",v_message,verbosity);
	  // Reset Digit infomation
	  this->Reset();
	  cout<<"DEBUG: DigitBuilder::Execute: Number of digit after reset = "<<fDigitList->size()<<endl;
	  // Build RecoDigit
	  this->BuildRecoDigit();
	  cout<<"DEBUG: DigitBuilder::Execute: Number of digit after build = "<<fDigitList->size()<<endl;
	  // Push to RecoEvent
	  this->PushRecoDigits(true); 	
	  cout<<"DEBUG: DigitBuilder::Execute: Number of digit after push = "<<fDigitList->size()<<endl;
	  return true;
	}
	else {
	  /// Get number of subentries
	  int TotalEntries = 0;
	  double mrdtracklength = 0.;
	  int mrdlayershit = 0;
	  
	  get_ok = m_data->Stores["MRDTracks"]->Header->Get("TotalEntries",TotalEntries);
	  if(not get_ok) {
	    Log("DigitBuilder Tool: Error retrieving MRDTracks header!",v_error,verbosity); 
	  	return false;	
	  }
	  logmessage = "DigitBuilder Tool: Found " + to_string(TotalEntries) + "entries of MRD tracks";
	  Log(logmessage,v_message,verbosity);
	  
	  // If mrd track isn't found
	  if(TotalEntries == 0) {
	    Log("DigitBuilder Tool: Found no MRD Tracks",v_message,verbosity);
	    return true;
	  }
	  // If mrd track is found
	  for(int entrynum=0; entrynum<TotalEntries; entrynum++) {
	    m_data->Stores["MRDTracks"]->GetEntry(entrynum);
	    m_data->Stores.at("MRDTracks")->Get("TrackLength",mrdtracklength);
	    m_data->Stores.at("MRDTracks")->Get("LayersHit",mrdlayershit);
	    if(fMRDTrackLengthMax<mrdtracklength) fMRDTrackLengthMax = mrdtracklength;
	  }
	  /// Print the information.
	  logmessage = "DigitBuilder Tool: Processing truth tracks and digits for "+fMCFile
	  			+", MCEvent "+to_string(fMCEventNum)+", MCTrigger "+to_string(fMCTriggernum);
	  Log(logmessage,v_debug,verbosity);
	  
	  // Reset Digit infomation
	  this->Reset();
	  // Build RecoDigit
	  this->BuildRecoDigit();
	  this->PushRecoDigits(true);
	  std::cout<<std::endl<<std::endl;
	  
    return true;
  }
}

bool DigitBuilder::Finalise(){
  return true;
}

bool DigitBuilder::BuildRecoDigit() {
	this->BuildPMTRecoDigit();
	this->BuildLAPPDRecoDigit();
  	
	return true;
}

bool DigitBuilder::BuildPMTRecoDigit() {
	Log("DigitBuilder Tool: Build PMT reconstructed digits",v_message,verbosity);
	// now move to digit retrieval
	int region = -999;
	double calT;
	double calQ = 0.;
	int digitType = -999;
	Detector det;
	Position  pos_sim, pos_reco;
	// MCHits is a std::map<ChannelKey,std::vector<Hit>>
	if(fMCHits){
		Log("DigitBuilder Tool: Num PMT Digits = "+to_string(fMCHits->size()),v_message, verbosity);
		// iterate over the map of sensors with a measurement
		for(std::pair<ChannelKey,std::vector<Hit>>&& apair : *fMCHits){
			ChannelKey chankey = apair.first;
			// a ChannelKey is a detector descriptor, containing 2 elements:
			// a 'subdetector' (enum class), with types ADC, LAPPD, TDC
			// and a DetectorElementIndex, i.e. the ID of the detector of that type
			// get PMT position
			det = fGeometry.GetDetector(chankey);
			if(det.GetDetectorElement() == "") {
				Log("DigitBuilder Tool: Detector not found! ",v_message,verbosity);
				continue;
			}
			
			// convert the WCSim coordinates to the ANNIEreco coordinates
			pos_sim = det.GetDetectorPosition();	
			pos_reco.SetX(pos_sim.X());
			pos_reco.SetY(pos_sim.Y()+14.46469);
			pos_reco.SetZ(pos_sim.Z()-168.1);
	
			if(chankey.GetSubDetectorType()==subdetector::ADC){
				std::vector<Hit>& hits = apair.second;	
			  for(Hit& ahit : hits){
			  	//ahit.Print();
					//if(v_message<verbosity) ahit.Print(); // << VERY verbose
					// get calibrated PMT time (Use the MC time for now)
					calT = ahit.GetTime().GetNs()*1.0;
					calQ = ahit.GetCharge();
					digitType = RecoDigit::PMT8inch;
					RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType);
				  //if(v_message<verbosity) recoDigit.Print();
				  fDigitList->push_back(recoDigit); 
        }
			}
		} // end loop over MCHits
	} else {
		cout<<"No MCHits"<<endl;
		return false;
	}
	return true;
}

bool DigitBuilder::BuildLAPPDRecoDigit() {
	Log("DigitBuilder Tool: Build LAPPD reconstructed digits",v_message,verbosity);
	int region = -999;
	double calT = 0;
	double calQ = 0;
	int digitType = -999;
	Detector det;
	Position  pos_sim, pos_reco;
  // repeat for LAPPD hits
	// MCLAPPDHits is a std::map<ChannelKey,std::vector<LAPPDHit>>
	if(fMCLAPPDHits){
		Log("DigitBuilder Tool: Num LAPPD Digits = "+to_string(fMCLAPPDHits->size()),v_message,verbosity);
		// iterate over the map of sensors with a measurement
		for(std::pair<ChannelKey,std::vector<LAPPDHit>>&& apair : *fMCLAPPDHits){
			ChannelKey chankey = apair.first;
			if(chankey.GetSubDetectorType()==subdetector::LAPPD){ // redundant
				std::vector<LAPPDHit>& hits = apair.second;
				for(LAPPDHit& ahit : hits){
					//if(v_message<verbosity) ahit.Print(); // << VERY verbose
					// an LAPPDHit has adds (global x-y-z) position, (in-tile x-y) local position
					// and time psecs
					// convert the WCSim coordinates to the ANNIEreco coordinates
					pos_reco.SetX(ahit.GetPosition().at(0)); //cm
					pos_reco.SetY(ahit.GetPosition().at(1)+14.4649); //cm
					pos_reco.SetZ(ahit.GetPosition().at(2)-168.1); //cm
					calT = ahit.GetTpsec()/1000 + 950.0;  // Add 950 ns offset relative to the trigger
					calQ = ahit.GetCharge();
					digitType = RecoDigit::lappd_v0;
					RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType);
					//if(v_message<verbosity) recoDigit.Print();
				  fDigitList->push_back(recoDigit);
				}
			}
		} // end loop over MCLAPPDHits
	} else {
		cout<<"No MCLAPPDHits"<<endl;
		return false;
	}	
	return true;
}

void DigitBuilder::PushRecoDigits(bool savetodisk) {
	Log("DigitBuilder Tool: Push reconstructed digits to the RecoEvent store",v_message,verbosity);
	m_data->Stores.at("RecoEvent")->Set("RecoDigit", fDigitList, savetodisk);  ///> Add digits to RecoEvent
	// this should be everything. save the entry to the BoostStore
	m_data->Stores.at("RecoEvent")->Save();
}

void DigitBuilder::Reset() {
  fDigitList->clear();
}