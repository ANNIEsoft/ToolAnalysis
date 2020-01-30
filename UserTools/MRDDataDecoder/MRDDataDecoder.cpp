#include "MRDDataDecoder.h"

MRDDataDecoder::MRDDataDecoder():Tool(){}


bool MRDDataDecoder::Initialise(std::string configfile, DataModel &data){
  std::cout << "Initializing MRDDataDecoder tool" << std::endl;
  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  verbosity = 0;
  EntriesPerExecute = 0;
  DummyRunNumber = 0;

  m_variables.Get("verbosity",verbosity);
  m_variables.Get("UseDummyRunNumber",DummyRunNumber);

  m_data->CStore.Get("MRDCrateSpaceToChannelNumMap",MRDCrateSpaceToChannelNumMap);
  m_data->CStore.Set("NewMRDDataAvailable",false);

  m_data->CStore.Set("PauseMRDDecoding",false);
  Log("MRDDataDecoder Tool: Initialized successfully",v_message,verbosity);
  return true;
}


bool MRDDataDecoder::Execute(){

  m_data->CStore.Set("NewMRDDataAvailable",false);
  bool PauseMRDDecoding = false;
  m_data->CStore.Get("PauseMRDDecoding",PauseMRDDecoding);
  if (PauseMRDDecoding){
    std::cout << "MRDDataDecoder tool: Pausing MRD decoding to let Tank data catch up..." << std::endl;
    return true;
  }

  
  /////////////////// getting MRD Data ////////////////////
  Log("MRDDataDecoder Tool: Accessing MRDData from CStore",v_message,verbosity); 
  CStore->Get("MRDData",mrddata);
  std::string mrdTriggertype = "No Loopback";
  std::vector<unsigned long> chankeys;
  unsigned long timestamp = mrddata.TimeStamp;    //in ms since 1970/1/1
  std::vector<std::pair<unsigned long, int>> ChankeyTimePairs;
  MRDEvents.emplace(timestamp,ChankeyTimePairs);
    
  //For each entry, loop over all crates and get data
  for (unsigned int i_data = 0; i_data < mrddata.Crate.size(); i_data++){
    int crate = mrddata.Crate.at(i_data);
    int slot = mrddata.Slot.at(i_data);
    int channel = mrddata.Channel.at(i_data);
    int hittimevalue = mrddata.Value.at(i_data);
    std::vector<int> CrateSlotChannel{crate,slot,channel};
    unsigned long chankey = MRDCrateSpaceToChannelNumMap[CrateSlotChannel];
    if (chankey !=0){
      std::pair <unsigned long,int> keytimepair(chankey,hittimevalue);  //chankey will be 0 when looking at loopback channels that don't have an entry in the mapping-->skip
      MRDEvents[timestamp].push_back(keytimepair);
    }
    if (crate == 7 && slot == 11 && channel == 14) mrdTriggertype = "Cosmic";   //FIXME: don't hard-code the trigger channels?
    if (crate == 7 && slot == 11 && channel == 15) mrdTriggertype = "Beam";     //FIXME: don't hard-code the trigger channels?
  }
  
  //Entry processing done.  Label the trigger type and increment index
  TriggerTypeMap.emplace(timestamp,mrdTriggertype);
  NumMRDDataProcessed += 1;
  ExecuteEntryNum += 1;
  EntryNum+=1;

  //MRD Data file fully processed.   
  //Push the map of TriggerTypeMap and FinishedMRDHits 
  //to the CStore for ANNIEEvent to start Building ANNIEEvents. 
  Log("MRDDataDecoder Tool: Saving Finished MRD Data into CStore.",v_debug, verbosity);
  //FIXME: add a check for if there is or is not an entry in the CStore
  m_data->CStore.Get("MRDEvents",CStoreMRDEvents);
  CStoreMRDEvents.insert(MRDEvents.begin(),MRDEvents.end());
  m_data->CStore.Set("MRDEvents",CStoreMRDEvents);

  m_data->CStore.Get("MRDEventTriggerTypes",CStoreTriggerTypeMap);
  CStoreTriggerTypeMap.insert(TriggerTypeMap.begin(),TriggerTypeMap.end());
  m_data->CStore.Set("MRDEventTriggerTypes",CStoreTriggerTypeMap);
  
  m_data->CStore.Set("NewMRDDataAvailable",true);
  m_data->CStore.Set("MRDRunInfoPostgress",Postgress);

  //Check the size of the WaveBank to see if things are bloating
  Log("MRDDataDecoder Tool: Size of MRDEvents in CStore (# MRD Triggers processed):" + 
          to_string(CStoreMRDEvents.size()),v_debug, verbosity);
  Log("MRDDataDecoder Tool: Size of TriggerTypeMap in CStore:" + 
          to_string(CStoreTriggerTypeMap.size()),v_debug, verbosity);

  std::cout << "MRD EVENT CSTORE ENTRIES SET SUCCESSFULLY.  Clearing MRDEvents map from this file." << std::endl;
  MRDEvents.clear();
  TriggerTypeMap.clear();

  ////////////// END EXECUTE LOOP ///////////////
  return true;
}


bool MRDDataDecoder::Finalise(){
  std::cout << "MRDDataDecoder tool exitting" << std::endl;
  return true;
}

