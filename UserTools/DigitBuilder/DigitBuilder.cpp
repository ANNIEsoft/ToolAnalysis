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
  if(fverbosity) cout<<"Initializing Tool DigitBuilder"<<endl;
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  
  /// Get the Tool configuration variables
	m_variables.Get("verbosity",fverbosity);
	Log("Initializing Tool DigitBuilder",fv_message,fverbosity);
	
	/// Construct the other objects we'll be setting at event level,
	/// pass managed pointers to the ANNIEEvent Store
	fDigitList = new std::vector<RecoDigit*>;
	
  return true;
}

bool DigitBuilder::Execute(){
	
	if(fverbosity) cout<<"Executing tool DigitBuilder with MC entry "<<fMCEventNum<<", trigger "<<fMCTriggernum<<endl;
	Log("DigitBuilder Tool: Executing",fv_debug,fverbosity);
	fget_ok = m_data->Stores.count("ANNIEEvent");
	if(!fget_ok){
		Log("DigitBuilder Tool: No ANNIEEvent store!",fv_error,fverbosity);
		return false;
	};
	
	/// First, see if this is a delayed trigger in the event
	fget_ok = m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum",fMCTriggernum);
	if(not fget_ok){ Log("DigitBuilder Tool: Error retrieving MCTriggernum from ANNIEEvent!",fv_error,fverbosity); return false; }
	
	/// if so, truth analysis is probably not interested in this trigger. Primary muon will not be in the listed tracks.
	if(fMCTriggernum>0){ Log("DigitBuilder Tool: Skipping delayed trigger",fv_debug,fverbosity); return true;}
	
	/// Retrieve the hit info from ANNIEEvent
	fget_ok = m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",fGeometry);
	if(not fget_ok){ Log("DigitBuilder Tool: Error retrieving Geometry,true from ANNIEEvent!",fv_error,fverbosity); return false; }
	fget_ok = m_data->Stores.at("ANNIEEvent")->Get("MCHits",fMCHits);
	if(not fget_ok){ Log("DigitBuilder Tool: Error retrieving MCHits,true from ANNIEEvent!",fv_error,fverbosity); return false; }
	fget_ok = m_data->Stores.at("ANNIEEvent")->Get("MCLAPPDHits",fMCLAPPDHits);
	if(not fget_ok){ Log("DigitBuilder Tool: Error retrieving MCLAPPDHits,true from ANNIEEvent!",fv_error,fverbosity); return false; }
  
	/// Print the information.
	flogmessage = "DigitBuilder Tool: Processing truth tracks and digits for "+fMCFile
				+", MCEvent "+to_string(fMCEventNum)+", MCTrigger "+to_string(fMCTriggernum);
	Log(flogmessage,fv_debug,fverbosity);
	
	// Reset Digit infomation
	this->Reset();
	// Build RecoDigit
	this->BuildRecoDigit();
	this->PushRecoDigits(false);
	
  return true;
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
	// now move to digit retrieval
	int region;
	TimeClass calT;
	double calQ;
	int digitType;
	Detector det;
	Position  pos_sim, pos_reco;
	// MCHits is a std::map<ChannelKey,std::vector<Hit>>
	if(fMCHits){
		Log("DigitBuilderool: Num PMT Digits = "+to_string(fMCHits->size()),fv_message, fverbosity);
		// iterate over the map of sensors with a measurement
		for(std::pair<ChannelKey,std::vector<Hit>>&& apair : *fMCHits){
			ChannelKey chankey = apair.first;
			// a ChannelKey is a detector descriptor, containing 2 elements:
			// a 'subdetector' (enum class), with types ADC, LAPPD, TDC
			// and a DetectorElementIndex, i.e. the ID of the detector of that type
			// get PMT position
			det = fGeometry.GetDetector(chankey);
			if(det.GetDetectorElement() == "") {
				Log("DigitBuilder Tool: Detector not found! ",fv_message,fverbosity);
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
			  	ahit.Print();
					//if(v_message<verbosity) ahit.Print(); // << VERY verbose
					// get calibrated PMT time (Use the MC time for now)
					calT = ahit.GetTime();
					calQ = ahit.GetCharge();
					digitType = RecoDigit::PMT8inch;
					RecoDigit* recoDigit = new RecoDigit(region, pos_reco, calT, calQ, digitType);
				  //recoDigit->Print();
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
	int region;
	TimeClass calT;
	double calQ;
	int digitType;
	Detector det;
	Position  pos_sim, pos_reco;
  // repeat for LAPPD hits
	// MCLAPPDHits is a std::map<ChannelKey,std::vector<LAPPDHit>>
	if(fMCLAPPDHits){
		Log("DigitBuilder Tool: Num LAPPD Digits = "+to_string(fMCLAPPDHits->size()),fv_message,fverbosity);
		// iterate over the map of sensors with a measurement
		for(std::pair<ChannelKey,std::vector<LAPPDHit>>&& apair : *fMCLAPPDHits){
			ChannelKey chankey = apair.first;
			if(chankey.GetSubDetectorType()==subdetector::LAPPD){ // redundant
				std::vector<LAPPDHit>& hits = apair.second;
				for(LAPPDHit& ahit : hits){
					ahit.Print();
					//if(v_message<verbosity) ahit.Print(); // << VERY verbose
					// an LAPPDHit has adds (global x-y-z) position, (in-tile x-y) local position
					// and time psecs
					pos_reco.SetX(ahit.GetPosition().at(0)); //cm
					pos_reco.SetY(ahit.GetPosition().at(1)+14.4649); //cm
					pos_reco.SetZ(ahit.GetPosition().at(2)-168.1); //cm
					calT = ahit.GetTime();
					calQ = ahit.GetCharge();
					digitType = RecoDigit::lappd_v0;
					RecoDigit* recoDigit = new RecoDigit(region, pos_reco, calT, calQ, digitType);
					//recoDigit->Print();
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
	m_data->Stores.at("ANNIEEvent")->Set("RecoDigit", fDigitList, savetodisk);  ///> Add digits to ANNIEEvent
}

void DigitBuilder::Reset() {
  fDigitList->clear();
}