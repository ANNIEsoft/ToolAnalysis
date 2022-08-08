#include "ClusterClassifiers.h"

ClusterClassifiers::ClusterClassifiers():Tool(){}


bool ClusterClassifiers::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  isData = 1;
  verbosity = 0;

  m_data->CStore.Get("ChannelNumToTankPMTSPEChargeMap",ChannelKeyToSPEMap);

  m_variables.Get("verbosity",verbosity);
  m_variables.Get("IsData",isData);

  auto get_geometry= m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",geom);
  if(!get_geometry){
  	Log("ClusterClassifiers Tool: Error retrieving Geometry from ANNIEEvent!",v_error,verbosity); 
  	return false; 
  }

  m_data->CStore.Get("pmt_tubeid_to_channelkey_data",pmtid_to_channelkey);
  m_data->CStore.Get("channelkey_to_pmtid",channelkey_to_pmtid);

  return true;
}


bool ClusterClassifiers::Execute(){

  //We're gonna make ourselves a couple cluster classifier maps boyeeee
  if(verbosity>4) std::cout << "ClusterClassifiers tool: Accessing cluster map in CStore" << std::endl;
  bool get_clusters = false;
  m_data->CStore.Get("ClusterMap",m_all_clusters);
  if (isData){
     get_clusters = m_data->CStore.Get("ClusterMap",m_all_clusters);
     if(!get_clusters){
       std::cout << "ClusterClassifiers tool: No clusters found!" << std::endl;
       return false;
     }
   } else {
     get_clusters = m_data->CStore.Get("ClusterMapMC",m_all_clusters_MC);
     if(!get_clusters){
       std::cout << "ClusterClassifiers tool: No clusters found! (MC)" << std::endl;
       return false;
     }
   }
   get_clusters = m_data->CStore.Get("ClusterMapDetkey",m_all_clusters_detkey);
   if (!get_clusters){
     std::cout << "ClusterClassifiers tool: No ClusterMapDetkey! (MC)" << std::endl;
     return false;
  }
  if(verbosity>3) std::cout << "ClusterClassifiers Tool: looping through clusters to get cluster info now" << std::endl;
  
  std::map<double,double> ClusterMaxPEs;
  std::map<double,Position> ClusterChargePoints;
  std::map<double,double> ClusterChargeBalances;

  if (isData){
    for (std::pair<double,std::vector<Hit>>&& cluster_pair : *m_all_clusters) {
      double cluster_time = cluster_pair.first;
      std::vector<Hit> cluster_hits = cluster_pair.second;
      if(verbosity>4) std::cout << "ClusterClassifiers Tool: cluster of hit time " << cluster_time << "processing.." << std::endl;
      Position ChargePoint = this->CalculateChargePoint(cluster_hits);
      ClusterChargePoints.emplace(cluster_time,ChargePoint);
      double ChargeBalance = this->CalculateChargeBalance(cluster_hits);
      ClusterChargeBalances.emplace(cluster_time,ChargeBalance);
      double max_PE = this->CalculateMaxPE(cluster_hits);
      ClusterMaxPEs.emplace(cluster_time,max_PE);
    }
  } else {
     for (std::pair<double,std::vector<MCHit>>&& cluster_pair : *m_all_clusters_MC) {
       double cluster_time = cluster_pair.first;
       std::vector<MCHit> cluster_hits = cluster_pair.second;
       std::vector<unsigned long> cluster_detkey = m_all_clusters_detkey->at(cluster_time);
       if(verbosity>4) std::cout << "ClusterClassifiers Tool: cluster of hit time " << cluster_time << "processing.." << std::endl;
       Position ChargePoint = this->CalculateChargePointMC(cluster_hits,cluster_detkey);
       ClusterChargePoints.emplace(cluster_time,ChargePoint);
       double ChargeBalance = this->CalculateChargeBalanceMC(cluster_hits,cluster_detkey);
       ClusterChargeBalances.emplace(cluster_time,ChargeBalance);
       double max_PE = this->CalculateMaxPEMC(cluster_hits,cluster_detkey);
       ClusterMaxPEs.emplace(cluster_time,max_PE);
     }
  }

  //Save classifiers to ANNIEEvent
  if(verbosity>4) std::cout << "ClusterClassifiers Tool: Save classifiers to ANNIEEvent" << std::endl;
  m_data->Stores.at("ANNIEEvent")->Set("ClusterChargePoints", ClusterChargePoints);
  m_data->Stores.at("ANNIEEvent")->Set("ClusterChargeBalances", ClusterChargeBalances);
  m_data->Stores.at("ANNIEEvent")->Set("ClusterMaxPEs", ClusterMaxPEs);
  return true;
}


