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

  return true;
}


bool TriggerDataDecoder::Execute(){
  bool PauseTriggerDecoding = false;
  m_data->CStore.Get("PauseTriggerDecoding",PauseTriggerDecoding);
  if (PauseTriggerDecoding){
    std::cout << "TriggerDataDecoder tool: Pausing trigger decoding to let Tank and MRD data catch up..." << std::endl;
    return true;
  }

  //Clear decoding maps if a new run/subrun is encountered
  this->CheckForRunChange();

  //Get the TriggerData vector pointer from the CStore
  m_data->CStore.Get("TrigData",Tdata);
  Log("TriggerDataDecoder Tool: entry has #CardData classes = "+to_string(Tdata->size()),v_debug, verbosity);
  
  for (unsigned int TDataInd=0; TDataInd<Tdata->size(); TDataInd++){
    TriggerData aTrigData = Tdata->at(TDataInd);
    if(verbosity>v_debug){
      std::cout<<"TriggerDataDecoder Tool: Loading next CardData from entry's index " << TDataInd <<std::endl;
      //FIXME: Soooo, how do we want to parse out the stuff in the TriggerData class?
    }


  m_data->CStore.Set("ParsedTrigData",ParsedTrigData);

  return true;
}


bool TriggerDataDecoder::Finalise(){

  return true;
}

bool TriggerDataDecoder::CheckForRunChange()
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
    //FIXME
  }
  else if (SubRunNumber != CurrentSubrunNum){ //New run has been encountered
    Log("TriggerDataDecoder Tool: New subrun encountered.",v_message,verbosity); 
    //TODO: Even though sequence IDs continue in a subrun, they haven't been
    // EXACTLY in sequence with the end of the prev. run.  Will they be eventually?
    // This prevents freezing up on OUT OF SEQUENCE!! errors
    // FIXME
  }
  return;
}
