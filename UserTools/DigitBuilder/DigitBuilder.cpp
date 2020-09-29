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
  fIsMC = 1;
  fDigitChargeThr = 10;


  /// Get the Tool configuration variables
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("IsMC",fIsMC);
  m_variables.Get("ParametricModel", fParametricModel);
  m_variables.Get("PhotoDetectorConfiguration", fPhotodetectorConfiguration);
  m_variables.Get("xshift", xshift);
  m_variables.Get("yshift", yshift);
  m_variables.Get("zshift", zshift);
  m_variables.Get("LAPPDIDFile", fLAPPDIDFile);
  m_variables.Get("DigitChargeThr",fDigitChargeThr);
  m_variables.Get("ChankeyToPMTIDMap",path_chankeymap);
  m_variables.Get("SinglePEGains",singlePEgains);

  /// Construct the other objects we'll be setting at event level,
  fDigitList = new std::vector<RecoDigit>;

  // Make the RecoDigit Store if it doesn't exist
  int recoeventexists = m_data->Stores.count("RecoEvent");
  if(recoeventexists==0) m_data->Stores["RecoEvent"] = new BoostStore(false,2);
    
  /// Retrieve necessary info from ANNIEEvent
  auto get_geometry= m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",fGeometry);
  if(!get_geometry){
    Log("DigitBuilder Tool: Error retrieving Geometry from ANNIEEvent!",v_error,verbosity); 
    return false; 
  }
  
  // Some hard-coded values of old WCSim LAPPDIDs are in this Tool
  // I would recommend moving away from the use of WCSim IDs if possible as they are liable to change
  // but for tools that need them, in the LoadWCSim tool I put a map of WCSim TubeId to channelkey
  if (fIsMC){
    m_data->CStore.Get("detectorkey_to_lappdid",detectorkey_to_lappdid);
    m_data->CStore.Get("channelkey_to_pmtid",channelkey_to_pmtid);
  } else {
    ifstream file_pmtid(path_chankeymap.c_str());
    while (!file_pmtid.eof()){
      unsigned long chankey;
      int pmtid;
      file_pmtid >> chankey >> pmtid;
      channelkey_to_pmtid.emplace(chankey,pmtid);
      pmtid_to_channelkey.emplace(pmtid,chankey);
      if (file_pmtid.eof()) break;
    }
    
    file_pmtid.close();
    m_data->CStore.Set("pmt_tubeid_to_channelkey",pmtid_to_channelkey);

    ifstream file_singlepe(singlePEgains.c_str());
    unsigned long temp_chankey;
    double temp_gain;
    while (!file_singlepe.eof()){
      file_singlepe >> temp_chankey >> temp_gain;
      if (file_singlepe.eof()) break;
      pmt_gains.emplace(temp_chankey,temp_gain);
    }
    file_singlepe.close();

  }


  //Read the LAPPDID file, if given
  if(fLAPPDIDFile!="none"){
    if(verbosity>2) std::cout << "Loading digits from LAPPD IDs in file " << fLAPPDIDFile << std::endl;
    this->ReadLAPPDIDFile();
  } else {
    if(verbosity>2) std::cout << "Loading digits from all LAPPDs" << std::endl;
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
	
    
  if (fIsMC){
    auto get_mchits = m_data->Stores.at("ANNIEEvent")->Get("MCHits",fMCPMTHits);
    if(!get_mchits){ 
      Log("DigitBuilder Tool: Error retrieving MCHits from ANNIEEvent!",v_error,verbosity); 
      return false;
    }
    auto get_mclappdhits = m_data->Stores.at("ANNIEEvent")->Get("MCLAPPDHits",fMCLAPPDHits);
    if(!get_mclappdhits){
      Log("DigitBuilder Tool: Error retrieving MCLAPPDHits from ANNIEEvent!",v_error,verbosity); 
      return false;
    }
  } else {
    auto get_clusters =  m_data->CStore.Get("ClusterMap",m_all_clusters);
    if (!get_clusters){
      Log("DigitBuilder Tool: ERROR retrieving clustered hits (ClusterMap) in Data mode!",v_error,verbosity);
      return false;
    }
    auto get_clusters_chankey = m_data->CStore.Get("ClusterMapDetkey",m_all_clusters_detkey);
    if (!get_clusters_chankey){
      Log("DigitBuilder Tool: ERROR retrieving clustered chankeys (ClusterMapDetkey) in Data mode!",v_error,verbosity);
      return false;
    }
  }

  /// Build RecoDigit
  if (fIsMC){
    this->BuildMCRecoDigit();
  } else {
    this->BuildDataRecoDigit();
  }

  /// Hit info. to RecoEvent
  this->PushRecoDigits(true); 
  return true;
}

