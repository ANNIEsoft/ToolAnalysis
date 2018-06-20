#include "ExampleSaveStore.h"

ExampleSaveStore::ExampleSaveStore():Tool(){}


bool ExampleSaveStore::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////

  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer

  /////////////////////////////////////////////////////////////////

  m_variables.Get("OutFile",outfile);
  m_variables.Get("verbose",verbose);

  return true;
}


bool ExampleSaveStore::Execute(){

  m_data->Stores["DataName"]->Save(outfile.c_str());
  m_data->Stores["DataName"]->Delete();
  return true;
}


bool ExampleSaveStore::Finalise(){

  m_data->Stores["DataName"]->Close();
  m_data->Stores["DataName"]->Delete();
  m_data->Stores.clear();

  return true;
}
