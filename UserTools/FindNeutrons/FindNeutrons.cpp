#include "FindNeutrons.h"

FindNeutrons::FindNeutrons():Tool(){}


bool FindNeutrons::Initialise(std::string configfile, DataModel &data){

  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  m_data= &data; //assigning transient data pointer

  //Set defaults
  Method = "CB";
  verbosity = 2;

  //Get configuration variables
  bool get_ok = m_variables.Get("verbosity",verbosity);
  get_ok = m_variables.Get("Method",Method);
  if (!get_ok){
    Log("FindNeutrons tool: Get variable >>> Method <<< failed. Check configuration file.",v_error,verbosity);
    Log("FindNeutrons tool: Stopping toolchain",v_error,verbosity);
    m_variables.Set("StopLoop",1);
  } else {
    if (Method == "CB" || Method == "CBStrict"){
      Log("FindNeutrons tool: Loaded neutron finding method >>> "+ Method + "<<< successfully",v_message,verbosity);
    } else {
      Log("FindNeutrons tool: Unknown neutron finding method >>> "+ Method + "<<< specified. Please check available options and restart toolchain.",v_error,verbosity);
      Log("FindNeutrons tool: Available options: CB",v_error,verbosity);
      m_variables.Set("StopLoop",1);
    }
  }

  return true;
}


bool FindNeutrons::Execute(){

  //Find neutron candidates for current event
  this->FindNeutronCandidates(Method);

  //Construct Particle objects for neutrons
  this->FillRecoParticles();

  //Append the reco particles to the main Particle object
  std::vector<Particle> Particles_Temp;
  bool get_ok = m_data->Stores["ANNIEEvent"]->Get("Particles",Particles_Temp);
  if (get_ok){
    Log("FindNeutrons tool: Particles object already exists in ANNIEEvent store. Append neutrons...",v_message,verbosity);
    for (int i_neutron=0; i_neutron < vec_neutrons.size(); i_neutron++){
      Particles_Temp.push_back(vec_neutrons.at(i_neutron));
    }
  } else {
    Log("FindNeutrons tool: Particles object did not exist in ANNIEEvent store yet. Set Particles object equal to vector of neutrons",vv_debug,verbosity);
    Particles_Temp = vec_neutrons;
  }
  m_data->Stores["ANNIEEvent"]->Set("Particles",Particles_Temp);

  //Store neutron candidate information
  m_data->Stores["ANNIEEvent"]->Set("ClusterIndicesNeutron",cluster_neutron);
  m_data->Stores["ANNIEEvent"]->Set("ClusterTimesNeutron",cluster_times_neutron);
  m_data->Stores["ANNIEEvent"]->Set("ClusterChargesNeutron",cluster_charges_neutron);
  m_data->Stores["ANNIEEvent"]->Set("ClusterCBNeutron",cluster_cb_neutron);
  m_data->Stores["ANNIEEvent"]->Set("ClusterTimes",cluster_times);
  m_data->Stores["ANNIEEvent"]->Set("ClusterCharges",cluster_charges);
  m_data->Stores["ANNIEEvent"]->Set("ClusterCB",cluster_cb);

  return true;
}


bool FindNeutrons::Finalise(){

  return true;
}


bool FindNeutrons::FindNeutronCandidates(std::string method){

  //Currently only Charge Balance cut available
  //TODO: add Machine Learning classifiers in the future

  //Clear vectors first
  if (cluster_neutron.size()>0) cluster_neutron.clear();
  if (cluster_times_neutron.size()>0) cluster_times_neutron.clear();
  if (cluster_charges_neutron.size()>0) cluster_charges_neutron.clear();
  if (cluster_cb_neutron.size()>0) cluster_cb_neutron.clear();
  if (cluster_times.size()>0) cluster_times.clear();
  if (cluster_charges.size()>0) cluster_charges.clear();
  if (cluster_cb.size()>0) cluster_cb.clear();

  if (method == "CB"){
    this->FindNeutronsByCB(false);
  }
  else if (method == "CBStrict"){
    this->FindNeutronsByCB(true);
  }
  else if (method == "ML"){
    Log("FindNeutrons: ML method not implemented yet in ToolAnalysis",v_message,verbosity);
    return false;
  }
  else {
    Log("FindNeutrons: Method "+method+" is unknown! Please choose one of the known options (CB / CBStrict / ...)",v_warning,verbosity);
    return false;
  }

  return true;

}
  
