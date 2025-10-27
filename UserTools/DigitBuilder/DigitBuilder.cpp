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
  cout<<"Initializing Tool DigitBuilder"<<endl;
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
  m_variables.Get("StripHit", striphit);
  m_variables.Get("MCPMTSmear", MCPMTResolution);
  m_variables.Get("end_of_window_time_cut", end_of_window_time_cut);
  m_variables.Get("AcqTimeWindow", AcqTimeWindow);
  m_variables.Get("CollectHits",fCollectHits);

  /// Construct the other objects we'll be setting at event level,
  fDigitList = new std::vector<RecoDigit>;
  fHitLAPPDs = new std::vector<int>;

  // Make the RecoDigit Store if it doesn't exist
  int recoeventexists = m_data->Stores.count("RecoEvent");
  if(recoeventexists==0) m_data->Stores["RecoEvent"] = new BoostStore(false,2);
    
  /// Retrieve necessary info from ANNIEEvent
  auto get_geometry= m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",fGeometry);
  if(!get_geometry){
    Log("DigitBuilder Tool: Error retrieving Geometry from ANNIEEvent!",v_error,verbosity); 
    return false; 
  }
  Log("Strip Hit Mode:" + to_string(striphit),v_debug,verbosity);
  
  // Some hard-coded values of old WCSim LAPPDIDs are in this Tool
  // I would recommend moving away from the use of WCSim IDs if possible as they are liable to change
  // but for tools that need them, in the LoadWCSim tool I put a map of WCSim TubeId to channelkey
  if (fIsMC){
    m_data->CStore.Get("detectorkey_to_lappdid",detectorkey_to_lappdid);
    m_data->CStore.Get("channelkey_to_pmtid",channelkey_to_pmtid);
  } else {
    ifstream file_pmtid(path_chankeymap.c_str());
    if (!file_pmtid) {
        Log("DigitBuilder Tool: Did not find chankeymap file",v_error,verbosity);
        return false;
    }
    while (!file_pmtid.eof()){
      unsigned long chankey;
      int pmtid;
      file_pmtid >> chankey >> pmtid;
      channelkey_to_pmtid.emplace(chankey,pmtid);
      pmtid_to_channelkey.emplace(pmtid,chankey);
      if (file_pmtid.eof()) break;
      Log("DigitBuilder Tool: still gathering chankeys",v_debug,verbosity);
    }
    Log("DigitBuilder Tool: chankeys done",v_debug,verbosity);
    file_pmtid.close();
    m_data->CStore.Set("pmt_tubeid_to_channelkey",pmtid_to_channelkey);

    ifstream file_singlepe(singlePEgains.c_str());
    if (!file_singlepe) {
        Log("DigitBuilder Tool: Did not find SinglePEgains file",v_error,verbosity);
        return false;
    }
    unsigned long temp_chankey;
    double temp_gain;

    std::string line;
    if (file_singlepe.is_open()) {
        //Loop over lines, collect all detector data (should only be one line here)
        while (getline(file_singlepe, line)) {
            if (verbosity > 3) std::cout << line << std::endl; //has our stuff;
            if (line.find("#") != std::string::npos) continue;
            std::vector<std::string> DataEntries;
            boost::split(DataEntries, line, boost::is_any_of(","), boost::token_compress_on);
            int channelkey = -9999;
            double SPECharge = -9999.;
            channelkey = std::stoi(DataEntries.at(0));
            SPECharge = std::stod(DataEntries.at(1));
            pmt_gains.emplace(channelkey, SPECharge);
        }
    }


    /*while (!file_singlepe.eof()) {
      file_singlepe >> temp_chankey >> temp_gain;
      if (file_singlepe.eof()) break;
      pmt_gains.emplace(temp_chankey,temp_gain);
      Log("DigitBuilder Tool: still collecting SPE gains: "+to_string(temp_gain), v_debug, verbosity);
    }*/
    Log("DigitBuilder Tool: SPE gains done",v_debug,verbosity);
    file_singlepe.close();

  }


  //Read the LAPPDID file, if given
  if(fLAPPDIDFile!="none"){
    Log("Loading digits from LAPPD IDs in file " + fLAPPDIDFile,v_debug,verbosity);
    this->ReadLAPPDIDFile();
  } else {
    Log("Loading digits from all LAPPDs",v_debug,verbosity);
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
      auto get_dhits = m_data->Stores.at("ANNIEEvent")->Get("Hits",Hits);
      if (!get_dhits) {
          Log("DigitBuilder Tool: ERROR retrieving hits in Data mode!",v_error,verbosity);
          return false;
      }
    /*auto get_clusters = m_data->CStore.Get("ClusterMap", m_all_clusters);
    if (!get_clusters){
      Log("DigitBuilder Tool: ERROR retrieving clustered hits (ClusterMap) in Data mode!",v_error,verbosity);
      return false;
    }
    auto get_clusters_chankey = m_data->CStore.Get("ClusterMapDetkey",m_all_clusters_detkey);
    if (!get_clusters_chankey){
      Log("DigitBuilder Tool: ERROR retrieving clustered chankeys (ClusterMapDetkey) in Data mode!",v_error,verbosity);
      return false;
    }*/
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
  Log("DigitBuilder exitting",v_message,verbosity);
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
    Log("Wrong PhotoDetector Configuration! Allowed configurations: PMT_only, LAPPD_only, All",v_error);
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
    Log("Wrong PhotoDetector Configuration! Allowed configurations: PMT_only, LAPPD_only, All",v_error,verbosity);
    return false;
  }

}

