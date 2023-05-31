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
  m_variables.Get("RunNumber",RunNumber);
  //RawData = new BoostStore(false,0);

  //RawData->Initialise("../RawData/RAWDataR4226S0p2");
  //RawData->Print(false);

  Stage1Data = new BoostStore(false,2);

  return true;
}


bool Stage1DataBuilder::Execute(){
  PsecData* LData = nullptr;
  m_data->CStore.Get("LAPPDData",LData);
  std::cout<<"LData: "<<LData<<std::endl;
  LData->Print();
  int lappdid = LData->LAPPD_ID;
  std::cout<<"LAPPD ID: "<<lappdid<<std::endl;

  switch(lappdid){
  case 0:
    LAPPD1Data = new BoostStore(false,0);
    LAPPD1Data->Set("LAPPDData",*LData);
    Stage1Data->Set("LAPPD1Data",LAPPD1Data);
    break;
  case 1:
    LAPPD2Data = new BoostStore(false,0);
    LAPPD2Data->Set("LAPPDData",*LData);
    Stage1Data->Set("LAPPD2Data",LAPPD2Data);
    break;
  case 2:
    LAPPD3Data = new BoostStore(false,0);
    LAPPD3Data->Set("LAPPDData",*LData);
    Stage1Data->Set("LAPPD3Data",LAPPD3Data);
    break;
  case 3:
    LAPPD4Data = new BoostStore(false,0);
    LAPPD4Data->Set("LAPPDData",*LData);
    Stage1Data->Set("LAPPD4Data",LAPPD4Data);
    break;
  case 4:
    LAPPD5Data = new BoostStore(false,0);
    LAPPD5Data->Set("LAPPDData",*LData);
    Stage1Data->Set("LAPPD5Data",LAPPD5Data);
    break;
  }

  this->GetANNIEEvent();
  std::cout<<"ANNIEEvent: "<<std::endl;
  ANNIEEvent1->Print(false);
  std::cout<<"made it here 3"<<std::endl;
  Stage1Data->Set("ANNIEEvent_noLAPPD",ANNIEEvent1);

  std::cout<<"made it here 4"<<std::endl;
  std::string filename = Basename + "R" + std::to_string(RunNumber);
  Stage1Data->Save(filename);
  std::cout<<"made it here 5"<<std::endl;

  return true;
}


bool Stage1DataBuilder::Finalise(){
  Stage1Data->Close();
  Stage1Data->Delete();
  delete Stage1Data;
  return true;
}

void Stage1DataBuilder::GetANNIEEvent(){
  ANNIEEvent1 = new BoostStore(false,0);
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