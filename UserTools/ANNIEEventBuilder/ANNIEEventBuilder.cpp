#include "ANNIEEventBuilder.h"

ANNIEEventBuilder::ANNIEEventBuilder():Tool(){}


bool ANNIEEventBuilder::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  
  SavePath = "./";
  ProcessedFilesBasename = "ProcessedRawData";
  BuildType = "TankAndMRD";
  EventsPerPairing = 200;
  NumWavesInCompleteSet = 140;
  OrphanOldTankTimestamps = true;
  OldTimestampThreshold = 120; //seconds
  OrphanWarningValue = 20;
  ExecutesPerBuild = 50;
  MRDTankTimeTolerance = 10;     //ms
  CTCTankTimeTolerance = 300;    //ns		//Edit: Changed Default CTCTankTolerance from 100ns to 300ns to allow for 256ns differences [M. Nieslony]
  CTCMRDTimeTolerance = 2000000; //ns
  CTCLAPPDTimeTolerance = 3000000;	//ns
  DriftWarningValue = 5000000;   //ns
  pause_threshold = 5*60;        //s
  save_raw_data = false;	//Default option: Do not save the raw data (processed files get very large)
  store_beam_status = false;	//Should the beam status be stored? If yes, need the BeamDecoder tool in the ToolChain
  LAPPDOffsetFile = "None";	//File specifying the offset variables for the LAPPD global timestamps (if automatic determination goes wrong)

  /////////////////////////////////////////////////////////////////
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("SavePath",SavePath);
  m_variables.Get("ProcessedFilesBasename",ProcessedFilesBasename);
  m_variables.Get("BuildType",BuildType);
  m_variables.Get("NumEventsPerPairing",EventsPerPairing);
  m_variables.Get("MinNumWavesInSet",NumWavesInCompleteSet);
  m_variables.Get("OrphanOldTankTimestamps",OrphanOldTankTimestamps);
  m_variables.Get("OldTimestampThreshold",OldTimestampThreshold);
  m_variables.Get("ExecutesPerBuild",ExecutesPerBuild);
  m_variables.Get("MRDTankTimeTolerance",MRDTankTimeTolerance);
  m_variables.Get("CTCTankTimeTolerance",CTCTankTimeTolerance);
  m_variables.Get("CTCMRDTimeTolerance",CTCMRDTimeTolerance);
  m_variables.Get("CTCLAPPDTimeTolerance",CTCLAPPDTimeTolerance);
  m_variables.Get("OrphanFileBase",OrphanFileBase);
  m_variables.Get("MaxStreamMatchingTimeSeparation",pause_threshold);
  m_variables.Get("SaveRawData",save_raw_data);
  m_variables.Get("StoreBeamStatus",store_beam_status);
  m_variables.Get("LAPPDOffsetFile",LAPPDOffsetFile);
  m_variables.Get("BuildStage1Data",BuildStage1Data);
  m_variables.Get("SaveSeparatePartfiles",SaveSeparatePartfiles);
  pause_threshold*=1E9;

  if(BuildType == "TankAndMRD" || BuildType == "TankAndMRDAndCTC" || BuildType == "TankAndMRDAndCTCAndLAPPD"){
    std::cout << "BuildANNIEEvent Building Tank and MRD-merged ANNIE events. " <<
        std::endl;
  }
  else if(BuildType == "Tank" || BuildType == "MRD" || BuildType == "TankAndCTC" || BuildType == "MRDAndCTC"){
    std::cout << "BuildANNIEEvent Building " << BuildType << "ANNIEEvents only."
         << std::endl;
  }
  else{
    std::cout << "BuildANNIEEvent ERROR: BuildType not recognized! " <<
        "Please select Tank, MRD, or TankAndMRD" << std::endl;
    return false;
  }

  m_data->CStore.Get("TankPMTCrateSpaceToChannelNumMap",TankPMTCrateSpaceToChannelNumMap);
  m_data->CStore.Get("AuxCrateSpaceToChannelNumMap",AuxCrateSpaceToChannelNumMap);
  m_data->CStore.Get("MRDCrateSpaceToChannelNumMap",MRDCrateSpaceToChannelNumMap);
  m_data->CStore.Get("ChannelNumToTankPMTCrateSpaceMap",ChannelNumToTankPMTCrateSpaceMap);
  m_data->CStore.Get("AuxChannelNumToCrateSpaceMap",AuxChannelNumToCrateSpaceMap);
  
  if(BuildType == "Tank" || BuildType == "TankAndMRD" || BuildType == "TankAndMRDAndCTC" || BuildType == "TankAndCTC" || BuildType == "TankAndMRDAndCTCAndLAPPD"){
    int NumTankPMTChannels = TankPMTCrateSpaceToChannelNumMap.size();
    int NumAuxChannels = AuxCrateSpaceToChannelNumMap.size();
    if(verbosity>4) std::cout << "TOTAL TANK + AUX CHANNELS: " << NumTankPMTChannels + NumAuxChannels << std::endl;
    if(verbosity>4) std::cout << "CURRENT SET THRESHOLD FOR BUILDING PMT EVENTS: " << NumWavesInCompleteSet << std::endl;
  }

  m_data->CStore.Set("SaveRawData",save_raw_data);
  m_data->CStore.Set("LastEntry",false);
  m_data->CStore.Set("NewCalibratedData",false);
  m_data->CStore.Set("NewHitsData",false);

  FinishedTankEvents = new std::map<uint64_t, std::map<std::vector<int>, std::vector<uint16_t> > >; 
  FinishedTankEventsSampleSize = new std::map<uint64_t, std::map<std::vector<int>, int>>;

  //////////////////////initialize subrun index//////////////
  ProcessedStore = new BoostStore(false,2);
  ANNIEEvent = new BoostStore(false,2);
  ANNIEEventNum = 0;
  CurrentRunNum = -1;
  CurrentSubRunNum = -1;
  CurrentDriftMean = 0;
  
  OrphanStore = new BoostStore(false,2);

  FinishedHits = new std::map<uint64_t, std::map<unsigned long,std::vector<Hit>>*>;
  FinishedHitsAux = new std::map<uint64_t, std::map<unsigned long,std::vector<Hit>>*>;
  FinishedRecoADCHits = new std::map<uint64_t, std::map<unsigned long,std::vector<std::vector<ADCPulse>>>>;
  FinishedRecoADCHitsAux = new std::map<uint64_t, std::map<unsigned long,std::vector<std::vector<ADCPulse>>>>;

  FinishedLAPPDPsecData = new std::map<uint64_t, PsecData>;

  return true;
}


