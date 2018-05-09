#include "SaveANNIEEvent.h"

SaveANNIEEvent::SaveANNIEEvent():Tool(){}


bool SaveANNIEEvent::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("path", path);
  return true;
}


bool SaveANNIEEvent::Execute(){

  m_data->Stores["ANNIEEvent"]->Save(path);
  m_data->Stores["ANNIEEvent"]->Delete();

  return true;
}


bool SaveANNIEEvent::Finalise(){


  m_data->Stores["ANNIEEvent"]->Close();

  return true;
}
