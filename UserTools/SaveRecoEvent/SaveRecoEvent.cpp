#include "SaveRecoEvent.h"

SaveRecoEvent::SaveRecoEvent():Tool(){}


bool SaveRecoEvent::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("path", path);
  return true;
}


bool SaveRecoEvent::Execute(){

  m_data->Stores["RecoEvent"]->Save(path);

  return true;
}


bool SaveRecoEvent::Finalise(){

  //m_data->Stores["RecoEvent"]->Delete();
  m_data->Stores["RecoEvent"]->Close();

  return true;
}