bool ANNIEEventBuilder::Execute(){
  if(BuildStage1Data){
    m_data->Stores.at("ANNIEEvent")->Delete();
    m_data->Stores["ANNIEEvent"] = new BoostStore(false, 0);
  }

  bool NewEntryAvailable;
  m_data->CStore.Get("NewRawDataEntryAccessed",NewEntryAvailable);
  if(!NewEntryAvailable){ //Something went wrong processing raw data.  Stop and save what's left
    Log("ANNIEEventBuilder Tool: There's no new PMT/MRD data.  Stopping loop, ANNIEEvent BoostStore will save.",v_warning,verbosity); 
    m_data->vars.Set("StopLoop",1);
  }
  
  m_data->CStore.Set("NewTankEvents",false);

  // ensure we always build if the toolchain is stopping
  bool toolchain_stopping=false;
  m_data->vars.Get("StopLoop",toolchain_stopping);
  // likewise ensure we try to do all possible matching if we've hit the end of the file
  bool file_completed=false;
  m_data->CStore.Get("FileCompleted",file_completed);
  //Don't include completed file boolean in the toolchain stopping condition for now, but might be an option later
  //toolchain_stopping |= file_completed;
  if(toolchain_stopping){
    Log("ANNIEEventBuilder: StopLoop or FileCompleted detected, forcing building of any remaining events in the timestream",v_warning,verbosity);
  }
    
  ExecuteCount+=1;
  
  bool last_entry = false;
  m_data->CStore.Get("LastEntry",last_entry);
  if (last_entry) this->ProcessNewTankPMTData();  

  if((ExecuteCount<ExecutesPerBuild)&&(!toolchain_stopping)) return true;

  //See if the MRD and Tank are at the same run/subrun for building
  m_data->CStore.Get("RunInfoPostgress",RunInfoPostgress);
  int RunNumber;
  int SubRunNumber;
  int PartNumber;
  uint64_t StarTime;
  int RunType;
  RunInfoPostgress.Get("RunNumber",RunNumber);
  RunInfoPostgress.Get("SubRunNumber",SubRunNumber);
  RunInfoPostgress.Get("PartNumber",PartNumber);
  RunInfoPostgress.Get("RunType",RunType);
  RunInfoPostgress.Get("StarTime",StarTime);
  //If our first execute loop, Initialize Current run information with Tank decoding progress
  if(CurrentRunNum == -1 || CurrentSubRunNum == -1){
    CurrentRunNum = RunNumber;
    CurrentSubRunNum = SubRunNumber;
    CurrentPartNum = PartNumber;
    CurrentStarTime = StarTime;
    CurrentRunType = RunType;
  }
  //If we're in a new run or subrun, make a new ANNIEEvent file. 
  if((CurrentRunNum != RunNumber) || (CurrentSubRunNum != SubRunNumber) || (CurrentPartNum != PartNumber)){
    this->OpenNewANNIEEvent(RunNumber,SubRunNumber,PartNumber,StarTime,RunType);
  }

  //Built ANNIE events with only PMT data
  if (BuildType == "Tank"){
    //Check to see if there's new PMT data
    m_data->CStore.Get("NewTankPMTDataAvailable",IsNewTankData);
    if((!IsNewTankData)&&(!toolchain_stopping)){
      Log("ANNIEEventBuilder:: No new Tank Data.  Not building ANNIEEvent. ",v_message, verbosity);
      return true;
    }
    else if(IsNewTankData) this->ProcessNewTankPMTData();
    this->ManageOrphanage();
  
    bool got_hits = false;
    got_hits = this->FetchWaveformsHits();

    std::vector<uint64_t> PMTEventsToDelete;
    if (save_raw_data){
      for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>>> apair : *FinishedTankEvents){
        std::map<std::string,bool> DataStreams;
        DataStreams.emplace("Tank",1);
        DataStreams.emplace("MRD",0);
        DataStreams.emplace("CTC",0);
        DataStreams.emplace("LAPPD",0);
        uint64_t PMTCounterTime = apair.first;
        std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = apair.second;
        this->BuildANNIEEventRunInfo(RunNumber,SubRunNumber,PartNumber,RunType,StarTime);
        this->BuildANNIEEventTankRaw(PMTCounterTime, aWaveMap);
        ANNIEEvent->Set("DataStreams",DataStreams);
        if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("DataStreams",DataStreams);
        this->SaveEntryToFile(CurrentRunNum,CurrentSubRunNum,CurrentPartNum);
        //Erase this entry from the InProgressTankEventsMap
        if(verbosity>4) std::cout << "Counter time will be erased from InProgressTankEvents: " << PMTCounterTime << std::endl;
        PMTEventsToDelete.push_back(PMTCounterTime);
      }
    } else if (got_hits){
      for(std::pair<uint64_t,std::map<unsigned long,std::vector<Hit>>*> apair : *FinishedHits){
        std::map<std::string,bool> DataStreams;
        DataStreams.emplace("Tank",1);
        DataStreams.emplace("MRD",0);
        DataStreams.emplace("CTC",0);
        DataStreams.emplace("LAPPD",0);
        uint64_t PMTCounterTime = apair.first;
        std::map<unsigned long,std::vector<Hit>>* aFinishedHits = apair.second;
        std::map<unsigned long,std::vector<std::vector<ADCPulse>>> aFinishedRecoADCHits = FinishedRecoADCHits->at(PMTCounterTime);
        std::map<unsigned long,std::vector<Hit>>* aFinishedHitsAux = FinishedHitsAux->at(PMTCounterTime);
        std::map<unsigned long,std::vector<std::vector<ADCPulse>>> aFinishedRecoADCHitsAux = FinishedRecoADCHitsAux->at(PMTCounterTime);
        std::map<unsigned long,std::vector<int>> RawAcqSize = FinishedRawAcqSize->at(PMTCounterTime);
        this->BuildANNIEEventRunInfo(RunNumber,SubRunNumber,PartNumber,RunType,StarTime);
        this->BuildANNIEEventTankHits(PMTCounterTime, aFinishedHits, aFinishedRecoADCHits, aFinishedHitsAux, aFinishedRecoADCHitsAux, RawAcqSize);
        ANNIEEvent->Set("DataStreams",DataStreams);
        if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("DataStreams",DataStreams);
        this->SaveEntryToFile(CurrentRunNum,CurrentSubRunNum,CurrentPartNum);
        //Erase this entry from the InProgressTankEventsMap
        if(verbosity>4) std::cout << "Counter time will be erased from InProgressTankEvents: " << PMTCounterTime << std::endl;
        PMTEventsToDelete.push_back(PMTCounterTime);
      }
    }
    //Update the current InProgressTankEvents map
    for(unsigned int i=0; i< PMTEventsToDelete.size(); i++) {
      FinishedTankEventsSampleSize->erase(PMTEventsToDelete.at(i));
      if (save_raw_data) FinishedTankEvents->erase(PMTEventsToDelete.at(i));
      else {
        FinishedHits->erase(PMTEventsToDelete.at(i));
        FinishedHitsAux->erase(PMTEventsToDelete.at(i));
        FinishedRecoADCHits->erase(PMTEventsToDelete.at(i));
        FinishedRecoADCHitsAux->erase(PMTEventsToDelete.at(i));
        FinishedRawAcqSize->erase(PMTEventsToDelete.at(i));
      }
    }
  }
  /*
    //Get the current InProgressTankEvents map
    if(verbosity>4) std::cout << "ANNIEEventBuilder: Getting waves and run info from CStore" << std::endl;
    bool got_inprogresstanks = m_data->CStore.Get("InProgressTankEvents",InProgressTankEvents);
    if(!got_inprogresstanks){
      std::cout << "ANNIEEventBuilder ERROR: No InProgressTankEvents pointer!" << std::endl;
      return false;
    }
 
    //Assume a whole processed file will have all it's PMT data finished
    std::vector<uint64_t> PMTEventsToDelete;
    for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>>> apair : *InProgressTankEvents){
      uint64_t PMTCounterTime = apair.first;
      if(verbosity>4) std::cout << "Finished waveset has clock counter: " << PMTCounterTime << std::endl;
      std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = apair.second;
      if(verbosity>4) std::cout << "Number of waves for this counter: " << aWaveMap.size() << std::endl;
      //For this counter, need to have the number of TankPMT channels plus number of aux channels
      if(aWaveMap.size() >= (NumWavesInCompleteSet)){
        this->BuildANNIEEventRunInfo(RunNumber,SubRunNumber,RunType,StarTime);
        this->BuildANNIEEventTank(PMTCounterTime, aWaveMap);
        this->SaveEntryToFile(CurrentRunNum,CurrentSubRunNum);
        //Erase this entry from the InProgressTankEventsMap
        if(verbosity>4) std::cout << "Counter time will be erased from InProgressTankEvents: " << PMTCounterTime << std::endl;
        PMTEventsToDelete.push_back(PMTCounterTime);
      }
    }

    //Update the current InProgressTankEvents map
    for(unsigned int i=0; i< PMTEventsToDelete.size(); i++) InProgressTankEvents->erase(PMTEventsToDelete.at(i));
    if(verbosity>3) std::cout << "Current number of unfinished PMT Waveforms: " << InProgressTankEvents->size() << std::endl;

  }
  */
  
  //Built ANNIE events with only MRD data
  else if (BuildType == "MRD"){
    m_data->CStore.Get("NewMRDDataAvailable",IsNewMRDData);
    std::vector<uint64_t> MRDEventsToDelete;
    if((!IsNewMRDData)&&(!toolchain_stopping)){
      Log("ANNIEEventBuilder:: No new MRD Data.  Not building ANNIEEvent: ",v_message, verbosity);
      return true;
    }
    m_data->CStore.Get("MRDEvents",myMRDMaps.MRDEvents);
    m_data->CStore.Get("MRDEventTriggerTypes",myMRDMaps.MRDTriggerTypeMap);
    m_data->CStore.Get("MRDBeamLoopback",myMRDMaps.MRDBeamLoopbackMap);
    m_data->CStore.Get("MRDCosmicLoopback",myMRDMaps.MRDCosmicLoopbackMap);
    int NumMRDTimestamps = myTimeStream.BeamMRDTimestamps.size();
    m_data->CStore.Set("NumMRDTimestamps",NumMRDTimestamps);
    
   //Loop through MRDEvents and process each into ANNIEEvent.
    for(std::pair<uint64_t,std::vector<std::pair<unsigned long,int>>> apair : myMRDMaps.MRDEvents){
      std::map<std::string,bool> DataStreams;
      DataStreams.emplace("Tank",0);
      DataStreams.emplace("MRD",1);
      DataStreams.emplace("CTC",0);
      DataStreams.emplace("LAPPD",0);
      uint64_t MRDTimeStamp = apair.first;
      std::vector<std::pair<unsigned long,int>> MRDHits = apair.second;
      std::string MRDTriggerType = myMRDMaps.MRDTriggerTypeMap[MRDTimeStamp];
      int beam_tdc = myMRDMaps.MRDBeamLoopbackMap[MRDTimeStamp];
      int cosmic_tdc = myMRDMaps.MRDCosmicLoopbackMap[MRDTimeStamp];
      this->BuildANNIEEventRunInfo(RunNumber,SubRunNumber,PartNumber,RunType,StarTime);
      this->BuildANNIEEventMRD(MRDHits, MRDTimeStamp, MRDTriggerType, beam_tdc, cosmic_tdc);
      ANNIEEvent->Set("DataStreams",DataStreams);
        if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("DataStreams",DataStreams);
      this->SaveEntryToFile(RunNumber,SubRunNumber,PartNumber);
      //Erase this entry from the InProgressTankEventsMap
      MRDEventsToDelete.push_back(MRDTimeStamp);
    }
    for (unsigned int i=0; i< MRDEventsToDelete.size(); i++){
      myMRDMaps.MRDEvents.erase(MRDEventsToDelete.at(i));
      myMRDMaps.MRDTriggerTypeMap.erase(MRDEventsToDelete.at(i));
      myMRDMaps.MRDBeamLoopbackMap.erase(MRDEventsToDelete.at(i));
      myMRDMaps.MRDCosmicLoopbackMap.erase(MRDEventsToDelete.at(i));
    }
    m_data->CStore.Set("MRDEvents",myMRDMaps.MRDEvents);
    m_data->CStore.Set("MRDEventTriggerTypes",myMRDMaps.MRDTriggerTypeMap);
    m_data->CStore.Set("MRDBeamLoopback",myMRDMaps.MRDBeamLoopbackMap);
    m_data->CStore.Set("MRDCosmicLoopback",myMRDMaps.MRDCosmicLoopbackMap);
  }

  //Built ANNIE events with Tank and MRD data
  else if (BuildType == "TankAndMRD"){
    //Check if any In-progress tank events now have all waveforms
    m_data->CStore.Get("NewTankPMTDataAvailable",IsNewTankData);
    if(IsNewTankData) this->ProcessNewTankPMTData();
    this->FetchWaveformsHits();
    this->ManageOrphanage();

    //Look through our MRD data for any new timestamps
    m_data->CStore.Get("MRDEventTriggerTypes",myMRDMaps.MRDTriggerTypeMap);
    m_data->CStore.Get("NewMRDDataAvailable",IsNewMRDData);
    m_data->CStore.Get("MRDEvents",myMRDMaps.MRDEvents);
    m_data->CStore.Get("MRDBeamLoopback",myMRDMaps.MRDBeamLoopbackMap);
    m_data->CStore.Get("MRDCosmicLoopback",myMRDMaps.MRDCosmicLoopbackMap);
    if(IsNewMRDData) this->ProcessNewMRDData();
    

    
    //Now, pair up PMT and MRD events...
    int NumTankTimestamps = myTimeStream.BeamTankTimestamps.size();
    int NumMRDTimestamps = myTimeStream.BeamMRDTimestamps.size();

    //Check if one of the streams needs to be paused
    
    // get get the most recent timestamp from each TimeStream
    uint64_t most_recent_beam = 0;
    if (NumTankTimestamps > 0) most_recent_beam = *std::max_element(myTimeStream.BeamTankTimestamps.begin(),
                                                  myTimeStream.BeamTankTimestamps.end());
    uint64_t most_recent_mrd = 1; //MRD will always be faster, so it's okay to have it faster for start values as well
    if (NumMRDTimestamps > 0) most_recent_mrd = *std::max_element(myTimeStream.BeamMRDTimestamps.begin(),
                                                  myTimeStream.BeamMRDTimestamps.end());
    
    // find which TimeStream is lagging the most, and what time it's currently read up to.
    std::vector<uint64_t> newest_timestamps{most_recent_beam,most_recent_mrd};
    uint64_t slowest_stream_timestamp = *std::min_element(newest_timestamps.begin(),newest_timestamps.end());
    
    if(verbosity>4){
      std::cout<<"ANNIEEventBuilder: most recent timestamps from each stream are: "
           <<most_recent_beam<<", "<<most_recent_mrd<<", "
           <<", with differences from the slowest stream being: "
           <<(static_cast<int64_t>(most_recent_beam)-static_cast<int64_t>(slowest_stream_timestamp))
           <<", "
           <<(static_cast<int64_t>(most_recent_mrd)-static_cast<int64_t>(slowest_stream_timestamp))
           <<std::endl;
    }
    
    // always ensure the slowest stream is unpaused
    if(slowest_stream_timestamp==most_recent_beam){
      m_data->CStore.Set("PauseTankDecoding",false);
    } else if(slowest_stream_timestamp==most_recent_mrd){
      m_data->CStore.Set("PauseMRDDecoding",false);
    }
    
    // if any other streams are more than X seconds ahead of the slowest one, pause them...
    if((static_cast<int64_t>(most_recent_beam)-static_cast<int64_t>(slowest_stream_timestamp))>pause_threshold){
      // secondly, only pause a stream once we have at least EventsPerPairing timestamps in it,
      // otherwise doing so will prevent building attempts
      if(NumTankTimestamps>EventsPerPairing){
        m_data->CStore.Set("PauseTankDecoding",true);
        Log("ANNIEEventBuilder: Pausing tank stream",v_debug,verbosity);
      }
    }
    if((static_cast<int64_t>(most_recent_mrd)-static_cast<int64_t>(slowest_stream_timestamp))>pause_threshold){
      if(NumMRDTimestamps>EventsPerPairing){
        m_data->CStore.Set("PauseMRDDecoding",true);
        Log("ANNIEEventBuilder: Pausing mrd stream",v_debug,verbosity);
      }
    }
    
    if(verbosity>3){
        std::cout << "Number of MRDTimes, PMTTimes: " << 
            NumMRDTimestamps << "," << NumTankTimestamps << std::endl;
    }

    int MinStamps = std::min(NumTankTimestamps,NumMRDTimestamps);


    std::map<uint64_t,uint64_t> BeamTankMRDPairs; //Pairs of beam-triggered Tank PMT/MRD counters ready to be built if all PMT waveforms are ready (TankAndMRD mode only)
    //*10 provides a buffer between paired events and events at the tail of the timestamp streams
    //Without it, events get moved to the orphanage that probably shouldn't
    if((MinStamps > (EventsPerPairing*10))||toolchain_stopping){
      this->RemoveCosmics();
      BeamTankMRDPairs = this->PairTankPMTAndMRDTriggers();
      this->ManageOrphanage();

      std::vector<uint64_t> BuiltTankTimes;
      for(std::pair<uint64_t,uint64_t> cpair : BeamTankMRDPairs){
        std::map<std::string,bool> DataStreams;
        DataStreams.emplace("Tank",1);
        DataStreams.emplace("MRD",1);
        DataStreams.emplace("CTC",0);
        DataStreams.emplace("LAPPD",0);
        uint64_t MRDTimeStamp = cpair.second;
        if(verbosity>4) std::cout << "MRD TIMESTAMP: " << MRDTimeStamp << std::endl;
        std::vector<std::pair<unsigned long,int>> MRDHits = myMRDMaps.MRDEvents.at(MRDTimeStamp);
        std::string MRDTriggerType = myMRDMaps.MRDTriggerTypeMap[MRDTimeStamp];
        int beam_tdc = myMRDMaps.MRDBeamLoopbackMap[MRDTimeStamp];
        int cosmic_tdc = myMRDMaps.MRDCosmicLoopbackMap[MRDTimeStamp];
        uint64_t TankCounterTime = cpair.first;
        if(verbosity>4) std::cout << "TANK EVENT WITH TIMESTAMP " << TankCounterTime << " HAS REQUIRED MINIMUM NUMBER OF WAVES TO BUILD" << std::endl;
        this->BuildANNIEEventRunInfo(CurrentRunNum, CurrentSubRunNum, CurrentPartNum, CurrentRunType,CurrentStarTime);
        if (save_raw_data){
          std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = FinishedTankEvents->at(TankCounterTime);
          if(verbosity>4) std::cout << "BUILDING AN ANNIE EVENT" << std::endl;
          this->BuildANNIEEventTankRaw(TankCounterTime, aWaveMap);
        } else {
          std::map<unsigned long,std::vector<Hit>>* aFinishedHits = FinishedHits->at(TankCounterTime);
          std::map<unsigned long,std::vector<std::vector<ADCPulse>>> aFinishedRecoADCHits = FinishedRecoADCHits->at(TankCounterTime);
          std::map<unsigned long,std::vector<Hit>>* aFinishedHitsAux = FinishedHitsAux->at(TankCounterTime);
          std::map<unsigned long,std::vector<std::vector<ADCPulse>>> aFinishedRecoADCHitsAux = FinishedRecoADCHitsAux->at(TankCounterTime);
          std::map<unsigned long,std::vector<int>> RawAcqSize = FinishedRawAcqSize->at(TankCounterTime);
          this->BuildANNIEEventTankHits(TankCounterTime, aFinishedHits, aFinishedRecoADCHits, aFinishedHitsAux, aFinishedRecoADCHitsAux, RawAcqSize);
      
        }
        this->BuildANNIEEventMRD(MRDHits, MRDTimeStamp, MRDTriggerType, beam_tdc, cosmic_tdc);
        ANNIEEvent->Set("DataStreams",DataStreams);
        if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("DataStreams",DataStreams);
        this->SaveEntryToFile(CurrentRunNum,CurrentSubRunNum,CurrentPartNum);
        if(verbosity>4) std::cout << "BUILT AN ANNIE EVENT (TANK + MRD) SUCCESSFULLY" << std::endl;
        //Erase this entry from maps/vectors used when pairing completed events 
        BuiltTankTimes.push_back(TankCounterTime);
        FinishedTankEventsSampleSize->erase(TankCounterTime);
        if (save_raw_data) FinishedTankEvents->erase(TankCounterTime);
        else {
          FinishedHits->erase(TankCounterTime);
          FinishedHitsAux->erase(TankCounterTime);
          FinishedRecoADCHits->erase(TankCounterTime);
          FinishedRecoADCHitsAux->erase(TankCounterTime);
          FinishedRawAcqSize->erase(TankCounterTime);
        }
        myMRDMaps.MRDEvents.erase(MRDTimeStamp);
        myMRDMaps.MRDTriggerTypeMap.erase(MRDTimeStamp);
        myMRDMaps.MRDBeamLoopbackMap.erase(MRDTimeStamp);
        myMRDMaps.MRDCosmicLoopbackMap.erase(MRDTimeStamp);
      }
      for(int i=0; i< (int) BuiltTankTimes.size(); i++){
        BeamTankMRDPairs.erase(BuiltTankTimes.at(i));
      }
    }
  }

  //Build ANNIE events based on matching Tank,MRD, and CTC timestamps
  else if (BuildType == "TankAndMRDAndCTC" || BuildType == "TankAndMRDAndCTCAndLAPPD"){
    m_data->CStore.Get("NewTankPMTDataAvailable",IsNewTankData);
    if(IsNewTankData) this->ProcessNewTankPMTData();
    this->FetchWaveformsHits();
    this->ManageOrphanage();
    
    //Look through our MRD data for any new timestamps
    m_data->CStore.Get("MRDEventTriggerTypes",myMRDMaps.MRDTriggerTypeMap);
    m_data->CStore.Get("MRDEvents",myMRDMaps.MRDEvents);
    m_data->CStore.Get("MRDBeamLoopback",myMRDMaps.MRDBeamLoopbackMap);
    m_data->CStore.Get("MRDCosmicLoopback",myMRDMaps.MRDCosmicLoopbackMap);
    m_data->CStore.Get("NewMRDDataAvailable",IsNewMRDData);
    if(IsNewMRDData) this->ProcessNewMRDData();

    //Look through CTC data for any new timestamps
    //std::cout <<"Get TimeToTriggerWordMap + BeamStatusMap"<<std::endl;
    m_data->CStore.Get("TimeToTriggerWordMap",TimeToTriggerWordMap);
    m_data->CStore.Get("TimeToTriggerWordMapComplete",TimeToTriggerWordMapComplete);
    if (store_beam_status) m_data->CStore.Get("BeamStatusMap",BeamStatusMap);
    m_data->CStore.Get("NewCTCDataAvailable",IsNewCTCData);
    std::cout <<"IsNewCTCData: "<<IsNewCTCData<<std::endl;
    if(IsNewCTCData) this->ProcessNewCTCData();
    //std::cout <<" Done "<<std::endl;

    //Look through LAPPD data for any new timestamps
    if (BuildType == "TankAndMRDAndCTCAndLAPPD"){
      m_data->CStore.Get("NewLAPPDDataAvailable",IsNewLAPPDData);
      std::cout <<"IsNewLAPPDData: "<<IsNewLAPPDData<<std::endl;
      if (IsNewLAPPDData) this->ProcessNewLAPPDData();
    }
    
    bool continue_eventbuilding = true;

    if (BuildType == "TankAndMRDAndCTCAndLAPPD"){
      bool LAPPDEntriesCompleted = false;
      bool TrigEntriesCompleted = false;
      m_data->CStore.Get("LAPPDEntriesCompleted",LAPPDEntriesCompleted);
      m_data->CStore.Get("TrigEntriesCompleted",TrigEntriesCompleted);
      //bool LAPPDAndTrigEntriesCompleted = (LAPPDEntriesCompleted && TrigEntriesCompleted);
      bool LAPPDAndTrigEntriesCompleted = file_completed;
      std::cout <<"LAPPDAndTrigEntriesCompleted: "<<LAPPDAndTrigEntriesCompleted<<std::endl;
      if (!LAPPDAndTrigEntriesCompleted){
        std::cout <<"Event Building has not completed all Trig and LAPPD entries yet. Wait until they are parsed"<<std::endl;
        continue_eventbuilding = false;
      } else {
        if (!lappd_aligned) this->AlignLAPPDTimestamps();//Align LAPPD timestamps with CTC timestamps
      }
    }

    if (continue_eventbuilding){
    //Now, pair up PMT/MRD/Triggers...
    int NumTankTimestamps = myTimeStream.BeamTankTimestamps.size();
    int NumMRDTimestamps = myTimeStream.BeamMRDTimestamps.size();
    int NumTrigs = myTimeStream.CTCTimestamps.size();
    int NumLAPPDTimestamps = 0;
    if (BuildType == "TankAndMRDAndCTCAndLAPPD") NumLAPPDTimestamps = myTimeStream.LAPPDTimestamps.size();
    if (NumLAPPDTimestamps == 0) NumLAPPDTimestamps = myTimeStream.LAPPDGlobalTimestamps.size();

    // get get the most recent timestamp from each TimeStream
    uint64_t most_recent_beam = 0;
    if (NumTankTimestamps > 0) most_recent_beam = *std::max_element(myTimeStream.BeamTankTimestamps.begin(), myTimeStream.BeamTankTimestamps.end());
    uint64_t most_recent_mrd = 1; //MRD will always be faster, so it's okay to have it faster for start values as well
    if (NumMRDTimestamps > 0) most_recent_mrd = *std::max_element(myTimeStream.BeamMRDTimestamps.begin(), myTimeStream.BeamMRDTimestamps.end());
    uint64_t most_recent_ctc = 0;
    if (NumTrigs > 0 ) most_recent_ctc = *std::max_element(myTimeStream.CTCTimestamps.begin(),
                                                  myTimeStream.CTCTimestamps.end());
    
    uint64_t most_recent_lappd = 0;
    if (BuildType == "TankAndMRDAndCTCAndLAPPD"){
      if (NumLAPPDTimestamps > 0) most_recent_lappd = *std::max_element(myTimeStream.LAPPDTimestamps.begin(),myTimeStream.LAPPDTimestamps.end());
    }

    // find which TimeStream is lagging the most, and what time it's currently read up to.
    std::vector<uint64_t> newest_timestamps{most_recent_beam,most_recent_mrd,most_recent_ctc};
    if (BuildType == "TankAndMRDAndCTCAndLAPPD") newest_timestamps.push_back(most_recent_lappd);
    uint64_t slowest_stream_timestamp = *std::min_element(newest_timestamps.begin(),newest_timestamps.end());
    
    if(verbosity>4){
      std::cout<<"ANNIEEventBuilder: most recent timestamps from each stream are: "
           <<most_recent_beam<<", "<<most_recent_mrd<<", "<<most_recent_ctc;
      if (BuildType == "TankAndMRDAndCTCAndLAPPD") std::cout <<", "<<most_recent_lappd;
           std::cout <<", with differences from the slowest stream being: "
           <<(static_cast<int64_t>(most_recent_beam)-static_cast<int64_t>(slowest_stream_timestamp))
           <<", "
           <<(static_cast<int64_t>(most_recent_mrd)-static_cast<int64_t>(slowest_stream_timestamp))
           <<", "
           <<(static_cast<int64_t>(most_recent_ctc)-static_cast<int64_t>(slowest_stream_timestamp));
       if (BuildType == "TankAndMRDAndCTCAndLAPPD") std::cout << ", " << (static_cast<int64_t>(most_recent_lappd)-static_cast<int64_t>(slowest_stream_timestamp));           
std::cout<<std::endl;
    }
    
    // always ensure the slowest stream is unpaused
    if(slowest_stream_timestamp==most_recent_beam){
      m_data->CStore.Set("PauseTankDecoding",false);
    } else if(slowest_stream_timestamp==most_recent_mrd){
      m_data->CStore.Set("PauseMRDDecoding",false);
    } else if(slowest_stream_timestamp==most_recent_ctc){
      m_data->CStore.Set("PauseCTCDecoding",false);
    } else if(slowest_stream_timestamp==most_recent_lappd){
      m_data->CStore.Set("PauseLAPPDDecoding",false);
    }
    
    if (verbosity > 4) std::cout <<"ANNIEEventBuilder Tool: Check if other streams are ahead of the slowest one"<<std::endl;
    // if any other streams are more than X seconds ahead of the slowest one, pause them...
    if((static_cast<int64_t>(most_recent_beam)-static_cast<int64_t>(slowest_stream_timestamp))>pause_threshold){
      // secondly, only pause a stream once we have at least EventsPerPairing timestamps in it,
      // otherwise doing so will prevent building attempts
      if(NumTankTimestamps>EventsPerPairing){
        m_data->CStore.Set("PauseTankDecoding",true);
        Log("ANNIEEventBuilder: Pausing tank stream",v_debug,verbosity);
      } else {
        m_data->CStore.Set("PauseTankDecoding",false);
      }
    }
    if((static_cast<int64_t>(most_recent_mrd)-static_cast<int64_t>(slowest_stream_timestamp))>pause_threshold){
      if(NumMRDTimestamps>EventsPerPairing){
        m_data->CStore.Set("PauseMRDDecoding",true);
        Log("ANNIEEventBuilder: Pausing mrd stream",v_debug,verbosity);
      } else {
        m_data->CStore.Set("PauseMRDDecoding",false);
      }
    }
    if((static_cast<int64_t>(most_recent_ctc)-static_cast<int64_t>(slowest_stream_timestamp))>pause_threshold){
      if(NumTrigs>EventsPerPairing){
        m_data->CStore.Set("PauseCTCDecoding",true);
        Log("ANNIEEventBuilder: Pausing ctc stream",v_debug,verbosity);
      } else {
        m_data->CStore.Set("PauseCTCDecoding",false);
      }
    }
    if (BuildType == "TankAndMRDAndCTCAndLAPPD"){
      if((static_cast<int64_t>(most_recent_lappd)-static_cast<int64_t>(slowest_stream_timestamp))>pause_threshold){
      if(NumLAPPDTimestamps>EventsPerPairing){
        m_data->CStore.Set("PauseLAPPDDecoding",true);
        Log("ANNIEEventBuilder: Pausing LAPPD stream",v_debug,verbosity);
      } else {
        m_data->CStore.Set("PauseLAPPDDecoding",false);
      }
      }
    }
    
    if(verbosity>3){
        std::cout << "Number of CTCTimes, MRDTimes, PMTTimes: " << 
            NumTrigs << "," << NumMRDTimestamps << "," << NumTankTimestamps << std::endl;
        if (BuildType == "TankAndMRDAndCTCAndLAPPD"){
          std::cout <<"Number of LAPPDTimes: "<< NumLAPPDTimestamps << std::endl;
        }
    }

    std::map<uint64_t,std::map<std::string,uint64_t>> ThisBuildMap; //key: CTC timestamp, value: vector of maps.  Each map has the keys: "Tank", "MRD", "CTC", or "LAPPD" with values: 
                                                                    //timestamp of that stream (exception: "CTC" key has the trigger word as the value).
    
    int MinStamps = (NumTankTimestamps<NumMRDTimestamps) ? NumTankTimestamps : NumMRDTimestamps;
    MinStamps = (NumTrigs<MinStamps) ? NumTrigs : MinStamps;
    
    // only attempt timestamp matching if we have enough timestamps to try to match,
    // OR if the toolchain is being stopped (reached and of file, for example)
    if((MinStamps>EventsPerPairing)||toolchain_stopping){
      /*if(verbosity>4) std::cout << "MERGING COSMIC/MRD PAIRS " << std::endl;
      ThisBuildMap = this->PairCTCCosmicPairs(ThisBuildMap,std::max(most_recent_mrd,most_recent_ctc),toolchain_stopping);
      this->ManageOrphanage();*/

      if(verbosity>4) std::cout << "BEGINNING STREAM MERGING " << std::endl;
      //Prioritize tank matching vs. MRD matching -> check slowest in progress timestamp
      uint64_t max_matching_time = (slowest_stream_timestamp < slowest_in_progress_tank)? slowest_stream_timestamp : slowest_in_progress_tank;
      if (verbosity > 3) std::cout <<"ANNIEEventBuilder Tool: slowest_stream_timestamp: "<<slowest_stream_timestamp<<", slowest_in_progress_tank: "<<slowest_in_progress_tank<<", max_matching_time: "<<max_matching_time<<std::endl;
      //ThisBuildMap = this->MergeStreams(ThisBuildMap,slowest_stream_timestamp,toolchain_stopping);
      ThisBuildMap = this->MergeStreams(ThisBuildMap,max_matching_time,toolchain_stopping);
      Log("ANNIEEventBuilder: Calling ManageOrphanage post MergeStreams",v_debug,verbosity);
      this->ManageOrphanage();
      Log("ANNIEEventBuilder: Done managing orphanage",v_debug,verbosity);

      std::vector<uint64_t> TimesToDelete;

      for(std::pair<uint64_t, std::map<std::string,uint64_t>> buildmap_entries : ThisBuildMap){
        uint64_t CTCtimestamp = buildmap_entries.first;
        //std::cout <<"CTCTimestamp: "<<CTCtimestamp<<std::endl;
        std::map<std::string,uint64_t> aBuildSet = buildmap_entries.second; 
        std::map<std::string,bool> DataStreams;
        DataStreams.emplace("Tank",0);
        DataStreams.emplace("MRD",0);
        DataStreams.emplace("CTC",0);
        DataStreams.emplace("LAPPD",0);
        for(std::pair<std::string,uint64_t> buildset_entries : aBuildSet){
          this->BuildANNIEEventRunInfo(CurrentRunNum, CurrentSubRunNum, CurrentPartNum, CurrentRunType,CurrentStarTime);
          //If we have PMT and MRD infor for this trigger, build it 
          std::string label = buildset_entries.first;
          if(label == "CTC"){
            uint64_t CTCWord = buildset_entries.second;
            int CTCWordExtended = CTCExtended[CTCtimestamp];
            this->BuildANNIEEventCTC(CTCtimestamp,CTCWord,CTCWordExtended);
            TimeToTriggerWordMap->erase(CTCtimestamp);
            CTCExtended.erase(CTCtimestamp);
            DataStreams["CTC"]=1;
            if (store_beam_status){
              if (BeamStatusMap->count(CTCtimestamp) == 0){
                Log("ANNIEEventBuilder: Did not find CTCtimestamp "+std::to_string(CTCtimestamp)+" in BeamStatusMap! Don't save BeamStatus in ANNIEEvent",v_error,verbosity);
              } else {
                BeamStatus beam_status = BeamStatusMap->at(CTCtimestamp);
                ANNIEEvent->Set("BeamStatus",beam_status);
                if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("BeamStatus",beam_status);
                BeamStatusMap->erase(CTCtimestamp);
              }
            }
          }
          if(label == "TankPMT"){
            uint64_t TankPMTTime = buildset_entries.second;
            if(verbosity>4) std::cout << "TANK EVENT WITH TIMESTAMP " << TankPMTTime << "HAS REQUIRED MINIMUM NUMBER OF WAVES TO BUILD" << std::endl;
            if (save_raw_data) {
              std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = FinishedTankEvents->at(TankPMTTime);
              this->BuildANNIEEventTankRaw(TankPMTTime, aWaveMap);
            }
            else {
              std::map<unsigned long,std::vector<Hit>>* aFinishedHits = FinishedHits->at(TankPMTTime);
              std::map<unsigned long,std::vector<std::vector<ADCPulse>>> aFinishedRecoADCHits = FinishedRecoADCHits->at(TankPMTTime);
              std::map<unsigned long,std::vector<Hit>>* aFinishedHitsAux = FinishedHitsAux->at(TankPMTTime);
              std::map<unsigned long,std::vector<std::vector<ADCPulse>>> aFinishedRecoADCHitsAux = FinishedRecoADCHitsAux->at(TankPMTTime);
              std::map<unsigned long,std::vector<int>> RawAcqSize = FinishedRawAcqSize->at(TankPMTTime);
              this->BuildANNIEEventTankHits(TankPMTTime, aFinishedHits, aFinishedRecoADCHits, aFinishedHitsAux, aFinishedRecoADCHitsAux, RawAcqSize);
              TimesToDelete.push_back(TankPMTTime);
            }
            FinishedTankEventsSampleSize->erase(TankPMTTime);
            if (save_raw_data) FinishedTankEvents->erase(TankPMTTime);
            DataStreams["Tank"]=1;
          }
          if(label == "MRD"){
            uint64_t MRDTimeStamp = buildset_entries.second;
            if(verbosity>4) std::cout << "MRD TIMESTAMP: " << MRDTimeStamp << std::endl;
            std::vector<std::pair<unsigned long,int>> MRDHits = myMRDMaps.MRDEvents.at(MRDTimeStamp);
            std::string MRDTriggerType = myMRDMaps.MRDTriggerTypeMap[MRDTimeStamp];
            int beam_tdc = myMRDMaps.MRDBeamLoopbackMap[MRDTimeStamp];
            int cosmic_tdc = myMRDMaps.MRDCosmicLoopbackMap[MRDTimeStamp];
            this->BuildANNIEEventMRD(MRDHits, MRDTimeStamp, MRDTriggerType, beam_tdc, cosmic_tdc);
            myMRDMaps.MRDEvents.erase(MRDTimeStamp);
            myMRDMaps.MRDTriggerTypeMap.erase(MRDTimeStamp);
            myMRDMaps.MRDBeamLoopbackMap.erase(MRDTimeStamp);
            myMRDMaps.MRDCosmicLoopbackMap.erase(MRDTimeStamp);
            DataStreams["MRD"]=1;
          }
          if(label == "LAPPD"){
            uint64_t LAPPDTimeStamp = buildset_entries.second;
            if (verbosity > 4) std::cout <<"LAPPD timestamp: "<<LAPPDTimeStamp << std::endl;
            PsecData psec = FinishedLAPPDPsecData->at(LAPPDTimeStamp);
            this->BuildANNIEEventLAPPD(psec,LAPPDTimeStamp,lappd_time_offset);
            FinishedLAPPDPsecData->erase(LAPPDTimeStamp);
            DataStreams["LAPPD"]=1;
          }
        }
	//Set empty data variables in case some stream is not built
	if (DataStreams["MRD"]==0){
          //std::cout <<"datastreams[mrd]=0"<<std::endl;
          std::vector<std::pair<unsigned long,int>> empty_mrdhits;
          uint64_t default_mrdtimestamp=0;
          std::string default_mrdtrigger="None";
          int default_beamtdc=0;
          int default_cosmictdc=0;
          this->BuildANNIEEventMRD(empty_mrdhits, default_mrdtimestamp, default_mrdtrigger, default_beamtdc, default_cosmictdc);
	} else if (DataStreams["Tank"]==0){
          //std::cout <<"datastreams[tank]=0"<<std::endl;
          uint64_t default_clocktime=0;
          if (save_raw_data){
            std::map<std::vector<int>, std::vector<uint16_t>> empty_wavemap;
            this->BuildANNIEEventTankRaw(default_clocktime, empty_wavemap);
          } else {
            std::map<unsigned long,std::vector<Hit>>* emptyHits = new std::map<unsigned long,std::vector<Hit>>;
            std::map<unsigned long,std::vector<Hit>>* emptyHitsAux = new std::map<unsigned long,std::vector<Hit>>;
            std::map<unsigned long,std::vector<std::vector<ADCPulse>>> emptyRecoADC;
            std::map<unsigned long,std::vector<int>> emptyAcqSize;
            this->BuildANNIEEventTankHits(default_clocktime,emptyHits,emptyRecoADC,emptyHitsAux,emptyRecoADC,emptyAcqSize);
          }
	} else if (DataStreams["LAPPD"]==0){
          uint64_t default_lappdtimestamp = 0;
          PsecData empty_psec;
          this->BuildANNIEEventLAPPD(empty_psec,default_lappdtimestamp,lappd_time_offset);
        }
	ANNIEEvent->Set("DataStreams",DataStreams);
        if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("DataStreams",DataStreams);
        this->SaveEntryToFile(CurrentRunNum,CurrentSubRunNum,CurrentPartNum);
        if(verbosity>4) std::cout << "BUILT AN ANNIE EVENT (TANK + MRD + CTC) SUCCESSFULLY" << std::endl;
      }
      for (int i_delete = 0; i_delete < (int) TimesToDelete.size(); i_delete++){
        FinishedHits->erase(TimesToDelete.at(i_delete));
        FinishedHitsAux->erase(TimesToDelete.at(i_delete));
        FinishedRecoADCHits->erase(TimesToDelete.at(i_delete));
        FinishedRecoADCHitsAux->erase(TimesToDelete.at(i_delete));
        FinishedRawAcqSize->erase(TimesToDelete.at(i_delete));
      }   
      m_data->CStore.Set("FinishedTankEvents",FinishedTankEvents);
    }
    ThisBuildMap.clear();
  }}
 //Build ANNIE events based on matching Tank and CTC timestamps
  else if (BuildType == "TankAndCTC"){
    m_data->CStore.Get("NewTankPMTDataAvailable",IsNewTankData);
    if(IsNewTankData) this->ProcessNewTankPMTData();
    this->ManageOrphanage();
    this->FetchWaveformsHits();

    //Look through CTC data for any new timestamps
    m_data->CStore.Get("TimeToTriggerWordMap",TimeToTriggerWordMap);
    m_data->CStore.Get("TimeToTriggerWordMapComplete",TimeToTriggerWordMapComplete);
    if (store_beam_status) m_data->CStore.Get("BeamStatusMap",BeamStatusMap);
    m_data->CStore.Get("NewCTCDataAvailable",IsNewCTCData);
    if(IsNewCTCData) this->ProcessNewCTCData();

    //Now, pair up PMT/MRD/Triggers...
    int NumTankTimestamps = myTimeStream.BeamTankTimestamps.size();
    int NumTrigs = myTimeStream.CTCTimestamps.size();
    
    // get get the most recent timestamp from each TimeStream
    uint64_t most_recent_beam = 0;
    if (NumTankTimestamps > 0) most_recent_beam = *std::max_element(myTimeStream.BeamTankTimestamps.begin(),
                                                  myTimeStream.BeamTankTimestamps.end());
    uint64_t most_recent_ctc = 0;
    if (NumTrigs > 0) most_recent_ctc = *std::max_element(myTimeStream.CTCTimestamps.begin(),
                                                  myTimeStream.CTCTimestamps.end());
    
    // find which TimeStream is lagging the most, and what time it's currently read up to.
    std::vector<uint64_t> newest_timestamps{most_recent_beam,most_recent_ctc};
    uint64_t slowest_stream_timestamp = *std::min_element(newest_timestamps.begin(),newest_timestamps.end());
    
    if(verbosity>4){
      std::cout<<"ANNIEEventBuilder: most recent timestamps from each stream are: "
           <<most_recent_beam<<", "<<most_recent_ctc
           <<", with differences from the slowest stream being: "
           <<(static_cast<int64_t>(most_recent_beam)-static_cast<int64_t>(slowest_stream_timestamp))
           <<", "
           <<(static_cast<int64_t>(most_recent_ctc)-static_cast<int64_t>(slowest_stream_timestamp))
           <<std::endl;
    }
    
    // always ensure the slowest stream is unpaused
    if(slowest_stream_timestamp==most_recent_beam){
      m_data->CStore.Set("PauseTankDecoding",false);
    } else if(slowest_stream_timestamp==most_recent_ctc){
      m_data->CStore.Set("PauseCTCDecoding",false);
    }
    
    // if any other streams are more than X seconds ahead of the slowest one, pause them...
    if((static_cast<int64_t>(most_recent_beam)-static_cast<int64_t>(slowest_stream_timestamp))>pause_threshold){
      // secondly, only pause a stream once we have at least EventsPerPairing timestamps in it,
      // otherwise doing so will prevent building attempts
      if(NumTankTimestamps>EventsPerPairing){
        m_data->CStore.Set("PauseTankDecoding",true);
        Log("ANNIEEventBuilder: Pausing tank stream",v_debug,verbosity);
      } else {
      m_data->CStore.Set("PauseTankDecoding",false);
      }
    }
    if((static_cast<int64_t>(most_recent_ctc)-static_cast<int64_t>(slowest_stream_timestamp))>pause_threshold){
      if(NumTrigs>EventsPerPairing){
        m_data->CStore.Set("PauseCTCDecoding",true);
        Log("ANNIEEventBuilder: Pausing ctc stream",v_debug,verbosity);
      } else {
        m_data->CStore.Set("PauseCTCDecoding",false);
      }
    }
    
    if(verbosity>3){
        std::cout << "Number of CTCTimes, PMTTimes: " << 
            NumTrigs << "," << NumTankTimestamps << std::endl;
    }

    std::map<uint64_t,std::map<std::string,uint64_t>> ThisBuildMap; //key: CTC timestamp, value: vector of maps.  Each map has the keys: "Tank", "MRD", "CTC", or "LAPPD" with values: 
                                                                    //timestamp of that stream (exception: "CTC" key has the trigger word as the value).
    
    int MinStamps = (NumTankTimestamps<NumTrigs) ? NumTankTimestamps : NumTrigs;
    
    // only attempt timestamp matching if we have enough timestamps to try to match,
    // OR if the toolchain is being stopped (reached and of file, for example)
    if((MinStamps>EventsPerPairing)||toolchain_stopping){

      uint64_t max_matching_time = (slowest_stream_timestamp < slowest_in_progress_tank)? slowest_stream_timestamp : slowest_in_progress_tank;
      if(verbosity>4) std::cout << "BEGINNING STREAM MERGING " << std::endl;
      ThisBuildMap = this->MergeStreams(ThisBuildMap,max_matching_time,toolchain_stopping);
      //ThisBuildMap = this->MergeStreams(ThisBuildMap,slowest_stream_timestamp,toolchain_stopping);
      Log("ANNIEEventBuilder: Calling ManageOrphanage post MergeStreams",v_debug,verbosity);
      this->ManageOrphanage();
      Log("ANNIEEventBuilder: Done managing orphanage",v_debug,verbosity);

      std::vector<uint64_t> TimesToDelete;
      for(std::pair<uint64_t, std::map<std::string,uint64_t>> buildmap_entries : ThisBuildMap){
        std::map<std::string,bool> DataStreams;
        DataStreams.emplace("Tank",0);
        DataStreams.emplace("MRD",0);
        DataStreams.emplace("CTC",0);
        DataStreams.emplace("LAPPD",0);
        uint64_t CTCtimestamp = buildmap_entries.first;
        std::map<std::string,uint64_t> aBuildSet = buildmap_entries.second; 
        this->BuildANNIEEventRunInfo(CurrentRunNum, CurrentSubRunNum, CurrentPartNum, CurrentRunType,CurrentStarTime);
        for(std::pair<std::string,uint64_t> buildset_entries : aBuildSet){
          //If we have PMT and MRD infor for this trigger, build it 
          std::string label = buildset_entries.first;
          if(label == "CTC"){
            uint64_t CTCWord = buildset_entries.second;
            int CTCWordExtended = CTCExtended[CTCtimestamp];
            this->BuildANNIEEventCTC(CTCtimestamp,CTCWord,CTCWordExtended);
            TimeToTriggerWordMap->erase(CTCtimestamp);
            CTCExtended.erase(CTCtimestamp);
            DataStreams["CTC"]=1;
            if (store_beam_status){
              if (BeamStatusMap->count(CTCtimestamp) == 0){
                Log("ANNIEEventBuilder: Did not find CTCtimestamp "+std::to_string(CTCtimestamp)+" in BeamStatusMap! Don't save BeamStatus in ANNIEEvent",v_error,verbosity);
              } else {
                BeamStatus beam_status = BeamStatusMap->at(CTCtimestamp);
                ANNIEEvent->Set("BeamStatus",beam_status);
                if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("BeamStatus",beam_status);
                BeamStatusMap->erase(CTCtimestamp);
              }
            }
          }
          if(label == "TankPMT"){
            uint64_t TankPMTTime = buildset_entries.second;
            if(verbosity>4) std::cout << "TANK EVENT WITH TIMESTAMP " << TankPMTTime << "HAS REQUIRED MINIMUM NUMBER OF WAVES TO BUILD" << std::endl;
            if (save_raw_data) {
              std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = FinishedTankEvents->at(TankPMTTime);
              this->BuildANNIEEventTankRaw(TankPMTTime, aWaveMap);
            }
            else {
              std::map<unsigned long,std::vector<Hit>>* aFinishedHits = FinishedHits->at(TankPMTTime);
              std::map<unsigned long,std::vector<std::vector<ADCPulse>>> aFinishedRecoADCHits = FinishedRecoADCHits->at(TankPMTTime);
              std::map<unsigned long,std::vector<Hit>>* aFinishedHitsAux = FinishedHitsAux->at(TankPMTTime);
              std::map<unsigned long,std::vector<std::vector<ADCPulse>>> aFinishedRecoADCHitsAux = FinishedRecoADCHitsAux->at(TankPMTTime);
              std::map<unsigned long,std::vector<int>> RawAcqSize = FinishedRawAcqSize->at(TankPMTTime);
              this->BuildANNIEEventTankHits(TankPMTTime, aFinishedHits, aFinishedRecoADCHits, aFinishedHitsAux, aFinishedRecoADCHitsAux, RawAcqSize);
              TimesToDelete.push_back(TankPMTTime);
            }
            FinishedTankEventsSampleSize->erase(TankPMTTime);
            if (save_raw_data) FinishedTankEvents->erase(TankPMTTime);
            DataStreams["Tank"]=1;
          }
        }
        ANNIEEvent->Set("DataStreams",DataStreams);
        if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("DataStreams",DataStreams);
        this->SaveEntryToFile(CurrentRunNum,CurrentSubRunNum,CurrentPartNum);
        if(verbosity>4) std::cout << "BUILT AN ANNIE EVENT (TANK + CTC) SUCCESSFULLY" << std::endl;
      }
      
      for (int i_delete = 0; i_delete < (int) TimesToDelete.size(); i_delete++){
        FinishedHits->erase(TimesToDelete.at(i_delete));
        FinishedHitsAux->erase(TimesToDelete.at(i_delete));
        FinishedRecoADCHits->erase(TimesToDelete.at(i_delete));
        FinishedRecoADCHitsAux->erase(TimesToDelete.at(i_delete));
        FinishedRawAcqSize->erase(TimesToDelete.at(i_delete));
      }
    }
    ThisBuildMap.clear();
  }
 //Build ANNIE events based on matching MRD and CTC timestamps
  else if (BuildType == "MRDAndCTC"){

    //Look through our MRD data for any new timestamps
    m_data->CStore.Get("MRDEventTriggerTypes",myMRDMaps.MRDTriggerTypeMap);
    m_data->CStore.Get("MRDEvents",myMRDMaps.MRDEvents);
    m_data->CStore.Get("MRDBeamLoopback",myMRDMaps.MRDBeamLoopbackMap);
    m_data->CStore.Get("MRDCosmicLoopback",myMRDMaps.MRDCosmicLoopbackMap);
    m_data->CStore.Get("NewMRDDataAvailable",IsNewMRDData);
    if(IsNewMRDData) this->ProcessNewMRDData();

    //Look through CTC data for any new timestamps
    m_data->CStore.Get("TimeToTriggerWordMap",TimeToTriggerWordMap);
    m_data->CStore.Get("TimeToTriggerWordMapComplete",TimeToTriggerWordMapComplete);
    if (store_beam_status) m_data->CStore.Get("BeamStatusMap",BeamStatusMap);
    m_data->CStore.Get("NewCTCDataAvailable",IsNewCTCData);
    if(IsNewCTCData) this->ProcessNewCTCData();

    //Now, pair up PMT/MRD/Triggers...
    int NumMRDTimestamps = myTimeStream.BeamMRDTimestamps.size();
    int NumTrigs = myTimeStream.CTCTimestamps.size();
    
    // get get the most recent timestamp from each TimeStream
    uint64_t most_recent_mrd = 0;
    if (NumMRDTimestamps > 0) most_recent_mrd = *std::max_element(myTimeStream.BeamMRDTimestamps.begin(),
                                                  myTimeStream.BeamMRDTimestamps.end());
    uint64_t most_recent_ctc = 0;
    if (NumTrigs > 0) most_recent_ctc = *std::max_element(myTimeStream.CTCTimestamps.begin(),
                                                  myTimeStream.CTCTimestamps.end());
    
    // find which TimeStream is lagging the most, and what time it's currently read up to.
    std::vector<uint64_t> newest_timestamps{most_recent_mrd,most_recent_ctc};
    uint64_t slowest_stream_timestamp = *std::min_element(newest_timestamps.begin(),newest_timestamps.end());
    
    if(verbosity>4){
      std::cout<<"ANNIEEventBuilder: most recent timestamps from each stream are: "
           <<most_recent_mrd<<", "<<most_recent_ctc
           <<", with differences from the slowest stream being: "
           <<(static_cast<int64_t>(most_recent_mrd)-static_cast<int64_t>(slowest_stream_timestamp))
           <<", "
           <<(static_cast<int64_t>(most_recent_ctc)-static_cast<int64_t>(slowest_stream_timestamp))
           <<std::endl;
    }
    
    // always ensure the slowest stream is unpaused
    if(slowest_stream_timestamp==most_recent_mrd){
      m_data->CStore.Set("PauseMRDDecoding",false);
    } else if(slowest_stream_timestamp==most_recent_ctc){
      m_data->CStore.Set("PauseCTCDecoding",false);
    }
    
    // if any other streams are more than X seconds ahead of the slowest one, pause them...
    if((static_cast<int64_t>(most_recent_mrd)-static_cast<int64_t>(slowest_stream_timestamp))>pause_threshold){
      // secondly, only pause a stream once we have at least EventsPerPairing timestamps in it,
      // otherwise doing so will prevent building attempts
      if(NumMRDTimestamps>EventsPerPairing){
        m_data->CStore.Set("PauseMRDDecoding",true);
        Log("ANNIEEventBuilder: Pausing MRD stream",v_debug,verbosity);
      } else {
      m_data->CStore.Set("PauseMRDDecoding",false);
      }
    }
    if((static_cast<int64_t>(most_recent_ctc)-static_cast<int64_t>(slowest_stream_timestamp))>pause_threshold){
      if(NumTrigs>EventsPerPairing){
        m_data->CStore.Set("PauseCTCDecoding",true);
        Log("ANNIEEventBuilder: Pausing ctc stream",v_debug,verbosity);
      } else {
        m_data->CStore.Set("PauseCTCDecoding",false);
      }
    }
    
    if(verbosity>3){
        std::cout << "Number of CTCTimes, MRDTimes: " << 
            NumTrigs << "," << NumMRDTimestamps << std::endl;
    }

    std::map<uint64_t,std::map<std::string,uint64_t>> ThisBuildMap; //key: CTC timestamp, value: vector of maps.  Each map has the keys: "Tank", "MRD", "CTC", or "LAPPD" with values: 
                                                                    //timestamp of that stream (exception: "CTC" key has the trigger word as the value).
    
    int MinStamps = (NumMRDTimestamps<NumTrigs) ? NumMRDTimestamps : NumTrigs;
    
    // only attempt timestamp matching if we have enough timestamps to try to match,
    // OR if the toolchain is being stopped (reached and of file, for example)
    if((MinStamps>EventsPerPairing)||toolchain_stopping){

      if(verbosity>4) std::cout << "BEGINNING STREAM MERGING " << std::endl;
      ThisBuildMap = this->MergeStreams(ThisBuildMap,slowest_stream_timestamp,toolchain_stopping);
      Log("ANNIEEventBuilder: Calling ManageOrphanage post MergeStreams",v_debug,verbosity);
      this->ManageOrphanage();
      Log("ANNIEEventBuilder: Done managing orphanage",v_debug,verbosity);

      for(std::pair<uint64_t, std::map<std::string,uint64_t>> buildmap_entries : ThisBuildMap){
        std::map<std::string,bool> DataStreams;
        DataStreams.emplace("Tank",0);
        DataStreams.emplace("MRD",0);
        DataStreams.emplace("CTC",0);
        DataStreams.emplace("LAPPD",0);
        uint64_t CTCtimestamp = buildmap_entries.first;
        std::map<std::string,uint64_t> aBuildSet = buildmap_entries.second; 
        for(std::pair<std::string,uint64_t> buildset_entries : aBuildSet){
          this->BuildANNIEEventRunInfo(CurrentRunNum, CurrentSubRunNum, CurrentPartNum, CurrentRunType,CurrentStarTime);
          //If we have PMT and MRD infor for this trigger, build it 
          std::string label = buildset_entries.first;
          if(label == "CTC"){
            uint64_t CTCWord = buildset_entries.second;
            int CTCWordExtended = CTCExtended[CTCtimestamp];
            this->BuildANNIEEventCTC(CTCtimestamp,CTCWord,CTCWordExtended);
            TimeToTriggerWordMap->erase(CTCtimestamp);
            CTCExtended.erase(CTCtimestamp);
            DataStreams["CTC"]=1;
            if (store_beam_status){
              if (BeamStatusMap->count(CTCtimestamp) == 0){
                Log("ANNIEEventBuilder: Did not find CTCtimestamp "+std::to_string(CTCtimestamp)+" in BeamStatusMap! Don't save BeamStatus in ANNIEEvent",v_error,verbosity);
              } else {
                BeamStatus beam_status = BeamStatusMap->at(CTCtimestamp);
                ANNIEEvent->Set("BeamStatus",beam_status);
                if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("BeamStatus",beam_status);
                BeamStatusMap->erase(CTCtimestamp);
              }
            }
          }
          if(label == "MRD"){
            uint64_t MRDTimeStamp = buildset_entries.second;
            if(verbosity>4) std::cout << "MRD TIMESTAMP: " << MRDTimeStamp << std::endl;
            std::vector<std::pair<unsigned long,int>> MRDHits = myMRDMaps.MRDEvents.at(MRDTimeStamp);
            std::string MRDTriggerType = myMRDMaps.MRDTriggerTypeMap[MRDTimeStamp];
            int beam_tdc = myMRDMaps.MRDBeamLoopbackMap[MRDTimeStamp];
            int cosmic_tdc = myMRDMaps.MRDCosmicLoopbackMap[MRDTimeStamp];
            this->BuildANNIEEventMRD(MRDHits, MRDTimeStamp, MRDTriggerType, beam_tdc, cosmic_tdc);
            myMRDMaps.MRDEvents.erase(MRDTimeStamp);
            myMRDMaps.MRDTriggerTypeMap.erase(MRDTimeStamp);
            myMRDMaps.MRDBeamLoopbackMap.erase(MRDTimeStamp);
            myMRDMaps.MRDCosmicLoopbackMap.erase(MRDTimeStamp);
            DataStreams["MRD"]=1;
          }
        }
        ANNIEEvent->Set("DataStreams",DataStreams);
        if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("DataStreams",DataStreams);
        this->SaveEntryToFile(CurrentRunNum,CurrentSubRunNum,CurrentPartNum);
        if(verbosity>4) std::cout << "BUILT AN ANNIE EVENT (MRD + CTC) SUCCESSFULLY" << std::endl;
      }
    }
    ThisBuildMap.clear();
  }

  m_data->CStore.Set("MRDEvents",myMRDMaps.MRDEvents);
  m_data->CStore.Set("MRDEventTriggerTypes",myMRDMaps.MRDTriggerTypeMap);
  m_data->CStore.Set("MRDBeamLoopback",myMRDMaps.MRDBeamLoopbackMap);
  m_data->CStore.Set("MRDCosmicLoopback",myMRDMaps.MRDCosmicLoopbackMap);
  myMRDMaps.MRDEvents.clear();
  myMRDMaps.MRDTriggerTypeMap.clear();
  myMRDMaps.MRDBeamLoopbackMap.clear();
  myMRDMaps.MRDCosmicLoopbackMap.clear();

  ExecuteCount = 0;
  return true;
}


