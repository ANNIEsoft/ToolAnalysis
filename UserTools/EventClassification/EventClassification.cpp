#include "EventClassification.h"

EventClassification::EventClassification():Tool(){}


bool EventClassification::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool EventClassification::Execute(){

  return true;
}


bool EventClassification::Finalise(){

  return true;
}