bool DigitBuilder::BuildMCPMTRecoDigit() {
	
  Log("DigitBuilder Tool: Build PMT reconstructed digits (MC)",v_message,verbosity);
  /// now move to digit retrieval
  int region = -999;
  double calT = 0;
  double calQ = 0.;
  double maxT = -999;
  int digitType = -999;
  Detector* det=nullptr;
  Position  pos_sim, pos_reco;
  /// MCHits is a std::map<unsigned long,std::vector<Hit>>

  
  if(fMCPMTHits){
      if (fCollectHits) {
          Log("DigitBuilder Tool: Collecting Hits to make Digits",v_message,verbosity);
          CollectMCPMTHits();
          Log("DigitBuilder Tool: # of digits in this event: "+to_string(fDigitList->size()),v_debug,verbosity);
      }
      else {
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
          Log("Using parametric model to build PMT hits",v_debug,verbosity);
          //We'll get all hit info and then define a time/charge for each digit
          std::vector<double> hitTimes;
          std::vector<double> hitCharges;
          //std::vector<int> hitIDs;
          for(MCHit& ahit : hits){
              Log("This HIT'S TIME AND CHARGE: " + to_string(ahit.GetTime()) + ", " + to_string(ahit.GetCharge()),v_debug,verbosity);
            double hitTime = ahit.GetTime()*1.0;
          	if(hitTime>-10 && hitTime<70) {
			  hitTimes.push_back(ahit.GetTime()*1.0); 
              hitCharges.push_back(ahit.GetCharge());
              //hitIDs.push_back(ahit.GetHitID());
            }
          }
          // Do median and sum
          std::sort(hitTimes.begin(), hitTimes.end());
          size_t timesize = hitTimes.size();
          if (timesize == 0) continue;
          if (fParametricModel == 1) {                //Mean hit time
              if (timesize % 2 == 0) {
                  calT = (hitTimes.at(timesize / 2 - 1) + hitTimes.at(timesize / 2)) / 2;
              }
              else {
                  calT = hitTimes.at(timesize / 2);
              }
          }
          else if (fParametricModel == 2) {         //first hit time
              calT = hitTimes.at(0);
          }
          else if (fParametricModel == 3) {          //Average of first 20% of hit times
              for (int i = 0; i < timesize / 5; i++) {
                  calT += hitTimes.at(i);
              }
              if ((int)(timesize / 5) > 0)calT = calT / ((int)(timesize / 5));
          }
          else if (fParametricModel == 4) {         //Average hit time
              for (int i = 0; i < timesize; i++) {
                  calT += hitTimes.at(i);
              }
              calT = calT / timesize;
          }
          else if (fParametricModel == 5) {
              for (int i = 0; i < timesize; i++) {
                  if (hitCharges.at(i) > maxT) maxT = hitTimes.at(i);
             }
              calT = maxT;
          }
          if (MCPMTResolution > 0) calT = frand.Gaus(calT, MCPMTResolution);
          calQ = 0.;
          for(std::vector<double>::iterator it = hitCharges.begin(); it != hitCharges.end(); ++it){
            calQ += *it;
          }
            Log("PMT position (X<Y<Z): " + to_string(pos_reco.X()) + "," + to_string(pos_reco.Y()) + "," + to_string(pos_reco.Z()),v_debug,verbosity);
            Log("PMT Charge,Time: " + to_string(calQ) + "," + to_string(calT),v_debug,verbosity);

          if(calQ>fDigitChargeThr) {					//changed to 0 for cross-checks with other tools, change back later!
				    digitType = RecoDigit::PMT8inch;
				    RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType, PMTId);
                    //recoDigit.SetHitIDs(hitIDs);
				    fDigitList->push_back(recoDigit); 
				  }
        }else{
			    for(MCHit& ahit : hits){
				  	//if(v_message<verbosity) ahit.Print(); // << VERY verbose
				  	// get calibrated PMT time (Use the MC time for now)
				  	calT = ahit.GetTime()*1.0; 
            calQ = ahit.GetCharge(); 
              Log("PMT position (X<Y<Z): " + to_string(pos_reco.X()) + "," + to_string(pos_reco.Y()) + "," + to_string(pos_reco.Z()),v_debug,verbosity);
              Log("PMT Charge,Time: " + to_string(calQ) + "," + to_string(calT),v_debug,verbosity);
            calT = frand.Gaus(calT, 1.0);
				  	digitType = RecoDigit::PMT8inch;
				  	RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType, PMTId);
				    //recoDigit.Print();
                    //recoDigit.SetHitIDs(hitIDs);
				    fDigitList->push_back(recoDigit); 
          }
        }
      }
		} // end loop over MCHits
    }
        } else {
		Log("No MCHits",v_message,verbosity);
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
    int currentLAPPD = 0;
    if (fHitLAPPDs->size() > 0) {
        fHitLAPPDs->clear();
    }

  // repeat for LAPPD hits
	// MCLAPPDHits is a std::map<unsigned long,std::vector<LAPPDHit>>
	if(fMCLAPPDHits && fMCLAPPDHits->size() > 0){
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
        Log("Loading in digits for LAPPDID " + to_string(LAPPDId), v_message,verbosity);
        Log("located: ",v_message,verbosity);
        det->GetPositionInTank().Print();
        Log("directed: ",v_message,verbosity);
        det->GetDetectorDirection().Print();
      }

      if (det->GetDetectorElement() == "LAPPD") { // redundant, MCLAPPDHits are LAPPD hitss
          std::vector<MCLAPPDHit>& hits = apair.second;

          std::vector<double> sumposX;
          std::vector<double> sumposY;
          std::vector<double> sumposZ;
          std::vector<double> sumT;
          double posX;
          std::vector<int> nHitsOnStrip;
          int a;
          for (int i = 0; i < 28; i++) {
              sumposX.push_back(0);
              sumposY.push_back(0);
              sumposZ.push_back(0);
              sumT.push_back(0);
              nHitsOnStrip.push_back(0);
          }
				for(MCLAPPDHit& ahit : hits){
                    if (LAPPDId != currentLAPPD && hits.size() > 3) {
                        Log("THERE ARE HITS ON LAPPD " + to_string(LAPPDId),v_message,verbosity);
                        currentLAPPD = LAPPDId;                        
                        fHitLAPPDs->push_back(currentLAPPD);
                        Log("fHitLAPPDs size: " + to_string(fHitLAPPDs->size()),v_message,verbosity);
                    }
                    if (striphit == 0) {
                        //if(v_message<verbosity) ahit.Print(); // << VERY verbose
                        // an LAPPDHit has adds (global x-y-z) position, (in-tile x-y) local position
                        // and time psecs
                        // convert the WCSim coordinates to the ANNIEreco coordinates
                        // convert the unit from m to cm
                        pos_reco.SetX(ahit.GetPosition().at(0) * 100. + xshift); //cm
                        pos_reco.SetY(ahit.GetPosition().at(1) * 100. + yshift); //cm
                        pos_reco.SetZ(ahit.GetPosition().at(2) * 100. + zshift); //cm
                        calT = ahit.GetTime();  // 
                        calT = frand.Gaus(calT, 0.1); // time is smeared with 100 ps time resolution. Harded-coded for now.
                        calQ = ahit.GetCharge();
                            
                        Log("LAPPD position (X<Y<Z): " + to_string(pos_reco.X()) + "," + to_string(pos_reco.Y()) + "," + to_string(pos_reco.Z()),v_debug,verbosity);
                            
                        Log("LAPPD Charge,Time: " + to_string(calQ) + "," + to_string(calT),v_debug,verbosity);
                        // I found the charge is 0 for all the hits. In order to test the code, 
                        // here I just set the charge to 1. We should come back to this later. (Jingbo Wang)
                        calQ = 1.;
                        digitType = RecoDigit::lappd_v0;
                        RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType, LAPPDId);
                        //if(v_message<verbosity) recoDigit.Print();
                      //make some cuts here. It will be moved to the Hitcleaning tool
                        if (calT > 40 || calT < -10) continue; // cut off delayed hits
                        //recoDigit.SetHitIDs(hitIDs);
                        fDigitList->push_back(recoDigit);

                    }
                    else {
                        posX = (ahit.GetPosition().at(0) * 100 + xshift) - (det->GetPositionInTank().X() * 100 + xshift);
                        double dirX = det->GetDetectorDirection().X();
                        double AngX = std::asin(dirX);
                        int strip = (int)((posX / (20.* std::sin(AngX) / 28.)) + 14 - 1);
                        if (strip > 27 || strip < 0) {
                            Log("warning: LAPPD hit found off LAPPD: ", v_warning, verbosity);
                            Log("hit found on strip " + to_string(strip),v_warning,verbosity);
                            Log("LAPPD truehit X,Y,Z: " + to_string(ahit.GetPosition().at(0)) + ", " + to_string(ahit.GetPosition().at(1)) + ", " + to_string(ahit.GetPosition().at(2)),v_warning,verbosity);
                            Log("continuing loop without this hit.",v_warning,verbosity);
                            continue;
                        }
                        Log("hit found on strip " + to_string(strip),v_debug,verbosity);
                        sumposX.at(strip) += ahit.GetPosition().at(0) * 100 + xshift;
                        sumposY.at(strip) += ahit.GetPosition().at(1) * 100 + yshift;
                        sumposZ.at(strip) += ahit.GetPosition().at(2) * 100 + zshift;
                        Log("LAPPD truehit X,Y,Z: " + to_string(ahit.GetPosition().at(0)) + ", " + to_string(ahit.GetPosition().at(1)) + ", " + to_string(ahit.GetPosition().at(2)),v_debug,verbosity);
                        
                        if ((striphit == 3 && nHitsOnStrip.at(strip) < 3) || (striphit==2 && nHitsOnStrip.at(strip) == 0) || striphit == 1) 
                            sumT.at(strip) += frand.Gaus(calT, 0.1); // time is smeared with 100 ps time resolution. Harded-coded for now.
                        nHitsOnStrip.at(strip) += 1;

                    }
				}
                if (striphit != 0) {
                    for (int strip = 0; strip < 28; strip++) {
                        if (nHitsOnStrip.at(strip) != 0) {
                            pos_reco.SetX(sumposX.at(strip) / nHitsOnStrip.at(strip));
                            pos_reco.SetY(sumposY.at(strip) / nHitsOnStrip.at(strip));
                            pos_reco.SetZ(sumposZ.at(strip) / nHitsOnStrip.at(strip));
                            if(striphit == 1) calT = sumT.at(strip) / nHitsOnStrip.at(strip);
                            if (striphit == 3) calT = sumT.at(strip) / 3;
                            calQ = nHitsOnStrip.at(strip); //set to the number of hits for now.
                            digitType = RecoDigit::lappd_v0;

                            Log("LAPPD ID: " + to_string(LAPPDId),v_message,verbosity);
                            Log("LAPPD strip-hit position (X<Y<Z): " + to_string(pos_reco.X()) + "," + to_string(pos_reco.Y()) + "," + to_string(pos_reco.Z()),v_debug,verbosity);
                            Log("LAPPD strip-hit Charge,Time: " + to_string(calQ) + "," + to_string(calT),v_debug,verbosity);
                            RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType, LAPPDId);
                            Log("recoDigit created for striphit",v_message,verbosity);
                            //recoDigit.SetHitIDs(hitIDs);
                            fDigitList->push_back(recoDigit);
                        }
                    }
                }
                sumposX.clear();
                sumposY.clear();
                sumposZ.clear();
                sumT.clear();
                nHitsOnStrip.clear();
			}
		} // end loop over MCLAPPDHits
	} else {
		Log("No MCLAPPDHits",v_warning,verbosity);
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
    double calQ_temp = 0;
    int digitType = -999;
    Detector* det = nullptr;
    Position  pos_sim, pos_reco;

    for (std::pair<unsigned long, std::vector<Hit>>&& apair : *Hits) {
        unsigned long chankey = apair.first;
        Detector* thistube = fGeometry->ChannelToDetector(chankey);
        det = fGeometry->ChannelToDetector(chankey);
        int detectorkey = thistube->GetDetectorID();
        int PMTId = channelkey_to_pmtid.at(chankey);
        if (thistube->GetDetectorElement() == "Tank") {
            std::vector<Hit>& ThisPMTHits = apair.second;
            for (Hit& ahit : ThisPMTHits) {
                pos_reco=det->GetDetectorPosition();
                calT = ahit.GetTime();

                calQ = ahit.GetCharge();
          
                if (pmt_gains.find(chankey) != pmt_gains.end() && pmt_gains.at(chankey) > 0.0) {
                    calQ_temp = calQ / pmt_gains.at(chankey);
                }
                if (calQ_temp > fDigitChargeThr) {
                    digitType = RecoDigit::PMT8inch;
                    RecoDigit recoDigit(region, pos_reco, calT, calQ_temp, digitType, PMTId);

                    //recoDigit.SetHitIDs(hitIDs);
                    fDigitList->push_back(recoDigit);
                }

            }
            
        }
    }


    

    
    /*
	
	/// m_all_clusters is a std::map<double,std::vector<Hit>>
        
	if (m_all_clusters && m_all_clusters_detkey){
          int clustersize = m_all_clusters->size();
          Log("Clustersize of m_all_clusters: " + to_string(clustersize),v_debug,verbosity);
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
 
                Log("PMT position (X<Y<Z): " + to_string(pos_reco.X()) + "," + to_string(pos_reco.Y()) + "," + to_string(pos_reco.Z()),v_debug,verbosity);
                Log("PMT Charge,Time: " + to_string(calQ) + "," + to_string(calT),v_debug,verbosity);

	      double calQ_temp = calQ;
	      if (pmt_gains.find(chankey) != pmt_gains.end() && pmt_gains.at(chankey) > 0.0){
                calQ_temp = calQ / pmt_gains.at(chankey);
              }
              if(calQ_temp>fDigitChargeThr) {
                digitType = RecoDigit::PMT8inch;
	        RecoDigit recoDigit(region, pos_reco, calT, calQ_temp, digitType, PMTId);
            
            //recoDigit.SetHitIDs(hitIDs);
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

                  Log("PMT position (X<Y<Z): " + to_string(pos_reco.X()) + "," + to_string(pos_reco.Y()) + "," + to_string(pos_reco.Z()),v_debug,verbosity);
                  Log("PMT Charge,Time: " + to_string(calQ) + "," + to_string(calT),v_debug,verbosity);

                digitType = RecoDigit::PMT8inch;
                RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType, PMTId);
                //recoDigit.SetHitIDs(hitIDs);
		fDigitList->push_back(recoDigit);
               }
             }
           } 
         }
       }
     } else {
       Log("No Clustered Hits found.",v_warning,verbosity);
       return false;
     }*/
     
     return true;

}