bool ANNIEEventBuilder::Finalise(){

  //Deal with any remaining orphans
  if(OrphanOldTankTimestamps && (BuildType == "Tank" || BuildType == "TankAndMRD" || BuildType == "TankAndCTC" || BuildType == "TankAndMRDAndCTC")){
    std::map<uint64_t,std::string> MRDOrphans;
    std::map<uint64_t,double> MRDOrphansTDiff;
    std::map<uint64_t,std::string> CTCOrphans;
    std::map<uint64_t,std::string> TankOrphans;
    std::map<uint64_t,int> TankOrphansWaveMap;
    std::map<uint64_t,std::vector<std::vector<int>>> TankOrphansChannels;
    std::map<uint64_t,double> TankOrphansTDiff;
    if (save_raw_data){
    if (verbosity > 4) std::cout <<"ANNIEEventBuilder: Remaining in progress events in Finalise step: "<<InProgressTankEvents->size()<<std::endl;
    for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>>> apair : *InProgressTankEvents){
      uint64_t PMTCounterTimeNs = apair.first;
      std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = apair.second;
      if(aWaveMap.size() < (NumWavesInCompleteSet)){
        TankOrphans.emplace(PMTCounterTimeNs,"incomplete_tank_event");
        TankOrphansWaveMap.emplace(PMTCounterTimeNs,aWaveMap.size());
        std::vector<std::vector<int>> aWaveMapChannels = GetChannelsFromWaveMap(aWaveMap);
        TankOrphansChannels.emplace(PMTCounterTimeNs,aWaveMapChannels);
        TankOrphansTDiff.emplace(PMTCounterTimeNs,0.);
      }
    }
    }else {
    if (verbosity > 4) std::cout <<"ANNIEEventBuilder: Remaining in progress events in Finalise step: "<<InProgressHits->size()<<std::endl;
    for(std::pair<uint64_t,std::map<unsigned long, std::vector<Hit>>*> apair : *InProgressHits){
      uint64_t PMTCounterTimeNs = apair.first;
      std::map<unsigned long, std::vector<Hit>> *aHitMap = apair.second;
      std::vector<unsigned long> aChkey = InProgressChkey->at(PMTCounterTimeNs);
      if(aChkey.size() < (NumWavesInCompleteSet)){
        TankOrphans.emplace(PMTCounterTimeNs,"incomplete_tank_event");
        TankOrphansWaveMap.emplace(PMTCounterTimeNs,aChkey.size());
        std::vector<std::vector<int>> aWaveMapChannels = GetChannelsFromHitMap(aChkey);
        TankOrphansChannels.emplace(PMTCounterTimeNs,aWaveMapChannels);
        TankOrphansTDiff.emplace(PMTCounterTimeNs,0.);
      }
    }
    }
    this->MoveToOrphanage(TankOrphans, TankOrphansWaveMap, TankOrphansChannels, TankOrphansTDiff, MRDOrphans, MRDOrphansTDiff, CTCOrphans);
    this->ManageOrphanage();
  }

  if(verbosity>4) std::cout << "ANNIEEvent Finalising.  Closing any open ANNIEEvent Boostore" << std::endl;
  if(verbosity>2) std::cout << "ANNIEEventBuilder: Saving and closing file." << std::endl;
  ANNIEEvent->Close();
  ANNIEEvent->Delete();
  delete ANNIEEvent;
  if(verbosity>2) std::cout << "PMT/MRD Orphan number at finalise: " << myOrphanage.OrphanTankTimestamps.size() <<
          "," << myOrphanage.OrphanMRDTimestamps.size() << std::endl;
  //Save the current subrun and delete ANNIEEvent
  OrphanStore->Close();
  OrphanStore->Delete();
  delete OrphanStore;
  std::cout << "ANNIEEventBuilder Exitting" << std::endl;
  return true;
}

void ANNIEEventBuilder::ProcessNewCTCData(){
  for(std::pair<uint64_t,std::vector<uint32_t>> apair : *TimeToTriggerWordMap){
    uint64_t CTCTimeStamp = apair.first;
    myTimeStream.CTCTimestamps.push_back(CTCTimeStamp);
    std::vector<uint32_t> CTCWordVector = apair.second;
    for (int i_ctcword=0; i_ctcword < (int) CTCWordVector.size(); i_ctcword++){
      uint32_t CTCWord = CTCWordVector.at(i_ctcword);
      if(verbosity>5) std::cout << "CTCTIMESTAMP,WORD" << CTCTimeStamp << "," << CTCWord << std::endl;
    }
  }
  RemoveDuplicates(myTimeStream.CTCTimestamps);

  std::vector<uint64_t> aux_trigword_delete;
  //Read in auxiliary triggerword information, use information from trigword 40 (min-bias) & 41 (CC-extended readout)
  for (std::pair<uint64_t,std::vector<uint32_t>> apair : *TimeToTriggerWordMapComplete){
    uint64_t CTCTimestamp = apair.first;
    std::vector<uint32_t> CTCWordVector = apair.second;
    for (int i_ctcword=0; i_ctcword < (int) CTCWordVector.size(); i_ctcword++){
      uint32_t CTCWord = CTCWordVector.at(i_ctcword);
      if (CTCWord == 40) myTimeStream.CTCTimestampsExtendedNC.push_back(CTCTimestamp);
      if (CTCWord == 41) myTimeStream.CTCTimestampsExtendedCC.push_back(CTCTimestamp);
      if (CTCWord == 32) myTimeStream.CTCTimestampsPPS.push_back(CTCTimestamp);
      if (CTCWord == 5) {
        myTimeStream.CTCTimestampsBeam.push_back(CTCTimestamp);
        std::cout <<"CTCTimestampBeam: "<<CTCTimestamp<<std::endl;
      }
    }
    aux_trigword_delete.push_back(CTCTimestamp);
  }

  //Erase all processed TimeToTriggerWordMapComplete info
  for (int i_aux = 0; i_aux < (int) aux_trigword_delete.size(); i_aux++){
    TimeToTriggerWordMapComplete->erase(aux_trigword_delete.at(i_aux));
  }

  if (verbosity > 4) std::cout <<"ANNIEEventBuilder: Extended trigwords? myTimeStream.CTCTimestamps.size(): "<<myTimeStream.CTCTimestamps.size()<<std::endl;
  //Check for each trigger timestamp if there was an extended triggerword in the immediate vicinity
  for (int i_ctc=0; i_ctc < (int) myTimeStream.CTCTimestamps.size(); i_ctc++){
    uint64_t CTCTimeStamp = myTimeStream.CTCTimestamps.at(i_ctc);
    int extended_information=0;
    for (int i_ext=0; i_ext < (int) myTimeStream.CTCTimestampsExtendedCC.size(); i_ext++){
      uint64_t CTCTimeStampCC = myTimeStream.CTCTimestampsExtendedCC.at(i_ext);
      double diff_CC = double(CTCTimeStamp) - double(CTCTimeStampCC);
      if (fabs(diff_CC) < 5000) { extended_information = 1;}
    }
    for (int i_ext=0; i_ext < (int) myTimeStream.CTCTimestampsExtendedNC.size(); i_ext++){
      uint64_t CTCTimeStampNC = myTimeStream.CTCTimestampsExtendedNC.at(i_ext);
      double diff_NC = double(CTCTimeStamp) - double(CTCTimeStampNC);
      if (fabs(diff_NC) < 5000) { extended_information = 2;}
    }
    CTCExtended.emplace(CTCTimeStamp,extended_information);
  }

  m_data->CStore.Set("NewCTCDataAvailable",false);

  return;
}

