#include "DummyTool.h"

DummyTool::DummyTool():Tool(){}


bool DummyTool::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  m_data->Log->Log("test people",1);

  return true;
}


bool DummyTool::Execute(){

  return true;
}


bool DummyTool::Finalise(){

  return true;
}
