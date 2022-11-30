#include "LAPPDPEPulseFit.h"

LAPPDPEPulseFit::LAPPDPEPulseFit():Tool(){}


bool LAPPDPEPulseFit::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool LAPPDPEPulseFit::Execute(){

  return true;
}


bool LAPPDPEPulseFit::Finalise(){

  return true;
}