void ANNIEEventBuilder::ProcessNewLAPPDData(){

  std::cout <<"ProcessNewLAPPDData"<<std::endl;
  m_data->CStore.Get("InProgressLAPPDEvents",LAPPDPulses);
  m_data->CStore.Get("InProgressLAPPDPPS",LAPPDPPS);
  m_data->CStore.Get("InProgressLAPPDTimestamps",LAPPDTimestamps);
  m_data->CStore.Get("InProgressLAPPDBeamgate",LAPPDBeamgateTimestamps);

  for (int i_pps=0; i_pps < (int) LAPPDPPS->size(); i_pps++){
    myTimeStream.LAPPDPPSTimestamps.push_back(LAPPDPPS->at(i_pps));
  }
  RemoveDuplicates(myTimeStream.LAPPDPPSTimestamps);

  for (int i_beam=0; i_beam < (int) LAPPDBeamgateTimestamps->size(); i_beam++){
    myTimeStream.LAPPDBeamgateTimestamps.push_back(LAPPDBeamgateTimestamps->at(i_beam));
  }
  RemoveDuplicates(myTimeStream.LAPPDBeamgateTimestamps);

  for (int i_data=0; i_data < (int) LAPPDTimestamps->size(); i_data++){
    myTimeStream.LAPPDTimestamps.push_back(LAPPDTimestamps->at(i_data));
    LAPPDPsecMap.emplace(LAPPDTimestamps->at(i_data),LAPPDPulses->at(i_data));
  }
  RemoveDuplicates(myTimeStream.LAPPDTimestamps);

  //Clear vectors after transferring data to timestreams
  LAPPDPPS->clear();
  LAPPDTimestamps->clear();
  LAPPDBeamgateTimestamps->clear();
  LAPPDPulses->clear();

}

void ANNIEEventBuilder::ProcessNewMRDData(){
  for(std::pair<uint64_t,std::vector<std::pair<unsigned long,int>>> apair : myMRDMaps.MRDEvents){
    uint64_t MRDTimeStamp = apair.first;
    myTimeStream.BeamMRDTimestamps.push_back(MRDTimeStamp);
    if(verbosity>5) std::cout << "MRDTIMESTAMPTRIGTYPE," << MRDTimeStamp << "," << myMRDMaps.MRDTriggerTypeMap.at(MRDTimeStamp) << std::endl;
  }
  RemoveDuplicates(myTimeStream.BeamMRDTimestamps);
  return;
}

void ANNIEEventBuilder::ProcessNewTankPMTData(){
  //Check if any In-progress tank events now have all waveforms
  if (save_raw_data) m_data->CStore.Get("InProgressTankEvents",InProgressTankEvents);
  else {
    m_data->CStore.Get("InProgressHits",InProgressHits);
    m_data->CStore.Get("InProgressChkey",InProgressChkey);
  }
  std::vector<uint64_t> InProgressTankEventsToDelete;
  std::map<uint64_t,std::string> TankOrphans;
  std::map<uint64_t,std::string> MRDOrphans;
  std::map<uint64_t,double> MRDOrphansTDiff;
  std::map<uint64_t,std::string> CTCOrphans;
  std::map<uint64_t,int> TankOrphansWaveMap;  
  std::map<uint64_t,std::vector<std::vector<int>>> TankOrphansChannels;  
  std::map<uint64_t,double> TankOrphansTDiff;

  if(verbosity>5) std::cout << "ANNIEEventBuilder Tool: Processing new tank data " << std::endl;

  /// -----------------------------------
  /// ----- Correct 8ns offset issue ----
  /// -----------------------------------
 
  bool force_all_entries = false;
  bool FileCompleted = false;
  m_data->CStore.Get("FileCompleted",FileCompleted);
  if (FileCompleted) force_all_entries = true;

  if (save_raw_data) this->CorrectVMEOffsetRaw(force_all_entries);
  else this->CorrectVMEOffset(force_all_entries);

  /// ------------------------------
  ///---------RAW DATA case ----------
  /// -------------------------------
  
  if (save_raw_data){
  if (verbosity > 4) std::cout <<"ANNIEEventBuilder Tool: Number of InProgressTankEvents: "<<InProgressTankEvents->size()<<std::endl;
  for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>>> apair : *InProgressTankEvents){
    uint64_t PMTCounterTimeNs = apair.first;
    std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = apair.second;
    if(verbosity>4) std::cout << "TS: " << PMTCounterTimeNs <<", Number of waves for this counter: " << aWaveMap.size() << std::endl;
    //std::cout <<"MaxObservedNumWaves: "<<MaxObservedNumWaves<<std::endl;

    if ((int) aWaveMap.size() > MaxObservedNumWaves) MaxObservedNumWaves = int(aWaveMap.size());
    //Check if maximum number of observed waveform size is systematically smaller than the set value
    if (InProgressTankEvents->size() > 200 && MaxObservedNumWaves < NumWavesInCompleteSet && !max_waves_adapted && MaxObservedNumWaves >=130) {
      Log("ANNIEEventBuilder tool: Did not observe any waveforms for the total of "+std::to_string(NumWavesInCompleteSet)+" channels so far. Reducing minimum value to observed maximum number of waveforms >>> "+std::to_string(MaxObservedNumWaves)+" <<<",v_error,verbosity);
      NumWavesInCompleteSet = MaxObservedNumWaves;
      max_waves_adapted = true;	//only adapt nominal number of waves per event once per part file (should not change)
    }

    //In case we find a larger number of waveforms at a later stage, update NumWavesInCompleteSet
    if (MaxObservedNumWaves > NumWavesInCompleteSet) NumWavesInCompleteSet = MaxObservedNumWaves;

    //Push back any new timestamps, then remove duplicates in the end
    if(PMTCounterTimeNs>NewestTankTimestamp){
      NewestTankTimestamp = PMTCounterTimeNs;
      if(verbosity>3)std::cout << "TANKTIMESTAMP," << PMTCounterTimeNs << std::endl;
    }
    if (aWaveMap.size() == (NumWavesInCompleteSet-1)){
      if (AlmostCompleteWaveforms.find(PMTCounterTimeNs)!=AlmostCompleteWaveforms.end()) AlmostCompleteWaveforms[PMTCounterTimeNs]++;
      else AlmostCompleteWaveforms.emplace(PMTCounterTimeNs,0);
      if (verbosity > 4) std::cout <<"ANNIEEventBuilder Tool: AlmostCompleteWaveforms for PMTCounterTimeNs: "<<PMTCounterTimeNs<<": "<<AlmostCompleteWaveforms.at(PMTCounterTimeNs)<<std::endl;
    }
    //Events and delete it from the in-progress events
    int NumTankPMTChannels = TankPMTCrateSpaceToChannelNumMap.size();
    int NumAuxChannels = AuxCrateSpaceToChannelNumMap.size();
    //std::cout <<"aWaveMap.size(): "<<aWaveMap.size()<<", NumWavesInCompleteSet: "<<NumWavesInCompleteSet<<std::endl;
    if(aWaveMap.size() >= (NumWavesInCompleteSet) || ((aWaveMap.size() == NumWavesInCompleteSet-1) && (AlmostCompleteWaveforms.at(PMTCounterTimeNs)>=5))){
      FinishedTankEvents->emplace(PMTCounterTimeNs,aWaveMap);
      std::map<std::vector<int>,int> aWaveMapSampleSize;
      for (std::pair<std::vector<int>,std::vector<uint16_t>> wavemappair : aWaveMap){
        std::vector<int> temp_channel = wavemappair.first;
        int temp_size = int(aWaveMap.size());
        aWaveMapSampleSize.emplace(temp_channel,temp_size);
      }    
      FinishedTankEventsSampleSize->emplace(PMTCounterTimeNs,aWaveMapSampleSize);
      if (save_raw_data) myTimeStream.BeamTankTimestamps.push_back(PMTCounterTimeNs);
      //myTimeStream.BeamTankTimestamps.push_back(PMTCounterTimeNs);
      //Put PMT timestamp into the timestamp set for this run.
      if(verbosity>4) std::cout << "Finished waveset has clock counter: " << PMTCounterTimeNs << std::endl;
      InProgressTankEventsToDelete.push_back(PMTCounterTimeNs);
    }

    if ((aWaveMap.size() == NumWavesInCompleteSet-1)){
      if (AlmostCompleteWaveforms.at(PMTCounterTimeNs)>=5) AlmostCompleteWaveforms.erase(PMTCounterTimeNs);
    }

    //If this InProgressTankEvent is too old, clear it
    //out from all TankTimestamp maps
    if(OrphanOldTankTimestamps && ((NewestTankTimestamp - PMTCounterTimeNs) > OldTimestampThreshold*1E9) && (aWaveMap.size() < NumWavesInCompleteSet-1) && (InProgressTankEvents->size() > 250)){
      InProgressTankEventsToDelete.push_back(PMTCounterTimeNs);
      TankOrphans.emplace(PMTCounterTimeNs,"incomplete_tank_event");
      TankOrphansWaveMap.emplace(PMTCounterTimeNs,aWaveMap.size());
      std::vector<std::vector<int>> aWaveMapChannels = GetChannelsFromWaveMap(aWaveMap);
      TankOrphansChannels.emplace(PMTCounterTimeNs,aWaveMapChannels);
      TankOrphansTDiff.emplace(PMTCounterTimeNs,0);
    }
  }
  RemoveDuplicates(myTimeStream.BeamTankTimestamps);
 
  std::cout <<"myTimeStream.BeamTankTimestamps.size(): "<<myTimeStream.BeamTankTimestamps.size()<<", FinishedTankEventsSampleSize size: "<<FinishedTankEventsSampleSize->size()<<std::endl;

  slowest_in_progress_tank = NewestTankTimestamp;
  for (std::pair<uint64_t,int> almost_complete_waveform : AlmostCompleteWaveforms){
    if (almost_complete_waveform.first < slowest_in_progress_tank) slowest_in_progress_tank = almost_complete_waveform.first;
  }

  // move abandoned in-progress events to the orphanage
  this->MoveToOrphanage(TankOrphans, TankOrphansWaveMap, TankOrphansChannels, TankOrphansTDiff, MRDOrphans, MRDOrphansTDiff, CTCOrphans);
 
  //Since timestamp pairing has been done for finished Tank Events,
  //Erase the finished Tank Events from the InProgressTankEventsMap
  if(verbosity>3)std::cout << "Now erasing finished timestamps from InProgressTankEvents" << std::endl;
  for (unsigned int j=0; j< InProgressTankEventsToDelete.size(); j++){
    InProgressTankEvents->erase(InProgressTankEventsToDelete.at(j));
    
  }
  if(verbosity>3) std::cout << "Current number of unfinished PMT Waveforms: " << InProgressTankEvents->size() << std::endl;
  } else {

  // --------------------------
  // ---Processed hits case---
  // --------------------------

  for (std::pair<uint64_t,std::map<unsigned long,std::vector<Hit>>*> apair : *InProgressHits){
    uint64_t PMTCounterTimeNs = apair.first; 
    std::map<unsigned long,std::vector<Hit>>* aHit = apair.second;
    std::vector<unsigned long> aChkey = InProgressChkey->at(PMTCounterTimeNs);
    if ((int) aChkey.size() > MaxObservedNumWaves) MaxObservedNumWaves = int(aChkey.size());
    if(verbosity>4) {
      std::cout << "TS: " << PMTCounterTimeNs <<", Number of waves for this counter: " << aChkey.size() << std::endl;
      std::cout <<"MaxObservedNumWaves: "<<MaxObservedNumWaves<<std::endl;
    }
    if (InProgressHits->size()>500 && MaxObservedNumWaves < NumWavesInCompleteSet && !max_waves_adapted && MaxObservedNumWaves >= 130){
      Log("ANNIEEventBuilder tool: Did not observe any waveforms for the total of "+std::to_string(NumWavesInCompleteSet)+" channels so far. Reducing minimum value to observed maximum number of waveforms >>> "+std::to_string(MaxObservedNumWaves)+" <<<",v_error,verbosity);
      NumWavesInCompleteSet = MaxObservedNumWaves;
      max_waves_adapted = true; //only adapt nominal number of waves per event once per part file (should not change)
    }
      
    //In case we find a larger number of waveforms at a later stage, update NumWavesInCompleteSet
    if (MaxObservedNumWaves > NumWavesInCompleteSet) NumWavesInCompleteSet = MaxObservedNumWaves;

    //Push back any new timestamps, then remove duplicates in the end
    if(PMTCounterTimeNs>NewestTankTimestamp){
      NewestTankTimestamp = PMTCounterTimeNs;
      if(verbosity>3)std::cout << "TANKTIMESTAMP," << PMTCounterTimeNs << std::endl;
    }
    if (aChkey.size() == (NumWavesInCompleteSet-1)){
      if (AlmostCompleteWaveforms.find(PMTCounterTimeNs)!=AlmostCompleteWaveforms.end()) AlmostCompleteWaveforms[PMTCounterTimeNs]++;
      else AlmostCompleteWaveforms.emplace(PMTCounterTimeNs,0);
      if (verbosity > 4) std::cout <<"ANNIEEventBuilder Tool: AlmostCompleteWaveforms for PMTCounterTimeNs: "<<PMTCounterTimeNs<<": "<<AlmostCompleteWaveforms.at(PMTCounterTimeNs)<<std::endl;
    }
    //Events and delete it from the in-progress events
    int NumTankPMTChannels = TankPMTCrateSpaceToChannelNumMap.size();
    int NumAuxChannels = AuxCrateSpaceToChannelNumMap.size();
    //std::cout <<"aChkey.size(): "<<aChkey.size()<<", NumWavesInCompleteSet: "<<NumWavesInCompleteSet<<std::endl;
    if(aChkey.size() >= (NumWavesInCompleteSet) || ((aChkey.size() == NumWavesInCompleteSet-1) && (AlmostCompleteWaveforms.at(PMTCounterTimeNs)>=5))){
      FinishedHits->emplace(PMTCounterTimeNs,aHit);
      std::map<std::vector<int>,int> aWaveMapSampleSize;
      for (int i_ch=0; i_ch < (int) aChkey.size(); i_ch++){
        unsigned long temp_chkey = aChkey.at(i_ch);
        std::vector<int> temp_channel;
        if (ChannelNumToTankPMTCrateSpaceMap.count(int(temp_chkey))>0) temp_channel = ChannelNumToTankPMTCrateSpaceMap[int(temp_chkey)];
        else if (AuxChannelNumToCrateSpaceMap.count(int(temp_chkey))>0) temp_channel = AuxChannelNumToCrateSpaceMap[int(temp_chkey)];
	else Log("ANNIEEventBuilder tool: Did not find channelkey "+std::to_string(temp_chkey)+" in ChannelNumToTankPMTCrateSpaceMap or AuxChannelNumToCrateSpaceMap! Can't convert to electronics space",v_error,verbosity);
        int temp_size = int(aChkey.size());
        int temp_cardid;
        this->ElectronicsSpacetoCardID(temp_channel.at(0),temp_channel.at(1),temp_cardid);
        std::vector<int> temp_cardch{temp_cardid,temp_channel.at(2)};
        aWaveMapSampleSize.emplace(temp_cardch,temp_size);
      }    
      FinishedTankEventsSampleSize->emplace(PMTCounterTimeNs,aWaveMapSampleSize);
      myTimeStream.BeamTankTimestamps.push_back(PMTCounterTimeNs);
      //Put PMT timestamp into the timestamp set for this run.
      if(verbosity>4) std::cout << "Finished waveset has clock counter: " << PMTCounterTimeNs << std::endl;
      InProgressTankEventsToDelete.push_back(PMTCounterTimeNs);
    }

    if ((aChkey.size() == NumWavesInCompleteSet-1)){
      if (AlmostCompleteWaveforms.at(PMTCounterTimeNs)>=5) AlmostCompleteWaveforms.erase(PMTCounterTimeNs);
    }

    //If this InProgressTankEvent is too old, clear it
    //out from all TankTimestamp maps
    if(OrphanOldTankTimestamps && ((NewestTankTimestamp - PMTCounterTimeNs) > OldTimestampThreshold*1E9) && (aChkey.size() < NumWavesInCompleteSet-1) && (InProgressHits->size() > 1200)){
      InProgressTankEventsToDelete.push_back(PMTCounterTimeNs);
      TankOrphans.emplace(PMTCounterTimeNs,"incomplete_tank_event");
      TankOrphansWaveMap.emplace(PMTCounterTimeNs,aChkey.size());
      std::vector<std::vector<int>> aWaveMapChannels = GetChannelsFromHitMap(aChkey);
      TankOrphansChannels.emplace(PMTCounterTimeNs,aWaveMapChannels);
      TankOrphansTDiff.emplace(PMTCounterTimeNs,0);
    }
  }
  RemoveDuplicates(myTimeStream.BeamTankTimestamps);
 
  //std::cout <<"myTimeStream.BeamTankTimestamps.size(): "<<myTimeStream.BeamTankTimestamps.size()<<", FinishedTankEventsSampleSize size: "<<FinishedTankEventsSampleSize->size()<<std::endl;

  slowest_in_progress_tank = NewestTankTimestamp;
  for (std::pair<uint64_t,int> almost_complete_waveform : AlmostCompleteWaveforms){
    if (almost_complete_waveform.first < slowest_in_progress_tank) slowest_in_progress_tank = almost_complete_waveform.first;
  }

  // move abandoned in-progress events to the orphanage
  this->MoveToOrphanage(TankOrphans, TankOrphansWaveMap, TankOrphansChannels, TankOrphansTDiff, MRDOrphans, MRDOrphansTDiff, CTCOrphans);
 
  //Since timestamp pairing has been done for finished Tank Hits,
  //Erase the finished Tank Events from the InProgressHitsMap
  if(verbosity>3)std::cout << "Now erasing finished timestamps from InProgressHits" << std::endl;
  //std::cout <<"InProgressChkey size: "<<InProgressChkey->size()<<", InProgressHits size: "<<InProgressHits->size()<<std::endl;
  for (unsigned int j=0; j< InProgressTankEventsToDelete.size(); j++){
    //std::cout <<"Delete InProgressHits & InProgressChkey for TS "<<InProgressTankEventsToDelete.at(j)<<std::endl;
    InProgressHits->erase(InProgressTankEventsToDelete.at(j));
    InProgressChkey->erase(InProgressTankEventsToDelete.at(j));
  }
  //std::cout <<"After erase: InProgressChkey size: "<<InProgressChkey->size()<<", InProgressHits size: "<<InProgressHits->size()<<std::endl;
  if(verbosity>3) std::cout << "Current number of unfinished PMT Waveforms: " << InProgressHits->size() << std::endl;

  }// End of processing InProgressHits
  
  /*
  if (!save_raw_data){
    m_data->CStore.Set("FinishedTankEvents",FinishedTankEvents);
    m_data->CStore.Set("NewTankEvents",true);
  }*/

  return;
}

std::map<uint64_t,std::map<std::string,uint64_t>> ANNIEEventBuilder::PairCTCCosmicPairs(std::map<uint64_t,std::map<std::string,uint64_t>> BuildMap, uint64_t max_timestamp, bool force_matching){
  uint32_t CosmicWord = 36;  //TS_in(35) + 1, where 35 is the MRD_CR_Trigger
  std::vector<uint64_t> MRDCosmicTimes;
  std::vector<uint64_t> MRDStampsToDelete;
  std::map<uint64_t,uint64_t> PairedCTCMRDTimes;
  std::map<uint64_t,std::string> MRDOrphans;
  std::map<uint64_t,double> MRDOrphansTDiff;
  std::map<uint64_t,std::string> TankOrphans;
  std::map<uint64_t,std::string> CTCOrphans;
  std::map<uint64_t,int> TankOrphansWaveMap;
  std::map<uint64_t,std::vector<std::vector<int>>> TankOrphansChannels;
  std::map<uint64_t,double> TankOrphansTDiff;


  for(auto&& aTS : myTimeStream.BeamMRDTimestamps) {
    if((aTS>max_timestamp)&&(!force_matching)) break; // don't try to match yet
    std::string MRDTriggerType = myMRDMaps.MRDTriggerTypeMap[aTS];
    if(MRDTriggerType == "Cosmic"){
      MRDCosmicTimes.push_back(aTS);
    }
  }
 
  //Now, loop through CTC timestamps and pair any cosmic triggers
  //With Cosmic candidates in time tolerance

  //First, pair up myTimeStream.CTCTimestamps and MRD/PMT timestamps
  std::map<uint64_t,uint64_t> PairedCTCCosmicTimes;

  //Form CTC-MRD pairs
  if(verbosity>3) std::cout << "Finding CTC-MRD pairs..." << std::endl;
  for(auto&& aMrdTS : MRDCosmicTimes){
    double TSDiff_current=0;
    for(auto&& aCtcTS : myTimeStream.CTCTimestamps){
      if((aCtcTS>max_timestamp)&&(!force_matching)) continue;        // do not try to match just yet
      if(TimeToTriggerWordMap->at(aCtcTS).at(0) != CosmicWord) continue;   // not a cosmic CTC
      double TSDiff =  static_cast<double>(aMrdTS) - 
                       static_cast<double>(aCtcTS);
      if(verbosity>4) std::cout << "CosmicTS - CTCTS in nanoseconds is " << TSDiff << std::endl;
      if(TSDiff>CTCMRDTimeTolerance){ // MRD timestamp is later than this CTC one. Check next CTC timestamp.
        TSDiff_current=TSDiff;
        continue;
      } else if (TSDiff<(-1.*static_cast<double>(CTCMRDTimeTolerance))){ // We've crossed past where a pair would be found
        if(verbosity>4) std::cout << "NO CTC STAMP FOUND MATCHING COSMIC STAMP... MRD TO ORPHANAGE" << std::endl;
        MRDOrphans.emplace(aMrdTS,"mrd_cosmic_no_ctc");
        double min_tdiff = (fabs(TSDiff)<fabs(TSDiff_current))? TSDiff : TSDiff_current;
        MRDOrphansTDiff.emplace(aMrdTS,min_tdiff);
        break;
      } else { //We've found a valid CTC-MRD pair!
        if(verbosity>4) std::cout << "FOUND A MATCHING CTC TIME FOR THIS COSMIC MRD TIMESTAMP. NICE, PAIR EM." << std::endl;
        PairedCTCMRDTimes.emplace(aCtcTS, aMrdTS);
        break;
      }
    }
  }

  //Neat.  Now, add timestamps to the buildmap.
  std::vector<uint64_t> BuiltCTCs;
  for(std::pair<uint64_t, uint64_t> aPair : PairedCTCMRDTimes){
    uint64_t CTCTimestamp = aPair.first;
    uint64_t CosmicTimestamp = aPair.second;
    std::map<std::string,uint64_t> aBuildSet;
    aBuildSet.emplace("CTC",TimeToTriggerWordMap->at(CTCTimestamp).at(0));
    aBuildSet.emplace("MRD",CosmicTimestamp);
    myTimeStream.BeamMRDTimestamps.erase(std::remove(myTimeStream.BeamMRDTimestamps.begin(),
         myTimeStream.BeamMRDTimestamps.end(),CosmicTimestamp), 
         myTimeStream.BeamMRDTimestamps.end());
    if(verbosity>4) std::cout << "BUILDING A CTC/COSMIC /BeaBUILD MAP ENTRY. CTC IS " << CTCTimestamp << std::endl;
    BuildMap.emplace(CTCTimestamp,aBuildSet);
    BuiltCTCs.push_back(CTCTimestamp);
  }
  

  //Delete CTCTimestamps that have a PMT or MRD pair from CTC timestamp tracker
  for(int j=0;j < (int) BuiltCTCs.size();j++){
    if(verbosity>4) std::cout << "REMOVING CTC TIME OF " << BuiltCTCs.at(j) << "FROM CTCTIMESTAMPS VECTOR" << std::endl;
    myTimeStream.CTCTimestamps.erase(std::remove(myTimeStream.CTCTimestamps.begin(),
        myTimeStream.CTCTimestamps.end(),BuiltCTCs.at(j)), 
        myTimeStream.CTCTimestamps.end());
  }

  //Move MRD timestamps with no pairs to the orphanage.  Just empty Tank and CTC vectors for 
  //Input to function.  TODO; could overload function
  this->MoveToOrphanage(TankOrphans, TankOrphansWaveMap, TankOrphansChannels, TankOrphansTDiff, MRDOrphans, MRDOrphansTDiff, CTCOrphans);
  return BuildMap;
}

