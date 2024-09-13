#include "ClusterFinder.h"

ClusterFinder::ClusterFinder():Tool(){}


bool ClusterFinder::Initialise(std::string configfile, DataModel &data){

  if(verbose > 0) cout <<"Initialising Tool ClusterFinder ..."<<endl;

  /////////////////// Useful header ///////////////////////
  
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  m_data= &data; //assigning transient data pointer

  /////////////////////////////////////////////////////////////////
  
  //----------------------------------------------------------------------------
  //---------------Get configuration variables for this tool--------------------
  //----------------------------------------------------------------------------  
  
  m_variables.Get("HitStore",HitStoreName);
  m_variables.Get("OutputFile",outputfile);
  m_variables.Get("ClusterFindingWindow",ClusterFindingWindow); 
  m_variables.Get("AcqTimeWindow",AcqTimeWindow);
  m_variables.Get("ClusterIntegrationWindow",ClusterIntegrationWindow);
  m_variables.Get("MinHitsPerCluster",MinHitsPerCluster);
  m_variables.Get("Plots2D",draw_2D);
  m_variables.Get("verbosity",verbose);
  m_variables.Get("end_of_window_time_cut",end_of_window_time_cut);
  m_variables.Get("MC_pulse_width",mc_pulse_width);

  //----------------------------------------------------------------------------
  //---------------Get basic geometry properties -------------------------------
  //----------------------------------------------------------------------------

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geom);
  n_tank_pmts = geom->GetNumDetectorsInSet("Tank");
  n_lappds = geom->GetNumDetectorsInSet("LAPPD");
  n_mrd_pmts = geom->GetNumDetectorsInSet("MRD");
  n_veto_pmts = geom->GetNumDetectorsInSet("Veto");
  Position detector_center=geom->GetTankCentre();
  tank_center_x = detector_center.X();
  tank_center_y = detector_center.Y();
  tank_center_z = detector_center.Z();

  std::map<std::string, std::map<unsigned long,Detector*>>* Detectors = geom->GetDetectors();
  std::cout <<"Detectors size: "<<Detectors->size()<<std::endl;  // num detector sets...


  //----------------------------------------------------------------------------
  //---------------Read in geometry properties of PMTs--------------------------
  //----------------------------------------------------------------------------
  
  //FIXME: put PMT radii in geometry class, read directly and not via PMT type mapping
  map_type_radius.emplace("R5912HQE",0.1016);
  map_type_radius.emplace("R7081",0.127);
  map_type_radius.emplace("R7081HQE",0.127);
  map_type_radius.emplace("D784KFLB",0.1397);
  map_type_radius.emplace("EMI9954KB",0.0508);
 
  //----------------------------------------------------------------------------
  //---------------calculate expected hit times for PMTs------------------------
  //----------------------------------------------------------------------------
  
  for (std::map<unsigned long,Detector*>::iterator it  = Detectors->at("Tank").begin(); it != Detectors->at("Tank").end(); ++it){

    Detector* apmt = it->second;
    unsigned long detkey = it->first;
    unsigned long chankey = apmt->GetChannels()->begin()->first;
    pmt_detkeys.push_back(detkey);
    std::string dettype = apmt->GetDetectorType();

    if (verbose > 1) std::cout <<"PMT with detkey: "<<detkey<<", has type "<<dettype<<std::endl;
    if (verbose > 1) std::cout <<"PMT Print:" <<apmt->Print()<<std::endl;

    Position position_PMT = apmt->GetDetectorPosition();
    if (verbose > 2) std::cout <<"detkey: "<<detkey<<", chankey: "<<chankey<<std::endl;
    if (verbose > 2) std::cout <<"filling PMT position maps"<<std::endl;

    x_PMT.insert(std::pair<unsigned long,double>(detkey,position_PMT.X()-tank_center_x));
    y_PMT.insert(std::pair<unsigned long,double>(detkey,position_PMT.Y()-tank_center_y));
    z_PMT.insert(std::pair<unsigned long,double>(detkey,position_PMT.Z()-tank_center_z));

    double rho = sqrt(pow(x_PMT.at(detkey),2)+pow(y_PMT.at(detkey),2));
    if (x_PMT.at(detkey) < 0) rho*=-1;
    rho_PMT.insert(std::pair<unsigned long, double>(detkey,rho));
    double phi;
    if (x_PMT.at(detkey)>0 && z_PMT.at(detkey)>0) phi = atan(x_PMT.at(detkey)/z_PMT.at(detkey));
    if (x_PMT.at(detkey)>0 && z_PMT.at(detkey)<0) phi = TMath::Pi()/2+atan(x_PMT.at(detkey)/-z_PMT.at(detkey));
    if (x_PMT.at(detkey)<0 && z_PMT.at(detkey)<0) phi = TMath::Pi()+atan(x_PMT.at(detkey)/z_PMT.at(detkey));
    if (x_PMT.at(detkey)<0 && z_PMT.at(detkey)>0) phi = 3*TMath::Pi()/2+atan(-x_PMT.at(detkey)/z_PMT.at(detkey));
    phi_PMT.insert(std::pair<unsigned long,double>(detkey,phi));

    if (verbose > 2) std::cout <<"detectorkey: "<<detkey<<", position: ("<<position_PMT.X()<<","<<position_PMT.Y()<<","<<position_PMT.Z()<<")"<<std::endl;
    if (verbose > 2) std::cout <<"rho PMT "<<detkey<<": "<<rho<<std::endl;
    if (verbose > 2) std::cout <<"y PMT: "<<y_PMT.at(detkey)<<std::endl;

    PMT_ishit.insert(std::pair<unsigned long, int>(detkey,0));

    if (dettype.find("R5912") != std::string::npos) radius_PMT[detkey] = map_type_radius["R5912HQE"];
    else if (dettype.find("R7081") != std::string::npos) radius_PMT[detkey] = map_type_radius["R7081"];
    else if (dettype.find("D784KFLB") != std::string::npos) radius_PMT[detkey] = map_type_radius["D784KFLB"];
    else if (dettype.find("EMI9954KB") != std::string::npos) radius_PMT[detkey] = map_type_radius["EMI9954KB"];

    double expectedT = (sqrt(pow(x_PMT.at(detkey)-diffuser_x,2)+pow(y_PMT.at(detkey)-diffuser_y,2)+pow(z_PMT.at(detkey)-diffuser_z,2))-radius_PMT[detkey])/c_vacuum*n_water*1E9;
    expected_time.insert(std::pair<unsigned long,double>(detkey,expectedT));
  } 

  if (verbose > 1) std::cout <<"Number of tank PMTs: "<<n_tank_pmts<<std::endl;

  for (int i_pmt=0;i_pmt<n_tank_pmts;i_pmt++){
    unsigned long detkey = pmt_detkeys.at(i_pmt);
    if (verbose > 1) std::cout <<"Detkey: "<<detkey<<", Radius PMT "<<i_pmt<<": "<<radius_PMT[detkey]<<", expectedT: "<<expected_time[detkey]<<std::endl;
  }

  std::vector<unsigned long>::iterator it_minkey = std::min_element(pmt_detkeys.begin(),pmt_detkeys.end());
  std::vector<unsigned long>::iterator it_maxkey = std::max_element(pmt_detkeys.begin(),pmt_detkeys.end());

  int min_detkey = std::distance(pmt_detkeys.begin(),it_minkey);
  int max_detkey = std::distance(pmt_detkeys.begin(),it_maxkey);
  int n_detkey_bins = max_detkey-min_detkey;

  // User variables
  f_output = new TFile(TString(outputfile)+".root","RECREATE");
 
  h_Cluster_times = new TH1D("h_Cluster_times","Cluster times wrt trigger time",AcqTimeWindow,0,AcqTimeWindow);
  h_Cluster_charges = new TH1D("h_Cluster_charges","Cluster charges",10000,0,5);
  h_Cluster_deltaT = new TH1D("h_Cluster_deltaT","Time between first and current cluster", AcqTimeWindow,0,AcqTimeWindow);
  if (draw_2D){
    h_Cluster_charge_time = new TH2D("h_Cluster_charge_time","Cluster charges (P.E.) vs. time",AcqTimeWindow,0,AcqTimeWindow,1000,0,5);
    h_Cluster_charge_deltaT = new TH2D("h_Cluster_charge_deltaT","Cluster charges (P.E.) vs. #Delta t",AcqTimeWindow,0,AcqTimeWindow,1000,0,5);
  }
  m_all_clusters = new std::map<double,std::vector<Hit>>;
  m_all_clusters_MC = new std::map<double,std::vector<MCHit>>;
  m_all_clusters_detkey = new std::map<double,std::vector<unsigned long>>;

  return true;
}