bool DigitBuilder::Finalise(){
  //delete fDigitList; fDigitList = 0;		//Don't delete pointer to fDigitList, will be deleted by the BoostStore!
  if(verbosity>0) cout<<"DigitBuilder exitting"<<endl;
  return true;
}

bool DigitBuilder::BuildMCRecoDigit() {
	
  if(fPhotodetectorConfiguration == "PMT_only") {
    this->BuildMCPMTRecoDigit();
    return true;
  }
  if(fPhotodetectorConfiguration == "LAPPD_only") {
    this->BuildMCLAPPDRecoDigit();
    return true;
  }
  if(fPhotodetectorConfiguration == "All") {
    this->BuildMCPMTRecoDigit();
    this->BuildMCLAPPDRecoDigit();
    return true;
  }
  else {
    cout<<"Wrong PhotoDetector Configuration! Allowed configurations: PMT_only, LAPPD_only, All"<<endl;
    return false;
  }
	
}

bool DigitBuilder::BuildDataRecoDigit() {

  if(fPhotodetectorConfiguration == "PMT_only") {
    this->BuildDataPMTRecoDigit();
    return true;
  }
  else if(fPhotodetectorConfiguration == "LAPPD_only") {
    Log("DigitBuilder tool: Error: LAPPD only mode not implemented yet for data.",v_error,verbosity);
    return false;
  }
  else if(fPhotodetectorConfiguration == "All") {
    Log("DigitBuilder tool: Error: LAPPD + PMT mode not implemented yet for data.",v_error,verbosity);
    return false;
  }
  else {
    cout<<"Wrong PhotoDetector Configuration! Allowed configurations: PMT_only, LAPPD_only, All"<<endl;
    return false;
  }

}