void ANNIEEventBuilder::RemoveCosmics(){
  std::vector<uint64_t> MRDStampsToDelete;
  for (int i=0;i < (int) (myTimeStream.BeamMRDTimestamps.size()); i++) {
    std::string MRDTriggerType = myMRDMaps.MRDTriggerTypeMap[myTimeStream.BeamMRDTimestamps.at(i)];
    if(verbosity>4) std::cout << "THIS MRD TRIGGER TYPE IS: " << MRDTriggerType << std::endl;
    if(MRDTriggerType == "Cosmic"){
      MRDStampsToDelete.push_back(myTimeStream.BeamMRDTimestamps.at(i));
      if(verbosity>3){
        std::cout << "NO BEAM COSMIC COSMIC IS: " << myTimeStream.BeamMRDTimestamps.at(i) << std::endl;
        std::cout << "DELETING FROM TIMESTAMPS TO PAIR/BUILD" << std::endl;
      }
    }
  }
  for (int j=0; j < (int) MRDStampsToDelete.size(); j++){
    myTimeStream.BeamMRDTimestamps.erase(std::remove(myTimeStream.BeamMRDTimestamps.begin(),
        myTimeStream.BeamMRDTimestamps.end(),MRDStampsToDelete.at(j)), 
        myTimeStream.BeamMRDTimestamps.end());
    myMRDMaps.MRDEvents.erase(MRDStampsToDelete.at(j));
    myMRDMaps.MRDTriggerTypeMap.erase(MRDStampsToDelete.at(j));
    myMRDMaps.MRDBeamLoopbackMap.erase(MRDStampsToDelete.at(j));
    myMRDMaps.MRDCosmicLoopbackMap.erase(MRDStampsToDelete.at(j));
  }
  return;
}


std::map<uint64_t,std::map<std::string,uint64_t>> ANNIEEventBuilder::MergeStreams(std::map<uint64_t,std::map<std::string,uint64_t>> BuildMap, uint64_t max_timestamp, bool force_matching){
  //This method takes timestamps from the BeamMRDTimestamps, BeamTankTimestamps, and
  //CTCTimestamps vectors (acquired as the building continues) and builds maps 
  //stored in the BuildMap and used to build ANNIEEvents.
 
  //First, pair up CTCTimestamps and MRD/PMT timestamps
  std::map<uint64_t,uint64_t> PairedCTCTankTimes; 
  std::map<uint64_t,uint64_t> PairedCTCMRDTimes;
  std::map<uint64_t,uint64_t> PairedCTCLAPPDTimes;
  std::map<uint64_t,std::string> MRDOrphans;
  std::map<uint64_t,double> MRDOrphansTDiff;
  std::map<uint64_t,std::string> TankOrphans;
  std::map<uint64_t,std::string> CTCOrphans;
  std::map<uint64_t,std::string> LAPPDOrphans;
  std::map<uint64_t,int> TankOrphansWaveMap;
  std::map<uint64_t,std::vector<std::vector<int>>> TankOrphansChannels;
  std::map<uint64_t,double> TankOrphansTDiff;
  std::map<uint64_t,double> LAPPDOrphansTDiff;

  if (force_matching && (BuildType == "Tank" || BuildType == "TankAndMRD" || BuildType == "TankAndCTC" || BuildType == "TankAndMRDAndCTC" || BuildType == "TankAndMRDAndCTCAndLAPPD")){
    std::vector<uint64_t> InProgressTankEventsToDelete;
    //Add remaining Tank timestamps that have almost complete waveforms
    if (save_raw_data){
    for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>>> apair : *InProgressTankEvents){
      uint64_t PMTCounterTimeNs = apair.first;
      std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = apair.second;
      if(verbosity>4) std::cout << "TS: " << PMTCounterTimeNs <<", Number of waves for this counter: " << aWaveMap.size() << std::endl;
   
      //Push back any new timestamps, then remove duplicates in the end
      if(PMTCounterTimeNs>NewestTankTimestamp){
        NewestTankTimestamp = PMTCounterTimeNs;
        if(verbosity>3)std::cout << "TANKTIMESTAMP," << PMTCounterTimeNs << std::endl;
      }
      if (aWaveMap.size() >= (NumWavesInCompleteSet-1)){
        FinishedTankEvents->emplace(PMTCounterTimeNs,aWaveMap);
        std::map<std::vector<int>,int> aWaveMapSampleSize;
        for (std::pair<std::vector<int>,std::vector<uint16_t>> wavemappair : aWaveMap){
          std::vector<int> temp_channel = wavemappair.first;
          int temp_size = int(aWaveMap.size());
          aWaveMapSampleSize.emplace(temp_channel,temp_size);
        } 
        FinishedTankEventsSampleSize->emplace(PMTCounterTimeNs,aWaveMapSampleSize);
        myTimeStream.BeamTankTimestamps.push_back(PMTCounterTimeNs);
        //Put PMT timestamp into the timestamp set for this run.
        if(verbosity>4) std::cout << "Finished waveset has clock counter: " << PMTCounterTimeNs << std::endl;
        InProgressTankEventsToDelete.push_back(PMTCounterTimeNs);
      }
    }
    RemoveDuplicates(myTimeStream.BeamTankTimestamps);

    //Since timestamp pairing has been done for finished Tank Events,
    //Erase the finished Tank Events from the InProgressTankEventsMap
    if(verbosity>3)std::cout << "Now erasing finished timestamps from InProgressTankEvents" << std::endl;
    for (unsigned int j=0; j< InProgressTankEventsToDelete.size(); j++){
      InProgressTankEvents->erase(InProgressTankEventsToDelete.at(j));
    }
  } else { //Processed data case
    for (std::pair<uint64_t,std::map<unsigned long,std::vector<Hit>>*> apair : *InProgressHits){
    uint64_t PMTCounterTimeNs = apair.first;
    std::map<unsigned long,std::vector<Hit>>* aHit = apair.second;
    std::vector<unsigned long> aChkey = InProgressChkey->at(PMTCounterTimeNs);
 
    if(PMTCounterTimeNs>NewestTankTimestamp){
      NewestTankTimestamp = PMTCounterTimeNs;
      if(verbosity>3)std::cout << "TANKTIMESTAMP," << PMTCounterTimeNs << std::endl;
    }

    int NumTankPMTChannels = TankPMTCrateSpaceToChannelNumMap.size();
    int NumAuxChannels = AuxCrateSpaceToChannelNumMap.size();
    if(aChkey.size() >= (NumWavesInCompleteSet-1)){
      FinishedHits->emplace(PMTCounterTimeNs,aHit);
      std::map<std::vector<int>,int> aWaveMapSampleSize;
        for (int i_ch=0; i_ch < (int) aChkey.size(); i_ch++){
          unsigned long temp_chkey = aChkey.at(i_ch);
          std::vector<int> temp_channel;
          if (ChannelNumToTankPMTCrateSpaceMap.count(int(temp_chkey))>0) temp_channel = ChannelNumToTankPMTCrateSpaceMap[int(temp_chkey)];
          else if (AuxChannelNumToCrateSpaceMap.count(int(temp_chkey))>0) temp_channel = AuxChannelNumToCrateSpaceMap[int(temp_chkey)];
          else Log("ANNIEEventBuilder tool: Did not find channelkey "+std::to_string(temp_chkey)+" in ChannelNumToTankPMTCrateSpaceMap or AuxChannelNumToCrateSpaceMap! Can't convert to electronics space",v_error,verbosity);
          int temp_size = int(aChkey.size());
          int temp_cardid;
          this->ElectronicsSpacetoCardID(temp_channel.at(0),temp_channel.at(1),temp_cardid);
          std::vector<int> temp_cardch{temp_cardid,temp_channel.at(2)};
          aWaveMapSampleSize.emplace(temp_cardch,temp_size);
        }
        FinishedTankEventsSampleSize->emplace(PMTCounterTimeNs,aWaveMapSampleSize);
        myTimeStream.BeamTankTimestamps.push_back(PMTCounterTimeNs);
        if(verbosity>4) std::cout << "Finished waveset has clock counter: " << PMTCounterTimeNs << std::endl;
        InProgressTankEventsToDelete.push_back(PMTCounterTimeNs);
      }
    }
    RemoveDuplicates(myTimeStream.BeamTankTimestamps);

    for (unsigned int j=0; j< InProgressTankEventsToDelete.size(); j++){
      InProgressHits->erase(InProgressTankEventsToDelete.at(j));
      InProgressChkey->erase(InProgressTankEventsToDelete.at(j));
    }
  }
  }

  // only attempt matching of any kind on timestamps older than the newest timestamp we have
  // from ALL streams - i.e. if the slowest stream has only read up to 4pm, do not try to do
  // matching on any timestamps newer than this
  //Form CTC-MRD pairs
  uint64_t max_ctc=0;
  if(verbosity>3) std::cout << "Finding CTC-MRD pairs..." << std::endl;
  for(auto&& aMrdTS : myTimeStream.BeamMRDTimestamps){
    if((aMrdTS>max_timestamp)&&(!force_matching)) continue;   // don't try to match just yet
    double TSDiff_current=0;
    for(auto&& aCtcTS : myTimeStream.CTCTimestamps){
      if((aCtcTS>max_timestamp)&&(!force_matching)) continue; // don't try to match this just yet
      double TSDiff =  static_cast<double>(aMrdTS) - 
                       static_cast<double>(aCtcTS);
      if(verbosity>9) std::cout << "MRDTS - CTCTS in nanoseconds is " << TSDiff << std::endl;
      if(TSDiff>CTCMRDTimeTolerance){ // MRD timestamp is later than CTC timestamp; try next CTC timestamp
        TSDiff_current = TSDiff;
        continue;
      } else if (TSDiff<(-1.*static_cast<double>(CTCMRDTimeTolerance))){ //We've crossed past where a pair would be found
        if(verbosity>9) std::cout << "NO CTC STAMP FOUND MATCHING MRD STAMP... MRD TO ORPHANAGE" << std::endl;
        MRDOrphans.emplace(aMrdTS,"mrd_beam_no_ctc");
        double min_tdiff = (fabs(TSDiff)<fabs(TSDiff_current))? TSDiff:TSDiff_current;
        MRDOrphansTDiff.emplace(aMrdTS,min_tdiff);
        break;
      } else { //We've found a valid CTC-MRD pair!
        if(verbosity>4) std::cout << "FOUND A MATCHING CTC TIME FOR THIS MRD TIMESTAMP. NICE, PAIR EM." << std::endl;
        PairedCTCMRDTimes.emplace(aCtcTS, aMrdTS);
        max_ctc=aCtcTS;
        break;
      }
    }
  }

  //Now form CTC-PMT pairs
  if(verbosity>3) std::cout << "Finding CTC-Tank pairs..." << std::endl;
  for(auto&& aTankTS : myTimeStream.BeamTankTimestamps){
    if((aTankTS>max_timestamp)&&(!force_matching)) continue; // don't try to match just yet
    double TSDiff_current=0;
    for(auto&& aCtcTS : myTimeStream.CTCTimestamps){
      if((aCtcTS>max_timestamp)&&(!force_matching)) continue; // don't try to match just yet
      double TSDiff =  static_cast<double>(aTankTS) - 
                       static_cast<double>(aCtcTS);
      if(verbosity>9) std::cout << "TankTS - CTCTS in nanoseconds is " << TSDiff << std::endl;
      if(TSDiff>CTCTankTimeTolerance){ // Need to move forward in indices
        TSDiff_current = TSDiff;
        continue;
      } else if (TSDiff<(-1.*static_cast<double>(CTCTankTimeTolerance))){ //We've crossed past where a pair would be found
        if(verbosity>9) {
          std::cout << "NO CTC STAMP FOUND MATCHING TANK STAMP... TANK TO ORPHANAGE" << std::endl;
          std::cout <<"TSDiff: "<<TSDiff<<std::endl;
        }
        TankOrphans.emplace(aTankTS,"tank_no_ctc");
        TankOrphansWaveMap.emplace(aTankTS,NumWavesInCompleteSet);
        //std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = FinishedTankEvents->at(aTankTS);
        std::map<std::vector<int>, int> aWaveMapSampleSize = FinishedTankEventsSampleSize->at(aTankTS);
        std::vector<std::vector<int>> aWaveMapChannels = GetChannelsFromWaveMapSampleSize(aWaveMapSampleSize);
        TankOrphansChannels.emplace(aTankTS,aWaveMapChannels);
	double min_tdiff = (fabs(TSDiff_current) < fabs(TSDiff))? TSDiff_current : TSDiff;
	TankOrphansTDiff.emplace(aTankTS,min_tdiff);
        break;
      } else { //We've found a valid CTC-Tank pair!
        if(verbosity>4) std::cout << "FOUND A MATCHING CTC TIME FOR THIS TANK TIMESTAMP. NICE, PAIR EM." << std::endl;
        PairedCTCTankTimes.emplace(aCtcTS, aTankTS);
        if(aCtcTS>max_ctc) max_ctc=aCtcTS;
        break;
      }
    }
  }

  //Now match CTC-LAPPD timestamps
  if (BuildType == "TankAndMRDAndCTCAndLAPPD"){
    if(verbosity>3) std::cout << "Finding CTC-LAPPD pairs..." << std::endl;
    for(auto&& aLAPPDTS : myTimeStream.LAPPDGlobalTimestamps){
      if((aLAPPDTS>max_timestamp)&&(!force_matching)) continue;   // don't try to match just yet
      double TSDiff_current=0;
      for(auto&& aCtcTS : myTimeStream.CTCTimestamps){
        if((aCtcTS>max_timestamp)&&(!force_matching)) continue; // don't try to match this just yet
        double TSDiff =  static_cast<double>(aLAPPDTS) -
                       static_cast<double>(aCtcTS);
        if(verbosity>9) std::cout << "LAPPDTS - CTCTS in nanoseconds is " << TSDiff << std::endl;
        if(TSDiff>CTCLAPPDTimeTolerance){ // MRD timestamp is later than CTC timestamp; try next CTC timestamp
          TSDiff_current = TSDiff;
          continue;
        } else if (TSDiff<(-1.*static_cast<double>(CTCLAPPDTimeTolerance))){ //We've crossed past where a pair would be found    
          if(verbosity>9) std::cout << "NO CTC STAMP FOUND MATCHING LAPPD STAMP... LAPPD TO ORPHANAGE" << std::endl;
          LAPPDOrphans.emplace(aLAPPDTS,"lappd_beam_no_ctc");
          double min_tdiff = (fabs(TSDiff)<fabs(TSDiff_current))? TSDiff:TSDiff_current;
          LAPPDOrphansTDiff.emplace(aLAPPDTS,min_tdiff);
          break; 
        } else { //We've found a valid CTC-LAPPD pair!
          if(verbosity>4) std::cout << "FOUND A MATCHING CTC TIME FOR THIS LAPPD TIMESTAMP. NICE, PAIR EM." << std::endl;
          PairedCTCLAPPDTimes.emplace(aCtcTS, aLAPPDTS);
          max_ctc=aCtcTS;
          break;
        }
      }
    }
  }

  int LargestCTCIndex = std::distance(myTimeStream.CTCTimestamps.begin(),std::find(myTimeStream.CTCTimestamps.begin(),myTimeStream.CTCTimestamps.end(),max_ctc));
  if(max_ctc==0) LargestCTCIndex=0;
  if(verbosity>4) std::cout << "LARGEST CTC INDEX WAS " << LargestCTCIndex << std::endl;
  if(verbosity>4) std::cout << "AND CTCTIMESTAMPS SIZE IS " << myTimeStream.CTCTimestamps.size() << std::endl;
  
  //Neat.  Now, add timestamps to the buildmap.
  //FIXME: Right now, this will only build PMT/MRD/CTC or PMT/CTC pairs.  
  // Any logic needed for pairing CTC/MRD pairs?  I don't think so, since cosmics 
  // handled in a different method..
  std::vector<uint64_t> BuiltCTCs;
  for(int i=0; i<(LargestCTCIndex-1); i++){
    uint64_t CTCKey = myTimeStream.CTCTimestamps.at(i);
    if(verbosity>4) std::cout << "TRYING TO BUILD A SET WITH CTCTIMESTAMP INDEX " << i << std::endl;

    bool have_tankmatch = false;
    bool have_mrdmatch = false;    
    bool have_lappdmatch = false;
    std::map<uint64_t, uint64_t>::iterator it_tank = PairedCTCTankTimes.find(CTCKey);
    if(it_tank != PairedCTCTankTimes.end()) have_tankmatch = true;
    std::map<uint64_t, uint64_t>::iterator it_mrd = PairedCTCMRDTimes.find(CTCKey);
    if(it_mrd != PairedCTCMRDTimes.end()) have_mrdmatch = true;
    std::map<uint64_t, uint64_t>::iterator it_lappd = PairedCTCLAPPDTimes.find(CTCKey);
    if(it_lappd != PairedCTCLAPPDTimes.end()) have_lappdmatch = true;
    if (have_tankmatch && have_mrdmatch && have_lappdmatch){
      std::map<std::string,uint64_t> aBuildSet;
      for (int i_ctckey=0; i_ctckey < (int) TimeToTriggerWordMap->at(CTCKey).size(); i_ctckey++){
        if (TimeToTriggerWordMap->at(CTCKey).size() > 1){
          Log("ANNIEEventBuilding tool: Error! Multiple triggerwords for the same timestamp. Timestamp = "+std::to_string(CTCKey),v_error,verbosity);
        } else if (TimeToTriggerWordMap->at(CTCKey).size() == 0){
          Log("ANNIEEventBuilding tool: Error! No triggerwords available for timestamp. Timestamp = "+std::to_string(CTCKey),v_error,verbosity);
        }
        aBuildSet.emplace("CTC",TimeToTriggerWordMap->at(CTCKey).at(0));
      }
      aBuildSet.emplace("TankPMT",it_tank->second);
      aBuildSet.emplace("MRD",it_mrd->second);
      aBuildSet.emplace("LAPPD",it_lappd->second);
      myTimeStream.BeamTankTimestamps.erase(std::remove(myTimeStream.BeamTankTimestamps.begin(),
           myTimeStream.BeamTankTimestamps.end(),it_tank->second),
           myTimeStream.BeamTankTimestamps.end());
      myTimeStream.BeamMRDTimestamps.erase(std::remove(myTimeStream.BeamMRDTimestamps.begin(),
           myTimeStream.BeamMRDTimestamps.end(),it_mrd->second),
           myTimeStream.BeamMRDTimestamps.end());
      myTimeStream.LAPPDGlobalTimestamps.erase(std::remove(myTimeStream.LAPPDGlobalTimestamps.begin(),
           myTimeStream.LAPPDGlobalTimestamps.end(), it_lappd->second),
           myTimeStream.LAPPDGlobalTimestamps.end());
      if(verbosity>4) std::cout << "BUILDING A BUILD MAP (PMT+MRD+LAPPD) ENTRY. CTC IS " << CTCKey << std::endl;
      BuildMap.emplace(CTCKey,aBuildSet);
      BuiltCTCs.push_back(CTCKey);
    }
    if(have_tankmatch && have_mrdmatch){
      std::map<std::string,uint64_t> aBuildSet;
      for (int i_ctckey=0; i_ctckey < (int) TimeToTriggerWordMap->at(CTCKey).size(); i_ctckey++){
        if (TimeToTriggerWordMap->at(CTCKey).size() > 1){
          Log("ANNIEEventBuilding tool: Error! Multiple triggerwords for the same timestamp. Timestamp = "+std::to_string(CTCKey),v_error,verbosity);
        } else if (TimeToTriggerWordMap->at(CTCKey).size() == 0){
          Log("ANNIEEventBuilding tool: Error! No triggerwords available for timestamp. Timestamp = "+std::to_string(CTCKey),v_error,verbosity);
        }
        aBuildSet.emplace("CTC",TimeToTriggerWordMap->at(CTCKey).at(0));
      }
      aBuildSet.emplace("TankPMT",it_tank->second);
      aBuildSet.emplace("MRD",it_mrd->second);
      myTimeStream.BeamTankTimestamps.erase(std::remove(myTimeStream.BeamTankTimestamps.begin(),
           myTimeStream.BeamTankTimestamps.end(),it_tank->second), 
           myTimeStream.BeamTankTimestamps.end());
      myTimeStream.BeamMRDTimestamps.erase(std::remove(myTimeStream.BeamMRDTimestamps.begin(),
           myTimeStream.BeamMRDTimestamps.end(),it_mrd->second), 
           myTimeStream.BeamMRDTimestamps.end());
      if(verbosity>4) std::cout << "BUILDING A BUILD MAP ENTRY. CTC IS " << CTCKey << std::endl;
      BuildMap.emplace(CTCKey,aBuildSet);
      BuiltCTCs.push_back(CTCKey);
    } else if (have_tankmatch && !have_mrdmatch){
      std::map<std::string,uint64_t> aBuildSet;
      for (int i_ctckey=0; i_ctckey < (int) TimeToTriggerWordMap->at(CTCKey).size(); i_ctckey++){
        if (TimeToTriggerWordMap->at(CTCKey).size() > 1){
          Log("ANNIEEventBuilding tool: Error! Multiple triggerwords for the same timestamp. Timestamp = "+std::to_string(CTCKey),v_error,verbosity);
        } else if (TimeToTriggerWordMap->at(CTCKey).size() == 0){
          Log("ANNIEEventBuilding tool: Error! No triggerwords available for timestamp. Timestamp = "+std::to_string(CTCKey),v_error,verbosity);
        }
        aBuildSet.emplace("CTC",TimeToTriggerWordMap->at(CTCKey).at(0));
      }
      aBuildSet.emplace("TankPMT",it_tank->second);
      myTimeStream.BeamTankTimestamps.erase(std::remove(myTimeStream.BeamTankTimestamps.begin(),
           myTimeStream.BeamTankTimestamps.end(),it_tank->second), 
           myTimeStream.BeamTankTimestamps.end());
      if(verbosity>4) std::cout << "BUILDING A PMT ONLY BUILD MAP ENTRY. CTC IS " << CTCKey << std::endl;
      BuildMap.emplace(CTCKey,aBuildSet);
      BuiltCTCs.push_back(CTCKey);
    } else if (!have_tankmatch && have_mrdmatch){
      std::map<std::string,uint64_t> aBuildSet;
      for (int i_ctckey=0; i_ctckey < (int) TimeToTriggerWordMap->at(CTCKey).size(); i_ctckey++){
        if (TimeToTriggerWordMap->at(CTCKey).size() > 1){
          Log("ANNIEEventBuilding tool: Error! Multiple triggerwords for the same timestamp. Timestamp = "+std::to_string(CTCKey),v_error,verbosity);
        } else if (TimeToTriggerWordMap->at(CTCKey).size() == 0){
          Log("ANNIEEventBuilding tool: Error! No triggerwords available for timestamp. Timestamp = "+std::to_string(CTCKey),v_error,verbosity);
        }
        aBuildSet.emplace("CTC",TimeToTriggerWordMap->at(CTCKey).at(0));
      }
      aBuildSet.emplace("MRD",it_mrd->second);
      myTimeStream.BeamMRDTimestamps.erase(std::remove(myTimeStream.BeamMRDTimestamps.begin(),
             myTimeStream.BeamMRDTimestamps.end(),it_mrd->second),                             
             myTimeStream.BeamMRDTimestamps.end());
      if (verbosity > 4) std::cout << "BUILDING A MRD ONLY BUILD MAP ENTRY. CTC IS " << CTCKey << std::endl;   
      BuildMap.emplace(CTCKey,aBuildSet);
      BuiltCTCs.push_back(CTCKey);
   } else if (!have_tankmatch && !have_mrdmatch){
      if(verbosity>4) std::cout << "NO MRD OR TANK TIMESTAMP FOR THIS CTC TIME... ORPHAN THE CTC" << std::endl;
      //uint64_t latest_tank_orphan = TankOrphans.at(TankOrphans.size()-1);
      //uint64_t latest_MRD_orphan = MRDOrphans.at(MRDOrphans.size()-1);
      //if(CTCKey < latest_tank_orphan && CTCKey < latest_MRD_orphan){
      CTCOrphans.emplace(CTCKey,"ctc_no_mrd_or_tank");
      //}
    } else {
      //This case should not happen
      // beam ctc and mrd match, but no tank
      // at the least do we need to orphan the CTC and MRD events?
      //CTCOrphans.emplace(CTCKey,"ctc_mrd_no_tank");
      //MRDOrphans.emplace(it_mrd->second,"ctc_mrd_no_tank");
      //if(verbosity>4) std::cout << "CTC TIMESTAMP " << CTCKey << " PAIRS WITH EITHER A PMT OR MRD..."  << std::endl;
      Log("ANNIEEventBuilder tool: Something went wrong with merging: have_tankmatch: "+std::to_string(have_tankmatch)+", "+std::to_string(have_mrdmatch),v_error,verbosity);
    }
  }

  //Delete myTimeStream.CTCTimestamps that have a PMT or MRD pair from CTC timestamp tracker
  for(int j=0;j < (int) BuiltCTCs.size();j++){
    if(verbosity>4) std::cout << "REMOVING CTC TIME OF " << BuiltCTCs.at(j) << "FROM CTCTIMESTAMPS VECTOR" << std::endl;
    myTimeStream.CTCTimestamps.erase(std::remove(myTimeStream.CTCTimestamps.begin(),
        myTimeStream.CTCTimestamps.end(),BuiltCTCs.at(j)), 
        myTimeStream.CTCTimestamps.end());
  }

  //If the toolchain is stopping, move remaining incomplete PMT timestamps to orphanage
  if (force_matching) {
    if (verbosity > 2) std::cout <<"ANNIEEventBuilder Tool: Force matching at the end of toolchain, get InProgressTankEvents"<<std::endl;
    if(OrphanOldTankTimestamps && (BuildType == "Tank" || BuildType == "TankAndMRD" || BuildType == "TankAndCTC" || BuildType == "TankAndMRDAndCTC" || BuildType == "TankAndMRDAndCTCAndLAPPD")){
      std::vector<uint64_t> InProgressTankEventsToDelete;
      if (save_raw_data){
      if (verbosity > 2) std::cout <<"ANNIEEventBuilder Tool: Size of InprogressTankEvents: "<<InProgressTankEvents->size()<<std::endl;
      for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>>> apair : *InProgressTankEvents){
        uint64_t PMTCounterTimeNs = apair.first;
        if (verbosity > 4) std::cout <<"ANNIEEventBuilder Tool: PMTCounterTimeNs of InProgressTankEvent: "<<PMTCounterTimeNs<<std::endl;
        std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = apair.second;
        if(aWaveMap.size() < (NumWavesInCompleteSet)){
          InProgressTankEventsToDelete.push_back(PMTCounterTimeNs);
          TankOrphans.emplace(PMTCounterTimeNs,"incomplete_tank_event");
          TankOrphansWaveMap.emplace(PMTCounterTimeNs,aWaveMap.size());
          std::vector<std::vector<int>> aWaveMapChannels = GetChannelsFromWaveMap(aWaveMap);
          TankOrphansChannels.emplace(PMTCounterTimeNs,aWaveMapChannels);
          TankOrphansTDiff.emplace(PMTCounterTimeNs,0.);
        }
      }
      for (int i_del=0; i_del < (int) InProgressTankEventsToDelete.size(); i_del++){
        InProgressTankEvents->erase(InProgressTankEventsToDelete.at(i_del));
      }
    } else {
      if (verbosity > 2) std::cout <<"ANNIEEventBuilder Tool: Size of InprogressHits: "<<InProgressHits->size()<<std::endl;
      for (std::pair<uint64_t, std::map<unsigned long, std::vector<Hit>>*> apair : *InProgressHits){
        //std::cout <<"Get PMTCounterTimeNs"<<std::endl;
        uint64_t PMTCounterTimeNs = apair.first;
        if (verbosity > 4) std::cout <<"ANNIEEventBuilder Tool: PMTCounterTimeNs of InProgressHits: "<<PMTCounterTimeNs<<std::endl;
	//std::cout <<"Get aHitMap"<<std::endl;
        std::map<unsigned long, std::vector<Hit>>* aHitMap = apair.second;
        //std::cout <<"Get achkey"<<std::endl;
        std::vector<unsigned long> aChkey = InProgressChkey->at(PMTCounterTimeNs);
        //std::cout <<"Check if size < NumWavesInCompleteSet"<<std::endl;
        if(aChkey.size() < (NumWavesInCompleteSet)){
          //std::cout <<"yes"<<std::endl;
          InProgressTankEventsToDelete.push_back(PMTCounterTimeNs);
          TankOrphans.emplace(PMTCounterTimeNs,"incomplete_tank_event");
          TankOrphansWaveMap.emplace(PMTCounterTimeNs,aChkey.size());
          std::vector<std::vector<int>> aWaveMapChannels = GetChannelsFromHitMap(aChkey);
          TankOrphansChannels.emplace(PMTCounterTimeNs,aWaveMapChannels);
          TankOrphansTDiff.emplace(PMTCounterTimeNs,0.);
        }
      }
      for (int i_del=0; i_del < (int) InProgressTankEventsToDelete.size(); i_del++){
        //std::cout <<"Delete "<<i_del<<" / "<<InProgressTankEventsToDelete.size()<<" timestamps"<<std::endl;
        InProgressHits->erase(InProgressTankEventsToDelete.at(i_del));
        InProgressChkey->erase(InProgressTankEventsToDelete.at(i_del));
      }
    }
   }
  }

  //std::cout <<"MoveToOrphanage"<<std::endl;
  //Move timestamps with no pairs to the orphanage
  this->MoveToOrphanage(TankOrphans, TankOrphansWaveMap, TankOrphansChannels, TankOrphansTDiff, MRDOrphans, MRDOrphansTDiff, CTCOrphans);
  if (BuildType == "TankAndMRDAndCTCAndLAPPD") this->MoveToOrphanageLAPPD(LAPPDOrphans,LAPPDOrphansTDiff);
  //std::cout <<"ManageOprhanae"<<std::endl;
  this->ManageOrphanage();

  Log("ANNIEEventBuilder: Returning from Merging the Streams",v_debug,verbosity);

  return BuildMap;
}

