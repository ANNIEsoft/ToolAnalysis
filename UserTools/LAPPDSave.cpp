#include "LAPPDSave.h"

LAPPDSave::LAPPDSave():Tool(){}


bool LAPPDSave::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool LAPPDSave::Execute(){

  m_data->Stores["ANNIEEvent"]->Save("./testoutpt");
  m_data->Stores["ANNIEEvent"]->Delete();

  return true;
}


bool LAPPDSave::Finalise(){


  m_data->Stores["ANNIEEvent"]->Close();

  return true;
}
