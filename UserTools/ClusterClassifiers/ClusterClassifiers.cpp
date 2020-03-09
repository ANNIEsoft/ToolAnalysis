#include "ClusterClassifiers.h"

ClusterClassifiers::ClusterClassifiers():Tool(){}


bool ClusterClassifiers::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////


  verbosity = 0;
  ClusterMaxPEs = new std::map<double,double>;
  ClusterChargePoints = new std::map<double,Direction>;
  ClusterChargeBalances = new std::map<double,double>;

  m_data->CStore.Get("ChannelNumToTankPMTSPEChargeMap",ChannelKeyToSPEMap);

  return true;
}


bool ClusterClassifiers::Execute(){

  //We're gonna make ourselves a couple cluster classifier maps boyeeee

  bool get_clusters = m_data->CStore.Get("ClusterMap",m_all_clusters);
  if(!get_clusters){
    std::cout << "ClusterClassifiers tool: No clusters found!" << std::endl;
    return true;
  }
  if(verbosity>3) std::cout << "ClusterClassifiers Tool: looping through clusters to get cluster info now" << std::endl;
  
  double max_prompt_clustertime = -1;
  double max_prompt_clusterPE = -1;
  std::vector<double> delayed_cluster_times;
  std::vector<double> delayed_ncandidate_times;

  for (std::pair<double,std::vector<Hit>>&& cluster_pair : *m_all_clusters) {
    double cluster_time = cluster_pair.first;
    std::vector<Hit> cluster_hits = cluster_pair.second;
    Direction ChargePoint = this->CalculateChargePoint(cluster_hits);
    ClusterChargePoints->emplace(cluster_time,ChargePoint);
    double ChargeBalance = this->CalculateChargeBalance(cluster_hits);
    ClusterChargeBalances->emplace(cluster_time,ChargeBalance);
    double max_PE = this->CalculateMaxPE(cluster_hits);
    ClusterMaxPEs->emplace(cluster_time,max_PE);
  }

  //Save classifiers to ANNIEEvent
  m_data->Stores.at("ANNIEEvent")->Set("ClusterChargePoints", ClusterChargePoints);
  m_data->Stores.at("ANNIEEvent")->Set("ClusterChargeBalances", ClusterChargeBalances);
  m_data->Stores.at("ANNIEEvent")->Set("ClusterMaxPEs", ClusterMaxPEs);

  return true;
}


bool ClusterClassifiers::Finalise(){
  delete ClusterMaxPEs;
  delete ClusterChargePoints;
  delete ClusterChargeBalances;
  std::cout << "ClusterClassifiers tool exitting" << std::endl;
  return true;
}

Direction ClusterClassifiers::CalculateChargePoint(std::vector<Hit> cluster_hits)
{

  double x_weight = 0;
  double y_weight = 0;
  double z_weight = 0;
  Position detector_center=geom->GetTankCentre();
  double tank_center_x = detector_center.X();
  double tank_center_y = detector_center.Y();
  double tank_center_z = detector_center.Z();

  for (int i = 0; i < cluster_hits.size(); i++){
    Hit ahit = cluster_hits.at(i); 
    double hit_charge = ahit.GetCharge();
    int channel_key = ahit.GetTubeId();
    double hit_PE = 0;
    if(verbosity>4) std::cout << "Finding charge to SPE conversion for channel " << channel_key << std::endl;
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

  Direction charge_weight(x_weight,y_weight,z_weight);
  return charge_weight;
}

double ClusterClassifiers::CalculateChargeBalance(std::vector<Hit> cluster_hits)
{
  double total_Q = 0;
  double total_QSquared = 0;
  for (int i = 0; i < cluster_hits.size(); i++){
    Hit ahit = cluster_hits.at(i); 
    double hit_charge = ahit.GetCharge();
    total_Q+= hit_charge;
    total_QSquared += (hit_charge * hit_charge);
  } 
  double charge_balance  = sqrt((total_QSquared)/(total_Q*total_Q) - (1./126.));
  return charge_balance;
}

double ClusterClassifiers::CalculateMaxPE(std::vector<Hit> cluster_hits)
{
  double max_PE = 0;
  for (int i = 0; i < cluster_hits.size(); i++){
    Hit ahit = cluster_hits.at(i); 
    double hit_charge = ahit.GetCharge();
    int channel_key = ahit.GetTubeId();
    double hit_PE = 0;
    if(verbosity>4) std::cout << "Finding charge to SPE conversion for channel " << channel_key << std::endl;
    std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(channel_key);
    if(it != ChannelKeyToSPEMap.end()){ //Charge to SPE conversion is available
      hit_PE  = hit_charge / ChannelKeyToSPEMap.at(channel_key);
      if(hit_PE > max_PE) max_PE = hit_PE;
    }
  }
  return max_PE;
}
