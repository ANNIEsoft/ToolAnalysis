#include "MonitorSimReceive.h"

MonitorSimReceive::MonitorSimReceive():Tool(){}


bool MonitorSimReceive::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("MRDDataPath", MRDDataPath);
  //m_variables.Print();
  

  //std::cout<<"d1"<<std::endl;
  BoostStore* indata=new BoostStore(false,0); //tis leaks but its jsut for testing
  //std::cout<<"d2"<<std::endl;  
  indata->Initialise(MRDDataPath);

  //indata->Print(false);

 //std::cout<<"d3"<<std::endl;
 MRDData= new BoostStore(false,2);
 MRDData2= new BoostStore(false,2);
  //std::cout<<"d4"<<std::endl;  
  indata->Get("CCData",*MRDData);
  indata->Get("CCData",*MRDData2);
 //std::cout<<"d5"<<std::endl;
  //MRDData=new BoostStore(false,2);
  //MRDData->Initialise(MRDDataPath);

  srand(time(NULL));
  //std::cout<<"d6"<<std::endl;
  m_data->Stores["CCData"]=new BoostStore(false,2);  
  //  m_data->Stores["CCData"]->Save("tmp");
  //std::cout<<"d7"<<std::endl;
  return true;
}


bool MonitorSimReceive::Execute(){


  int a=rand() % 10;
  int b=rand() % 100;

  
  if(!a){
    //std::cout<<"f1"<<std::endl;
    int event=rand() % 1000;
    //std::cout<<"f2"<<std::endl;
    std::string State="MRDSingle";
    m_data->CStore.Set("State",State);
    //std::cout<<"f3"<<std::endl;
    MRDOut tmp;
    //MRDData->Print(false);
    long entries;
    MRDData->Header->Get("TotalEntries",entries);
    //std::cout<<"f4 "<< entries<<" "<<event<<std::endl;
    MRDData->GetEntry(event);
    //std::cout<<"f5"<<std::endl;
    MRDData->Get("Data", tmp);
    //std::cout<<"f6"<<std::endl;
    m_data->Stores["CCData"]->Set("Single",tmp);
    //std::cout<<"f7"<<std::endl;
  }
  else if(!b){
    //std::cout<<"f8"<<std::endl;
    std::string State="DataFile";
    //std::cout<<"f9"<<std::endl;
    m_data->CStore.Set("State",State);
    //std::cout<<"f10"<<std::endl;
    //m_data->Stores["CCData"]->Set("FileData",MRDData,false);        //false option creates problems in the monitoring tools--> check later
    MRDData2->Save("tmp");
    m_data->Stores["CCData"]->Set("FileData",MRDData2);
    //std::cout<<"f11"<<std::endl;

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