void ANNIEEventBuilder::MoveToOrphanageLAPPD(std::map<uint64_t, std::string> LAPPDOrphans,
                                             std::map<uint64_t, double> LAPPDOrphansTDiff){

  if (verbosity > 3) std::cout <<" Moving LAPPD TIMESTAMPS WITH NO FAMILY TO ORPHANAGE" << std::endl;
  for (auto&& nextorphan : LAPPDOrphans){
    uint64_t LAPPDOrphanStamp = nextorphan.first;

    //build map of orphan info to save to OrphanStore
    std::map<std::string,std::string> orphaninfo;
    orphaninfo.emplace("reason",nextorphan.second);

    // move to orphanage
    myOrphanage.OrphanLAPPDTimestamps.emplace(LAPPDOrphanStamp,orphaninfo);

    // remove the orphan from the timestream
    myTimeStream.LAPPDGlobalTimestamps.erase(std::remove(myTimeStream.LAPPDGlobalTimestamps.begin(),
         myTimeStream.LAPPDGlobalTimestamps.end(),LAPPDOrphanStamp),
         myTimeStream.LAPPDGlobalTimestamps.end());
  }

}

void ANNIEEventBuilder::MoveToOrphanage(std::map<uint64_t, std::string> TankOrphans,
					std::map<uint64_t, int> TankOrphansWaveMap,
                                        std::map<uint64_t, std::vector<std::vector<int>>> TankOrphansChannels,
					std::map<uint64_t, double> TankOrphansTDiff,
                                        std::map<uint64_t, std::string> MRDOrphans,
                                        std::map<uint64_t, double> MRDOrphansTDiff,
                                        std::map<uint64_t, std::string> CTCOrphans){
  if(verbosity>3) std::cout << "MOVING TIMESTAMPS WITH NO FAMILY TO ORPHANAGE" << std::endl;
  //Finally, we need to move data associated with our orphaned timestamps to the orphange
  for(auto&& nextorphan : CTCOrphans){
    uint64_t CTCOrphanStamp = nextorphan.first;
    
    // build map of orphan info to save to OrphanStore
    std::map<std::string,std::string> orphaninfo;
    orphaninfo.emplace("reason",nextorphan.second);
    orphaninfo.emplace("ctc_word",to_string(TimeToTriggerWordMap->at(CTCOrphanStamp).at(0)));
    
    // move to orphanage
    myOrphanage.OrphanCTCTimestamps.emplace(CTCOrphanStamp,orphaninfo);
    
    // remove the orphan from the timestream
    myTimeStream.CTCTimestamps.erase(std::remove(myTimeStream.CTCTimestamps.begin(),
         myTimeStream.CTCTimestamps.end(),CTCOrphanStamp), 
         myTimeStream.CTCTimestamps.end());
  }
  
  for(auto&& nextorphan : TankOrphans){
    uint64_t TankOrphanStamp = nextorphan.first;
    int num_waveforms = TankOrphansWaveMap[TankOrphanStamp];    
    std::vector<std::vector<int>> TankOrphansChannelsEntry = TankOrphansChannels[TankOrphanStamp];
    double TankOrphansTDiffEntry = TankOrphansTDiff[TankOrphanStamp];

    // build map of orphan info to save to OrphanStore
    std::map<std::string,std::string> orphaninfo;
    orphaninfo.emplace("reason",nextorphan.second);
    orphaninfo.emplace("numwaves",std::to_string(num_waveforms));    

    // move to orphanage
    myOrphanage.OrphanTankTimestamps.emplace(TankOrphanStamp,orphaninfo);
    myOrphanage.OrphanTankTimestampsChannels.emplace(TankOrphanStamp,TankOrphansChannelsEntry);
    myOrphanage.OrphanTankTimestampsTDiff.emplace(TankOrphanStamp,TankOrphansTDiffEntry);    

    // remove the orphan from the timestream
    myTimeStream.BeamTankTimestamps.erase(std::remove(myTimeStream.BeamTankTimestamps.begin(),
               myTimeStream.BeamTankTimestamps.end(),TankOrphanStamp), 
               myTimeStream.BeamTankTimestamps.end());
  }
  //std::cout <<"Move to orphanage: BeamTankTimestamps.size(): "<<myTimeStream.BeamTankTimestamps.size()<<std::endl;

  for(auto&& nextorphan : MRDOrphans){
    uint64_t MrdOrphanStamp = nextorphan.first;
    
    // build map of orphan info to save to OrphanStore
    std::map<std::string,std::string> orphaninfo;
    orphaninfo.emplace("reason",nextorphan.second);
   
    double MRDOrphansTDiffEntry = MRDOrphansTDiff[MrdOrphanStamp];
 
    // move to orphanage
    myOrphanage.OrphanMRDTimestamps.emplace(MrdOrphanStamp,orphaninfo);
    myOrphanage.OrphanMRDTimestampsTDiff.emplace(MrdOrphanStamp,MRDOrphansTDiffEntry);    

    // remove the orphan from the timestream
    myTimeStream.BeamMRDTimestamps.erase(std::remove(myTimeStream.BeamMRDTimestamps.begin(),
               myTimeStream.BeamMRDTimestamps.end(),MrdOrphanStamp), 
               myTimeStream.BeamMRDTimestamps.end());
  }
  if(verbosity>3) std::cout << "ORPHAN MOVEMENT COMPLETE" << std::endl;
  return;
}

void ANNIEEventBuilder::ManageOrphanage(){
  
  // TODO
  // try to find incomplete tank orphans to match ctc/mrd orphans
  // build a fixed type that says what data was present:
  // MRD? CTC? Tank? Right now without merging it'll be just one,
  // but later if we're able to merge orphans it could be more than one
  // std::cout<<"Going to manage the orphanage"<<std::endl;
  
  std::string OrphanFile = SavePath + OrphanFileBase +"_"+BuildType+"_R" + to_string(CurrentRunNum) + 
      "S" + to_string(CurrentSubRunNum) + "p" + to_string(CurrentPartNum);
  
  //For now, just save orphans then delete them
  
  for(auto&& nextorphan : myOrphanage.OrphanTankTimestamps){
    // copy the orphan info into OrphanStore
    OrphanStore->Set("EventType",std::string("Tank"));
    OrphanStore->Set("Timestamp",nextorphan.first);
    OrphanStore->Set("Reason",nextorphan.second.at("reason"));
    OrphanStore->Set("NumWaves",std::stoi(nextorphan.second.at("numwaves")));
    OrphanStore->Set("TriggerWord",-1);
    OrphanStore->Set("Info",nextorphan.second); // redundant for now, but maybe we'll add more info later
    std::vector<std::vector<int>> CurrentWaveMapChannels = myOrphanage.OrphanTankTimestampsChannels[nextorphan.first];
    OrphanStore->Set("WaveformChannels",CurrentWaveMapChannels);
    //Convert to channelkeys for convenience
    std::vector<unsigned long> CurrentWaveMapChankeys;
    for (int i_channel = 0; i_channel < (int) CurrentWaveMapChannels.size(); i_channel++){
      std::vector<int> current_cratespace = CurrentWaveMapChannels.at(i_channel);
      unsigned long current_chankey = 0;
      if (TankPMTCrateSpaceToChannelNumMap.count(current_cratespace)>0){
        current_chankey = TankPMTCrateSpaceToChannelNumMap[current_cratespace];
      } else if (AuxCrateSpaceToChannelNumMap.count(current_cratespace)>0){
        current_chankey  = AuxCrateSpaceToChannelNumMap.at(current_cratespace);
      } else {
        Log("ANNIEEventBuilder: Encountered invalid crate space during orphan movement. Setting chankey = 99999999",v_error,verbosity);
        current_chankey = 99999999;
        Log("ANNIEEventBuilder::CrateNum "+to_string(current_cratespace.at(0)),v_error, verbosity);
        Log("ANNIEEventBuilder::SlotNum "+to_string(current_cratespace.at(1)),v_error, verbosity);
        Log("ANNIEEventBuilder::ChannelID "+to_string(current_cratespace.at(2)),v_error, verbosity);   
      }
      CurrentWaveMapChankeys.push_back(current_chankey);
    }
    OrphanStore->Set("WaveformChankeys",CurrentWaveMapChankeys);
    double orphan_min_tdiff = myOrphanage.OrphanTankTimestampsTDiff[nextorphan.first];
    OrphanStore->Set("MinTDiff",orphan_min_tdiff);
    OrphanStore->Save(OrphanFile);
    OrphanStore->Delete();
    
    // cleanup from events to process
    if (std::find(myTimeStream.BeamTankTimestamps.begin(),myTimeStream.BeamTankTimestamps.end(),nextorphan.first)==myTimeStream.BeamTankTimestamps.end()) {
    if (FinishedTankEventsSampleSize->count(nextorphan.first)>0) FinishedTankEventsSampleSize->erase(nextorphan.first);
    if (!(FinishedTankEvents->count(nextorphan.first)>0)) std::cout <<"no nextorphan.first in FinishedTankEvents"<<std::endl;
    if (save_raw_data && FinishedTankEvents->count(nextorphan.first)>0) {
	FinishedTankEvents->erase(nextorphan.first);
    }
    else if (FinishedHits->count(nextorphan.first)>0){
        FinishedHits->erase(nextorphan.first);
        FinishedHitsAux->erase(nextorphan.first);
        FinishedRecoADCHits->erase(nextorphan.first);
        FinishedRecoADCHitsAux->erase(nextorphan.first);
        FinishedRawAcqSize->erase(nextorphan.first);
    }
    }    
    m_data->CStore.Set("FinishedTankEvents",FinishedTankEvents);

  }
  //std::cout<<"managing mrd orphans"<<std::endl;
  for(auto&& nextorphan : myOrphanage.OrphanMRDTimestamps){
    // copy the orphan info into OrphanStore
    OrphanStore->Set("EventType",std::string("MRD"));
    OrphanStore->Set("Timestamp",nextorphan.first);
    OrphanStore->Set("Reason",nextorphan.second.at("reason"));
    OrphanStore->Set("NumWaves",0);
    OrphanStore->Set("TriggerWord",-1);
    OrphanStore->Set("Info",nextorphan.second); // redundant for now, but maybe we'll add more info later
    std::vector<std::vector<int>> empty_vector;
    OrphanStore->Set("WaveformChannels",empty_vector);
    std::vector<unsigned long> empty_unsigned;
    OrphanStore->Set("WaveformChankeys",empty_unsigned);
    double min_tdiff = myOrphanage.OrphanMRDTimestampsTDiff[nextorphan.first];
    OrphanStore->Set("MinTDiff",min_tdiff);
    OrphanStore->Save(OrphanFile);
    OrphanStore->Delete();
    
    // cleanup from events to process
    myMRDMaps.MRDEvents.erase(nextorphan.first);
    myMRDMaps.MRDTriggerTypeMap.erase(nextorphan.first);
    myMRDMaps.MRDBeamLoopbackMap.erase(nextorphan.first);
    myMRDMaps.MRDCosmicLoopbackMap.erase(nextorphan.first);
  }
 // std::cout<<"Managing CTC orphans"<<std::endl;
  for(auto&& nextorphan : myOrphanage.OrphanCTCTimestamps){
    // copy the orphan info into OrphanStore
    OrphanStore->Set("EventType",std::string("CTC"));
    OrphanStore->Set("Timestamp",nextorphan.first);
    OrphanStore->Set("Reason",nextorphan.second.at("reason"));
    OrphanStore->Set("NumWaves",0);
    OrphanStore->Set("TriggerWord",std::stoi(nextorphan.second.at("ctc_word")));
    OrphanStore->Set("Info",nextorphan.second); // CTC word
    std::vector<std::vector<int>> empty_vector;
    OrphanStore->Set("WaveFormChannels",empty_vector);
    std::vector<unsigned long> empty_unsigned;
    OrphanStore->Set("WaveformChankeys",empty_unsigned);
    OrphanStore->Set("MinTDiff",0.);
    OrphanStore->Save(OrphanFile);
    OrphanStore->Delete();
    
    // cleanup from events to process
    TimeToTriggerWordMap->erase(nextorphan.first);
    CTCExtended.erase(nextorphan.first);
    if (store_beam_status){
      if (BeamStatusMap->count(nextorphan.first)>0) BeamStatusMap->erase(nextorphan.first);
    }
  }
 // std::cout<<"all CTC orphans handled, clearing the orphan timestamps"<<std::endl;
  
  //Managing LAPPD orphans
  for(auto&& nextorphan : myOrphanage.OrphanLAPPDTimestamps){
    // copy the orphan info into OrphanStore
    OrphanStore->Set("EventType",std::string("LAPPD"));
    OrphanStore->Set("Timestamp",nextorphan.first);
    OrphanStore->Set("Reason",nextorphan.second.at("reason"));
    OrphanStore->Set("NumWaves",0);
    OrphanStore->Set("TriggerWord",-1);
    OrphanStore->Set("Info",nextorphan.second); // CTC word
    std::vector<std::vector<int>> empty_vector;
    OrphanStore->Set("WaveFormChannels",empty_vector);
    std::vector<unsigned long> empty_unsigned;
    OrphanStore->Set("WaveformChankeys",empty_unsigned);
    OrphanStore->Set("MinTDiff",0.);
    OrphanStore->Save(OrphanFile);
    OrphanStore->Delete();

    // cleanup from events to process
    FinishedLAPPDPsecData->erase(nextorphan.first);
  }

  myOrphanage.OrphanTankTimestamps.clear();
  myOrphanage.OrphanTankTimestampsChannels.clear();
  myOrphanage.OrphanTankTimestampsTDiff.clear();
  myOrphanage.OrphanMRDTimestamps.clear();
  myOrphanage.OrphanMRDTimestampsTDiff.clear();
  myOrphanage.OrphanCTCTimestamps.clear();
  myOrphanage.OrphanLAPPDTimestamps.clear();
  myOrphanage.OrphanLAPPDTimestampsTDiff.clear();
//  std::cout<<"timestamps cleared, returning"<<std::endl;
  return;
}

