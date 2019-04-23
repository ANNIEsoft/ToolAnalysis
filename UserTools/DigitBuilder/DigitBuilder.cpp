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
  
  ///////////////////// Defaults for Config ///////////////
  fPhotodetectorConfiguration = "All";
  fParametricModel = 0;

  /// Get the Tool configuration variables
	m_variables.Get("verbosity",verbosity);
	m_variables.Get("ParametricModel", fParametricModel);
	m_variables.Get("PhotoDetectorConfiguration", fPhotodetectorConfiguration);
  m_variables.Get("GetPionKaonInfo", fGetPiKInfo);
  m_variables.Get("LAPPDIDFile", fLAPPDIDFile);


	/// Construct the other objects we'll be setting at event level,
	fDigitList = new std::vector<RecoDigit>;
  fMuonStartVertex = new RecoVertex();
  fMuonStopVertex = new RecoVertex();


	// Make the RecoDigit Store if it doesn't exist
	int recoeventexists = m_data->Stores.count("RecoEvent");
	if(recoeventexists==0) m_data->Stores["RecoEvent"] = new BoostStore(false,2);
  
  // Some hard-coded values of old WCSim LAPPDIDs are in this Tool
  // I would recommend moving away from the use of WCSim IDs if possible as they are liable to change
  // but for tools that need them, in the LoadWCSim tool I put a map of WCSim TubeId to channelkey
  m_data->CStore.Get("detectorkey_to_lappdid",detectorkey_to_lappdid);
  m_data->CStore.Get("channelkey_to_pmtid",channelkey_to_pmtid);

  //Read the LAPPDID file, if given
  if(fLAPPDIDFile!="none"){
    this->ReadLAPPDIDFile();
  }  
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
	
	/// see if "RecoEvent" exists.  If not, make it
 	auto get_recoevent = m_data->Stores.count("RecoEvent");
 	if(!get_recoevent){
  		Log("DigitBuilder Tool: No RecoEvent store!",v_error,verbosity); 
  		return false;
	};
	
    /// Retrieve necessary info from ANNIEEvent
	auto get_geometry= m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",fGeometry);
	if(!get_geometry){
		Log("DigitBuilder Tool: Error retrieving Geometry from ANNIEEvent!",v_error,verbosity); 
		return false; 
	}

    auto get_mcparticles = m_data->Stores.at("ANNIEEvent")->Get("MCParticles",
            fMCParticles);
    if(!get_mcparticles){
      Log("EventSelector:: Tool: Error retrieving MCParticles from ANNIEEvent!",
              v_error,verbosity);
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

  /// If simulated data, Get MC Particle information
  this->FindTrueVertexFromMC();
  if (fGetPiKInfo) this->FindPionKaonCountFromMC();

  /// Build RecoDigit
  this->BuildRecoDigit();
	
  /// Push Particle & hit info. to RecoEvent
  this->PushRecoDigits(true); 
  this->PushTrueVertex(true);
  this->PushTrueStopVertex(true);

  return true;
}

