#include "Stage1DataBuilder.h"

Stage1DataBuilder::Stage1DataBuilder():Tool(){}


bool Stage1DataBuilder::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("Verbosity",verbosity);
  m_variables.Get("Basename",Basename);


  Stage1Data = new BoostStore(false,2);

  return true;
}


bool Stage1DataBuilder::Execute(){
  Stage1Data->Delete();
  int RunNumber;
  m_data->Stores.at("ANNIEEvent")->Get("RunNumber",RunNumber);
  PsecData* LData = nullptr;
  m_data->CStore.Get("LAPPDData",LData);
  if (LData == nullptr){
    Log("No LAPPD Data retrieved from CStore! Moving on...", v_debug, verbosity);
    return true;
  }
  if (verbosity > v_debug){
    std::cout<<"LData: "<<LData<<std::endl;
    LData->Print();
  }
  int lappdid = LData->LAPPD_ID;
  Log("LAPPD ID: " + std::to_string(lappdid), v_debug, verbosity);

  std::string key = "LAPPD" + std::to_string(lappdid) + "Data";
  LAPPD1Data = new BoostStore(false,0);
  LAPPD1Data->Set("LAPPDData",*LData);
  Stage1Data->Set(key, LAPPD1Data);

  GetANNIEEvent();
  Stage1Data->Set("ANNIEEvent_noLAPPD",ANNIEEvent1);

  std::string filename = Basename + "R" + std::to_string(RunNumber);
  Stage1Data->Save(filename);
  

  return true;
}


bool Stage1DataBuilder::Finalise(){
  Stage1Data->Set("LAPPD0Data",LAPPD1Data);
  Stage1Data->Set("ANNIEEvent_noLAPPD",ANNIEEvent1);
  Stage1Data->Close();
  delete Stage1Data;
  return true;
}

void Stage1DataBuilder::GetANNIEEvent(){
  ANNIEEvent1->Delete();
  std::map<unsigned long,std::vector<Hit>> *AuxHits = nullptr;
  std::map<unsigned long,std::vector<Hit>> *Hits = nullptr;
  std::map<unsigned long,std::vector<Hit>> *TDCData = nullptr;
  BeamStatus BeamStatus;
  uint64_t CTCTimestamp, RunStartTime;
  std::map<std::string,bool> DataStreams;
  uint32_t EventNumber, TriggerWord;
  TimeClass EventTimeMRD, EventTimeTank;
  std::map<std::string, int> MRDLoopbackTDC;
  std::string MRDTriggerType;
  std::map<unsigned long, std::vector<int>> RawAcqSize;
  std::map<unsigned long,std::vector<std::vector<ADCPulse>>> RecoADCData, RecoAuxADCData;
  int PartNumber, RunNumber, RunType, SubrunNumber, TriggerExtended;
  TriggerClass TriggerData;


  m_data->Stores["ANNIEEvent"]->Get("DataStreams",DataStreams); ANNIEEvent1->Set("DataStreams",DataStreams);
  m_data->Stores["ANNIEEvent"]->Get("AuxHits",AuxHits);
  if (AuxHits != nullptr) ANNIEEvent1->Set("AuxHits",AuxHits,true);
  m_data->Stores["ANNIEEvent"]->Get("Hits",Hits);
  if (Hits != nullptr) ANNIEEvent1->Set("Hits",Hits, true);
  m_data->Stores["ANNIEEvent"]->Get("TDCData",TDCData);
  if (TDCData != nullptr) ANNIEEvent1->Set("TDCData",TDCData,true);

  m_data->Stores["ANNIEEvent"]->Get("BeamStatus",BeamStatus); ANNIEEvent1->Set("BeamStatus",BeamStatus);
  m_data->Stores["ANNIEEvent"]->Get("RunStartTime",RunStartTime); ANNIEEvent1->Set("RunStartTime",RunStartTime);
  m_data->Stores["ANNIEEvent"]->Get("EventNumber",EventNumber); ANNIEEvent1->Set("EventNumber",EventNumber);
  m_data->Stores["ANNIEEvent"]->Get("TriggerWord",TriggerWord); ANNIEEvent1->Set("TriggerWord",TriggerWord);
  m_data->Stores["ANNIEEvent"]->Get("EventTimeMRD",EventTimeMRD); ANNIEEvent1->Set("EventTimeMRD",EventTimeMRD);
  m_data->Stores["ANNIEEvent"]->Get("EventTimeTank",EventTimeTank); ANNIEEvent1->Set("EventTimeTank",EventTimeTank);
  m_data->Stores["ANNIEEvent"]->Get("MRDLoopbackTDC",MRDLoopbackTDC); ANNIEEvent1->Set("MRDLoopbackTDC",MRDLoopbackTDC);
  m_data->Stores["ANNIEEvent"]->Get("MRDTriggerType",MRDTriggerType); ANNIEEvent1->Set("MRDTriggerType",MRDTriggerType);
  m_data->Stores["ANNIEEvent"]->Get("RawAcqSize",RawAcqSize); ANNIEEvent1->Set("RawAcqSize",RawAcqSize);
  m_data->Stores["ANNIEEvent"]->Get("RecoADCData",RecoADCData); ANNIEEvent1->Set("RecoADCData",RecoADCData);
  m_data->Stores["ANNIEEvent"]->Get("RecoAuxADCData",RecoAuxADCData); ANNIEEvent1->Set("RecoAuxADCData",RecoAuxADCData);
  m_data->Stores["ANNIEEvent"]->Get("PartNumber",PartNumber); ANNIEEvent1->Set("PartNumber",PartNumber);
  m_data->Stores["ANNIEEvent"]->Get("RunNumber",RunNumber); ANNIEEvent1->Set("RunNumber",RunNumber);
  m_data->Stores["ANNIEEvent"]->Get("RunType",RunType); ANNIEEvent1->Set("RunType",RunType);
  m_data->Stores["ANNIEEvent"]->Get("SubrunNumber",SubrunNumber); ANNIEEvent1->Set("SubrunNumber",SubrunNumber);
  m_data->Stores["ANNIEEvent"]->Get("TriggerExtended",TriggerExtended); ANNIEEvent1->Set("TriggerExtended",TriggerExtended);
  m_data->Stores["ANNIEEvent"]->Get("TriggerData",TriggerData); ANNIEEvent1->Set("TriggerData",TriggerData);

}