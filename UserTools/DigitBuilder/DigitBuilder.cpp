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

  /////////////////// Usefull header ///////////////////////
  if(verbosity) cout<<"Initializing Tool DigitBuilder"<<endl;
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  fPhotodetectorConfiguration = "All";
  
  /// Get the Tool configuration variables
	m_variables.Get("verbosity",verbosity);
	m_variables.Get("PhotoDetectorConfiguration", fPhotodetectorConfiguration);
	
	/// Construct the other objects we'll be setting at event level,
	fDigitList = new std::vector<RecoDigit>;
		
	// Make the RecoDigit Store if it doesn't exist
	int recoeventexists = m_data->Stores.count("RecoEvent");
	if(recoeventexists==0) m_data->Stores["RecoEvent"] = new BoostStore(false,2);
  return true;
}

bool DigitBuilder::Execute(){
	Log("===========================================================================================",v_debug,verbosity);
	
	/// Reset everything
	this->Reset();
	
	// see if "ANNIEEvent" exists
	auto get_annieevent = m_data->Stores.count("ANNIEEvent");
	if(!get_annieevent){
		Log("DigitBuilder Tool: No ANNIEEvent store!",v_error,verbosity); 
		return false;
	};
	
	/// see if "RecoEvent" exists
 	auto get_recoevent = m_data->Stores.count("RecoEvent");
 	if(!get_recoevent){
  		Log("EventSelector Tool: No RecoEvent store!",v_error,verbosity); 
  		return false;
	};
	
	/// First, see if this is a delayed trigger in the event
	auto get_mctrigger = m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum",fMCTriggernum);
	if(!get_mctrigger){ 
		Log("DigitBuilder Tool: Error retrieving MCTriggernum from ANNIEEvent!",v_error,verbosity); 
		return false; 
	}
	
	/// if so, truth analysis is probably not interested in this trigger. Primary muon will not be in the listed tracks.
	if(fMCTriggernum>0){ 
		Log("DigitBuilder Tool: Skipping delayed trigger",v_debug,verbosity); 
		return true;
	}
	
	/// check if EventCutStatus exists
  auto get_evtcutstatus = m_data->Stores.at("RecoEvent")->Get("EventCutStatus",fEventCutStatus);
	if(!get_evtcutstatus){
		Log("DigitBuilder Tool: Error retrieving EventCutStatus from RecoEvent! Need to run EventSelector first!", v_error,verbosity);
		return false;

	  /// Check if event passed all cuts checked with EventSelector
	  if(!fEventCutStatus){
	     Log("DigitBuilder Tool: Event doesn't pass all event cuts from EventSelector",v_message,verbosity);
	     return true;
	  }
  } 
  
  /// Retrieve the hit info from ANNIEEvent
	auto get_geometry= m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",fGeometry);
	if(!get_geometry){
		Log("DigitBuilder Tool: Error retrieving Geometry from ANNIEEvent!",v_error,verbosity); 
		return false; 
	}
	auto get_mchits = m_data->Stores.at("ANNIEEvent")->Get("MCHits",fMCHits);
	if(!get_mchits){ 
		Log("DigitBuilder Tool: Error retrieving MCHits from ANNIEEvent!",v_error,verbosity); 
		return false; 
	}
	auto get_mclappdhits = m_data->Stores.at("ANNIEEvent")->Get("MCLAPPDHits",fMCLAPPDHits);
	if(!get_mclappdhits){
		Log("DigitBuilder Tool: Error retrieving MCLAPPDHits from ANNIEEvent!",v_error,verbosity); 
		return false;
	}
	 
	/// Build RecoDigit
	this->BuildRecoDigit();
	
	if(fDigitList->size()<4) {
		Log("DigitBuilder Tool: Event has less than 4 digits",v_message,verbosity);
		return true;
	}
	
	/// Push recodigits to RecoEvent
	this->PushRecoDigits(true); 
	
  return true;
}

bool DigitBuilder::Finalise(){
	delete fDigitList; fDigitList = 0;
	if(verbosity>0) cout<<"DigitBuilder exitting"<<endl;
  return true;
}

bool DigitBuilder::BuildRecoDigit() {
	
	if(fPhotodetectorConfiguration == "PMT_only") {
		this->BuildPMTRecoDigit();
		return true;
	}
	if(fPhotodetectorConfiguration == "LAPPD_only") {
		this->BuildLAPPDRecoDigit();
		return true;
	}
	if(fPhotodetectorConfiguration == "All") {
		this->BuildPMTRecoDigit();
	  this->BuildLAPPDRecoDigit();
	  return true;
	}
	else {
	  cout<<"Wrong PhotoDetector Configuration! Allowed configurations: PMT_only, LAPPD_only, All"<<endl;
	  return false;
	}
	
}