std::map<uint64_t,uint64_t> ANNIEEventBuilder::PairTankPMTAndMRDTriggers(){
  if(verbosity>4) std::cout << "ANNIEEventBuilder Tool: Beginning to pair events" << std::endl;

  std::map<uint64_t,uint64_t> TankMRDTimePairs; //Pairs of beam-triggered Tank PMT/MRD counters ready to be built if all PMT waveforms are ready (TankAndMRD mode only)
  std::vector<uint64_t> TankStampsToDelete;
  std::vector<uint64_t> MRDStampsToDelete;
  int NumPairsToMake = EventsPerPairing;
  std::vector<double> ThisPairingTSDiffs;
  
  std::map<uint64_t,std::string> MRDOrphans;
  std::map<uint64_t,double> MRDOrphansTDiff;
  std::map<uint64_t,std::string> TankOrphans;
  std::map<uint64_t,std::string> CTCOrphans;
  std::map<uint64_t,int> TankOrphansWaveMap;
  std::map<uint64_t,std::vector<std::vector<int>>> TankOrphansChannels;
  std::map<uint64_t,double> TankOrphansTDiff;

  int NumOrphans = 0;

  //Pair PMT and MRD timestamps and calculate how much PMTTime - MRDTime has drifted 
  if(verbosity>4) std::cout << "MEAN OF PMT-MRD TIME DIFFERENCE LAST LOOP: " << CurrentDriftMean << std::endl;
  for (int i=0;i<NumPairsToMake; i++) {
    double TSDiff = static_cast<double>(myTimeStream.BeamTankTimestamps.at(i)) - 
                    static_cast<double>(myTimeStream.BeamMRDTimestamps.at(i));
    if(verbosity>4){
      std::cout << "PAIRED TANK TIMESTAMP: " << myTimeStream.BeamTankTimestamps.at(i) << std::endl;
      std::cout << "PAIRED MRD TIMESTAMP: " << myTimeStream.BeamMRDTimestamps.at(i) << std::endl;
      std::cout << "DIFFERENCE BETWEEN PMT AND MRD TIMESTAMP (ns): " << 
      (static_cast<double>(myTimeStream.BeamTankTimestamps.at(i)) - static_cast<double>(myTimeStream.BeamMRDTimestamps.at(i))) << std::endl;
    }
    if(std::abs(TSDiff-CurrentDriftMean) > MRDTankTimeTolerance){ // PMT/MRD timestamps farther apart than set tolerance
      if(verbosity>3) std::cout << "DEVIATION OF " << MRDTankTimeTolerance << " ms DETECTED IN STREAMS" << std::endl;
      if(TSDiff > 0) {
        if(verbosity>3) std::cout << "MOVING MRD TIMESTAMP TO ORPHANAGE" << std::endl;
        MRDOrphans.emplace(myTimeStream.BeamMRDTimestamps.at(i),"mrd_beam_no_tank");
        MRDOrphansTDiff.emplace(myTimeStream.BeamMRDTimestamps.at(i),TSDiff);
        NumOrphans +=1;
        myTimeStream.BeamMRDTimestamps.erase(std::remove(myTimeStream.BeamMRDTimestamps.begin(),
            myTimeStream.BeamMRDTimestamps.end(),myTimeStream.BeamMRDTimestamps.at(i)), 
            myTimeStream.BeamMRDTimestamps.end());
      } else {
        if(verbosity>3) std::cout << "MOVING TANK TIMESTAMP TO ORPHANAGE" << std::endl;
        TankOrphans.emplace(myTimeStream.BeamTankTimestamps.at(i),"tank_no_mrd");
        TankOrphansWaveMap.emplace(myTimeStream.BeamTankTimestamps.at(i),NumWavesInCompleteSet);
        std::map<std::vector<int>, int> aWaveMapSampleSize = FinishedTankEventsSampleSize->at(myTimeStream.BeamTankTimestamps.at(i));
        std::vector<std::vector<int>> aWaveMapChannels = GetChannelsFromWaveMapSampleSize(aWaveMapSampleSize);
        TankOrphansChannels.emplace(myTimeStream.BeamTankTimestamps.at(i),aWaveMapChannels);
        TankOrphansTDiff.emplace(myTimeStream.BeamTankTimestamps.at(i),TSDiff);
        NumOrphans +=1;
        myTimeStream.BeamTankTimestamps.erase(std::remove(myTimeStream.BeamTankTimestamps.begin(),
            myTimeStream.BeamTankTimestamps.end(),myTimeStream.BeamTankTimestamps.at(i)), 
            myTimeStream.BeamTankTimestamps.end());
      }
      i-=1;
      NumPairsToMake-=1;
    } else {  //PMT/MRD timestamp times within tolerance; Leave stamps in stream to be paired
      ThisPairingTSDiffs.push_back(TSDiff);
    }
  }
  this->MoveToOrphanage(TankOrphans, TankOrphansWaveMap, TankOrphansChannels, TankOrphansTDiff, MRDOrphans, MRDOrphansTDiff, CTCOrphans);

  //With the last set of pairs calculate what the mean drift is
  //TODO: Error handling for high drift and high orphan rates?
  double ThisPairingMean, ThisPairingVariance;
  if(ThisPairingTSDiffs.size()>2){
    ComputeMeanAndVariance(ThisPairingTSDiffs,ThisPairingMean,ThisPairingVariance);
    if((std::abs(CurrentDriftMean-ThisPairingMean)>DriftWarningValue) && (verbosity>=v_warning)){
      std::cout << "ANNIEEventBuilder tool: WARNING! Shift in drift greater than " << DriftWarningValue << " since last pairings." << std::endl;
    }
    if((NumOrphans > OrphanWarningValue) && (verbosity>=v_warning)){
      std::cout << "ANNIEEventBuilder tool: WARNING! High orphan rate detected.  More than " << OrphanWarningValue << " this pairing sequence." << std::endl;
    }
    CurrentDriftMean = ThisPairingMean;
    CurrentDriftVariance = ThisPairingVariance;
  }
  for (int i=0;i<(NumPairsToMake); i++) {
    TankMRDTimePairs.emplace(myTimeStream.BeamTankTimestamps.at(i),myTimeStream.BeamMRDTimestamps.at(i));
    TankStampsToDelete.push_back(myTimeStream.BeamTankTimestamps.at(i));
    MRDStampsToDelete.push_back(myTimeStream.BeamMRDTimestamps.at(i));
  }

  if(verbosity>4) std::cout << "DELETE PAIRED TIMESTAMPS FROM THE TIMESTAMP STREAMS " << std::endl;
  //Delete all paired timestamps still in the unpaired unbuilt timestamps vectors 
  for (int i=0; i<(NumPairsToMake); i++){
    myTimeStream.BeamTankTimestamps.erase(std::remove(myTimeStream.BeamTankTimestamps.begin(),
        myTimeStream.BeamTankTimestamps.end(),TankStampsToDelete.at(i)), 
        myTimeStream.BeamTankTimestamps.end());
    myTimeStream.BeamMRDTimestamps.erase(std::remove(myTimeStream.BeamMRDTimestamps.begin(),
               myTimeStream.BeamMRDTimestamps.end(),MRDStampsToDelete.at(i)), 
               myTimeStream.BeamMRDTimestamps.end());
  }
  return TankMRDTimePairs;
}


void ANNIEEventBuilder::BuildANNIEEventMRD(std::vector<std::pair<unsigned long,int>> MRDHits, 
        uint64_t MRDTimeStamp, std::string MRDTriggerType, int beam_tdc, int cosmic_tdc)
{
  std::cout << "Building an ANNIE Event (MRD), ANNIEEventNum = "<<ANNIEEventNum << std::endl;
  TDCData = new std::map<unsigned long, std::vector<Hit>>;

  for (unsigned int i_value=0; i_value< MRDHits.size(); i_value++){
    unsigned long channelkey = MRDHits.at(i_value).first;
    int hitTimeADC = MRDHits.at(i_value).second;
    double hitTime = 4000. - 4.*(double)hitTimeADC;
    if (verbosity > 4) std::cout <<"creating hit with TDC value "<<hitTimeADC<<"and chankey "<<channelkey<<std::endl;
    if (verbosity > 3) std::cout <<"creating hit with time vlaue "<<hitTime<<"and chankey "<<channelkey<<std::endl;
    if (TDCData->count(channelkey)==0){
      std::vector<Hit> newhitvector;
      newhitvector.push_back(Hit(0,hitTime,1.));    //Hit(tubeid, time, charge). 1 TDC tick corresponds to 4ns, no charge information (set to 1)
      TDCData->emplace(channelkey,newhitvector);
    } else {
      TDCData->at(channelkey).push_back(Hit(0,hitTime,1.));
    }
  }

  std::map<std::string, int> mrd_loopback_tdc;
  mrd_loopback_tdc.emplace("BeamLoopbackTDC",beam_tdc);
  mrd_loopback_tdc.emplace("CosmicLoopbackTDC",cosmic_tdc);

  Log("ANNIEEventBuilder: TDCData size: "+std::to_string(TDCData->size()),v_debug,verbosity);

  ANNIEEvent->Set("TDCData",TDCData,true);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("TDCData",TDCData,true);
  TimeClass timeclass_timestamp(MRDTimeStamp);
  ANNIEEvent->Set("EventTimeMRD",timeclass_timestamp);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("EventTimeMRD",timeclass_timestamp);
  ANNIEEvent->Set("MRDTriggerType",MRDTriggerType);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("MRDTriggerType",MRDTriggerType);
  ANNIEEvent->Set("MRDLoopbackTDC",mrd_loopback_tdc);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("MRDLoopbackTDC",mrd_loopback_tdc);
  return;
}

void ANNIEEventBuilder::BuildANNIEEventLAPPD(PsecData psecdata, uint64_t LAPPDTimeStamp, uint64_t LAPPDOffset){

  Log("ANNIEEventBuilder: Build LAPPD event with timestamp "+std::to_string(LAPPDTimeStamp)+" and offset "+std::to_string(LAPPDOffset),0,verbosity);
  
  ANNIEEvent->Set("LAPPDData",psecdata);
  ANNIEEvent->Set("EventTimeLAPPD",LAPPDTimeStamp);
  ANNIEEvent->Set("LAPPDOffset",LAPPDOffset);

}

void ANNIEEventBuilder::BuildANNIEEventRunInfo(int RunNumber, int SubRunNumber, int PartNumber,
        int RunType, uint64_t StartTime)
{
  if(verbosity>v_message)std::cout << "Building an ANNIE Event Run Info" << std::endl;
  ANNIEEvent->Set("EventNumber",ANNIEEventNum);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("EventNumber",ANNIEEventNum);
  ANNIEEvent->Set("RunNumber",RunNumber);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("RunNumber",RunNumber);
  ANNIEEvent->Set("SubrunNumber",SubRunNumber);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("SubrunNumber",SubRunNumber);
  ANNIEEvent->Set("PartNumber",PartNumber);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("PartNumber",PartNumber);
  ANNIEEvent->Set("RunType",RunType);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("RunType",RunType);
  ANNIEEvent->Set("RunStartTime",StartTime);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("RunStartTime",StartTime);
  return;
}

void ANNIEEventBuilder::BuildANNIEEventCTC(uint64_t CTCTime, uint32_t CTCWord, int CTCWordExtended)
{
  if(verbosity>v_message)std::cout << "Building an ANNIE Event CTC Info" << std::endl;
  ANNIEEvent->Set("CTCTimestamp",CTCTime);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("CTCTimestamp",CTCTime);
  ANNIEEvent->Set("TriggerWord",CTCWord);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("TriggerWord",CTCWord);
  ANNIEEvent->Set("TriggerExtended",CTCWordExtended);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("TriggerExtended",CTCWordExtended);
  //Build a TriggerClass object to be more in line with the ANNIEEvent spreadsheet
  TimeClass TriggerTime(CTCTime);
  std::string TriggerName = "";
  if (CTCWord == 5) TriggerName = "Beam";
  else if (CTCWord == 31) TriggerName = "LED";
  else if (CTCWord == 35) TriggerName = "AmBe";
  else if (CTCWord == 36) TriggerName = "MRDCR";
  TriggerClass TriggerData(TriggerName,CTCWord,CTCWordExtended,true,TriggerTime);
  ANNIEEvent->Set("TriggerData",TriggerData);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("TriggerData",TriggerData);
  if (verbosity > 2) std::cout <<"Done setting ANNIE Event CTC Info"<<std::endl;
  return;
}

void ANNIEEventBuilder::BuildANNIEEventTankRaw(uint64_t ClockTime, 
        std::map<std::vector<int>, std::vector<uint16_t>> WaveMap)
{
  if(verbosity>v_message)std::cout << "Building an ANNIE Event Tank (RAW)" << std::endl;

  ///////////////LOAD RAW PMT DATA INTO ANNIEEVENT///////////////
  std::map<unsigned long, std::vector<Waveform<uint16_t>> > RawADCData;
  std::map<unsigned long, std::vector<Waveform<uint16_t>> > RawADCAuxData;
  for(std::pair<std::vector<int>, std::vector<uint16_t>> apair : WaveMap){
    int CardID = apair.first.at(0);
    int ChannelID = apair.first.at(1);
    int CrateNum=-1;
    int SlotNum=-1;
    this->CardIDToElectronicsSpace(CardID, CrateNum, SlotNum);
    std::vector<uint16_t> TheWaveform = apair.second;
    Waveform<uint16_t> TheWave(ClockTime, TheWaveform);
    //Placing waveform in a vector in case we want a hefty-mode minibuffer storage eventually
    std::vector<Waveform<uint16_t>> WaveVec{TheWave};
    
    std::vector<int> CrateSpace{CrateNum,SlotNum,ChannelID};
    unsigned long ChannelKey;
    if(TankPMTCrateSpaceToChannelNumMap.count(CrateSpace)>0){
      ChannelKey = TankPMTCrateSpaceToChannelNumMap.at(CrateSpace);
      RawADCData.emplace(ChannelKey,WaveVec);
    }
    else if (AuxCrateSpaceToChannelNumMap.count(CrateSpace)>0){
      ChannelKey = AuxCrateSpaceToChannelNumMap.at(CrateSpace);
      RawADCAuxData.emplace(ChannelKey,WaveVec);
    } else{
      Log("ANNIEEventBuilder:: Cannot find channel key for crate space entry: ",v_error, verbosity);
      Log("ANNIEEventBuilder::CrateNum "+to_string(CrateNum),v_error, verbosity);
      Log("ANNIEEventBuilder::SlotNum "+to_string(SlotNum),v_error, verbosity);
      Log("ANNIEEventBuilder::ChannelID "+to_string(ChannelID),v_error, verbosity);
      Log("ANNIEEventBuilder:: Passing over the wave; PMT DATA LOST",v_error, verbosity);
      continue;
    }
  }
  if(RawADCData.size() == 0){
    std::cout << "No Raw ADC Data in entry.  Not putting to ANNIEEvent." << std::endl;
  }
  //std::cout << "Setting ANNIE Event information" << std::endl;
  ANNIEEvent->Set("RawADCData",RawADCData);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("RawADCData",RawADCData);
  ANNIEEvent->Set("RawADCAuxData",RawADCAuxData);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("RawADCAuxData",RawADCAuxData);
  ANNIEEvent->Set("EventTimeTank",ClockTime);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("EventTimeTank",ClockTime);
  if(verbosity>v_debug) std::cout << "ANNIEEventBuilder: ANNIE Event "+
      to_string(ANNIEEventNum)+" built." << std::endl;
  return;
}

void ANNIEEventBuilder::BuildANNIEEventTankHits(uint64_t ClockTime, 
        std::map<unsigned long,std::vector<Hit>>* PMTHits,
        std::map<unsigned long,std::vector<std::vector<ADCPulse>>> PMTRecoADCHits,
        std::map<unsigned long,std::vector<Hit>>* PMTHitsAux,
        std::map<unsigned long,std::vector<std::vector<ADCPulse>>> PMTRecoADCHitsAux,
        std::map<unsigned long,std::vector<int>> PMTRawAcqSize)
{
  if(verbosity>v_message)std::cout << "Building an ANNIE Event Tank (Hits)" << std::endl;

  //std::cout << "Setting ANNIE Event information" << std::endl;
  ANNIEEvent->Set("Hits",PMTHits, true);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("Hits",PMTHits,true);
  ANNIEEvent->Set("RecoADCData",PMTRecoADCHits);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("RecoADCData",PMTRecoADCHits);
  ANNIEEvent->Set("AuxHits",PMTHitsAux,true);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("AuxHits",PMTHitsAux,true);
  ANNIEEvent->Set("RecoAuxADCData",PMTRecoADCHitsAux);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("RecoAuxADCData",PMTRecoADCHitsAux);
  ANNIEEvent->Set("RawAcqSize",PMTRawAcqSize);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("RawAcqSize",PMTRawAcqSize);
  ANNIEEvent->Set("EventTimeTank",ClockTime);
  if (BuildStage1Data) m_data->Stores["ANNIEEvent"]->Set("EventTimeTank",ClockTime);
  if(verbosity>v_debug) std::cout << "ANNIEEventBuilder: ANNIE Event "+
      to_string(ANNIEEventNum)+" built." << std::endl;
  return;
}

std::vector<std::vector<int>> ANNIEEventBuilder::GetChannelsFromWaveMapSampleSize(std::map<std::vector<int>,int> WaveMap){

    std::vector<std::vector<int>> CrateSpaceVector;
      for(std::pair<std::vector<int>, int> apair : WaveMap){
      int CardID = apair.first.at(0);
      int ChannelID = apair.first.at(1);
      int CrateNum=-1;
      int SlotNum=-1;
      this->CardIDToElectronicsSpace(CardID, CrateNum, SlotNum);
      std::vector<int> CrateSpace{CrateNum,SlotNum,ChannelID};
      CrateSpaceVector.push_back(CrateSpace);
    }

    return CrateSpaceVector;
}

std::vector<std::vector<int>> ANNIEEventBuilder::GetChannelsFromWaveMap(std::map<std::vector<int>,std::vector<uint16_t>> WaveMap){

    std::vector<std::vector<int>> CrateSpaceVector;
      for(std::pair<std::vector<int>, std::vector<uint16_t>> apair : WaveMap){
      int CardID = apair.first.at(0);
      int ChannelID = apair.first.at(1);
      int CrateNum=-1;
      int SlotNum=-1;
      this->CardIDToElectronicsSpace(CardID, CrateNum, SlotNum);
      std::vector<int> CrateSpace{CrateNum,SlotNum,ChannelID};
      CrateSpaceVector.push_back(CrateSpace);
    }

    return CrateSpaceVector;
}

std::vector<std::vector<int>> ANNIEEventBuilder::GetChannelsFromHitMap(std::vector<unsigned long> HitMap){

   std::vector<std::vector<int>> CrateSpaceVector;
   for (int i_vec=0; i_vec < (int) HitMap.size(); i_vec++){
     unsigned long temp_chkey = HitMap.at(i_vec);
     std::vector<int> temp_channel;
     if (ChannelNumToTankPMTCrateSpaceMap.count(int(temp_chkey))>0) temp_channel = ChannelNumToTankPMTCrateSpaceMap[int(temp_chkey)];
     else if (AuxChannelNumToCrateSpaceMap.count(int(temp_chkey))>0) temp_channel = AuxChannelNumToCrateSpaceMap[int(temp_chkey)];
     else Log("ANNIEEventBuilder tool: Did not find channelkey "+std::to_string(temp_chkey)+" in ChannelNumToTankPMTCrateSpaceMap or AuxChannelNumToCrateSpaceMap! Can't convert to electronics space",v_error,verbosity);
     CrateSpaceVector.push_back(temp_channel);
   }

   return CrateSpaceVector;
}
void ANNIEEventBuilder::SaveEntryToFile(int RunNum, int SubRunNum, int PartNum)
{
  /*if(verbosity>4)*/ std::cout << "ANNIEEvent: Saving ANNIEEvent entry"+to_string(ANNIEEventNum) << std::endl;
  std::string Filename = SavePath + ProcessedFilesBasename + "_"+BuildType+"_R" + to_string(RunNum) + 
      "S" + to_string(SubRunNum) + "p" + to_string(PartNum);
  std::string config_info; //Save the ConfigInfo if it exists
  if(  m_data->CStore.Get("ConfigInfo",config_info) ){
    ANNIEEvent->Header->Set("ConfigInfo",config_info);
  }
  if (SaveSeparatePartfiles) ANNIEEvent->Save(Filename);
  std::cout<<"ANNIEEvent: "<<std::endl;
  ANNIEEvent->Print(false);
  //std::cout <<"ANNIEEvent saved, now delete"<<std::endl;
  ANNIEEvent->Delete();		//Delete() will delete the last entry in the store from memory and enable us to set a new pointer (won't erase the entry from saved file)
  //std::cout <<"ANNIEEvent deleted"<<std::endl;
  ANNIEEventNum+=1;
  return;
}

void ANNIEEventBuilder::CardIDToElectronicsSpace(int CardID, 
        int &CrateNum, int &SlotNum)
{
  //CardID = CrateNum * 1000 + SlotNum.  This logic works if we have less than
  // 10 crates and less than 100 Slots (which we do).
  SlotNum = CardID % 100;
  CrateNum = CardID / 1000;
  return;
}

void ANNIEEventBuilder::ElectronicsSpacetoCardID(int CrateNum, int SlotNum, int &CardID){

  CardID = CrateNum*1000+SlotNum;
  return;

}

void ANNIEEventBuilder::OpenNewANNIEEvent(int RunNum, int SubRunNum, int PartNum, uint64_t StarT, int RunT){
  uint64_t StarTime;
  int RunType;
  if(verbosity>v_warning) std::cout << "ANNIEEventBuilder: New run or subrun encountered. Opening new BoostStore" << std::endl;
  if(verbosity>v_debug) std::cout << "ANNIEEventBuilder: Current run,subrun:" << CurrentRunNum << "," << CurrentSubRunNum << std::endl;
  if(verbosity>v_debug) std::cout << "ANNIEEventBuilder: Encountered run,subrun,part:" << RunNum << "," << SubRunNum << ","<<PartNum<<std::endl;
  ANNIEEvent->Close();
  ANNIEEvent->Delete();
  delete ANNIEEvent; ANNIEEvent = new BoostStore(false,2);
  OrphanStore->Close();
  OrphanStore->Delete();
  delete OrphanStore; OrphanStore = new BoostStore(false,2);
  CurrentRunNum = RunNum;
  CurrentSubRunNum = SubRunNum;
  CurrentPartNum = PartNum;
  CurrentRunType = RunT;
  CurrentStarTime = StarT;
  if((CurrentRunNum != RunNum)) CurrentDriftMean = 0;
}

bool ANNIEEventBuilder::FetchWaveformsHits(){

  std::cout <<"FetchWaveformHits"<<std::endl;
  bool has_calibrated = false;
  bool has_hits = false;
  m_data->CStore.Get("NewCalibratedData",has_calibrated);
  m_data->CStore.Get("NewHitsData",has_hits);

  if (!has_calibrated || !has_hits){
    //No calibrated waveforms or hits available
    return false;
  }

  //Get In Progress Hits
 
  //std::cout <<"Get InProgressRecoADCHits, FinishedRawAcqSize, ..."<<std::endl;
  m_data->CStore.Get("InProgressRecoADCHits",InProgressRecoADCHits);
  m_data->CStore.Get("InProgressHitsAux",InProgressHitsAux);
  m_data->CStore.Get("InProgressRecoADCHitsAux",InProgressRecoADCHitsAux);
  m_data->CStore.Get("FinishedRawAcqSize",FinishedRawAcqSize);	//Filled in PhaseIIADCCalibrator
  std::cout <<"InProgressRecoADCHits size (event builder): "<<InProgressRecoADCHits->size()<<std::endl;

  // Fill BeamTankTimestamps vector
  /*
  for (std::pair<uint64_t,std::map<unsigned long,std::vector<Hit>>*> apair : *FinishedHits){
    uint64_t aTimestamp = apair.first;
    myTimeStream.BeamTankTimestamps.push_back(aTimestamp);
  }*/

  // Fill all FinishedHits objects
  std::vector<uint64_t> TimeStampsToDelete;
  for (std::pair<uint64_t, std::map<unsigned long,std::vector<Hit>>*> apair : *FinishedHits){
    uint64_t aTimestamp = apair.first;
    if (FinishedRecoADCHits->count(apair.first)==0){
      FinishedRecoADCHits->emplace(apair.first,InProgressRecoADCHits->at(aTimestamp));
      FinishedRecoADCHitsAux->emplace(apair.first,InProgressRecoADCHitsAux->at(aTimestamp));
      FinishedHitsAux->emplace(apair.first,InProgressHitsAux->at(aTimestamp));
      myTimeStream.BeamTankTimestamps.push_back(aTimestamp);
      TimeStampsToDelete.push_back(aTimestamp);
    }
  }

  RemoveDuplicates(myTimeStream.BeamTankTimestamps);

  std::cout <<"Erase processed timestamps"<<std::endl;
  for (int i_del=0; i_del < (int) TimeStampsToDelete.size(); i_del++){
    InProgressRecoADCHits->erase(TimeStampsToDelete.at(i_del));
    InProgressRecoADCHitsAux->erase(TimeStampsToDelete.at(i_del));
    InProgressHitsAux->erase(TimeStampsToDelete.at(i_del));
  }

  std::cout <<"FinishedHits.size(): "<<FinishedHits->size()<<", BeamTankTimestamps.size(): "<<myTimeStream.BeamTankTimestamps.size()<<", FinishedTankEventsSampleSize.size(): "<<FinishedTankEventsSampleSize->size()<<std::endl;

  m_data->CStore.Set("NewCalibratedData",false);
  m_data->CStore.Set("NewHitsData",false);

  return true;

}

