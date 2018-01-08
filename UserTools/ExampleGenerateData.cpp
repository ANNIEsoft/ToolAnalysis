#include "ExampleGenerateData.h"

ExampleGenerateData::ExampleGenerateData():Tool(){}


bool ExampleGenerateData::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  //// loading values from config file
  m_variables.Get("NumEvents",NumEvents);
  m_variables.Get("verbose",verbose);

  ////// Save some data to inter Tool/module persistant store
  std::string configvalue="important info";
  int a=5;
  double b=5.4;
  m_data->CStore.Set("name",configvalue);
  m_data->CStore.Set("a",a);
  m_data->CStore.Set("b",b);

  /////Set up a new store with multiple entries
  m_data->Stores["DataName"]=new BoostStore(false,2);

  // set multi entry header info 
  std::string headervalue="info";
  m_data->Stores["DataName"]->Header->Set("name",headervalue);
  m_data->Stores["DataName"]->Header->Set("a",a);
  m_data->Stores["DataName"]->Header->Set("b",b);

  srand (time(NULL)); // random seed

  currentevent=0;

  return true;
}


bool ExampleGenerateData::Execute(){

  //// Generate random data
  int a=rand() % 10 + 1;
  double b=(rand() % 1000 + 1)/10.0;
  std::string c="stuff";

  m_data->Stores["DataName"]->Set("a",a);
  m_data->Stores["DataName"]->Set("b",b);
  m_data->Stores["DataName"]->Set("c",c);

  currentevent++;
  
  if(currentevent==NumEvents) m_data->vars.Set("StopLoop",1);

  return true;
}


bool ExampleGenerateData::Finalise(){


  return true;
}
