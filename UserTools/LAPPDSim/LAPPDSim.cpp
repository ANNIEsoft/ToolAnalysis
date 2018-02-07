#include "LAPPDSim.h"

LAPPDSim::LAPPDSim():Tool(){}


bool LAPPDSim::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool LAPPDSim::Execute(){

  std::cout<<"Hi This is LAPPDSim"<<std::endl;

  return true;
}


bool LAPPDSim::Finalise(){

  return true;
}