bool DigitBuilder::Finalise(){
  delete fDigitList; fDigitList = 0;
  delete fMuonStartVertex;
  delete fMuonStopVertex;
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
	Detector* det=nullptr;
	Position  pos_sim, pos_reco;
	/// MCHits is a std::map<unsigned long,std::vector<Hit>>
	if(fMCHits){
		Log("DigitBuilder Tool: Num PMT Digits = "+to_string(fMCHits->size()),v_message, verbosity);
		/// iterate over the map of sensors with a measurement
		for(std::pair<unsigned long,std::vector<Hit>>&& apair : *fMCHits){
			unsigned long chankey = apair.first;
			// the channel key is a unique identifier of this signal input channel
			det = fGeometry->ChannelToDetector(chankey);
			int PMTId = channelkey_to_pmtid.at(chankey);  //PMTID In WCSim
			if(det==nullptr){
				Log("DigitBuilder Tool: Detector not found! ",v_message,verbosity);
				continue;
			}
			
			// convert the WCSim coordinates to the ANNIEreco coordinates
			// convert the unit from m to cm
			pos_sim = det->GetDetectorPosition();
			pos_sim.UnitToCentimeter();
			pos_reco.SetX(pos_sim.X());
			pos_reco.SetY(pos_sim.Y()+14.46469);
			pos_reco.SetZ(pos_sim.Z()-168.1);
	
			if(det->GetDetectorElement()=="Tank"){
				std::vector<Hit>& hits = apair.second;
        if(fParametricModel){
          //We'll get all hit info and then define a time/charge for each digit
          std::vector<double> hitTimes;
          std::vector<double> hitCharges;
          for(Hit& ahit : hits){
          	if(calT>-10 && calT<40) {
					    hitTimes.push_back(ahit.GetTime()*1.0); 
              hitCharges.push_back(ahit.GetCharge());
            }
          }
          // Do median and sum
          std::sort(hitTimes.begin(), hitTimes.end());
          size_t timesize = hitTimes.size();
          if (timesize == 0) continue;
          if (timesize % 2 == 0){
            calT = (hitTimes.at(timesize/2 - 1) + hitTimes.at(timesize/2))/2;
          } else {
            calT = hitTimes.at(timesize/2);
          }
          calQ = 0.;
          for(std::vector<double>::iterator it = hitCharges.begin(); it != hitCharges.end(); ++it){
            calQ += *it;
          }
          if(calQ>10) {
				    digitType = RecoDigit::PMT8inch;
				    RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType, PMTId);
				    fDigitList->push_back(recoDigit); 
				  }
        } else {
			    for(Hit& ahit : hits){
				  	//if(v_message<verbosity) ahit.Print(); // << VERY verbose
				  	// get calibrated PMT time (Use the MC time for now)
				  	calT = ahit.GetTime()*1.0; 
            calQ = ahit.GetCharge();
				  	digitType = RecoDigit::PMT8inch;
				  	RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType, PMTId);
				    //recoDigit.Print();
				    fDigitList->push_back(recoDigit); 
          }
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
	std::string name = "DigitBuilder::BuildLAPPDRecoDigit(): ";
	Log(name + " Build LAPPD reconstructed digits",v_message,verbosity);
	int region = -999;
	double calT = 0;
	double calQ = 0;
	int digitType = -999;
	Detector* det=nullptr;
	Position  pos_sim, pos_reco;
  // repeat for LAPPD hits
	// MCLAPPDHits is a std::map<unsigned long,std::vector<LAPPDHit>>
	if(fMCLAPPDHits){
		Log("DigitBuilder Tool: Num LAPPD Digits = "+to_string(fMCLAPPDHits->size()),v_message,verbosity);
		// iterate over the map of sensors with a measurement
		for(std::pair<unsigned long,std::vector<LAPPDHit>>&& apair : *fMCLAPPDHits){
			unsigned long chankey = apair.first;
			det = fGeometry->ChannelToDetector(chankey);
			if(det==nullptr){
				Log("DigitBuilder Tool: LAPPD Detector not found! ",v_message,verbosity);
				continue;
			}
			int detkey = det->GetDetectorID();
			int LAPPDId = detectorkey_to_lappdid.at(detkey);
      //Check if LAPPD is in selected LAPPDs
      bool isSelectedLAPPD = false;
      for(int i=0;i<fLAPPDId.size();i++){
			  if(LAPPDId == fLAPPDId.at(i)) isSelectedLAPPD=true;
      }
      if(!isSelectedLAPPD && fLAPPDId.size()>0) continue;
      if(verbosity>3){
        std::cout << "Loading in digit for LAPPDID " << LAPPDId << std::endl;
      }

			if(det->GetDetectorElement()=="LAPPD"){ // redundant, MCLAPPDHits are LAPPD hitss
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
					calT = ahit.GetTime();  // 
					calT = frand.Gaus(calT, 0.1); // time is smeared with 100 ps time resolution. Harded-coded for now.
					calQ = ahit.GetCharge();
					// I found the charge is 0 for all the hits. In order to test the code, 
					// here I just set the charge to 1. We should come back to this later. (Jingbo Wang)
					calQ = 1.;
					digitType = RecoDigit::lappd_v0;
					RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType,LAPPDId);
					//if(v_message<verbosity) recoDigit.Print();
				  //make some cuts here. It will be moved to the Hitcleaning tool
				  //if(calT>5) continue; // cut off delayed hits
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

void DigitBuilder::FindTrueVertexFromMC() {
  
  // loop over the MCParticles to find the highest enery primary muon
  // MCParticles is a std::vector<MCParticle>
  MCParticle primarymuon;  // primary muon
  bool mufound=false;
  if(fMCParticles){
    Log("DigitBuilder::  Tool: Num MCParticles = "+to_string(fMCParticles->size()),v_message,verbosity);
    for(int particlei=0; particlei<fMCParticles->size(); particlei++){
      MCParticle aparticle = fMCParticles->at(particlei);
      //if(v_debug<verbosity) aparticle.Print();       // print if we're being *really* verbose
      if(aparticle.GetParentPdg()!=0) continue;      // not a primary particle
      if(aparticle.GetPdgCode()!=13) continue;       // not a muon
      primarymuon = aparticle;                       // note the particle
      mufound=true;                                  // note that we found it
      //primarymuon.Print();
      break;                                         // won't have more than one primary muon
    }
  } else {
    Log("DigitBuilder::  Tool: No MCParticles in the event!",v_error,verbosity);
  }
  if(not mufound){
    Log("DigitBuilder::  Tool: No muon in this event",v_warning,verbosity);
    return;
  }
  
  // retrieve desired information from the particle
  Position muonstartpos = primarymuon.GetStartVertex();    // only true if the muon is primary
  double muonstarttime = primarymuon.GetStartTime();
  Position muonstoppos = primarymuon.GetStopVertex();    // only true if the muon is primary
  double muonstoptime = primarymuon.GetStopTime();
  
  Direction muondirection = primarymuon.GetStartDirection();
  double muonenergy = primarymuon.GetStartEnergy();
  // set true vertex
  // change unit
  muonstartpos.UnitToCentimeter(); // convert unit from meter to centimeter
  muonstoppos.UnitToCentimeter(); // convert unit from meter to centimeter
  // change coordinate for muon start vertex
  muonstartpos.SetY(muonstartpos.Y()+14.46469);
  muonstartpos.SetZ(muonstartpos.Z()-168.1);
  fMuonStartVertex->SetVertex(muonstartpos, muonstarttime);
  fMuonStartVertex->SetDirection(muondirection);
  //  charge coordinate for muon stop vertex
  muonstoppos.SetY(muonstoppos.Y()+14.46469);
  muonstoppos.SetZ(muonstoppos.Z()-168.1);
  fMuonStopVertex->SetVertex(muonstoppos, muonstoptime); 
  
  logmessage = "  trueVtx = (" +to_string(muonstartpos.X()) + ", " + to_string(muonstartpos.Y()) + ", " + to_string(muonstartpos.Z()) +", "+to_string(muonstarttime)+ "\n"
            + "           " +to_string(muondirection.X()) + ", " + to_string(muondirection.Y()) + ", " + to_string(muondirection.Z()) + ") " + "\n";
  
  Log(logmessage,v_debug,verbosity);
	logmessage = "  muonStop = ("+to_string(muonstoppos.X()) + ", " + to_string(muonstoppos.Y()) + ", " + to_string(muonstoppos.Z()) + ") "+ "\n";
	Log(logmessage,v_debug,verbosity);
}

void DigitBuilder::FindPionKaonCountFromMC() {
  
  // loop over the MCParticles to find the highest enery primary muon
  // MCParticles is a std::vector<MCParticle>
  bool pionfound=false;
  bool kaonfound=false;
  int pi0count = 0;
  int pipcount = 0;
  int pimcount = 0;
  int K0count = 0;
  int Kpcount = 0;
  int Kmcount = 0;
  if(fMCParticles){
    Log("DigitBuilder::  Tool: Num MCParticles = "+to_string(fMCParticles->size()),v_message,verbosity);
    for(int particlei=0; particlei<fMCParticles->size(); particlei++){
      MCParticle aparticle = fMCParticles->at(particlei);
      //if(v_debug<verbosity) aparticle.Print();       // print if we're being *really* verbose
      if(aparticle.GetParentPdg()!=0) continue;      // not a primary particle
      if(aparticle.GetPdgCode()==111){               // is a primary pi0
        pionfound = true;
        pi0count++;
      }
      if(aparticle.GetPdgCode()==211){               // is a primary pi+
        pionfound = true;
        pipcount++;
      }
      if(aparticle.GetPdgCode()==-211){               // is a primary pi-
        pionfound = true;
        pimcount++;
      }
      if(aparticle.GetParentPdg()!=0) continue;      // not a primary particle
      if(aparticle.GetPdgCode()==311){               // is a primary K0
        kaonfound = true;
        K0count++;
      }
      if(aparticle.GetPdgCode()==321){               // is a primary K+
        kaonfound = true;
        Kpcount++;
      }
      if(aparticle.GetPdgCode()==-321){               // is a primary K-
        kaonfound = true;
        Kmcount++;
      }
    }
  } else {
    Log("DigitBuilder::  Tool: No MCParticles in the event!",v_error,verbosity);
  }
  if(not pionfound){
    Log("DigitBuilder::  Tool: No primary pions in this event",v_warning,verbosity);
  }
  if(not kaonfound){
    Log("DigitBuilder::  Tool: No kaons in this event",v_warning,verbosity);
  }
  //Fill in pion counts for this event
  m_data->Stores.at("RecoEvent")->Set("MCPi0Count", pi0count);
  m_data->Stores.at("RecoEvent")->Set("MCPiPlusCount", pipcount);
  m_data->Stores.at("RecoEvent")->Set("MCPiMinusCount", pimcount);
  m_data->Stores.at("RecoEvent")->Set("MCK0Count", K0count);
  m_data->Stores.at("RecoEvent")->Set("MCKPlusCount", Kpcount);
  m_data->Stores.at("RecoEvent")->Set("MCKMinusCount", Kmcount);
}

void DigitBuilder::PushTrueVertex(bool savetodisk) {
  Log("DigitBuilder Tool: Push true vertex to the RecoEvent store",v_message,verbosity);
  m_data->Stores.at("RecoEvent")->Set("TrueVertex", fMuonStartVertex, savetodisk); 
}


void DigitBuilder::PushTrueStopVertex(bool savetodisk) {
  Log("DigitBuilder Tool: Push true stop vertex to the RecoEvent store",v_message,verbosity);
  m_data->Stores.at("RecoEvent")->Set("TrueStopVertex", fMuonStopVertex, savetodisk); 
}


void DigitBuilder::PushRecoDigits(bool savetodisk) {
	Log("DigitBuilder Tool: Push reconstructed digits to the RecoEvent store",v_message,verbosity);
	m_data->Stores.at("RecoEvent")->Set("RecoDigit", fDigitList, savetodisk);  ///> Add digits to RecoEvent
}

void DigitBuilder::Reset() {
	// Reset 
  fDigitList->clear();
  fMuonStartVertex->Reset();
  fMuonStopVertex->Reset();
}

void DigitBuilder::ReadLAPPDIDFile() {
  std::string line;
  ifstream myfile(fLAPPDIDFile);
  if (myfile.is_open()){
    while(getline(myfile,line)){
      if(verbosity>0){
        std::cout << "DigitBuilder tool: Loading hits from LAPPD ID " << line << std::endl;
      }
      int thisID = std::atoi(line.c_str());
      fLAPPDId.push_back(thisID);
    }
  } else {
    Log("Unable to open given LAPPD ID File",v_error,verbosity);
  }
}
