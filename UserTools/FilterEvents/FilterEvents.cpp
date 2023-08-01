#include "FilterEvents.h"

FilterEvents::FilterEvents():Tool(){}


bool FilterEvents::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("verbosity",verbosity);
  m_variables.Get("FilteredFilesBasename",FilteredFilesBasename);
  m_variables.Get("SavePath",SavePath);

  matched = 0;
    
  FilteredEvents = new BoostStore(false,2);   
  
  std::string EventSelectorCutConfiguration;
  m_data->CStore.Get("CutConfiguration",EventSelectorCutConfiguration);
  FilterName = EventSelectorCutConfiguration;

  FilteredEvents->Header->Set("FilteredEvent",true);
  FilteredEvents->Header->Set("FilterName",FilterName);

  return true;
}


bool FilterEvents::Execute(){

  std::map<std::string,bool> dataStreams;
  int runNumber, eventNumber, localEventNumber, partNumber;
  m_data->Stores["ANNIEEvent"]->Get("DataStreams",dataStreams);
  m_data->Stores["ANNIEEvent"]->Get("RunNumber",runNumber);
  m_data->Stores["ANNIEEvent"]->Get("EventNumber",eventNumber);
  m_data->Stores["ANNIEEvent"]->Get("LocalEventNumber",localEventNumber);
  m_data->Stores["ANNIEEvent"]->Get("PartNumber",partNumber);


  bool pass_filter = false;
  m_data->Stores.at("RecoEvent")->Get("EventCutStatus", pass_filter); 

  if (pass_filter){
    this->SetAndSaveEvent();
    matched++;
  }

  return true;
}


bool FilterEvents::Finalise(){

  FilteredEvents->Close();
  FilteredEvents->Delete();
  delete FilteredEvents;
  std::cout<<"Number of matched run numbers, part numbers, and (local) event numbers: "<<matched<<std::endl;

  return true;
}

