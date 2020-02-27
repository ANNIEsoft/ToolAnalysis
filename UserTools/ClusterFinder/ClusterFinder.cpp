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

  m_all_clusters = new std::map<double,std::vector<Hit>>;

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
  m_time_Nhits.clear();
  v_clusters.clear();
  v_local_cluster_times.clear();
  m_all_clusters->clear();

  //----------------------------------------------------------------------------
  //---------------get the members of the ANNIEEvent----------------------------
  //----------------------------------------------------------------------------
  
  m_data->Stores["ANNIEEvent"]->Get("EventNumber", evnum);
  m_data->Stores["ANNIEEvent"]->Get("BeamStatus", BeamStatus);
  bool got_recoadc = m_data->Stores["ANNIEEvent"]->Get("RecoADCHits",RecoADCHits);

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
    int vectsize = MCHits->size();
    if (verbose > 0) std::cout <<"MCHits size: "<<vectsize<<std::endl;
    for(std::pair<unsigned long, std::vector<MCHit>>&& apair : *MCHits){
      unsigned long chankey = apair.first;
      Detector* thistube = geom->ChannelToDetector(chankey);
      int detectorkey = thistube->GetDetectorID();
      if (thistube->GetDetectorElement()=="Tank"){
        std::vector<MCHit>& ThisPMTHits = apair.second;
        PMT_ishit[detectorkey] = 1;
        for (MCHit &ahit : ThisPMTHits){
          if (verbose > 2) std::cout <<"Charge "<<ahit.GetCharge()<<", time "<<ahit.GetTime()<<std::endl;
        }
      }
    }
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
          v_hittimes.push_back(ahit.GetTime()); // fill a vector with all hit times (unsorted)
        }
      }
    }
  }

  if (v_hittimes.size() == 0) {
    if (verbose > 1) cout << "No hits, event is skipped..." << endl;
    return true;
  }

  if (verbose > 2) {
    for (std::vector<double>::iterator it = v_hittimes.begin(); it != v_hittimes.end(); ++it) {
      cout << "Hit time -> " << *it << endl;
    }
  }

  // Now sort the hit time array, fill the highest time in a new array until the old array is empty
  do {
    double max_time =0;
    int i_max_time =0;
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
  for (double i_time = v_hittimes_sorted.at(0); i_time <= v_hittimes_sorted.at(v_hittimes_sorted.size()-1); i_time++){
    if (i_time + ClusterFindingWindow > AcqTimeWindow || i_time > 0.9*AcqTimeWindow) {
      if (verbose > 2) cout << "Cluster Finding loop: Reaching the end of the acquisition time window.." << endl;
      break;
    }
    thiswindow_Nhits = 0;   
    for (double j_time = i_time; j_time < i_time + ClusterFindingWindow; j_time++){  // loops through times in the window and check if there's a hit at this time
      for(std::vector<double>::iterator it = v_hittimes_sorted.begin(); it != v_hittimes_sorted.end(); ++it) {
        if (*it == j_time) {
          thiswindow_Nhits++;
        }
      }
    }
    m_time_Nhits.insert(std::pair<double,int>(i_time,thiswindow_Nhits)); // fill a map with a pair (window start time; number of hits in window)  
  }
  if (verbose > 1) cout << "Map of times and Nhits filled..." << endl;

  // Now loop on the time/Nhits map to find maxima (clusters) 
  max_Nhits = 0;
  local_cluster = 0;
  v_clusters.clear();
  do {
    max_Nhits = 0;
    for (std::map<double,int>::iterator it = m_time_Nhits.begin(); it != m_time_Nhits.end(); ++it) {
      if (it->second > max_Nhits) {
        max_Nhits = it->second;
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
      for (std::map<double,int>::iterator it = m_time_Nhits.begin(); it != m_time_Nhits.end(); ++it) {
        if (it->first > local_cluster - ClusterIntegrationWindow/2 && it->first < local_cluster + ClusterIntegrationWindow/2) {
          if (verbose > 2) cout << "Erasing " << it->first << " " << it->second << endl;
          m_time_Nhits.erase(it);
        } 
      }
    }
  } while (true); 

  // Now loop on the hit map again to get info about those local maxima, cluster per cluster
  for (std::vector<double>::iterator it = v_clusters.begin(); it != v_clusters.end(); ++it) {
    double local_cluster_charge = 0;
    double local_cluster_time = 0;
    v_local_cluster_times.clear();
    for(std::pair<unsigned long, std::vector<Hit>>&& apair : *Hits){
      unsigned long chankey = apair.first;
      Detector* thistube = geom->ChannelToDetector(chankey);
      int detectorkey = thistube->GetDetectorID();
      if (thistube->GetDetectorElement()=="Tank"){
        std::vector<Hit>& ThisPMTHits = apair.second;
        PMT_ishit[detectorkey] = 1;
        for (Hit &ahit : ThisPMTHits){
          if (ahit.GetTime() > *it + ClusterFindingWindow/2 - ClusterIntegrationWindow/2 && ahit.GetTime() < *it + ClusterFindingWindow/2 + ClusterIntegrationWindow/2) {
            local_cluster_charge += ahit.GetCharge();
            v_local_cluster_times.push_back(ahit.GetTime());
          }
        }
      }
    }
    
    if (verbose > 2) cout << "Next cluster ..." << endl;
    for (std::vector<double>::iterator itt = v_local_cluster_times.begin(); itt != v_local_cluster_times.end(); ++itt) {
     local_cluster_time += *itt;
    }
    local_cluster_time /= v_local_cluster_times.size();
    if (verbose > 0) cout << "Local cluster at " << local_cluster_time << " ns with a total charge of " << local_cluster_charge << endl;
    h_Cluster_times->Fill(local_cluster_time);
    h_Cluster_charges->Fill(local_cluster_charge);
    if (v_clusters.size() > 1) h_Cluster_deltaT->Fill(local_cluster_time - *std::min_element(v_clusters.begin(),v_clusters.end()));

    // Fills the map of clusters (to be passed through CStore)
    for(std::pair<unsigned long, std::vector<Hit>>&& apair : *Hits) {   
      unsigned long chankey = apair.first;
      Detector* thistube = geom->ChannelToDetector(chankey);
      int detectorkey = thistube->GetDetectorID();
      if (thistube->GetDetectorElement()=="Tank"){
        std::vector<Hit>& ThisPMTHits = apair.second;
        PMT_ishit[detectorkey] = 1;
        for (Hit &ahit : ThisPMTHits){
          if (ahit.GetTime() > *it + ClusterFindingWindow/2 - ClusterIntegrationWindow/2 && ahit.GetTime() < *it + ClusterFindingWindow/2 + ClusterIntegrationWindow/2) { 
            if(m_all_clusters->count(local_cluster_time)==0) {
              m_all_clusters->emplace(local_cluster_time, std::vector<Hit>{ahit});
            } else { 
              m_all_clusters->at(local_cluster_time).push_back(ahit);
            }
          }
        }  
      }
    }
  
  }

  // Load the cluster map in a CStore for use by a subsequent tool
  m_data->CStore.Set("ClusterMap",m_all_clusters);

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
        for (int i_minibuffer = 0; i_minibuffer < pulses.size(); i_minibuffer++){
          std::vector<ADCPulse> apulsevector = pulses.at(i_minibuffer);
          for (int i_pulse=0; i_pulse < apulsevector.size(); i_pulse++){
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

    std::cout <<"ClusterFinder: RecoADCHits Store does not exist and is not read out"<<std::endl;
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
  f_output->Close();
  
  cout <<"ClusterFinder exiting..."<<endl;
  return true;
}
