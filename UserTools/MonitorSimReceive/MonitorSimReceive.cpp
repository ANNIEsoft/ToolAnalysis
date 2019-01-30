#include "MonitorSimReceive.h"

MonitorSimReceive::MonitorSimReceive():Tool(){}


bool MonitorSimReceive::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("MRDDataPath", MRDDataPath);
  m_variables.Print();
  
  MRDData=new BoostStore(false,2);
  MRDData->Initialise(MRDDataPath);

  srand(time(NULL));

  m_data->Stores["CCData"]=new BoostStore(false,2);  

  return true;
}


bool MonitorSimReceive::Execute(){


  int a=rand() % 10;
  int b=rand() % 100;

  
  if(!a){

    int event=rand() % 1000;

    std::string State="MRDSingle";
    m_data->CStore.Set("State",State);

    MRDOut tmp;
    MRDData->GetEntry(event);
    MRDData->Get("Data", tmp);
    m_data->Stores["CCData"]->Set("Single",tmp);

  }
  else if(!b){

    std::string State="DataFile";
    m_data->CStore.Set("State",State);

    m_data->Stores["CCData"]->Set("FileData",MRDData,false);

  }
  else{

    std::string State="Wait";
    m_data->CStore.Set("State",State);
  
}

  return true;
}


bool MonitorSimReceive::Finalise(){

  MRDData=0;

  m_data->CStore.Remove("State");

  m_data->Stores["CCData"]->Remove("FileData");

  m_data->Stores.clear();

  return true;
}
