#include "DataDecoder.h"

DataDecoder::DataDecoder():Tool(){}


bool DataDecoder::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  verbosity = 0;

  m_variables.Get("verbosity",verbosity);
  m_variables.Get("InputFile",InputFile);

  // Initialize BoostStores
  BoostStore VMEData(false,0);
  //PMTData = new BoostStore(false,2);
  //TriggerData = new BoostStore(false,2);

  return true;
}


bool DataDecoder::Execute(){

  BoostStore in(false,0);

  in.Initialise(argv[1]);
  in.Print(false);

  /////////////////// getting PMT Data ////////////////////
  BoostStore PMTData(false,2);
  in.Get("PMTData",PMTData);

  PMTData.Print(false);

  long entries=0;
  PMTData.Header->Get("TotalEntries",entries);
  std::cout<<"entries = "<<entries<<std::endl;

  for( int i=0;i<5;i++){
    std::cout<<"entry "<<i<<" of "<<entries<<std::endl;
    PMTData.GetEntry(i);
    std::vector<CardData> Cdata;
    PMTData.Get("CardData",Cdata);
    std::cout<<"Cdata size="<<Cdata.size()<<std::endl;
    std::cout<<"Cdata entry 0 CardID="<<Cdata.at(0).CardID<<std::endl;
    std::cout<<"Cdata entry 0 Data vector of size="<<Cdata.at(0).Data.size()<<std::endl;
    for (int j=0;j<Cdata.at(0).Data.size();j++){
      std::cout<<Cdata.at(0).Data.at(j)<<" , ";
    }
    std::cout<<std::endl;

  }
  ///////////////////////////////////////////

  ////////////////////////getting trigger data ////////////////
 BoostStore TrigData(false,2);
  in.Get("TrigData",TrigData);

  TrigData.Print(false);
  long trigentries=0;
  TrigData.Header->Get("TotalEntries",trigentries);
  std::cout<<"entries = "<<entries<<std::endl;

  for( int i=0;i<5;i++){
    std::cout<<"entry "<<i<<" of "<<trigentries<<std::endl;
    TrigData.GetEntry(i);
    TriggerData Tdata;
    PMTData.Get("TrigData",Tdata);
    int EventSize=Tdata.EventSize;
    std::cout<<"EventSize="<<EventSize<<std::endl;
    std::cout<<"SequenceID="<<Tdata.SequenceID<<std::endl;
    std::cout<<"EventTimes: " << std:;endl;
    for(int j=0;j<Tdata.EventTimes.size();j++){

     std::cout<< Tdata.EventTimes.at(j)<<" , ";
    }
    std::cout<<"EventIDs: " << std:;endl;
    for(int j=0;j<Tdata.EventIDs.size();j++){

     std::cout<< Tdata.EventIDs.at(j)<<" , ";
    }

    std::cout<<std::endl;
 }
  return true;
}


bool DataDecoder::Finalise(){
  std::cout << "DataDecoder tool exitting" << std::endl;
  return true;
}
