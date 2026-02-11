#include "LAPPDBSCharging.h"

LAPPDBSCharging::LAPPDBSCharging():Tool(){}


bool LAPPDBSCharging::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool LAPPDBSCharging::Execute(){

  return true;
}


bool LAPPDBSCharging::Finalise(){

  return true;
}
