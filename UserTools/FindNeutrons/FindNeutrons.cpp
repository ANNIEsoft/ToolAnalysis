#include "FindNeutrons.h"

FindNeutrons::FindNeutrons():Tool(){}


bool FindNeutrons::Initialise(std::string configfile, DataModel &data){

  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  m_data= &data; //assigning transient data pointer

  //Set defaults
  Method = "CB";

  //Get configuration variables
  bool get_ok = m_variables.Get("Method",Method);
  if (!get_ok){
    Log("FindNeutrons tool: Get variable >>> Method <<< failed. Check configuration file.",v_error,verbosity);
    Log("FindNeutrons tool: Stopping toolchain",v_error,verbosity);
    m_variables.Set("StopLoop",1);
  } else {
    if (Method == "CB"){
      Log("FindNeutrons tool: Loaded neutron finding method CB successfully",v_message,verbosity);
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

  return true;
}


bool FindNeutrons::Finalise(){

  return true;
}


bool FindNeutrons::FindNeutronCandidates(std::string method){

  //Currently only Charge Balance cut available
  //TODO: add Machine Learning classifiers in the future

  if (method == "CB"){
    this->FindNeutronsByCB();
  }
  else {
    Log("FindNeutrons: Method "+method+" is unknown! Please choose one of the known options (CB / ...)",v_warning,verbosity);
    return false;
  }

  return true;

}
  
bool FindNeutrons::FindNeutronsByCB(){

  bool return_val=false;

  return return_val;

}
  
bool FindNeutrons::FillRecoParticle(){

  bool return_val=false;

int neutron_pdg = 2112;
//tracktype neutron_tracktype = ;
double neutron_E_start = -9999;
double neutron_E_stop = -9999;
Position neutron_vtx_start(-999,-999,-999);
Position neutron_vtx_stop(-999,-999,-999);
Direction neutron_start_dir(0,0,1);
TimeClass neutron_start_time;
TimeClass neutron_stop_time;
double neutron_tracklength = -9999;

  return return_val;

}
