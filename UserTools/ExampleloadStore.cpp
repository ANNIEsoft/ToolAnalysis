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
  m_data->Stores["DataName"]->Header->Get("TotalEntries",NumEvents);
 
  currententry=0;
 
  return true;
}


bool ExampleloadStore::Execute(){

  logmessage<<"current event="<<currententry<<":"<<NumEvents;  
  m_data->Log->Log(logmessage.str(),1,verbose);
  logmessage.str(""); 
 
  m_data->Stores["DataName"]->GetEntry(currententry);  
  currententry++;
  
  if(currententry==NumEvents) m_data->vars.Set("StopLoop",1);
  
  return true;
}


bool ExampleloadStore::Finalise(){

  return true;
}
