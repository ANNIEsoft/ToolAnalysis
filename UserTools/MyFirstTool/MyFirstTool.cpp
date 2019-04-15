#include "MyFirstTool.h"

MyFirstTool::MyFirstTool():Tool(){}


bool MyFirstTool::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////


  return true;
}


bool MyFirstTool::Execute(){

  return true;
}


bool MyFirstTool::Finalise(){

  return true;
}
