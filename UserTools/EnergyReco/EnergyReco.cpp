#include "EnergyReco.h"

EnergyReco::EnergyReco():Tool(){}


bool EnergyReco::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool EnergyReco::Execute(){

  return true;
}


bool EnergyReco::Finalise(){

  return true;
}