bool DigitBuilder::BuildPMTRecoDigit() {
	
	Log("DigitBuilder Tool: Build PMT reconstructed digits",v_message,verbosity);
	/// now move to digit retrieval
	int region = -999;
	double calT;
	double calQ = 0.;
	int digitType = -999;
	Detector det;
	Position  pos_sim, pos_reco;
	/// MCHits is a std::map<ChannelKey,std::vector<Hit>>
	if(fMCHits){
		Log("DigitBuilder Tool: Num PMT Digits = "+to_string(fMCHits->size()),v_message, verbosity);
		/// iterate over the map of sensors with a measurement
		for(std::pair<ChannelKey,std::vector<Hit>>&& apair : *fMCHits){
			ChannelKey chankey = apair.first;
			// a ChannelKey is a detector descriptor, containing 2 elements:
			// a 'subdetector' (enum class), with types ADC, LAPPD, TDC
			// and a DetectorElementIndex, i.e. the ID of the detector of that type
			// get PMT position
			det = fGeometry.GetDetector(chankey);
			int PMTId = det.GetDetectorId();
			if(det.GetDetectorElement() == "") {
				Log("DigitBuilder Tool: Detector not found! ",v_message,verbosity);
				continue;
			}
			
			// convert the WCSim coordinates to the ANNIEreco coordinates
			// convert the unit from m to cm
			pos_sim = det.GetDetectorPosition();	
			pos_sim.UnitToCentimeter();
			pos_reco.SetX(pos_sim.X());
			pos_reco.SetY(pos_sim.Y()+14.46469);
			pos_reco.SetZ(pos_sim.Z()-168.1);
	
			if(chankey.GetSubDetectorType()==subdetector::ADC){
				std::vector<Hit>& hits = apair.second;	
			  for(Hit& ahit : hits){
			  	//ahit.Print();
					//if(v_message<verbosity) ahit.Print(); // << VERY verbose
					// get calibrated PMT time (Use the MC time for now)
					//calT = ahit.GetTime()*1.0;
					calT = ahit.GetTime()*1.0 - 950.0; // remove 950 ns offs
					calQ = ahit.GetCharge();
					digitType = RecoDigit::PMT8inch;
					RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType, PMTId);
				  //recoDigit.Print();
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
        int LAPPDId = -1;
	Detector det;
	Position  pos_sim, pos_reco;
  // repeat for LAPPD hits
	// MCLAPPDHits is a std::map<ChannelKey,std::vector<LAPPDHit>>
	if(fMCLAPPDHits){
		Log("DigitBuilder Tool: Num LAPPD Digits = "+to_string(fMCLAPPDHits->size()),v_message,verbosity);
		// iterate over the map of sensors with a measurement
		for(std::pair<ChannelKey,std::vector<LAPPDHit>>&& apair : *fMCLAPPDHits){
			ChannelKey chankey = apair.first;
			det = fGeometry.GetDetector(chankey);
			//Tube ID is different from that in ANNIEReco
			LAPPDId = det.GetDetectorId();
			//if(LAPPDId != 266 && LAPPDId != 271 && LAPPDId != 236 && LAPPDId != 231 && LAPPDId != 206) continue;
			if(LAPPDId != 90 && LAPPDId != 83 && LAPPDId != 56 && LAPPDId != 59 && LAPPDId != 22) continue;
                        if(chankey.GetSubDetectorType()==subdetector::LAPPD){ // redundant
				std::vector<LAPPDHit>& hits = apair.second;
				for(LAPPDHit& ahit : hits){
					//if(v_message<verbosity) ahit.Print(); // << VERY verbose
					// an LAPPDHit has adds (global x-y-z) position, (in-tile x-y) local position
					// and time psecs
					// convert the WCSim coordinates to the ANNIEreco coordinates
					// convert the unit from m to cm
					pos_reco.SetX(ahit.GetPosition().at(0)*100.); //cm
					pos_reco.SetY(ahit.GetPosition().at(1)*100.+14.4649); //cm
					pos_reco.SetZ(ahit.GetPosition().at(2)*100.-168.1); //cm
					//calT = ahit.GetTime() + ahit.GetTpsec()/1000 + 950.;  // Add 950 ns offset
					calT = ahit.GetTime() + ahit.GetTpsec()/1000;  // 
					calT = frand.Gaus(calT, 0.1); // time is smeared with 100 ps time resolution. Harded-coded for now.
					calQ = ahit.GetCharge();
					// I found the charge is 0 for all the hits. In order to test the code, 
					// here I just set the charge to 1. We should come back to this later. (Jingbo Wang)
					calQ = 1.;
					digitType = RecoDigit::lappd_v0;
					RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType,LAPPDId);
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
}

void DigitBuilder::Reset() {
	// Reset 
  fDigitList->clear();
}
