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


  m_variables.Get("verbosity",verbosity);
  m_variables.Get("Mode",Mode);
  m_variables.Get("InputFile",InputFile);

  //Default mode of operation is the continuous flow of data for the live monitoring
  //The other possibility is reading in data from a specified list of files
  if (Mode != "Continuous" && Mode != "SingleFile") {
    if (verbosity > 0) std::cout <<"ERROR (MRDDataDecoder): Specified mode of operation ("<<Mode<<") is not an option [Continuous/SingleFile]. Setting default Continuous mode"<<std::endl;
    Mode = "Continuous";
  }
  
  m_data->CStore.Get("MRDCrateSpaceToChannelNumMap",MRDCrateSpaceToChannelNumMap);
  std::cout << "MRDDataDecoder Tool: Initialized successfully" << std::endl;
  return true;
}


bool MRDDataDecoder::Execute(){
  // Load 
  if(Mode=="SingleFile"){
    if(SingleFileLoaded){
      std::cout << "MRDDataDecoder tool: Single file has already been loaded" << std::endl;
      return true;
    } else {
      Log("MRDDataDecoder Tool: Raw Data file as BoostStore",v_message,verbosity); 
      // Initialize RawData
      MRDData = new BoostStore(false,0);
      MRDData->Initialise(InputFile.c_str());
      MRDData->Print(false);

    }
  }

  else if (Mode == "Continuous"){
    std::string State;
    m_data->CStore.Get("State",State);
    std::cout << "MRDDataDecoder tool: checking CStore for status of data stream" << std::endl;
    if (State == "PMTSingle" || State == "Wait"){
      //Single event file available for monitoring; not relevant for this tool
      if (verbosity > 2) std::cout <<"MRDDataDecoder: State is "<<State<< ". No data file available" << std::endl;
      return true; 
    } else if (State == "DataFile"){
      // Full PMTData file ready to parse
      // FIXME: Not sure if the booststore or key are right, or even the DataFile State
      if (verbosity > 1) std::cout<<"MRDDataDecoder: New data file available."<<std::endl;
      m_data->Stores["CCData"]->Get("FileData",MRDData);
      MRDData->Print(false);
      
    }
  }


  /////////////////// getting MRD Data ////////////////////
  Log("MRDDataDecoder Tool: Accessing MRD Data in raw data",v_message,verbosity); 
  // Show the total entries in this file  
  MRDData->Header->Get("TotalEntries",totalentries);
  if(verbosity>v_message) std::cout<<"Total entries in MRDData store: "<<totalentries<<std::endl;

  NumMRDDataProcessed = 0;
  Log("MRDDataDecoder Tool: Parsing entire MRDData booststore this loop",v_message,verbosity);
  EntryNum = 0;
  while(EntryNum < totalentries){
	Log("MRDDataDecoder Tool: Procesing MRDData Entry "+to_string(CDEntryNum),v_debug, verbosity);
    MRDOut mrddata;
    MRDData->GetEntry(EntryNum);
    MRDData->Get("Data",mrddata);
   
    std::string mrdTriggertype = "No Loopback";
    std::vector<unsigned long> chankeys;
    unsigned long timestamp = mrddata.TimeStamp;    //in ms since 1970/1/1
    std::vector<std::pair<unsigned long, int>> ChankeyTimePairs;
    MRDEvents.emplace(timestamp,ChankeyTimePairs)
    
    //For each entry, loop over all crates and get data
    for (int i_data = 0; i_data < mrddata.Crate.size(); i_data++){
      int crate = mrddata.Crate.at(i_data);
      int slot = mrddata.Slot.at(i_data);
      int channel = mrddata.Channel.at(i_data);
      int hittimevalue = mrddata.Value.at(i_data);
      std::vector<int> CrateSlotChannel{crate,slot,channel};
      unsigned long chankey = MRDCrateSpaceToChannelNumMap[CrateSlotChannel];
      std::pair <unsigned long,int> keytimepair(chankey,hittimevalue);
      MRDEvents[timestamp].push_back(keytimepair);
      if (crate == 7 && slot == 11 && channel == 14) mrdTriggertype = "Cosmic";   //FIXME: don't hard-code the trigger channels?
      if (crate == 7 && slot == 11 && channel == 15) mrdTriggertype = "Beam";     //FIXME: don't hard-code the trigger channels?
    }
    
    //Entry processing done.  Label the trigger type and increment index
    TriggerTypeMap.emplace(timestamp,mrdTriggertype);
    NumMRDDataProcessed += 1;
    EntryNum+=1
  }

  //MRD Data file fully processed.   
  //Push the map of TriggerTypeMap and FinishedMRDHits 
  //to the CStore for ANNIEEvent to start Building ANNIEEvents. 
  Log("MRDDataDecoder Tool: Saving Finished MRD Data into CStore.",v_debug, verbosity);
  //FIXME: add a check for if there is or is not an entry in the CStore
  m_data->CStore.Get("MRDEvents",CStoreMRDEvents);
  CStorePMTWaves.insert(MRDEvents.begin(),MRDEvents.end());
  m_data->CStore.Set("MRDEvents",CStoreMRDEvents);

  m_data->CStore.Get("MRDEventTriggerTypes",CStoreTriggerTypeMap);
  CStoreTriggerTypeMap.insert(TriggerTypeMap.begin(),TriggerTypeMap.end());
  m_data->CStore.Set("MRDEventTriggerTypes",CStoreTriggerTypeMap);
  //FIXME: Should we now clear CStoreMRDEvents and CStoreTriggerTypesto free up memory?
  
  //Check the size of the WaveBank to see if things are bloating
  Log("MRDDataDecoder Tool: Size of MRDEvents (# MRD Triggers processed):" + 
          to_string(MRDEvents.size()),v_debug, verbosity);
  Log("MRDDataDecoder Tool: Size of MRDEvents in CStore:" + 
          to_string(CStoreTriggerTypeMap.size()),v_debug, verbosity);

  std::cout << "MRD EVENT CSTORE ENTRIES SET SUCCESSFULLY.  Clearing MRDEvents map from this file." << std::endl;
  MRDEvents.clear();

  ////////////// END EXECUTE LOOP ///////////////
  return true;
}


bool MRDDataDecoder::Finalise(){
  delete MRDData;
  std::cout << "MRDDataDecoder tool exitting" << std::endl;
  return true;
}

