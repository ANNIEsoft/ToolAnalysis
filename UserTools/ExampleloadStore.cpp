#include "ExampleloadStore.h"

ExampleloadStore::ExampleloadStore():Tool(){}


bool ExampleloadStore::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////

  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer

  /////////////////////////////////////////////////////////////////

  m_variables.Get("verbose",verbose);
  m_variables.Get("InputFile",inputfile);

  m_data->Stores["DataName"]=new BoostStore(false,2);
  m_data->Stores["DataName"]->Initialise(inputfile);
  m_data->Stores["DataName"]->GetHeader();
  m_data->Stores["DataName"]->Get("TotalEnteries",NumEvents);

  currententry=0;
  
  return true;
}


bool ExampleloadStore::Execute(){

  std::cout<<"d1"<<std::endl;
  m_data->Stores["DataName"]->GetEntry(currententry);
  std::cout<<"d2"<<std::endl;
  currententry++;
  std::cout<<"d3"<<std::endl;
  if(currententry==NumEvents) m_data->vars.Set("StopLoop",1);

  return true;
}


bool ExampleloadStore::Finalise(){

  return true;
}
