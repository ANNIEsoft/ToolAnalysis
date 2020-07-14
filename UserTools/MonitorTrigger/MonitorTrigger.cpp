#include "MonitorTrigger.h"

MonitorTrigger::MonitorTrigger():Tool(){}


bool MonitorTrigger::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool MonitorTrigger::Execute(){

  return true;
}


bool MonitorTrigger::Finalise(){

  return true;
}
