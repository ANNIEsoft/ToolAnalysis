#include "TriggerDataDecoder.h"

TriggerDataDecoder::TriggerDataDecoder():Tool(){}


bool TriggerDataDecoder::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  verbosity = 0;

  m_variables.Get("verbosity",verbosity);

  CurrentRunNum = -1;
  CurrentSubrunNum = -1;

  TimeToTriggerWordMap = new std::map<uint64_t,uint32_t>;
  m_data->CStore.Set("PauseCTCDecoding",false);
  return true;
}


bool TriggerDataDecoder::Execute(){
  m_data->CStore.Set("NewCTCDataAvailable",false);
  bool PauseCTCDecoding = false;
  m_data->CStore.Get("PauseCTCDecoding",PauseCTCDecoding);
  if (PauseCTCDecoding){
    std::cout << "TriggerDataDecoder tool: Pausing trigger decoding to let Tank and MRD data catch up..." << std::endl;
    return true;
  }
  //Clear decoding maps if a new run/subrun is encountered
  this->CheckForRunChange();

  //Get the TriggerData vector pointer from the CStore
  Log("TriggerDataDecoder Tool: Accessing TrigData vector in CStore",v_debug, verbosity);
  bool got_tdata = m_data->CStore.Get("TrigData",Tdata);
  if(!got_tdata){
    if(verbosity>0) std:;cout << "TriggerDataDecoder error: No TriggerData in CStore!" << std::endl;
    return false;
  }
  bool new_ts_available = false;
  std::vector<uint32_t> aTimeStampData = Tdata->TimeStampData;
  for(int i = 0; i < aTimeStampData.size(); i++){
    if(verbosity>v_debug) std::cout<<"TriggerDataDecoder Tool: Loading next TrigData from entry's index " << i <<std::endl;
    new_ts_available = this->AddWord(aTimeStampData.at(i));
    if(new_ts_available){
      if(verbosity>4){
        std::cout << "PARSED TRIGGER TIME: " << processed_ns.back() << std::endl;
        std::cout << "PARSED TRIGGER WORD: " << processed_sources.back() << std::endl;
      }
      m_data->CStore.Set("NewCTCDataAvailable",true);
      TimeToTriggerWordMap->emplace(processed_ns.back(),processed_sources.back());
    }
  }
  /*Log("TriggerDataDecoder Tool: entry has #TriggerData classes = "+to_string(Tdata->size()),v_debug, verbosity);
  for (unsigned int TDataInd=0; TDataInd<Tdata->size(); TDataInd++){
    TriggerDataP aTrigData = Tdata->at(TDataInd);
    std::vector<uint32_t> aTimeStampData = aTrigData.TimeStampData;
    bool new_ts_available = false;  
    for(int i = 0; i < aTimeStampData.size(); i++){
      if(verbosity>v_debug) std::cout<<"TriggerDataDecoder Tool: Loading next TrigData from entry's index " << i <<std::endl;
      new_ts_available = this->AddWord(aTimeStampData.at(i));
      if(new_ts_available){
        if(verbosity>4){
          std::cout << "PARSED TRIGGER TIME: " << processed_ns.back() << std::endl;
          std::cout << "PARSED TRIGGER WORD: " << processed_sources.back() << std::endl;
          m_data->CStore.Set("NewMRDDataAvailable",true);
        }
        TimeToTriggerWordMap->emplace(processed_ns.back(),processed_sources.back());
      }
    }
  }*/
  if(verbosity>3) Log("TriggerDataDecoder Tool: size of TimeToTriggerWordMap: "+to_string(TimeToTriggerWordMap->size()),v_message,verbosity); 
 
  m_data->CStore.Set("TimeToTriggerWordMap",TimeToTriggerWordMap);

  return true;
}


bool TriggerDataDecoder::Finalise(){
  delete TimeToTriggerWordMap;
  std::cout << "TriggerDataDecoder tool exitting" << std::endl;
  return true;
}

bool TriggerDataDecoder::AddWord(uint32_t word){
  bool new_timestamp_available = false;
  uint64_t payload = 0;
  uint32_t wordid = 0;
  if(word == 0xF1F0E5E7){
    have_c1 = false;
    c1 = 0;
    have_c2 = false;
    c2 = 0;
    fiforesets.push_back(processed_sources.size()); 
    return new_timestamp_available;
  }

  wordid = word>>24;
  payload = word & 0x00FFFFFF;

  if(wordid == 0xC0){
    return new_timestamp_available;
  }
  else if (wordid == 0xC1 || wordid == 0xC3){
    c1 = (payload&0x0000FFFF)<<24;
    have_c1 = true;
  } else if (wordid == 0xC2 || wordid == 0xC4){
    c2 = payload << 40;
    have_c2 = true;
  } else {
    if(have_c1 && have_c2){
      uint64_t ns = (c1 + c2 + payload)*8;
      processed_sources.push_back(wordid);
      processed_ns.push_back(ns);
      new_timestamp_available = true;
    }
  }
  return new_timestamp_available;
}

void TriggerDataDecoder::CheckForRunChange()
{
  // Load RawData BoostStore to use in execute loop
  Store RunInfoPostgress;
  m_data->CStore.Get("RunInfoPostgress",RunInfoPostgress);
  int RunNumber;
  int SubRunNumber;
  uint64_t StarTime;
  int RunType;
  RunInfoPostgress.Get("RunNumber",RunNumber);
  RunInfoPostgress.Get("SubRunNumber",SubRunNumber);
  RunInfoPostgress.Get("RunType",RunType);
  RunInfoPostgress.Get("StarTime",StarTime);

  //If we have moved onto a new run number, we should clear the event building maps 
  if (CurrentRunNum == -1){
    CurrentRunNum = RunNumber;
    CurrentSubrunNum = SubRunNumber;
  }
  else if (RunNumber != CurrentRunNum){ //New run has been encountered
    Log("TriggerDataDecoder Tool: New run encountered.  Clearing event building maps",v_message,verbosity); 
    TimeToTriggerWordMap->clear();
  }
  else if (SubRunNumber != CurrentSubrunNum){ //New run has been encountered
    Log("TriggerDataDecoder Tool: New subrun encountered.",v_message,verbosity); 
    TimeToTriggerWordMap->clear();
  }
  return;
}