void DigitBuilder::PushRecoDigits(bool savetodisk) {
	Log("DigitBuilder Tool: Push reconstructed digits to the RecoEvent store",v_message,verbosity);
	m_data->Stores.at("RecoEvent")->Set("RecoDigit", fDigitList, savetodisk);  ///> Add digits to RecoEvent
    m_data->Stores.at("RecoEvent")->Set("HitLAPPDs", fHitLAPPDs, savetodisk);
}

void DigitBuilder::Reset() {
  // Reset 
  fDigitList->clear();
  fHitLAPPDs->clear();
}

void DigitBuilder::ReadLAPPDIDFile() {
  std::string line;
  ifstream myfile(fLAPPDIDFile);
  if (myfile.is_open()){
    while(getline(myfile,line)){
      if(verbosity>0){
        Log("DigitBuilder tool: Loading hits from LAPPD ID " + line,v_message,verbosity);
      }
      int thisID = std::atoi(line.c_str());
      fLAPPDId.push_back(thisID);
    }
  } else {
    Log("Unable to open given LAPPD ID File. Using all LAPPDs",v_error,verbosity);
  }
}

void DigitBuilder::CollectMCPMTHits() {
    double calT = 0;
    double calQ = 0;
    Position pos_reco, pos_sim;
    std::map<unsigned long, int> PMT_ishit;


    for (std::pair<unsigned long, std::vector<MCHit>>&& apair : *fMCPMTHits) {
        unsigned long chankey = apair.first;
        Detector* thistube = fGeometry->ChannelToDetector(chankey);
        int detectorkey = thistube->GetDetectorID();
        int PMTId = channelkey_to_pmtid.at(chankey);
        Log("DigitBuilder Tool: collecting hits for PMT "+to_string(detectorkey)+", which corresponds to PMTId "+to_string(PMTId),v_debug,verbosity);
        if (thistube->GetDetectorElement() == "Tank") {
            std::vector<MCHit>& hits = apair.second;
            PMT_ishit[detectorkey] = 1;
            std::vector<double> hit_times;
            std::vector<double> temp_times;
            double first_time=0;
            double mid_time=0;
            std::vector<double> datalike_hits;
            std::vector<double> datalike_hits_charge;
            std::vector<int> Parents_by_hit;
            std::vector<int> temp_parents;
            double max_charge=-999;
            pos_sim = thistube->GetDetectorPosition();
            pos_sim.UnitToCentimeter();
            pos_reco.SetX(pos_sim.X() + xshift);
            pos_reco.SetY(pos_sim.Y() + yshift);
            pos_reco.SetZ(pos_sim.Z() + zshift);


            for (MCHit& ahit : hits) {
                //if (ahit.GetTime() < end_of_window_time_cut * AcqTimeWindow) {
                    hit_times.push_back(ahit.GetTime());
                //}
            }
            /*for (MCHit& ahit : hits) {
                //std::cout <<"Key: "<<detectorkey<<", charge "<<ahit.GetCharge()<<", time "<<ahit.GetTime()<<std::endl;
                //if (ahit.GetTime() > 2000.) std::cout <<"Found hit later than 2us! Hit time : "<<ahit.GetTime()<<", chankey: "<<chankey<<std::endl;
                if (ahit.GetTime() < end_of_window_time_cut * AcqTimeWindow) {
                    //Make MC more like data --> combine multiple photons if they are within a 10ns range
                    //hit times can only be recorded with 2ns precision --> possible times are 0ns, 2ns, 4ns, ...
                    hits_2ns_res.push_back(2 * (int(ahit.GetTime()) / 2.) + (int(ahit.GetTime()) % 2));
                    hits_2ns_res_charge.push_back(ahit.GetCharge());
                }
            }*/

            if (hit_times.size() == 0) {
                Log("DigitBuilder tool: no hits in window.",v_message,verbosity);
                return;
            }

            //Combine multiple MC hits to one pulse
            std::sort(hit_times.begin(), hit_times.end());
            for (int i_hit = 0; i_hit < (int)hit_times.size(); i_hit++) {
                Log("check 0: hits, hit_times, datalike_hits, parents size: "+to_string(hits.size())+" "+to_string(hit_times.size())+" "+to_string(datalike_hits.size())+" " + to_string(hits.at(i_hit).GetParents()->size()), v_debug, verbosity);
                double hit1 = hit_times.at(i_hit);
                if(hit1>end_of_window_time_cut * AcqTimeWindow) continue;
                int j_hit=0;
                if (datalike_hits.size() == 0) {
                    Log("check 0-b", v_debug, verbosity);
                    
                    datalike_hits.push_back(hit1);
                    first_time=hit1;

                    datalike_hits_charge.push_back(hits.at(i_hit).GetCharge());
                    Log("check 0-c", v_debug, verbosity);
                    if (hits.at(i_hit).GetParents()->size() == 0) {
                        Parents_by_hit.push_back(-5);
                        Log("found hit with no parent at time "+to_string(hits.at(i_hit).GetTime()),v_debug,verbosity);
                    }
                    else {
                        Parents_by_hit.push_back(hits.at(i_hit).GetParents()->at(0));
                    Log("check 0-d: hit parent " + to_string(hits.at(i_hit).GetParents()->at(0)) + " and time: " + to_string(hits.at(i_hit).GetTime()), v_debug, verbosity);
                    }

                    temp_times.push_back(hit1);
                }
                else {
                    bool new_pulse = false;
                    Log("check 0-a", v_debug, verbosity);
                    for (int j_hit = 0; j_hit < (int)datalike_hits.size(); j_hit++) {
                        
                        if (fabs(first_time - hit1) < 10.) {
                            new_pulse = false;
                            datalike_hits_charge.at(j_hit) += hits.at(i_hit).GetCharge();
                            temp_times.push_back(hit1);
                            
                        }
                        else {
                            new_pulse = true;
                            temp_times.push_back(hit1);
                            
                        }
                        Log("check 1-"+to_string(j_hit)+" new_pulse: "+to_string(new_pulse), v_debug, verbosity);
                    }

                    if (new_pulse) {
                        first_time=hit1;
                        
                        // following the DigitBuilder tool --> take median photon hit time as the hit time of the "pulse"
                        sort(temp_times.begin(),temp_times.end());
                        if (temp_times.size() % 2 == 0) {
                            mid_time = (temp_times.at(temp_times.size() / 2 - 1) + temp_times.at(temp_times.size() / 2)) / 2;
                        }
                        else {
                            mid_time = temp_times.at(temp_times.size() / 2);
                        }
                        Log("check 1-a", v_debug, verbosity);
                        datalike_hits.at(j_hit)=mid_time;  //datalike_hits.push_back(mid_time);      //Only count as a new pulse if it was 10ns away from every other pulse
                        datalike_hits_charge.push_back(hits.at(i_hit).GetCharge());
                        j_hit++;

                        datalike_hits.push_back(hit1);

                        if (hits.at(i_hit).GetParents()->size() == 0) Parents_by_hit.push_back(-5);
                        else {
                            Parents_by_hit.push_back(hits.at(i_hit).GetParents()->at(0));
                        Log("check 0-e: hit parent " + to_string(hits.at(i_hit).GetParents()->at(0)) + " and time: " + to_string(hits.at(i_hit).GetTime()), v_debug, verbosity);
                        }
                        temp_times.clear();

                    }
                }
            }

            Log("check 2", v_debug, verbosity);
            //hits.clear();
            
            for (int i_hit = 0; i_hit < (int)datalike_hits.size(); i_hit++) {
                Log("check 1", v_debug, verbosity);
                calT = datalike_hits.at(i_hit);
                Log("check 2", v_debug, verbosity);
                if (MCPMTResolution > 0) calT= frand.Gaus(calT, MCPMTResolution);
                calQ = datalike_hits_charge.at(i_hit);
                Log("check 3",v_debug,verbosity);
                RecoDigit recoDigit(-999 /*region*/, pos_reco, calT, calQ, RecoDigit::PMT8inch, PMTId);
                temp_parents.push_back(Parents_by_hit.at(i_hit));
                Log("check 4",v_debug,verbosity);
                recoDigit.SetParents(temp_parents);
                fDigitList->push_back(recoDigit);
                temp_parents.clear();

                Log("PMT position (X<Y<Z): " + to_string(pos_reco.X()) + "," + to_string(pos_reco.Y()) + "," + to_string(pos_reco.Z()), v_debug, verbosity);
                Log("PMT Charge,Time: " + to_string(calQ) + "," + to_string(calT), v_debug, verbosity);
                //newMCHits.push_back(MCHit(chankey, datalike_hits.at(i_hit), datalike_hits_charge.at(i_hit), parents));
            }
            
        }
    }
}