bool ClusterFinder::Execute(){
 

  if (verbose > 0) std::cout <<"Executing Tool ClusterFinder ..."<<endl;

  // get the ANNIEEvent

  int annieeventexists = m_data->Stores.count("ANNIEEvent");
  if(!annieeventexists){ cerr<<"no ANNIEEvent store!"<<endl;}
  
  std::map<unsigned long, std::vector<std::vector<ADCPulse>>> RecoADCHits;

  // Some initialization
  v_hittimes.clear();
  v_hittimes_sorted.clear();
  v_mini_hits.clear();
  m_time_Nhits.clear();
  v_clusters.clear();
  v_local_cluster_times.clear();
  m_all_clusters->clear();
  m_all_clusters_MC->clear();
  m_all_clusters_detkey->clear();

  //----------------------------------------------------------------------------
  //---------------get the members of the ANNIEEvent----------------------------
  //----------------------------------------------------------------------------
  
  m_data->Stores["ANNIEEvent"]->Get("EventNumber", evnum);
  //m_data->Stores["ANNIEEvent"]->Get("BeamStatus", BeamStatus);
  bool got_recoadc = m_data->Stores["ANNIEEvent"]->Get("RecoADCData",RecoADCHits);

  if (HitStoreName == "MCHits"){
    bool got_mchits = m_data->Stores["ANNIEEvent"]->Get("MCHits", MCHits);
    if (!got_mchits){
      std::cout << "No MCHits store in ANNIEEvent!" <<  std::endl;
      return false;
    }
  } else if (HitStoreName == "Hits"){
    bool got_hits = m_data->Stores["ANNIEEvent"]->Get("Hits", Hits);
    if (!got_hits){
      std::cout << "No Hits store in ANNIEEvent! " << std::endl;
      return false;
    }
  } else {
    std::cout << "Selected Hits store invalid.  Must be Hits or MCHits" << std::endl;
    return false;
  }
  // Also load hits from the Hits Store, if available


  //----------------------------------------------------------------------------
  //---------------Read out charge and hit values of PMTs-----------------------
  //----------------------------------------------------------------------------

  for (int i_pmt = 0; i_pmt < n_tank_pmts; i_pmt++){
    unsigned long detkey = pmt_detkeys[i_pmt];
    PMT_ishit[detkey] = 0;
  }

  if(HitStoreName=="MCHits"){
    if (verbose > 0) std::cout << "ClusterFinder MC pulse_width: " << mc_pulse_width << " [ns]" << endl;
    int vectsize = MCHits->size();
    if (verbose > 3) std::cout <<"ClusterFinder tool: MCHits size: "<<vectsize<<std::endl;
    for(std::pair<unsigned long, std::vector<MCHit>>&& apair : *MCHits){
      unsigned long chankey = apair.first;
      Detector* thistube = geom->ChannelToDetector(chankey);
      int detectorkey = thistube->GetDetectorID();
      if (thistube->GetDetectorElement()=="Tank"){
        std::vector<MCHit>& ThisPMTHits = apair.second;
        PMT_ishit[detectorkey] = 1;
        std::vector<double> hits_2ns_res;
        std::vector<double> hits_2ns_res_charge;
        std::vector<double> datalike_hits;
        std::vector<double> datalike_hits_charge;
        for (MCHit &ahit : ThisPMTHits){
          if (ahit.GetTime() < end_of_window_time_cut*AcqTimeWindow) {
            //Make MC more like data --> combine multiple photons if they are within a 10ns range
            hits_2ns_res.push_back(ahit.GetTime());
            hits_2ns_res_charge.push_back(ahit.GetCharge());
          }
        }
	//Combine multiple MC hits to one pulse
        std::sort(hits_2ns_res.begin(),hits_2ns_res.end());
	std::vector<double> temp_times;
	double temp_charges = 0.0;
	double mid_time;
	if (verbose > 0){
		std::cout << " " << std::endl;
		std::cout << hits_2ns_res.size() << " total photon hits(s)" << std::endl;
	}
	
	if (hits_2ns_res.size() > 0){
          for (int i_hit=0; i_hit < (int) hits_2ns_res.size(); i_hit++){
            double hit1 = hits_2ns_res.at(i_hit);
            if (temp_times.size()==0) {
              temp_times.push_back(hit1);
              temp_charges += hits_2ns_res_charge.at(i_hit);
            }
        
	else {
            bool new_pulse = false;
        	if (fabs(temp_times[0]-hit1)< mc_pulse_width) {
                new_pulse=false;
                temp_charges+=hits_2ns_res_charge.at(i_hit);
		temp_times.push_back(hit1);
              } else new_pulse=true;
		  
            if (new_pulse) {
		// following the DigitBuilder tool --> take median photon hit time as the hit time of the "pulse"
		if (temp_times.size() % 2 == 0){
                  mid_time = (temp_times.at(temp_times.size()/2 - 1) + temp_times.at(temp_times.size()/2))/2;
                } else{
                  mid_time = temp_times.at(temp_times.size()/2);
                }

              datalike_hits.push_back(mid_time);
              datalike_hits_charge.push_back(temp_charges);
              temp_times.clear();
              temp_charges = 0;
              temp_times.push_back(hit1);
              temp_charges += hits_2ns_res_charge.at(i_hit);
              
            }
          }
        }

	if (temp_times.size() % 2 == 0){
            mid_time = (temp_times.at(temp_times.size()/2 - 1) + temp_times.at(temp_times.size()/2))/2;
        } else{
            mid_time = temp_times.at(temp_times.size()/2);
        }

        datalike_hits.push_back(mid_time);
        datalike_hits_charge.push_back(temp_charges);

        if (verbose > 0){
          std::cout << " " << std::endl;
          std::cout << datalike_hits.size() << " MC pulse(s) identified from the raw photon hit(s)" << std::endl;
          std::cout << "Pulse time(s):" << std::endl;
          for (int ih=0; ih < (int) datalike_hits.size(); ih++){
            double junk = datalike_hits.at(ih);
            std::cout << junk << " ";
          }
          std::cout << " " << std::endl;
          std::cout << "Pulse charge(s):" << std::endl;
          for (int ih=0; ih < (int) datalike_hits_charge.size(); ih++){
            double junk2 = datalike_hits_charge.at(ih);
            std::cout << junk2 << " ";
          }
          std::cout << " " << std::endl;
        }
        }

        for (int i_hit = 0; i_hit < (int) datalike_hits.size(); i_hit++){
          //v_hittimes.push_back(ahit.GetTime()); // fill a vector with all hit times (unsorted)
          v_hittimes.push_back(datalike_hits.at(i_hit));
        }
        std::vector<int> parents = *(ThisPMTHits.at(0).GetParents());
        ThisPMTHits.clear();
        std::vector<MCHit> newMCHits;
        for (int i_hit=0; i_hit < (int) datalike_hits.size(); i_hit++){
          newMCHits.push_back(MCHit(chankey,datalike_hits.at(i_hit),datalike_hits_charge.at(i_hit),parents));
        }
        MCHits->at(chankey) = newMCHits;
      }
    }
    m_data->Stores["ANNIEEvent"]->Set("MCHits",MCHits,true);
  }

  if(HitStoreName=="Hits"){
    int vectsize = Hits->size();
    if (verbose > 0) std::cout <<"Hits size: "<<vectsize<<std::endl;
    for(std::pair<unsigned long, std::vector<Hit>>&& apair : *Hits){
      unsigned long chankey = apair.first;
      Detector* thistube = geom->ChannelToDetector(chankey);
      int detectorkey = thistube->GetDetectorID();
      if (thistube->GetDetectorElement()=="Tank"){
        std::vector<Hit>& ThisPMTHits = apair.second;
        PMT_ishit[detectorkey] = 1;
        for (Hit &ahit : ThisPMTHits){
          if (verbose > 2) std::cout << "Key: " << detectorkey << ", charge "<<ahit.GetCharge()<<", time "<<ahit.GetTime()<<std::endl;
          if (ahit.GetTime() < end_of_window_time_cut*AcqTimeWindow) v_hittimes.push_back(ahit.GetTime()); // fill a vector with all hit times (unsorted)
        }
      }
    }
  }

  
  if (v_hittimes.size() == 0) {
    if (verbose > 1) cout << "No hits, event is skipped..." << endl;
      if (HitStoreName == "Hits") m_data->CStore.Set("ClusterMap",m_all_clusters);
      else if (HitStoreName == "MCHits") m_data->CStore.Set("ClusterMapMC",m_all_clusters_MC);
      m_data->CStore.Set("ClusterMapDetkey",m_all_clusters_detkey);
      return true;
  }

  if (verbose > 2) {
    for (std::vector<double>::iterator it = v_hittimes.begin(); it != v_hittimes.end(); ++it) {
      cout << "Hit time -> " << *it << endl;
    }
  }

  // Now sort the hit time array, fill the highest time in a new array until the old array is empty
  do {
    double max_time = -9999;
    int i_max_time = 0;
    for (std::vector<double>::iterator it = v_hittimes.begin(); it != v_hittimes.end(); ++it) {
      if (*it > max_time) {
        max_time = *it;
        i_max_time = std::distance(v_hittimes.begin(),it);
      } 
    }
    v_hittimes_sorted.insert(v_hittimes_sorted.begin(),max_time);
    v_hittimes.erase(v_hittimes.begin() + i_max_time);
  } while (v_hittimes.size() != 0);
  
  if (verbose > 2) {
    for (std::vector<double>::iterator it = v_hittimes_sorted.begin(); it != v_hittimes_sorted.end(); ++it) {
      cout << "Hit time (sorted) -> " << *it << endl;
    }
  }

  // Move a time window within the array and look for the window with the highest number of hits
  for (std::vector<double>::iterator it = v_hittimes_sorted.begin(); it != v_hittimes_sorted.end(); ++it) {
    if (*it + ClusterFindingWindow > AcqTimeWindow || *it > end_of_window_time_cut*AcqTimeWindow) {
      if (verbose > 2) cout << "Cluster Finding loop: Reaching the end of the acquisition time window.." << endl;
      break;
    }
    thiswindow_Nhits = 0;   
    v_mini_hits.clear();
    for (double j_time = *it; j_time < *it + ClusterFindingWindow; j_time+=1){  // loops through times in the window and check if there's a hit at this time
      for(std::vector<double>::iterator it2 = v_hittimes_sorted.begin(); it2 != v_hittimes_sorted.end(); ++it2) {
        if(HitStoreName=="MCHits"){
          if (static_cast<int>(j_time) == static_cast<int>(*it2)) {     // accept all hit times (some may be smeared to negative values)
            thiswindow_Nhits++;
            v_mini_hits.push_back(*it2);
          }
        }
        if(HitStoreName=="Hits"){
          if (*it2 > 0 && static_cast<int>(j_time) == static_cast<int>(*it2)) {  // reject hit times in the data that are 0
            thiswindow_Nhits++;
            v_mini_hits.push_back(*it2);
          }
        }

      }
    }
    if (!v_mini_hits.empty()) {
      m_time_Nhits.insert(std::pair<double,std::vector<double>>(*it,v_mini_hits)); // fill a map with a pair (window start time; vector of hit times in window)  
    }
  }
  if (verbose > 1) cout << "Map of times and Nhits filled..." << endl;
  if (verbose > 2){
      for (std::map<double,std::vector<double>>::iterator it = m_time_Nhits.begin(); it != m_time_Nhits.end(); ++it) {
        if (int(it->second.size()) > MinHitsPerCluster) {
          if(verbose>3) cout << "Map of time and NHits: Time = " << it->first << ", NHits = " << it->second.size() << endl;
          if(verbose>3) cout << "Look at the back of the vector (before): " << it->second.back() << endl;
          for (std::vector<double>::iterator itt = it->second.begin(); itt != it->second.end(); ++itt) {
          cout << "At this time, hits are: " << *itt << endl;  
        }       
      }
    }
  }
  v_mini_hits.clear();

  // Now loop on the time/Nhits map to find maxima (clusters) 
  max_Nhits = 0;
  local_cluster = 0;
  v_clusters.clear();
  do {
    //cout << "Start do loop" << endl;
    max_Nhits = 0;
    for (std::map<double,std::vector<double>>::iterator it = m_time_Nhits.begin(); it != m_time_Nhits.end(); ++it) {
      if (int(it->second.size()) > max_Nhits) {
        max_Nhits = it->second.size();
        local_cluster = it->first;
      } 
    }
    if (max_Nhits < MinHitsPerCluster) {
      if (verbose > 1 ) cout << "No more clusters with > " << MinHitsPerCluster<< " hits" << endl; 
      break;
    } else {
      if (verbose > 0) cout << "Cluster found at " << local_cluster << " ns with " << max_Nhits << " hits" << endl;
      v_clusters.push_back(local_cluster);
      // Remove the cluster and its surroundings for the next loop over the cluster map
      for (std::map<double,std::vector<double>>::iterator it = m_time_Nhits.begin(); it != m_time_Nhits.end(); ++it) {
        //cout << "On the map hits: time " << it->first << " and hits tot " << it->second.size() << endl;
        //cout << "Look at the back of the vector (between): " << it->second.back() << endl;
        for (std::vector<double>::iterator itt = it->second.begin(); itt != it->second.end(); ++itt) {
          //cout << "hits are " << *itt << endl;
          if (*itt >= local_cluster && *itt <= local_cluster + ClusterFindingWindow) { //if hit time is in the window, replace it with dummy value to be removed later
            it->second.at(std::distance(it->second.begin(), itt)) = dummy_hittime_value;
          }
        }
        //cout << "Loop of setting dummy hit time values is done..." << endl;
        //cout << "Before erasing values, vector of hits is " << it->second.size() << " hits long" << endl;
        //cout << "Look at the back of the vector (after): " << it->second.back() << endl;

        // This loops erases the dummy hit times values that were flagged before so they are not used anymore by other clusters
        for(std::vector<double>::iterator itt = it->second.end()-1; itt != it->second.begin()-1; --itt) {
          if (verbose > 2) cout << "Time: " << it->first << ", hit time: " << *itt << endl;
          if (*itt == dummy_hittime_value) {
            it->second.erase(it->second.begin() + std::distance(it->second.begin(), itt)); 
            if (verbose > 2) cout << "Erasing " << it->first << " " << *itt << endl;
          }
        }
        if (verbose > 2) cout << "Erasing loop is done and new size of mini_hits is " << it->second.size() << " hits" << endl;
      }
    }
  } while (true); 
  m_time_Nhits.clear();

  // Now loop on the hit map again to get info about those local maxima, cluster per cluster
  for (std::vector<double>::iterator it = v_clusters.begin(); it != v_clusters.end(); ++it) {
    double local_cluster_charge = 0;
    double local_cluster_time = 0;
    v_local_cluster_times.clear();
    if (HitStoreName == "Hits"){
      for(std::pair<unsigned long, std::vector<Hit>>&& apair : *Hits){
        unsigned long chankey = apair.first;
        Detector* thistube = geom->ChannelToDetector(chankey);
        int detectorkey = thistube->GetDetectorID();
        if (thistube->GetDetectorElement()=="Tank"){
          std::vector<Hit>& ThisPMTHits = apair.second;
          PMT_ishit[detectorkey] = 1;
          for (Hit &ahit : ThisPMTHits){
            if (ahit.GetTime() >= *it && ahit.GetTime() <= *it + ClusterFindingWindow) {
              local_cluster_charge += ahit.GetCharge();
              v_local_cluster_times.push_back(ahit.GetTime());
              if (verbose > 2) cout << "Local cluster at " << *it << " and hit is " << ahit.GetTime() << endl;
            }
          }
        }
      }
    } else if (HitStoreName == "MCHits"){
      for(std::pair<unsigned long, std::vector<MCHit>>&& apair : *MCHits){
        unsigned long chankey = apair.first;
        Detector* thistube = geom->ChannelToDetector(chankey);
        int detectorkey = thistube->GetDetectorID();
        if (thistube->GetDetectorElement()=="Tank"){
          std::vector<MCHit>& ThisPMTHits = apair.second;
          PMT_ishit[detectorkey] = 1;
          for (MCHit &ahit : ThisPMTHits){
            if (ahit.GetTime() >= *it && ahit.GetTime() <= *it + ClusterFindingWindow) {
              local_cluster_charge += ahit.GetCharge();
              v_local_cluster_times.push_back(ahit.GetTime());
              if (verbose > 2) cout << "Local cluster at " << *it << " and hit is " << ahit.GetTime() << endl;
            }
          }
        }
      }
    }
    
    for (std::vector<double>::iterator itt = v_local_cluster_times.begin(); itt != v_local_cluster_times.end(); ++itt) {
     local_cluster_time += *itt;
    }
    local_cluster_time /= v_local_cluster_times.size();
    if (verbose > 0) cout << "Local cluster at " << local_cluster_time << " ns with a total charge of " << local_cluster_charge << " (" << v_local_cluster_times.size() << " hits)" << endl;
    h_Cluster_times->Fill(local_cluster_time);
    h_Cluster_charges->Fill(local_cluster_charge);
    if (draw_2D) h_Cluster_charge_time->Fill(local_cluster_time,local_cluster_charge);
    if (v_clusters.size() > 1) {
      h_Cluster_deltaT->Fill(local_cluster_time - *std::min_element(v_clusters.begin(),v_clusters.end()));
      if (draw_2D) h_Cluster_charge_deltaT->Fill(local_cluster_time - *std::min_element(v_clusters.begin(),v_clusters.end()),local_cluster_charge);
    }
    if (verbose > 2) cout << "Next cluster ..." << endl;

    // Fills the map of clusters (to be passed through CStore)
    if (HitStoreName == "Hits"){
      for(std::pair<unsigned long, std::vector<Hit>>&& apair : *Hits) {   
        unsigned long chankey = apair.first;
        Detector* thistube = geom->ChannelToDetector(chankey);
        unsigned long detectorkey = thistube->GetDetectorID();
        if (thistube->GetDetectorElement()=="Tank"){
          std::vector<Hit>& ThisPMTHits = apair.second;
          PMT_ishit[detectorkey] = 1;
          for (Hit &ahit : ThisPMTHits){
            if (ahit.GetTime() >= *it && ahit.GetTime() <= *it + ClusterFindingWindow) { 
              if(m_all_clusters->count(local_cluster_time)==0) {
                m_all_clusters->emplace(local_cluster_time, std::vector<Hit>{ahit});
                m_all_clusters_detkey->emplace(local_cluster_time, std::vector<unsigned long>{detectorkey});
              } else { 
                m_all_clusters->at(local_cluster_time).push_back(ahit);
                m_all_clusters_detkey->at(local_cluster_time).push_back(detectorkey);
              }
            }
          }  
        }
      }
    } else if (HitStoreName == "MCHits"){
      for(std::pair<unsigned long, std::vector<MCHit>>&& apair : *MCHits) {   
        unsigned long chankey = apair.first;
        Detector* thistube = geom->ChannelToDetector(chankey);
        unsigned long detectorkey = thistube->GetDetectorID();
        if (thistube->GetDetectorElement()=="Tank"){
          std::vector<MCHit>& ThisPMTHits = apair.second;
          PMT_ishit[detectorkey] = 1;
          for (MCHit &ahit : ThisPMTHits){
            if (ahit.GetTime() >= *it && ahit.GetTime() <= *it + ClusterFindingWindow) { 
              if(m_all_clusters_MC->count(local_cluster_time)==0) {
                m_all_clusters_MC->emplace(local_cluster_time, std::vector<MCHit>{ahit});
                m_all_clusters_detkey->emplace(local_cluster_time, std::vector<unsigned long>{detectorkey});
              } else { 
                m_all_clusters_MC->at(local_cluster_time).push_back(ahit);
                m_all_clusters_detkey->at(local_cluster_time).push_back(detectorkey);
              }
            }
          }  
        }
      }
    }
  }

  // Load the cluster map in a CStore for use by a subsequent tool
  if (HitStoreName == "Hits") m_data->CStore.Set("ClusterMap",m_all_clusters);
  else if (HitStoreName == "MCHits") m_data->CStore.Set("ClusterMapMC",m_all_clusters_MC);
  m_data->CStore.Set("ClusterMapDetkey",m_all_clusters_detkey);

  //check whether PMT_ishit is filled correctly
  for (int i_pmt = 0; i_pmt < n_tank_pmts ; i_pmt++){
    unsigned long detkey = pmt_detkeys[i_pmt];
    if (verbose > 2) std::cout <<"PMT "<<i_pmt<<" is hit: "<<PMT_ishit[detkey]<<std::endl;
  }    

  //----------------------------------------------------------------------------
  //---------------Read out RecoADCHits properties of PMTs----------------------
  //----------------------------------------------------------------------------

  if (got_recoadc){

    int recoadcsize = RecoADCHits.size();
    int adc_loop = 0;
    if (verbose > 0) std::cout <<"RecoADCHits size: "<<recoadcsize<<std::endl;
    for (std::pair<unsigned long, std::vector<std::vector<ADCPulse>>> apair : RecoADCHits){
      unsigned long chankey = apair.first;
      Detector *thistube = geom->ChannelToDetector(chankey);
      int detectorkey = thistube->GetDetectorID();
      if (thistube->GetDetectorElement()=="Tank"){
        std::vector<std::vector<ADCPulse>> pulses = apair.second;
        for (int i_minibuffer = 0; i_minibuffer < int(pulses.size()); i_minibuffer++){
          std::vector<ADCPulse> apulsevector = pulses.at(i_minibuffer);
          for (int i_pulse=0; i_pulse < int(apulsevector.size()); i_pulse++){
            ADCPulse apulse = apulsevector.at(i_pulse);
            double start_time = apulse.start_time();
            double peak_time = apulse.peak_time();
            double baseline = apulse.baseline();
            double sigma_baseline = apulse.sigma_baseline();
            double raw_amplitude = apulse.raw_amplitude();
            double amplitude = apulse.amplitude();
            double raw_area = apulse.raw_area();
          }
        }
      }else {
        if (verbose > 0) std::cout <<"ClusterFinder: RecoADCHit does not belong to a tank PMT and is ommited. Detector key/element = "<<detectorkey<<" / "<<thistube->GetDetectorElement()<<std::endl;
      }
      adc_loop++;    
    }

  } else {

    if (verbose > 0) std::cout <<"ClusterFinder: RecoADCHits Store does not exist and is not read out"<<std::endl;
  }

  return true;
}


bool ClusterFinder::Finalise(){

  f_output->cd();
  /*canvas_Cluster = new TCanvas("canvas_Cluster","canvas_Cluster",1200,1200);
  canvas_Cluster->Divide(2,2);
  canvas_Cluster->cd(1);
  h_Cluster_times->Draw();
  canvas_Cluster->cd(2);
  h_Cluster_charges->Draw();
  canvas_Cluster->cd(3);
  h_Cluster_deltaT->Draw();
  canvas_Cluster->SaveAs("Cluster.jpg"); 
  canvas_Cluster->Write();*/
  
  h_Cluster_times->Write();
  h_Cluster_charges->Write();
  h_Cluster_deltaT->Write();
  if (draw_2D){
    h_Cluster_charge_time->Write();
    h_Cluster_charge_deltaT->Write();
  }
  f_output->Close();
  
  Log("ClusterFinder exiting...",v_message,verbose);

  return true;
}