bool DigitBuilder::BuildMCPMTRecoDigit() {
	
  Log("DigitBuilder Tool: Build PMT reconstructed digits (MC)",v_message,verbosity);
  /// now move to digit retrieval
  int region = -999;
  double calT;
  double calQ = 0.;
  int digitType = -999;
  Detector* det=nullptr;
  Position  pos_sim, pos_reco;
  /// MCHits is a std::map<unsigned long,std::vector<Hit>>
  if(fMCPMTHits){
    Log("DigitBuilder Tool: Num PMT Digits = "+to_string(fMCPMTHits->size()),v_message, verbosity);
    /// iterate over the map of sensors with a measurement
    for(std::pair<unsigned long,std::vector<MCHit>>&& apair : *fMCPMTHits){
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
      pos_reco.SetX(pos_sim.X()+xshift);
      pos_reco.SetY(pos_sim.Y()+yshift);
      pos_reco.SetZ(pos_sim.Z()+zshift);
	
      if(det->GetDetectorElement()=="Tank"){
        std::vector<MCHit>& hits = apair.second;
        if(fParametricModel){
          if(verbosity>2) std::cout << "Using parametric model to build PMT hits" << std::endl;
          //We'll get all hit info and then define a time/charge for each digit
          std::vector<double> hitTimes;
          std::vector<double> hitCharges;
          for(MCHit& ahit : hits){
            if(verbosity>3){
              std::cout << "This HIT'S TIME AND CHARGE: " << ahit.GetTime() <<
                  "," << ahit.GetCharge() << std::endl;
            }
            double hitTime = ahit.GetTime()*1.0;
          	if(hitTime>-10 && hitTime<40) {
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
          if (verbosity>4) { 
            std::cout << "PMT position (X<Y<Z): " << 
                    to_string(pos_reco.X()) << "," << to_string(pos_reco.Y()) <<
                    "," << to_string(pos_reco.Z()) << std::endl;
            std::cout << "PMT Charge,Time: " << to_string(calQ) << "," <<
                    to_string(calT) << std::endl;
          }
          if(calQ>fDigitChargeThr) {					//changed to 0 for cross-checks with other tools, change back later!
				    digitType = RecoDigit::PMT8inch;
				    RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType, PMTId);
				    fDigitList->push_back(recoDigit); 
				  }
        } else {
			    for(MCHit& ahit : hits){
				  	//if(v_message<verbosity) ahit.Print(); // << VERY verbose
				  	// get calibrated PMT time (Use the MC time for now)
				  	calT = ahit.GetTime()*1.0; 
            calQ = ahit.GetCharge();
            if (verbosity>4) { 
              std::cout << "PMT position (X<Y<Z): " << 
                      to_string(pos_reco.X()) << "," << to_string(pos_reco.Y()) <<
                      "," << to_string(pos_reco.Z()) << std::endl;
              std::cout << "PMT Charge,Time: " << to_string(calQ) << "," <<
                      to_string(calT) << std::endl;
            }
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

bool DigitBuilder::BuildMCLAPPDRecoDigit() {
	std::string name = "DigitBuilder::BuildMCLAPPDRecoDigit(): ";
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
		for(std::pair<unsigned long,std::vector<MCLAPPDHit>>&& apair : *fMCLAPPDHits){
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
      for(int i=0;i<int(fLAPPDId.size());i++){
			  if(LAPPDId == fLAPPDId.at(i)) isSelectedLAPPD=true;
      }
      if(!isSelectedLAPPD && fLAPPDId.size()>0) continue;
      if(verbosity>2){
        std::cout << "Loading in digits for LAPPDID " << LAPPDId << std::endl;
      }

			if(det->GetDetectorElement()=="LAPPD"){ // redundant, MCLAPPDHits are LAPPD hitss
				std::vector<MCLAPPDHit>& hits = apair.second;
				for(MCLAPPDHit& ahit : hits){
					//if(v_message<verbosity) ahit.Print(); // << VERY verbose
					// an LAPPDHit has adds (global x-y-z) position, (in-tile x-y) local position
					// and time psecs
					// convert the WCSim coordinates to the ANNIEreco coordinates
					// convert the unit from m to cm
					pos_reco.SetX(ahit.GetPosition().at(0)*100.+xshift); //cm
					pos_reco.SetY(ahit.GetPosition().at(1)*100.+yshift); //cm
					pos_reco.SetZ(ahit.GetPosition().at(2)*100.+zshift); //cm
					calT = ahit.GetTime();  // 
					calT = frand.Gaus(calT, 0.1); // time is smeared with 100 ps time resolution. Harded-coded for now.
					calQ = ahit.GetCharge();
          if (verbosity>4) { 
            std::cout << "LAPPD position (X<Y<Z): " << 
                    to_string(pos_reco.X()) << "," << to_string(pos_reco.Y()) <<
                    "," << to_string(pos_reco.Z()) << std::endl;
            std::cout << "LAPPD Charge,Time: " << to_string(calQ) << "," <<
                    to_string(calT) << std::endl;
          }
					// I found the charge is 0 for all the hits. In order to test the code, 
					// here I just set the charge to 1. We should come back to this later. (Jingbo Wang)
					calQ = 1.;
					digitType = RecoDigit::lappd_v0;
					RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType,LAPPDId);
					//if(v_message<verbosity) recoDigit.Print();
				  //make some cuts here. It will be moved to the Hitcleaning tool
				  if(calT>40 || calT<-10) continue; // cut off delayed hits
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

bool DigitBuilder::BuildDataPMTRecoDigit(){

	Log("DigitBuilder Tool: Build PMT reconstructed digits (data)",v_message,verbosity);
	/// now move to digit retrieval
	int region = -999;
	double calT;
	double calQ = 0.;
	int digitType = -999;
	Detector* det=nullptr;
	Position  pos_sim, pos_reco;
	/// m_all_clusters is a std::map<double,std::vector<Hit>>
        
	if (m_all_clusters && m_all_clusters_detkey){
          int clustersize = m_all_clusters->size();
          std::cout <<"Clustersize of m_all_clusters: "<<clustersize<<std::endl;
          bool clusters_available = false;
          bool muon_available = false;
          if (clustersize != 0) clusters_available = true;
          if (clusters_available){
	  //determine the main cluster (max charge and in [0 ... 2000ns] time window)
	  double max_cluster = 0;
          double max_charge = 0;
          for(std::pair<double,std::vector<Hit>>&& apair : *m_all_clusters){
            std::vector<Hit>&Hits = apair.second;
            double time = 0;
            int hits=0;
            double charge = 0;
            for (unsigned int i_hit = 0; i_hit < Hits.size(); i_hit++){
              hits++;
              time+=Hits.at(i_hit).GetTime();
              charge+=Hits.at(i_hit).GetCharge();
          }
          if (hits>0) {
            time/=hits;
          }
          if (time > 2000.) continue;	//not a beam muon if not in primary window
	  if (charge > max_charge) {
            muon_available = true;
            max_charge = charge;
            max_cluster = apair.first;
          }
	}
	if (muon_available){
	  std::vector<Hit>& Hits = m_all_clusters->at(max_cluster);
          std::vector<unsigned long> detkeys = m_all_clusters_detkey->at(max_cluster);
          int hits_pmt = 0;

          std::map<unsigned long,std::vector<double>> hitTimes;
          std::map<unsigned long,std::vector<double>> hitCharges;

	  Log("DigitBuilder Tool: Num PMT Clustered Digits = "+to_string(Hits.size()),v_message, verbosity);
	  for (unsigned int i_hit = 0; i_hit < Hits.size(); i_hit++){
	    Hit ahit = Hits.at(i_hit);
            unsigned long chankey = detkeys.at(i_hit);
	    

	    if (hitTimes.find(chankey)!=hitTimes.end()){
	      hitTimes.at(chankey).push_back(ahit.GetTime());
	      hitCharges.at(chankey).push_back(ahit.GetCharge());
            }
	    else {
	      std::vector<double> temp_hittimes{ahit.GetTime()};
	      std::vector<double> temp_hitcharges{ahit.GetCharge()};
              hitTimes.emplace(chankey,temp_hittimes);
              hitCharges.emplace(chankey,temp_hitcharges);
	    }
          }

          if(fParametricModel){
            Log("DigitBuilder tool: Use Parametric Model to create digits",v_message,verbosity);
            // Do median and sum
            std::map<unsigned long,std::vector<double>>::iterator it, it2;
            for (it=hitTimes.begin(),it2 = hitCharges.begin(); it != hitTimes.end(), it2 != hitCharges.end(); it++, it2++){
	      unsigned long chankey = it->first;
	      std::vector<double> hittimes = it->second;
	      std::vector<double> hitcharges = it2->second;
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
	      pos_reco.SetX(pos_sim.X()+xshift);
	      pos_reco.SetY(pos_sim.Y()+yshift);
	      pos_reco.SetZ(pos_sim.Z()+zshift);

              std::sort(hittimes.begin(), hittimes.end());
              size_t timesize = hittimes.size();
              if (timesize == 0) continue;
              if (timesize % 2 == 0){
                calT = (hittimes.at(timesize/2 - 1) + hittimes.at(timesize/2))/2;
              } else {
                calT = hittimes.at(timesize/2);
              }
              calQ = 0.;
              for(std::vector<double>::iterator it3 = hitcharges.begin(); it3 != hitcharges.end(); ++it3){
                calQ += *it3;
              }
              if (verbosity>4) { 
                std::cout << "PMT position (X<Y<Z): " << 
                to_string(pos_reco.X()) << "," << to_string(pos_reco.Y()) <<
                "," << to_string(pos_reco.Z()) << std::endl;
                std::cout << "PMT Charge,Time: " << to_string(calQ) << "," <<
                to_string(calT) << std::endl;
              }
	      double calQ_temp = calQ;
	      if (pmt_gains.find(chankey) != pmt_gains.end() && pmt_gains.at(chankey) > 0.0){
                calQ_temp = calQ / pmt_gains.at(chankey);
              }
              if(calQ_temp>fDigitChargeThr) {
                digitType = RecoDigit::PMT8inch;
	        RecoDigit recoDigit(region, pos_reco, calT, calQ_temp, digitType, PMTId);
                fDigitList->push_back(recoDigit); 
	      }
            }
          } else {
            std::map<unsigned long,std::vector<double>>::iterator it, it2;
            for (it=hitTimes.begin(), it2 = hitCharges.begin(); it != hitTimes.end(), it2 != hitCharges.end(); it++, it2++){
              unsigned long chankey = it->first;
              std::vector<double> hittimes = it->second;
              std::vector<double> hitcharges = it2->second;
              det = fGeometry->ChannelToDetector(chankey);
              int PMTId = channelkey_to_pmtid.at(chankey);  //PMTID In WCSim
              if(det==nullptr){
                Log("DigitBuilder Tool: Detector not found! ",v_message,verbosity);
                continue;
              }
              pos_sim = det->GetDetectorPosition();
              pos_sim.UnitToCentimeter();
              pos_reco.SetX(pos_sim.X()+xshift);
              pos_reco.SetY(pos_sim.Y()+yshift);
              pos_reco.SetZ(pos_sim.Z()+zshift);
                
              for (int i=0; i< int(hitcharges.size()); i++){
		calT = hittimes.at(i);
                calQ = hitcharges.at(i);
                if (verbosity>4) {
                  std::cout << "PMT position (X<Y<Z): " <<
                  to_string(pos_reco.X()) << "," << to_string(pos_reco.Y()) <<
                  "," << to_string(pos_reco.Z()) << std::endl;
                  std::cout << "PMT Charge,Time: " << to_string(calQ) << "," <<
                  to_string(calT) << std::endl;
                }
                digitType = RecoDigit::PMT8inch;
                RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType, PMTId);
		fDigitList->push_back(recoDigit);
               }
             }
           } 
         }
       }
     } else {
       cout<<"No Clustered Hits found."<<endl;
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
    Log("Unable to open given LAPPD ID File. Using all LAPPDs",v_error,verbosity);
  }
}