bool FindNeutrons::FindNeutronsByCB(bool strict){

  bool return_val=false;

  //Get cluster objects (filled in ClusterClassifiers tool)
  std::map<double,double> ClusterChargeBalances;
  std::map<double,double> ClusterTotalPEs;
  bool get_ok;
  get_ok = m_data->Stores.at("ANNIEEvent")->Get("ClusterChargeBalances", ClusterChargeBalances);
  if (!get_ok){
    Log("FindNeutrons tool: Did not find ClusterChargeBalances object! Please add ClusterClassifiers tool to your toolchain!",v_error,verbosity);
    m_variables.Set("StopLoop",1);
  }
  get_ok = m_data->Stores.at("ANNIEEvent")->Get("ClusterTotalPEs", ClusterTotalPEs);
  if (!get_ok){
    Log("FindNeutrons tool: Did not find ClusterTotalPEs object! Please add ClusterClassifiers tool to your toolchain!",v_error,verbosity);
    m_variables.Set("StopLoop",1);
  }

  //Loop through clusters and find neutrons
  int tmp_cluster_id = 0;
  
  for (std::map<double,double>::iterator it = ClusterTotalPEs.begin(); it!= ClusterTotalPEs.end(); it++){
    //First fill general cluster information into vectors
    cluster_times.push_back(it->first);
    cluster_charges.push_back(ClusterTotalPEs.at(it->first));
    cluster_cb.push_back(ClusterChargeBalances.at(it->first));

    //Check if the cluster is in the delayed window and has a time > 10 us (exclude afterpulses)
    //The window should should be extended in the future after relevant exlusion cuts for afterpulsing have been implemented
    //check if the charge balance cut for neutrons is passed -> consider a neutron candidate
    //Improve the neutron selection cuts in the future, probably cutting more signal than necessary at the moment
    if (it->first > 10000){
      double current_cb = ClusterChargeBalances.at(it->first);
      double current_q = ClusterTotalPEs.at(it->first);
      bool pass_cut = false;
      if (current_cb < 0.4 && current_q < 150){
        if (!strict) pass_cut = true;
        else if (current_cb <= (1. - current_q/150.)*0.5) pass_cut = true;
      }
      if (pass_cut){
        Log("FindNeutrons tool: Found neutron candidate at cluster # "+std::to_string(tmp_cluster_id)+", time: "+std::to_string(it->first)+" ns!!!",v_message,verbosity);
        cluster_neutron.push_back(tmp_cluster_id);
        cluster_times_neutron.push_back(it->first);
        cluster_charges_neutron.push_back(current_q);
        cluster_cb_neutron.push_back(current_cb);
        return_val = true;
      }
    }
    tmp_cluster_id ++;
  }

  return return_val;

}
  
bool FindNeutrons::FillRecoParticles(){

  bool return_val=false;

  vec_neutrons.clear();

  int neutron_pdg = 2112;
  tracktype neutron_tracktype = tracktype::UNDEFINED;
  double neutron_E_start = -9999;
  double neutron_E_stop = -9999;
  Position neutron_vtx_start(-999,-999,-999);
  Position neutron_vtx_stop(-999,-999,-999);
  Direction neutron_start_dir(0,0,1);
  double neutron_start_time;
  double neutron_stop_time;
  double neutron_tracklength = -9999;

  for (int i_neutron=0; i_neutron < cluster_times_neutron.size(); i_neutron++){
    double stop_time = cluster_times_neutron.at(i_neutron);
    neutron_stop_time = stop_time;
    neutron_start_time = 0;

    Particle neutron(neutron_pdg,neutron_E_start,neutron_E_stop,neutron_vtx_start,neutron_vtx_stop,neutron_start_time,neutron_stop_time,neutron_start_dir,neutron_tracklength,neutron_tracktype);
    vec_neutrons.push_back(neutron);

    if (verbosity > 2){
      Log("FindNeutrons: Added neutron with the following properties as a particle:",v_message,verbosity);
      neutron.Print();
    }

    return_val = true;
  }

  return return_val;

}