bool ClusterClassifiers::Finalise(){
  std::cout << "ClusterClassifiers tool exitting" << std::endl;
  return true;
}

Position ClusterClassifiers::CalculateChargePoint(std::vector<Hit> cluster_hits)
{
  if(verbosity>4) std::cout << "Calculating charge point" << std::endl;
  double x_weight = 0;
  double y_weight = 0;
  double z_weight = 0;
  Position detector_center=geom->GetTankCentre();
  double tank_center_x = detector_center.X();
  double tank_center_y = detector_center.Y();
  double tank_center_z = detector_center.Z();

  for (int i = 0; i < (int) cluster_hits.size(); i++){
    Hit ahit = cluster_hits.at(i); 
    double hit_charge = ahit.GetCharge();
    int channel_key = ahit.GetTubeId();
    double hit_PE = 0;
    std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(channel_key);
    if(it != ChannelKeyToSPEMap.end()){ //Charge to SPE conversion is available
      hit_PE  = hit_charge / ChannelKeyToSPEMap.at(channel_key);
      Detector* this_detector = geom->ChannelToDetector(channel_key);
      Position det_position = this_detector->GetDetectorPosition();
      double pos_mag = sqrt(pow(det_position.X(),2) + pow(det_position.Y(),2) + pow(det_position.Z(),2));
      x_weight+= (det_position.X()-tank_center_x)*hit_PE/pos_mag;
      y_weight+= (det_position.Y()-tank_center_y)*hit_PE/pos_mag;
      z_weight+= (det_position.Z()-tank_center_z)*hit_PE/pos_mag;
    }
  }

  Position charge_weight(x_weight,y_weight,z_weight);
  if(verbosity>4) std::cout << "ClusterClassifiers Tool: Calculated charge weight direction of  (" << charge_weight.X() << "," << charge_weight.Y() << "," << charge_weight.Z() << ")" << std::endl;
  return charge_weight;
}

Position ClusterClassifiers::CalculateChargePointMC(std::vector<MCHit> cluster_hits, std::vector<unsigned long> cluster_detkeys)
 {
   if(verbosity>4) std::cout << "Calculating charge point" << std::endl;
   double x_weight = 0;
   double y_weight = 0;
   double z_weight = 0;
   Position detector_center=geom->GetTankCentre();
   double tank_center_x = detector_center.X();
   double tank_center_y = detector_center.Y();
   double tank_center_z = detector_center.Z();

   for (int i = 0; i < (int) cluster_hits.size(); i++){
     MCHit ahit = cluster_hits.at(i); 
     double hit_PE = ahit.GetCharge();
     unsigned long detkey = cluster_detkeys.at(i);
     int channel_key = (int) detkey;
     int tubeid = ahit.GetTubeId();
     unsigned long utubeid = (unsigned long) tubeid;
     int wcsimid = channelkey_to_pmtid.at(tubeid);
     unsigned long detkey_data = pmtid_to_channelkey[wcsimid];
     int channel_key_data = (int) detkey_data;
     double hit_charge = 0;
     std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(channel_key_data);
     if(it != ChannelKeyToSPEMap.end()){ //Charge to SPE conversion is available
       hit_charge  = hit_PE * ChannelKeyToSPEMap.at(channel_key_data);
       Detector* this_detector = geom->ChannelToDetector(tubeid);
       Position det_position = this_detector->GetDetectorPosition();
       double pos_mag = sqrt(pow(det_position.X(),2) + pow(det_position.Y(),2) + pow(det_position.Z(),2));
       x_weight+= (det_position.X()-tank_center_x)*hit_PE/pos_mag;
       y_weight+= (det_position.Y()-tank_center_y)*hit_PE/pos_mag;
       z_weight+= (det_position.Z()-tank_center_z)*hit_PE/pos_mag;
     }
   }

   Position charge_weight(x_weight,y_weight,z_weight);
   if(verbosity>4) std::cout << "ClusterClassifiers Tool: Calculated charge weight direction of  (" << charge_weight.X() << "," << charge_weight.Y() << "," << charge_weight.Z() << ")" << std::endl;
   return charge_weight;
 }