int ANNIEEventBuilder::AlignLAPPDTimestamps(){

  std::cout <<"AlignLAPPDTimestamps"<<std::endl;
  //For now, print everything to outputfiles
  //
  std::stringstream ss_ctc_pps, ss_ctc_beam, ss_lappd_pps, ss_lappd_beam;
  ss_ctc_pps << "ctc_pps_" << CurrentRunNum <<"_p"<<CurrentPartNum<<".txt";
  ss_ctc_beam << "ctc_beam_" << CurrentRunNum <<"_p"<<CurrentPartNum<<".txt";
  ss_lappd_pps << "lappd_pps_" << CurrentRunNum <<"_p"<<CurrentPartNum<<".txt";
  ss_lappd_beam << "lappd_beam_" << CurrentRunNum <<"_p"<<CurrentPartNum<<".txt";

  std::ofstream ctc_pps(ss_ctc_pps.str().c_str());
  std::ofstream ctc_beam(ss_ctc_beam.str().c_str());
  std::ofstream lappd_pps(ss_lappd_pps.str().c_str());
  std::ofstream lappd_beam(ss_lappd_beam.str().c_str());

  std::vector<uint64_t> CTCTimestampsPPS;       //Contains CTC timestamps of triggerword 32 (PPS sync), used for LAPPD alignment
  std::vector<uint64_t> CTCTimestampsBeam;      //Contains CTC timestamps of triggerword 5 (beam), used for LAPPD alignment
  std::vector<uint64_t> LAPPDPPSTimestamps;     //LAPPD PPS timestamps
  std::vector<uint64_t> LAPPDBeamgateTimestamps;        //LAPPD beamgate timestamps
  std::vector<uint64_t> LAPPDTimestamps;        //LAPPD data timestamps


  for (int i_pps=0; i_pps < (int) myTimeStream.CTCTimestampsPPS.size(); i_pps++){
    ctc_pps << myTimeStream.CTCTimestampsPPS.at(i_pps) << std::endl;
    CTCTimestampsPPS.push_back(myTimeStream.CTCTimestampsPPS.at(i_pps));
  } 
  for (int i_b=0; i_b < (int) myTimeStream.CTCTimestampsBeam.size(); i_b++){
    ctc_beam << myTimeStream.CTCTimestampsBeam.at(i_b) << std::endl;
    CTCTimestampsBeam.push_back(myTimeStream.CTCTimestampsBeam.at(i_b));
    //std::cout <<"CTC Beam: "<<myTimeStream.CTCTimestampsBeam.at(i_b)<<std::endl;
  } 
  for (int i_pps=0; i_pps < (int) myTimeStream.LAPPDPPSTimestamps.size(); i_pps++){
    lappd_pps << myTimeStream.LAPPDPPSTimestamps.at(i_pps) << std::endl;
    LAPPDPPSTimestamps.push_back(myTimeStream.LAPPDPPSTimestamps.at(i_pps));
  } 
  for (int i_b=0; i_b < (int) myTimeStream.LAPPDBeamgateTimestamps.size(); i_b++){
    lappd_beam << myTimeStream.LAPPDBeamgateTimestamps.at(i_b) << std::endl;
    LAPPDBeamgateTimestamps.push_back(myTimeStream.LAPPDBeamgateTimestamps.at(i_b));
  }
  std::cout <<"CTCPPS size: "<<CTCTimestampsPPS.size()<<std::endl;
  std::cout <<"CTCBeam size: "<<CTCTimestampsBeam.size()<<std::endl;
  std::cout <<"LAPPDPPS size: "<<LAPPDPPSTimestamps.size()<<std::endl;
  std::cout <<"LAPPDBeam size: "<<LAPPDBeamgateTimestamps.size()<<std::endl;

  ctc_pps.close();
  ctc_beam.close();
  lappd_pps.close();
  lappd_beam.close();

  
  
  std::vector<double> mean_beam_deviation;
  std::vector<double> stddev_beam;
  std::vector<int> num_missing_beam;
  std::vector<int> num_missing_pps;
  std::vector<uint64_t> offset_vector;

  uint64_t first_ctc_pps = CTCTimestampsPPS.at(0);
  uint64_t last_ctc_pps = CTCTimestampsPPS.at(CTCTimestampsPPS.size()-1);
  int counter=0;

  for (int i_lappd_pps=0; i_lappd_pps < (int) LAPPDPPSTimestamps.size(); i_lappd_pps++){
  for (int i_ctc=0; i_ctc < (int) CTCTimestampsPPS.size(); i_ctc++){
  
    uint64_t offset = CTCTimestampsPPS.at(i_ctc) - LAPPDPPSTimestamps.at(i_lappd_pps);
    std::vector<double> dev_beam;
    int missing_beam=0;
    double first_dev=0;
    int missing_pps=0;
    for (int i_lappd=0; i_lappd < (int) LAPPDPPSTimestamps.size(); i_lappd++){
      if ((LAPPDPPSTimestamps.at(i_lappd)+offset) < first_ctc_pps) missing_pps++;
      else if ((LAPPDPPSTimestamps.at(i_lappd)+offset) > last_ctc_pps) missing_pps++;
    }
    num_missing_pps.push_back(missing_pps); 

    //Loop through all beam timestamps  
    for (int i_beam=0; i_beam < (int) LAPPDBeamgateTimestamps.size(); i_beam++){
      double lappdbeam = double(LAPPDBeamgateTimestamps.at(i_beam)+offset);
      double min_deviation = 999999999999999;
      bool use_value = true;
      for (int i_beam2=0; i_beam2 < (int) CTCTimestampsBeam.size(); i_beam2++){
        double ctcbeam = double(CTCTimestampsBeam.at(i_beam2));
        double dev = ctcbeam-lappdbeam;
        if (i_beam2 ==0) first_dev = dev;
        if (fabs(dev) < min_deviation) min_deviation = fabs(dev);
        if (i_beam2 == CTCTimestampsBeam.size()-1 && fabs(min_deviation-fabs(dev))<0.01) use_value = false;
        else if (i_beam2 == CTCTimestampsBeam.size()-1 && fabs(min_deviation-fabs(first_dev))<0.01) use_value = false;
      }
      if (use_value) dev_beam.push_back(min_deviation);
      else missing_beam++;
    }
    num_missing_beam.push_back(missing_beam);
    double mean_dev=0;
    for (int i_dev=0; i_dev < (int) dev_beam.size(); i_dev++){
      mean_dev += dev_beam.at(i_dev);
    }
    if (dev_beam.size() > 0) mean_dev /= dev_beam.size();
    std::vector<double> diff(dev_beam.size());
    std::transform(dev_beam.begin(), dev_beam.end(), diff.begin(), [mean_dev](double x) { return x - mean_dev; });
    double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / dev_beam.size());
    mean_beam_deviation.push_back(mean_dev);
    stddev_beam.push_back(stdev);
    offset_vector.push_back(offset);
   /* if (counter>2330 && counter<2350){
        std::cout <<"counter: "<<counter<<std::endl;
      for (int i_diff=0; i_diff < diff.size(); i_diff++){
        std::cout <<"diff: "<<diff.at(i_diff)<<std::endl;
      }
      std::cout <<"mean: "<<mean_dev<<", stddev: "<<stdev<<std::endl;
    }*/
    
    counter++;

  }
  }
  
  //Loop through deviations and find the best match
  double min_dev=99999999999;
  double min_mean=99999999999;
  int best_match_ctc=-1;
  uint64_t best_match_offset = 0;
  int max_missing_beam = 2;
  if (LAPPDBeamgateTimestamps.size() <=4) max_missing_beam = 0;

  //Normal offset finding procedure
  if (LAPPDOffsetFile== "None"){
  for (int i_dev=0; i_dev < mean_beam_deviation.size(); i_dev++){
    std::cout <<"LAPPD CTC timestamp # 0, CTC Timestamp #"<<i_dev<<", mean dev: "<<mean_beam_deviation.at(i_dev)<<", std dev: "<<stddev_beam.at(i_dev)<<", missing beam: "<<num_missing_beam.at(i_dev)<<", offset: "<<offset_vector.at(i_dev)<<std::endl;
   //if (num_missing_beam.at(i_dev)== 0 && stddev_beam.at(i_dev) < min_dev){
   if (num_missing_beam.at(i_dev) <= max_missing_beam && stddev_beam.at(i_dev) < min_dev && num_missing_pps.at(i_dev)<=1 && mean_beam_deviation.at(i_dev) < min_mean){
      min_dev = stddev_beam.at(i_dev);
      min_mean = mean_beam_deviation.at(i_dev);
      best_match_ctc = i_dev;
      best_match_offset = offset_vector.at(i_dev);
std::cout <<"***NEW BEST MATCH: min_dev: "<<min_dev<<", min_mean: "<<min_mean<<", best_match_offset: "<<best_match_offset<<std::endl;
    //} else if (num_missing_beam.at(i_dev) <= max_missing_beam && stddev_beam.at(i_dev) == min_dev && num_missing_pps.at(i_dev)<=1 && mean_beam_deviation.at(i_dev) < min_mean){
    //} else if (num_missing_beam.at(i_dev) <= max_missing_beam && stddev_beam.at(i_dev) < 100000 && stddev_beam.at(i_dev) >= 0 && num_missing_pps.at(i_dev)<=1 && mean_beam_deviation.at(i_dev) < min_mean){
    } else if (num_missing_beam.at(i_dev) <= max_missing_beam && stddev_beam.at(i_dev) < 100000000 && stddev_beam.at(i_dev) >= 0 && num_missing_pps.at(i_dev)<=1 && mean_beam_deviation.at(i_dev) < min_mean){
      min_dev = stddev_beam.at(i_dev);
      min_mean = mean_beam_deviation.at(i_dev);
      best_match_ctc = i_dev;
      best_match_offset = offset_vector.at(i_dev);
      std::cout <<"***NEW BEST MATCH: min_dev: "<<min_dev<<", min_mean: "<<min_mean<<", best_match_offset: "<<best_match_offset<<std::endl;
    }
  }
  } else {
    //Offset finding procedure based on input parameters
    double temp, c, m;
    int counter=0;
    ifstream offsetfile(LAPPDOffsetFile.c_str());
    while (!offsetfile.eof()){
     offsetfile >> temp;
     if (counter==0) c = temp;
     else if (counter==1) m = temp;
     counter++;
     if (offsetfile.eof()) break; 
    }
    offsetfile.close();

    std::cout <<"Read in c: "<<c<<std::endl;
    std::cout <<"Read in m: "<<m<<std::endl;

    double expected_offset = c + m*CTCTimestampsPPS.at(0)/1000000000.;
    std::cout <<"expected_offset: "<<expected_offset<<std::endl;
    
    for (int i_dev=0; i_dev < offset_vector.size(); i_dev++){
      double diff_obs_exp = fabs(offset_vector.at(i_dev)/1000000000.-expected_offset);
      if (diff_obs_exp < min_dev){
        min_dev = diff_obs_exp;
      best_match_ctc = i_dev;
      best_match_offset = offset_vector.at(i_dev);
      std::cout <<"***NEW BEST MATCH: offset: "<<offset_vector.at(i_dev)<<", expected_offset: "<<expected_offset<<", deviation: "<<min_dev<<std::endl;
      }
    }

   }

  if (best_match_ctc == -1){
    std::cout <<"Did not find a best match between the CTC and the LAPPD timestreams!"<<std::endl;
    std::cout <<"Check manually what went wrong..."<<std::endl;
    return 1;
  } else {
    lappd_aligned = true;
    lappd_time_offset = best_match_offset;
    std::cout <<"Did find a best match between the CTC and the LAPPD timestream!"<<std::endl;
    std::cout <<"Offset is "<<lappd_time_offset<<std::endl;

    std::vector<uint64_t> LAPPD_timestamps_to_delete;
    //Fill the corrected timestamps in FinishedLAPPDPsecData object
    for (std::map<uint64_t,PsecData>::iterator it = LAPPDPsecMap.begin(); it!= LAPPDPsecMap.end(); it++){
      uint64_t timestamp_lappd = it->first;
      PsecData lappd_data = it->second;
      FinishedLAPPDPsecData->emplace(timestamp_lappd+lappd_time_offset,lappd_data);
      myTimeStream.LAPPDGlobalTimestamps.push_back(timestamp_lappd+lappd_time_offset);
      LAPPD_timestamps_to_delete.push_back(timestamp_lappd);
    }
    //Erase corresponding entry from in-progress map
    for (int i_delete=0; i_delete < (int) LAPPD_timestamps_to_delete.size(); i_delete++){
      LAPPDPsecMap.erase(LAPPD_timestamps_to_delete.at(i_delete));
    }
    //Erase PPS and beamgate timestamp information (not needed anymore after matching)
    myTimeStream.LAPPDPPSTimestamps.clear();
    myTimeStream.LAPPDBeamgateTimestamps.clear();
    myTimeStream.LAPPDTimestamps.clear();    

    return 0;
  }

}

void ANNIEEventBuilder::CorrectVMEOffsetRaw(bool force_all_entries){

  Log("ANNIEEventBuilder: CorrectVMEOffsetRaw",v_message,verbosity);

  std::vector<uint64_t> timestamps_tank;
  std::map<uint64_t, uint64_t> timestamps_to_shift;

  //Make a list of PMT timestamps
  for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>>> apair : *InProgressTankEvents){
    uint64_t PMTCounterTimeNs = apair.first;
    timestamps_tank.push_back(PMTCounterTimeNs);
  }

  //Go through the list of PMT timestamps and look for abnormally close timestamps (8ns)
  //Start with the second entry and always compare to previous entry
  for (int i_timestamp = 1; i_timestamp < timestamps_tank.size(); i_timestamp++){
    uint64_t FirstTS = timestamps_tank.at(i_timestamp-1);
    uint64_t SecondTS = timestamps_tank.at(i_timestamp);
    uint64_t TSDiff = (SecondTS > FirstTS)? (SecondTS-FirstTS) : (FirstTS-SecondTS);
    if (TSDiff == 8 || TSDiff == 16){
      //if 8ns, 16ns offset is detected between the VME crates, map the entry with less waveforms onto the one with more waveforms
      int waveforms_first = int(InProgressTankEvents->at(FirstTS).size());
      int waveforms_second = int(InProgressTankEvents->at(SecondTS).size());
      bool first_entry_larger = (waveforms_first > waveforms_second);
      Log("ANNIEEventBuilder::CorrectVMEOffsetRaw: First TS = "+std::to_string(FirstTS)+", waveforms = "+std::to_string(waveforms_first)+", Second TS = "+std::to_string(SecondTS)+", waveforms = "+std::to_string(waveforms_second)+", first_entry_larger = "+std::to_string(first_entry_larger),v_debug,verbosity);
      if (first_entry_larger) timestamps_to_shift.emplace(SecondTS,FirstTS);
      else timestamps_to_shift.emplace(FirstTS,SecondTS); 
    }
    else if(TSDiff < 1600){ //For the log-file, to investigate why the processing does not produce Processed files
      Log("ANNIEEventBuilder::CorrectVMEOffsetRaw: Gross missmatch in TSDiff = "+std::to_string(TSDiff),v_error,verbosity);
      return;
    }
  }

  //Go through the timestamps to shift and apply the correction
  for (std::map<uint64_t, uint64_t>::iterator it=timestamps_to_shift.begin(); it!= timestamps_to_shift.end(); it++){
    uint64_t FirstTS = it->first;
    uint64_t SecondTS = it->second;
    Log("ANNIEEventBuilder::CorrectVMEOffsetRaw: Map Timestamp "+std::to_string(FirstTS)+" to timestamp "+std::to_string(SecondTS),v_debug,verbosity);
    if(InProgressTankEvents->count(FirstTS) == 0 || InProgressTankEvents->count(SecondTS) == 0){ //map object at FirstTS, SecondTS may not exist yet
      Log("ANNIEEventBuilder::CorrectVMEOffset: InProgressTankEvents->count(FirstTS) == "+std::to_string(InProgressTankEvents->count(FirstTS))+", InProgressTankEvents->count(SecondTS) == "+std::to_string(InProgressTankEvents->count(SecondTS)),v_debug,verbosity);
      break; 
    }
    std::map<std::vector<int>, std::vector<uint16_t>> FirstTankEvents = InProgressTankEvents->at(FirstTS);
    std::map<std::vector<int>, std::vector<uint16_t>> SecondTankEvents = InProgressTankEvents->at(SecondTS);
    
    //Merge the two waveform maps
    SecondTankEvents.insert(FirstTankEvents.begin(), FirstTankEvents.end());
    Log("ANNIEEventBuilder: Size of Merged Waveforms map: "+std::to_string(SecondTankEvents.size()),v_debug,verbosity);

    //Associated merged map with preferred TS, delete other TS
    (*InProgressTankEvents)[SecondTS] = SecondTankEvents;
    InProgressTankEvents->erase(FirstTS);
    Log("ANNIEEventBuilder: Size of merged TS in InProgressTankEvents: "+std::to_string(InProgressTankEvents->at(SecondTS).size()),v_debug,verbosity);
  }

}

void ANNIEEventBuilder::CorrectVMEOffset(bool force_all_entries){

  Log("ANNIEEventBuilder: CorrectVMEOffset",v_message,verbosity);
  std::vector<uint64_t> timestamps_tank;
  std::map<uint64_t, uint64_t> timestamps_to_shift;
  
  //Make a list of PMT timestamps
  for(std::pair<uint64_t, std::map<unsigned long,std::vector<Hit>>*> apair : *InProgressHits){
    uint64_t PMTCounterTimeNs = apair.first;
    timestamps_tank.push_back(PMTCounterTimeNs);
  }

  //Go through the list of PMT timestamps and look for abnormally close timestamps (8ns)
  //Start with the second entry and always compare to previous entry
  for (int i_timestamp = 1; i_timestamp < (int) timestamps_tank.size(); i_timestamp++){
    uint64_t FirstTS = timestamps_tank.at(i_timestamp-1);
    uint64_t SecondTS = timestamps_tank.at(i_timestamp);
    uint64_t TSDiff = (SecondTS > FirstTS)? (SecondTS-FirstTS) : (FirstTS-SecondTS);
    if (TSDiff == 8 || TSDiff == 16){
      //if 8ns, 16ns offset is detected between the VME crates, map the entry with less waveforms onto the one with more waveforms
      int waveforms_first = int(InProgressHits->at(FirstTS)->size());
      int waveforms_second = int(InProgressHits->at(SecondTS)->size());
      bool first_entry_larger = (waveforms_first > waveforms_second);
      Log("ANNIEEventBuilder::CorrectVMEOffset: TSDiff = "+std::to_string(TSDiff)+", First TS = "+std::to_string(FirstTS)+", waveforms = "+std::to_string(waveforms_first)+", Second TS = "+std::to_string(SecondTS)+", waveforms = "+std::to_string(waveforms_second)+", first_entry_larger = "+std::to_string(first_entry_larger),v_debug,verbosity);
      if (first_entry_larger) timestamps_to_shift.emplace(SecondTS,FirstTS);
      else timestamps_to_shift.emplace(FirstTS,SecondTS); 
    }
    else if(TSDiff < 1600){ //For the log-file, to investigate why the processing does not produce Processed files
      Log("ANNIEEventBuilder::CorrectVMEOffset: Gross missmatch in TSDiff = "+std::to_string(TSDiff),v_error,verbosity);
      return;
    }
  }

  //Go through the timestamps to shift and apply the correction
  if( InProgressHitsAux != NULL && InProgressRecoADCHitsAux != NULL ){ //InProgressHitsAux,InProgressRecoADCHitsAux may exist yet
    for (std::map<uint64_t, uint64_t>::iterator it=timestamps_to_shift.begin(); it!= timestamps_to_shift.end(); it++){
      uint64_t FirstTS = it->first;
      uint64_t SecondTS = it->second;
      Log("ANNIEEventBuilder::CorrectVMEOffset: Map Timestamp "+std::to_string(FirstTS)+" to timestamp "+std::to_string(SecondTS),v_debug,verbosity);
      if(InProgressHits->count(FirstTS) == 0 || InProgressHits->count(SecondTS) == 0){ //map object at FirstTS, SecondTS may not exist yet
        Log("ANNIEEventBuilder::CorrectVMEOffset: InProgressHits->count(FirstTS) == "+std::to_string(InProgressHits->count(FirstTS))+", InProgressHits->count(SecondTS) == "+std::to_string(InProgressHits->count(SecondTS)),v_debug,verbosity);
        break;
      }

      //Get InProgress* {Hits, Chkey, and RecoADCHits} objects
      std::map<unsigned long,std::vector<Hit>>* FirstTankHits = InProgressHits->at(FirstTS);
      std::map<unsigned long,std::vector<Hit>>* SecondTankHits = InProgressHits->at(SecondTS);
      std::vector<unsigned long> FirstChankey = InProgressChkey->at(FirstTS);
      std::vector<unsigned long> SecondChankey = InProgressChkey->at(SecondTS);
      std::map<unsigned long,std::vector<Hit>>* FirstTankHitsAux = InProgressHitsAux->at(FirstTS);
      std::map<unsigned long,std::vector<Hit>>* SecondTankHitsAux = InProgressHitsAux->at(SecondTS);
      std::map<unsigned long,std::vector<std::vector<ADCPulse>>> FirstRecoADCHits = InProgressRecoADCHits->at(FirstTS);
      std::map<unsigned long,std::vector<std::vector<ADCPulse>>> SecondRecoADCHits = InProgressRecoADCHits->at(SecondTS);
      std::map<unsigned long,std::vector<std::vector<ADCPulse>>> FirstRecoADCHitsAux = InProgressRecoADCHitsAux->at(FirstTS);
      std::map<unsigned long,std::vector<std::vector<ADCPulse>>> SecondRecoADCHitsAux = InProgressRecoADCHitsAux->at(SecondTS);

      //Merge the two hits maps
      SecondTankHits->insert(FirstTankHits->begin(), FirstTankHits->end());
      Log("ANNIEEventBuilder: Size of Merged Hits map: "+std::to_string(SecondTankHits->size()),v_debug,verbosity);

      //Merge the two aux hits maps
      SecondTankHitsAux->insert(FirstTankHitsAux->begin(), FirstTankHitsAux->end());
      Log("ANNIEEventBuilder: Size of Merged AuxHits map: "+std::to_string(SecondTankHitsAux->size()),v_debug,verbosity);

      //Merge the two channelkey vectors
      SecondChankey.insert(SecondChankey.end(), FirstChankey.begin(), FirstChankey.end());
      Log("ANNIEEventBuilder: Size of Merged Chkey vector: "+std::to_string(SecondChankey.size()),v_debug,verbosity);

      //Merge the two RecoADCHits maps
      SecondRecoADCHits.insert(FirstRecoADCHits.begin(),FirstRecoADCHits.end());
      Log("ANNIEEventBuilder: Size of Merged RecoADCHits map: "+std::to_string(SecondRecoADCHits.size()),v_debug,verbosity);

      //Merge the two RecoADCHitsAux maps
      SecondRecoADCHitsAux.insert(FirstRecoADCHitsAux.begin(),FirstRecoADCHitsAux.end());
      Log("ANNIEEventBuilder: Size of Merged RecoADCHitsAux map: "+std::to_string(SecondRecoADCHitsAux.size()),v_debug,verbosity);

      //Associated merged map with preferred TS, delete other TS
      (*InProgressHits)[SecondTS] = SecondTankHits;
      InProgressHits->erase(FirstTS);
      Log("ANNIEEventBuilder: Size of merged TS in InProgressHits: "+std::to_string(InProgressHits->at(SecondTS)->size()),v_debug,verbosity);

      (*InProgressHitsAux)[SecondTS] = SecondTankHitsAux;
      InProgressHitsAux->erase(FirstTS);
      Log("ANNIEEventBuilder: Size of merged TS in InProgressHitsAux: "+std::to_string(InProgressHitsAux->at(SecondTS)->size()),v_debug,verbosity);

      //Do the same for Chkey map
      (*InProgressChkey)[SecondTS] = SecondChankey;
      InProgressChkey->erase(FirstTS);
      Log("ANNIEEventBuilder: Size of merged TS in InProgressChkey: "+std::to_string(InProgressChkey->at(SecondTS).size()),v_debug,verbosity);

      (*InProgressRecoADCHits)[SecondTS] = SecondRecoADCHits;
      InProgressRecoADCHits->erase(FirstTS);
      Log("ANNIEEventBuilder: Size of merged TS in InProgressRecoADCHits: "+std::to_string(InProgressRecoADCHits->at(SecondTS).size()),v_debug,verbosity);
      
      (*InProgressRecoADCHitsAux)[SecondTS] = SecondRecoADCHitsAux;
      InProgressRecoADCHitsAux->erase(FirstTS);
      Log("ANNIEEventBuilder: Size of merged TS in InProgressRecoADCHitsAux: "+std::to_string(InProgressRecoADCHitsAux->at(SecondTS).size()),v_debug,verbosity);

    }
  }
}

