#include "DummyTool.h"

DummyTool::DummyTool():Tool(){}


bool DummyTool::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;
 
  m_variables.Get("verbose",m_verbose);
 
  Log("test 1",1,m_verbose);

  return true;
}


bool DummyTool::Execute(){
  
  Log("test 2",2,m_verbose);

  return true;
}


bool DummyTool::Finalise(){

  return true;
}