double ClusterClassifiers::CalculateChargeBalance(std::vector<Hit> cluster_hits)
{
  double total_Q = 0;
  double total_QSquared = 0;
  std::map<int, double> CBMap;
  for (int i = 0; i < cluster_hits.size(); i++){
    Hit ahit = cluster_hits.at(i); 
    double hit_charge = ahit.GetCharge();
    int hit_ID = ahit.GetTubeId();
    std::map<int, double>::iterator it = CBMap.find(hit_ID);
    if(it != CBMap.end()){ //A hit from this tube has been seen before
      CBMap.at(hit_ID)+=hit_charge;
    } else {
      CBMap.emplace(hit_ID,hit_charge);
    }
  }
  for (std::pair<int,double> CBPair : CBMap) {
    double tube_charge = CBPair.second;
    total_Q+= tube_charge;
    total_QSquared += (tube_charge * tube_charge);
  }
  //FIXME: Need a method to have the 123 be equal to the number of operating detectors
  double charge_balance  = sqrt((total_QSquared)/(total_Q*total_Q) - (1./123.));
  if(verbosity>4) std::cout << "ClusterClassifiers Tool: Calculated charge balance of " << charge_balance << std::endl;
  return charge_balance;
}

double ClusterClassifiers::CalculateChargeBalanceMC(std::vector<MCHit> cluster_hits, std::vector<unsigned long> cluster_detkeys)
 {
   double total_Q = 0;
   double total_QSquared = 0;
   std::map<int, double> CBMap;
   for (int i = 0; i < (int) cluster_hits.size(); i++){
     MCHit ahit = cluster_hits.at(i); 
     double hit_charge = ahit.GetCharge();
     int hit_ID = ahit.GetTubeId();
     std::map<int, double>::iterator it = CBMap.find(hit_ID);
     if(it != CBMap.end()){ //A hit from this tube has been seen before
       CBMap.at(hit_ID)+=hit_charge;
     } else {
       CBMap.emplace(hit_ID,hit_charge);
     }
   }
   for (std::pair<int,double> CBPair : CBMap) {
     double tube_charge = CBPair.second;
     total_Q+= tube_charge;
     total_QSquared += (tube_charge * tube_charge);
   }
   //FIXME: Need a method to have the 123 be equal to the number of operating detectors
   double charge_balance  = sqrt((total_QSquared)/(total_Q*total_Q) - (1./123.));
   if(verbosity>4) std::cout << "ClusterClassifiers Tool: Calculated charge balance of " << charge_balance << std::endl;
   return charge_balance;
 }


double ClusterClassifiers::CalculateMaxPE(std::vector<Hit> cluster_hits)
{
  double max_PE = 0;
  for (int i = 0; i < (int) cluster_hits.size(); i++){
    Hit ahit = cluster_hits.at(i); 
    double hit_charge = ahit.GetCharge();
    int channel_key = ahit.GetTubeId();
    double hit_PE = 0;
    std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(channel_key);
    if(it != ChannelKeyToSPEMap.end()){ //Charge to SPE conversion is available
      hit_PE  = hit_charge / ChannelKeyToSPEMap.at(channel_key);
      if(hit_PE > max_PE) max_PE = hit_PE;
    }
  }
  if(verbosity>4) std::cout << "ClusterClassifiers Tool: Calculated max PE hit of " << max_PE << std::endl;
  return max_PE;
}

double ClusterClassifiers::CalculateMaxPEMC(std::vector<MCHit> cluster_hits, std::vector<unsigned long> cluster_detkeys)
 {
   double max_PE = 0;
   for (int i = 0; i < (int) cluster_hits.size(); i++){
     MCHit ahit = cluster_hits.at(i); 
     double hit_charge = ahit.GetCharge();
     //int channel_key = ahit.GetTubeId();
     unsigned long detkey = cluster_detkeys.at(i);
     int channel_key = (int) detkey;
     int tubeid = ahit.GetTubeId();
     unsigned long utubeid = (unsigned long) tubeid;
     int wcsimid = channelkey_to_pmtid.at(utubeid);
     unsigned long detkey_data = pmtid_to_channelkey[wcsimid];
     int channel_key_data = (int) detkey_data;
     std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(channel_key_data);
     if(it != ChannelKeyToSPEMap.end()){ //Charge to SPE conversion is available
       if (hit_charge > max_PE) max_PE = hit_charge;
     }
   }
   if(verbosity>4) std::cout << "ClusterClassifiers Tool: Calculated max PE hit of " << max_PE << std::endl;
   return max_PE;
 }
