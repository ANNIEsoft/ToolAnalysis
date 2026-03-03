#include "CCMCCorrection.h"

CCMCCorrection::CCMCCorrection():Tool(){}


bool CCMCCorrection::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool CCMCCorrection::Execute(){

  return true;
}


bool CCMCCorrection::Finalise(){

  return true;
}