void FilterEvents::SetAndSaveEvent(){

  uint32_t RunNumber, SubrunNumber, EventNumber, TriggerWord;
  size_t LocalEventNumber;
  uint64_t RunStartTime, EventTimeTank, EventTimeLAPPD, CTCTimestamp, LAPPDOffset;
  TimeClass EventTimeMRD;
  int PartNumber, RunType, TriggerExtended;
  std::map<std::string, bool> DataStreams;
  std::string MRDTriggerType;
  std::map<std::string, int> MRDLoopbackTDC;
  TriggerClass TriggerData;
  BeamStatus BeamStatus;
  std::map<unsigned long, std::vector<Hit>>* TDCData;
  std::map<unsigned long, std::vector<Hit>> *Hits, *AuxHits;
  std::map<unsigned long, std::vector<int>> RawAcqSize;
  std::map<unsigned long, std::vector<std::vector<ADCPulse>>> RecoADCData, RecoAuxADCData;
  PsecData LAPPDData;

  std::map<unsigned long, std::vector<Hit>> *NewTDCData = new std::map<unsigned long, std::vector<Hit>>;
  std::map<unsigned long, std::vector<Hit>> *NewHits = new  std::map<unsigned long, std::vector<Hit>>;
  std::map<unsigned long, std::vector<Hit>> *NewAuxHits = new std::map<unsigned long, std::vector<Hit>>;

  // Get and Set all variables in the event to the new booststore
  m_data->Stores["ANNIEEvent"]->Get("AuxHits",AuxHits);
    for (auto&& entry : (*AuxHits)){
        NewAuxHits->emplace(entry.first,entry.second);
        }
    FilteredEvents->Set("AuxHits",NewAuxHits,true);

    m_data->Stores["ANNIEEvent"]->Get("BeamStatus",BeamStatus);  FilteredEvents->Set("BeamStatus",BeamStatus);
    m_data->Stores["ANNIEEvent"]->Get("CTCTimestamp",CTCTimestamp);  FilteredEvents->Set("CTCTimestamp",CTCTimestamp);
    m_data->Stores["ANNIEEvent"]->Get("DataStreams",DataStreams);  FilteredEvents->Set("DataStreams",DataStreams);
    m_data->Stores["ANNIEEvent"]->Get("EventNumber",EventNumber);  FilteredEvents->Set("EventNumber",EventNumber);
    m_data->Stores["ANNIEEvent"]->Get("EventTimeLAPPD",EventTimeLAPPD);  FilteredEvents->Set("EventTimeLAPPD",EventTimeLAPPD);
    m_data->Stores["ANNIEEvent"]->Get("EventTimeMRD",EventTimeMRD);  FilteredEvents->Set("EventTimeMRD",EventTimeMRD);
    m_data->Stores["ANNIEEvent"]->Get("EventTimeTank",EventTimeTank);  FilteredEvents->Set("EventTimeTank",EventTimeTank);
    m_data->Stores["ANNIEEvent"]->Get("Hits",Hits);
    for (auto&& entry : (*Hits)){
        NewHits->emplace(entry.first,entry.second);
        }
    FilteredEvents->Set("Hits",NewHits,true);
 
/*   
    m_data->Stores["ANNIEEvent"]->Get("LAPPDData",LAPPDData);  FilteredEvents->Set("LAPPDData",LAPPDData);
    m_data->Stores["ANNIEEvent"]->Get("LAPPDOffset",LAPPDOffset);  FilteredEvents->Set("LAPPDOffset",LAPPDOffset);
*/
    m_data->Stores["ANNIEEvent"]->Get("LocalEventNumber",LocalEventNumber);  FilteredEvents->Set("LocalEventNumber",LocalEventNumber);
    m_data->Stores["ANNIEEvent"]->Get("MRDLoopbackTDC",MRDLoopbackTDC);  FilteredEvents->Set("MRDLoopbackTDC",MRDLoopbackTDC);
    m_data->Stores["ANNIEEvent"]->Get("MRDTriggerType",MRDTriggerType);  FilteredEvents->Set("MRDTriggerType",MRDTriggerType);
    m_data->Stores["ANNIEEvent"]->Get("PartNumber",PartNumber);  FilteredEvents->Set("PartNumber",PartNumber);
    m_data->Stores["ANNIEEvent"]->Get("RawAcqSize",RawAcqSize);  FilteredEvents->Set("RawAcqSize",RawAcqSize);
    m_data->Stores["ANNIEEvent"]->Get("RecoADCData",RecoADCData);  FilteredEvents->Set("RecoADCData",RecoADCData);
    m_data->Stores["ANNIEEvent"]->Get("RecoAuxADCData",RecoAuxADCData);  FilteredEvents->Set("RecoAuxADCData",RecoAuxADCData);
    m_data->Stores["ANNIEEvent"]->Get("RunNumber",RunNumber);  FilteredEvents->Set("RunNumber",RunNumber);
    m_data->Stores["ANNIEEvent"]->Get("RunStartTime",RunStartTime);  FilteredEvents->Set("RunStartTime",RunStartTime);
    m_data->Stores["ANNIEEvent"]->Get("RunType",RunType);    FilteredEvents->Set("RunType",RunType);
    m_data->Stores["ANNIEEvent"]->Get("SubrunNumber",SubrunNumber);  FilteredEvents->Set("SubrunNumber",SubrunNumber);
    m_data->Stores["ANNIEEvent"]->Get("TDCData",TDCData);
    for (auto&& entry : (*TDCData)){
        NewTDCData->emplace(entry.first,entry.second);
        }
    
    FilteredEvents->Set("TDCData",NewTDCData,true);
    m_data->Stores["ANNIEEvent"]->Get("TriggerData",TriggerData);  FilteredEvents->Set("TriggerData",TriggerData);
    m_data->Stores["ANNIEEvent"]->Get("TriggerExtended",TriggerExtended);  FilteredEvents->Set("TriggerExtended",TriggerExtended);
    m_data->Stores["ANNIEEvent"]->Get("TriggerWord",TriggerWord);  FilteredEvents->Set("TriggerWord",TriggerWord);

    std::string Filename = SavePath + "/" + FilteredFilesBasename + "_" + FilterName;

    if (verbosity>0) std::cout<<"Filename is "<<Filename<<std::endl;
    FilteredEvents->Save(Filename);
    FilteredEvents->Delete();

}
