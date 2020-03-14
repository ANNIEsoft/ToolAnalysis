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
  OrphanWarningValue = 20;
  MRDPMTTimeDiffTolerance = 10;   //ms
  DriftWarningValue = 5;           //ms
  NumWavesInCompleteSet = 140;
  OrphanOldTankTimestamps = true;
  OldTimestampThreshold = 30; //seconds
  ExecutesPerBuild = 50;

  /////////////////////////////////////////////////////////////////
  //FIXME: Need rough scan and tolerances in variable settings
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("SavePath",SavePath);
  m_variables.Get("ProcessedFilesBasename",ProcessedFilesBasename);
  m_variables.Get("BuildType",BuildType);
  m_variables.Get("NumEventsPerPairing",EventsPerPairing);
  m_variables.Get("MinNumWavesInSet",NumWavesInCompleteSet);
  m_variables.Get("OrphanOldTankTimestamps",OrphanOldTankTimestamps);
  m_variables.Get("OldTimestampThreshold",OldTimestampThreshold);
  m_variables.Get("ExecutesPerBuild",ExecutesPerBuild);

  if(BuildType == "TankAndMRD" || BuildType == "TankAndMRDAndCTC"){
    std::cout << "BuildANNIEEvent Building Tank and MRD-merged ANNIE events. " <<
        std::endl;
  }
  else if(BuildType == "Tank" || BuildType == "MRD"){
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

  if(BuildType == "Tank" || BuildType == "TankAndMRD" || BuildType == "TankAndMRDAndCTC"){
    int NumTankPMTChannels = TankPMTCrateSpaceToChannelNumMap.size();
    int NumAuxChannels = AuxCrateSpaceToChannelNumMap.size();
    if(verbosity>4) std::cout << "TOTAL TANK + AUX CHANNELS: " << NumTankPMTChannels + NumAuxChannels << std::endl;
    if(verbosity>4) std::cout << "CURRENT SET THRESHOLD FOR BUILDING PMT EVENTS: " << NumWavesInCompleteSet << std::endl;
  }

  //////////////////////initialize subrun index//////////////
  ANNIEEvent = new BoostStore(false,2);
  ANNIEEventNum = 0;
  CurrentRunNum = -1;
  CurrentSubRunNum = -1;
  CurrentDriftMean = 0;

  return true;
}


bool ANNIEEventBuilder::Execute(){
  bool NewEntryAvailable;
  m_data->CStore.Get("NewRawDataEntryAccessed",NewEntryAvailable);
  if(!NewEntryAvailable){ //Something went wrong processing raw data.  Stop and save what's left
    Log("ANNIEEventBuilder Tool: There's no new PMT/MRD data.  Stopping loop, ANNIEEvent BoostStore will save.",v_warning,verbosity); 
    m_data->vars.Set("StopLoop",1);
    return true;
  }
    
    
  ExecuteCount+=1;
  if(ExecuteCount<ExecutesPerBuild) return true;

  //See if the MRD and Tank are at the same run/subrun for building
  m_data->CStore.Get("RunInfoPostgress",RunInfoPostgress);
  int RunNumber;
  int SubRunNumber;
  uint64_t StarTime;
  int RunType;
  RunInfoPostgress.Get("RunNumber",RunNumber);
  RunInfoPostgress.Get("SubRunNumber",SubRunNumber);
  RunInfoPostgress.Get("RunType",RunType);
  RunInfoPostgress.Get("StarTime",StarTime);
  
  //If our first execute loop, Initialize Current run information with Tank decoding progress
  if(CurrentRunNum == -1 || CurrentSubRunNum == -1){
    CurrentRunNum = RunNumber;
    CurrentSubRunNum = SubRunNumber;
    CurrentStarTime = StarTime;
    CurrentRunType = RunType;
  }

  //If we're in a new run or subrun, make a new ANNIEEvent file. 
  if((CurrentRunNum != RunNumber) || (CurrentSubRunNum != SubRunNumber)){
    this->OpenNewANNIEEvent(RunNumber,SubRunNumber,StarTime,RunType);
  }

  if (BuildType == "Tank"){
    //Check to see if there's new PMT data
    m_data->CStore.Get("NewTankPMTDataAvailable",IsNewTankData);
    if(!IsNewTankData){
      Log("ANNIEEventBuilder:: No new Tank Data.  Not building ANNIEEvent. ",v_message, verbosity);
      return true;
    }
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

  } else if (BuildType == "MRD"){
    m_data->CStore.Get("NewMRDDataAvailable",IsNewMRDData);
    std::vector<unsigned long> MRDEventsToDelete;
    if(!IsNewMRDData){
      Log("ANNIEEventBuilder:: No new MRD Data.  Not building ANNIEEvent: ",v_message, verbosity);
      return true;
    }
    m_data->CStore.Get("MRDEvents",MRDEvents);
    m_data->CStore.Get("MRDEventTriggerTypes",MRDTriggerTypeMap);
    
   //Loop through MRDEvents and process each into ANNIEEvent.
    for(std::pair<unsigned long,std::vector<std::pair<unsigned long,int>>> apair : MRDEvents){
      unsigned long MRDTimeStamp = apair.first;
      std::vector<std::pair<unsigned long,int>> MRDHits = apair.second;
      std::string MRDTriggerType = MRDTriggerTypeMap[MRDTimeStamp];
      this->BuildANNIEEventRunInfo(RunNumber,SubRunNumber,RunType,StarTime);
      this->BuildANNIEEventMRD(MRDHits, MRDTimeStamp, MRDTriggerType);
      this->SaveEntryToFile(RunNumber,SubRunNumber);
      //Erase this entry from the InProgressTankEventsMap
      MRDEventsToDelete.push_back(MRDTimeStamp);
    }
    for (unsigned int i=0; i< MRDEventsToDelete.size(); i++){
      MRDEvents.erase(MRDEventsToDelete.at(i));
      MRDTriggerTypeMap.erase(MRDEventsToDelete.at(i));
    }
    m_data->CStore.Set("MRDEvents",MRDEvents);
    m_data->CStore.Set("MRDEventTriggerTypes",MRDTriggerTypeMap);
  }

  else if (BuildType == "TankAndMRD"){

    //Check if any In-progress tank events now have all waveforms
    m_data->CStore.Get("InProgressTankEvents",InProgressTankEvents);
    m_data->CStore.Get("NewTankPMTDataAvailable",IsNewTankData);
    std::vector<uint64_t> InProgressTankEventsToDelete;
    if(IsNewTankData) InProgressTankEventsToDelete = this->ProcessNewTankPMTData();
    
    //Look through our MRD data for any new timestamps
    m_data->CStore.Get("MRDEventTriggerTypes",MRDTriggerTypeMap);
    m_data->CStore.Get("NewMRDDataAvailable",IsNewMRDData);
    m_data->CStore.Get("MRDEvents",MRDEvents);
    if(IsNewMRDData) this->ProcessNewMRDData();
    
    
    //Since timestamp pairing has been done for finished Tank Events,
    //Erase the finished Tank Events from the InProgressTankEventsMap
    if(verbosity>3)std::cout << "Now erasing finished timestamps from InProgressTankEvents" << std::endl;
    for (unsigned int j=0; j< InProgressTankEventsToDelete.size(); j++){
      InProgressTankEvents->erase(InProgressTankEventsToDelete.at(j));
    }
    if(verbosity>3) std::cout << "Current number of unfinished PMT Waveforms: " << InProgressTankEvents->size() << std::endl;

    
    //Now, pair up PMT and MRD events...
    int NumTankTimestamps = BeamTankTimestamps.size();
    int NumMRDTimestamps = BeamMRDTimestamps.size();
    int MinStamps = std::min(NumTankTimestamps,NumMRDTimestamps);

    if(MinStamps > (EventsPerPairing*10)){
      this->RemoveCosmics();
      this->PairTankPMTAndMRDTriggers();

      std::vector<uint64_t> BuiltTankTimes;
      for(std::pair<uint64_t,uint64_t> cpair : BeamTankMRDPairs){
        uint64_t TankCounterTime = cpair.first;
        if(verbosity>4) std::cout << "TANK EVENT WITH TIMESTAMP " << TankCounterTime << "HAS REQUIRED MINIMUM NUMBER OF WAVES TO BUILD" << std::endl;
        std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = FinishedTankEvents.at(TankCounterTime);
        uint64_t MRDTimeStamp = cpair.second;
        if(verbosity>4) std::cout << "MRD TIMESTAMP: " << MRDTimeStamp << std::endl;
        std::vector<std::pair<unsigned long,int>> MRDHits = MRDEvents.at(MRDTimeStamp);
        std::string MRDTriggerType = MRDTriggerTypeMap[MRDTimeStamp];
        if(verbosity>4) std::cout << "BUILDING AN ANNIE EVENT" << std::endl;
        this->BuildANNIEEventRunInfo(CurrentRunNum, CurrentSubRunNum, CurrentRunType,CurrentStarTime);
        this->BuildANNIEEventTank(TankCounterTime, aWaveMap);
        this->BuildANNIEEventMRD(MRDHits, MRDTimeStamp, MRDTriggerType);
        this->SaveEntryToFile(CurrentRunNum,CurrentSubRunNum);
        if(verbosity>4) std::cout << "BUILT AN ANNIE EVENT SUCCESSFULLY" << std::endl;
        //Erase this entry from maps/vectors used when pairing completed events 
        BuiltTankTimes.push_back(TankCounterTime);
        FinishedTankEvents.erase(TankCounterTime);
        MRDEvents.erase(MRDTimeStamp);
        MRDTriggerTypeMap.erase(MRDTimeStamp);
      }
      for(int i=0; i<BuiltTankTimes.size(); i++){
        BeamTankMRDPairs.erase(BuiltTankTimes.at(i));
      }
    }
  
  }
 
  else if (BuildType == "TankAndMRDAndCTC"){

    m_data->CStore.Get("InProgressTankEvents",InProgressTankEvents);
    m_data->CStore.Get("NewTankPMTDataAvailable",IsNewTankData);
    std::vector<uint64_t> InProgressTankEventsToDelete;
    if(IsNewTankData) InProgressTankEventsToDelete = this->ProcessNewTankPMTData();
    if(verbosity>3)std::cout << "Erasing finished timestamps from InProgressTankEvents" << std::endl;
    for (unsigned int j=0; j< InProgressTankEventsToDelete.size(); j++){
      InProgressTankEvents->erase(InProgressTankEventsToDelete.at(j));
    }
    if(verbosity>3) std::cout << "Current number of unfinished PMT Waveforms: " << InProgressTankEvents->size() << std::endl;

    
    //Look through our MRD data for any new timestamps
    m_data->CStore.Get("MRDEventTriggerTypes",MRDTriggerTypeMap);
    m_data->CStore.Get("MRDEvents",MRDEvents);
    m_data->CStore.Get("NewMRDDataAvailable",IsNewMRDData);
    if(IsNewMRDData) this->ProcessNewMRDData();


    m_data->CStore.Get("TimeToTriggerWordMap",TimeToTriggerWordMap);
    m_data->CStore.Get("NewCTCDataAvailable",IsNewCTCData);
    if(IsNewCTCData) this->ProcessNewCTCData();

    
    //Now, pair up PMT/MRD/Triggers...
    int NumTankTimestamps = BeamTankTimestamps.size();
    int NumMRDTimestamps = BeamMRDTimestamps.size();
    int MinTMStamps = std::min(NumTankTimestamps,NumMRDTimestamps);
    int NumTrigs = CTCTimestamps.size();
    int MinStamps = std::min(MinTMStamps,NumTrigs);

    if(verbosity>3){
        std::cout << "Number of CTCTimes, MRDTimes, PMTTimes: " << 
            NumTrigs << "," << NumTankTimestamps << "," << NumMRDTimestamps << std::endl;
    }
    if(MinStamps > (EventsPerPairing*10)){
      if(verbosity>3) std::cout << "BEGINNING STREAM MERGING " << std::endl;
      //We have enough Trigger,PMT, and MRD data to hopefully do some pairings.
      this->MergeStreams();

      std::vector<uint64_t> BuiltSetKeys;
      for(std::pair<uint64_t, std::map<std::string,uint64_t>> builder_entries : BuildMap){
        uint64_t CTCtimestamp = builder_entries.first;
        std::map<std::string,uint64_t> aBuildSet = builder_entries.second; 
        for(std::pair<std::string,uint64_t> buildset_entries : aBuildSet){
          this->BuildANNIEEventRunInfo(CurrentRunNum, CurrentSubRunNum, CurrentRunType,CurrentStarTime);
          //If we have PMT and MRD infor for this trigger, build it 
          std::string label = buildset_entries.first;
          if(label == "CTC"){
            uint64_t CTCWord = buildset_entries.second;
            this->BuildANNIEEventCTC(CTCtimestamp,CTCWord);//TODO: Have a BuildANNIEEventTriggerInfo()
            TimeToTriggerWordMap->erase(CTCtimestamp);
          }
          if(label == "TankPMT"){
            uint64_t TankPMTTime = buildset_entries.second;
            if(verbosity>4) std::cout << "TANK EVENT WITH TIMESTAMP " << TankPMTTime << "HAS REQUIRED MINIMUM NUMBER OF WAVES TO BUILD" << std::endl;
            std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = FinishedTankEvents.at(TankPMTTime);
            this->BuildANNIEEventTank(TankPMTTime, aWaveMap);
            FinishedTankEvents.erase(TankPMTTime);
          }
          if(label == "MRD"){
            uint64_t MRDTimeStamp = buildset_entries.second;
            if(verbosity>4) std::cout << "MRD TIMESTAMP: " << MRDTimeStamp << std::endl;
            std::vector<std::pair<unsigned long,int>> MRDHits = MRDEvents.at(MRDTimeStamp);
            std::string MRDTriggerType = MRDTriggerTypeMap[MRDTimeStamp];
            this->BuildANNIEEventMRD(MRDHits, MRDTimeStamp, MRDTriggerType);
            MRDEvents.erase(MRDTimeStamp);
            MRDTriggerTypeMap.erase(MRDTimeStamp);
          }
          this->SaveEntryToFile(CurrentRunNum,CurrentSubRunNum);
          if(verbosity>4) std::cout << "BUILT AN ANNIE EVENT SUCCESSFULLY" << std::endl;
        }
        BuiltSetKeys.push_back(CTCtimestamp);
        //TODO: clear out any built BuildMap entries.
        for(int k=0;k<BuiltSetKeys.size(); k++){
          BuildMap.erase(BuiltSetKeys.at(k));
        }
      } 
    }
  }



  //m_data->CStore.Set("InProgressTankEvents",InProgressTankEvents);
  m_data->CStore.Set("MRDEvents",MRDEvents);
  m_data->CStore.Set("MRDEventTriggerTypes",MRDTriggerTypeMap);
  MRDEvents.clear();
  MRDTriggerTypeMap.clear();

  ExecuteCount = 0;
  return true;
}


bool ANNIEEventBuilder::Finalise(){
  if(verbosity>4) std::cout << "ANNIEEvent Finalising.  Closing any open ANNIEEvent Boostore" << std::endl;
  if(verbosity>2) std::cout << "ANNIEEventBuilder: Saving and closing file." << std::endl;
  ANNIEEvent->Close();
  ANNIEEvent->Delete();
  delete ANNIEEvent;
  if(verbosity>2) std::cout << "PMT/MRD Orphan number at finalise: " << OrphanTankTimestamps.size() <<
          "," << OrphanMRDTimestamps.size() << std::endl;
  //Save the current subrun and delete ANNIEEvent
  std::cout << "ANNIEEventBuilder Exitting" << std::endl;
  return true;
}

void ANNIEEventBuilder::ProcessNewCTCData(){
  for(std::pair<uint64_t,uint32_t> apair : *TimeToTriggerWordMap){
    uint64_t CTCTimeStamp = apair.first;
    uint32_t CTCWord = apair.second;
    CTCTimestamps.push_back(CTCTimeStamp);
    if(verbosity>5)std::cout << "CTCTIMESTAMP,WORD" << CTCTimeStamp << "," << CTCWord << std::endl;
  }
  RemoveDuplicates(CTCTimestamps);
  return;
}

void ANNIEEventBuilder::ProcessNewMRDData(){
  for(std::pair<unsigned long,std::vector<std::pair<unsigned long,int>>> apair : MRDEvents){
    unsigned long MRDTimeStamp = apair.first;
    BeamMRDTimestamps.push_back(MRDTimeStamp);
    if(verbosity>5)std::cout << "MRDTIMESTAMPTRIGTYPE," << MRDTimeStamp << "," << MRDTriggerTypeMap.at(MRDTimeStamp) << std::endl;
  }
  RemoveDuplicates(BeamMRDTimestamps);
  return;
}

std::vector<uint64_t> ANNIEEventBuilder::ProcessNewTankPMTData(){
  //Check if any In-progress tank events now have all waveforms
  std::vector<uint64_t> InProgressTankEventsToDelete;
  if(verbosity>5) std::cout << "ANNIEEventBuilder Tool: Processing new tank data " << std::endl;
  for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>>> apair : *InProgressTankEvents){
    uint64_t PMTCounterTimeNs = apair.first;
    std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = apair.second;
    if(verbosity>4) std::cout << "Number of waves for this counter: " << aWaveMap.size() << std::endl;
    
    //Push back any new timestamps, then remove duplicates in the end
    if(PMTCounterTimeNs>NewestTimestamp){
      NewestTimestamp = PMTCounterTimeNs;
      if(verbosity>3)std::cout << "TANKTIMESTAMP," << PMTCounterTimeNs << std::endl;
    }
    //Events and delete it from the in-progress events
    int NumTankPMTChannels = TankPMTCrateSpaceToChannelNumMap.size();
    int NumAuxChannels = AuxCrateSpaceToChannelNumMap.size();
    if(aWaveMap.size() >= (NumWavesInCompleteSet)){
      FinishedTankEvents.emplace(PMTCounterTimeNs,aWaveMap);
      BeamTankTimestamps.push_back(PMTCounterTimeNs);
      //Put PMT timestamp into the timestamp set for this run.
      if(verbosity>4) std::cout << "Finished waveset has clock counter: " << PMTCounterTimeNs << std::endl;
      InProgressTankEventsToDelete.push_back(PMTCounterTimeNs);
    }

    //If this InProgressTankEvent is too old, clear it
    //out from all TankTimestamp maps
    //FIXME: This needs to move timestamps to the orphanage!!
    if(OrphanOldTankTimestamps && ((NewestTimestamp - PMTCounterTimeNs) > OldTimestampThreshold*1E9)){
      InProgressTankEventsToDelete.push_back(PMTCounterTimeNs);
    }
  }
  RemoveDuplicates(BeamTankTimestamps);
  
  return InProgressTankEventsToDelete;
}

void ANNIEEventBuilder::RemoveCosmics(){
  std::vector<uint64_t> MRDStampsToDelete;
  for (int i=0;i<(BeamMRDTimestamps.size()); i++) {
    std::string MRDTriggerType = MRDTriggerTypeMap[BeamMRDTimestamps.at(i)];
    if(verbosity>4) std::cout << "THIS MRD TRIGGER TYPE IS: " << MRDTriggerType << std::endl;
    if(MRDTriggerType == "Cosmic"){
      MRDStampsToDelete.push_back(BeamMRDTimestamps.at(i));
      if(verbosity>3){
        std::cout << "NO BEAM COSMIC COSMIC IS: " << BeamMRDTimestamps.at(i) << std::endl;
        std::cout << "DELETING FROM TIMESTAMPS TO PAIR/BUILD" << std::endl;
      }
    }
  }
  for (int j=0; j<MRDStampsToDelete.size(); j++){
    BeamMRDTimestamps.erase(std::remove(BeamMRDTimestamps.begin(),BeamMRDTimestamps.end(),MRDStampsToDelete.at(j)), 
               BeamMRDTimestamps.end());
    MRDEvents.erase(MRDStampsToDelete.at(j));
    MRDTriggerTypeMap.erase(MRDStampsToDelete.at(j));
  }
  return;
}


void ANNIEEventBuilder::MergeStreams(){
  //This method takes timestamps from the BeamMRDTimestamps, BeamTankTimestamps, and
  //CTCTimestamps vectors (acquired as the building continues) and builds maps 
  //stored in the BuildMap and used to build ANNIEEvents.
 
  int CTCMRDTolerance = 5;  //ms
  int CTCTankTolerance = 100; //ns

  //First, pair up CTCTimestamps and MRD/PMT timestamps
  std::map<uint64_t,uint64_t> PairedCTCTankTimes; 
  std::map<uint64_t,uint64_t> PairedCTCMRDTimes;
  int j = 0;
  int MRDSize = BeamMRDTimestamps.size();
  for(int i=0; i<EventsPerPairing; i++){
    while(j<MRDSize){
      double TSDiff = (static_cast<double>(CTCTimestamps.at(i)/1E6) - 21600000.0) - static_cast<double>(BeamMRDTimestamps.at(i));
      if(verbosity>4) std::cout << "CTC,MRDTIMESTAMP (NS): " << static_cast<double>((CTCTimestamps.at(i)/1E6) - 21600000.0) << "," << static_cast<double>(BeamMRDTimestamps.at(i)) << std::endl;
      if( abs(TSDiff) < CTCMRDTolerance){
        if(verbosity>4) std::cout << "CTC,MRD TIMESTAMP FORMED" << std::endl;
        PairedCTCMRDTimes.emplace(CTCTimestamps.at(i), BeamMRDTimestamps.at(j));
        BeamMRDTimestamps.erase(BeamMRDTimestamps.begin() + j);
        MRDSize-=1;
        j-=1;
        break;
      }
      j+=1;
    }
  }
  j = 0;
  int PMTSize = BeamTankTimestamps.size();
  for(int i=0; i<EventsPerPairing; i++){
    while(j<PMTSize){
      double TSDiff = (static_cast<double>(CTCTimestamps.at(i)) - static_cast<double>(BeamTankTimestamps.at(j)));
      if(verbosity>4) std::cout << "CTC,TANK TIMESTAMPS (NS): " << static_cast<double>(CTCTimestamps.at(i)) << "," << static_cast<double>(BeamTankTimestamps.at(j)) << std::endl;
      if( abs(TSDiff) < CTCTankTolerance){
        PairedCTCTankTimes.emplace(CTCTimestamps.at(i), BeamTankTimestamps.at(j));
        BeamTankTimestamps.erase(BeamTankTimestamps.begin() + j);
        PMTSize-=1;
        j-=1;
        break;
      }
      j+=1;
    }
  }

  //Neat.  Now, add timestamps to the buildmap.
  for(int i=0; i<EventsPerPairing; i++){
    uint64_t CTCKey = CTCTimestamps.at(i);
    bool buildSetMade = false;
    std::map<std::string,uint64_t> aBuildSet;
    //See if there's an MRD or PMT timestamp associated with this CTC time
    //Check if this CardData is next in it's sequence for processing
    aBuildSet.emplace("CTC",TimeToTriggerWordMap->at(CTCKey));
    std::map<uint64_t, uint64_t>::iterator it = PairedCTCTankTimes.find(CTCKey);
    if(it != PairedCTCTankTimes.end()){ //Have PMT data for this CTC timestamp
      aBuildSet.emplace("TankPMT",PairedCTCTankTimes.at(it->second));
      buildSetMade = true;
    }
    std::map<uint64_t, uint64_t>::iterator it2 = PairedCTCMRDTimes.find(CTCKey);
    if(it2 != PairedCTCMRDTimes.end()){ //Have PMT data for this CTC timestamp
      buildSetMade = true;
      aBuildSet.emplace("MRD",PairedCTCTankTimes.at(it2->second));
    }
    if(buildSetMade) {
      BuildMap.emplace(CTCKey,aBuildSet);
    } else {
      OrphanCTCTimeWordPairs.emplace(CTCKey,TimeToTriggerWordMap->at(CTCKey));
      TimeToTriggerWordMap->erase(CTCKey);
    }
  }
  return;
}


void ANNIEEventBuilder::PairTankPMTAndMRDTriggers(){
  if(verbosity>4) std::cout << "ANNIEEventBuilder Tool: Beginning to pair events" << std::endl;
  
  std::vector<uint64_t> TankStampsToDelete;
  std::vector<uint64_t> MRDStampsToDelete;
  int NumPairsToMake = EventsPerPairing;
  std::vector<double> ThisPairingTSDiffs;

  int NumOrphans = 0;
  if(verbosity>4) std::cout << "MEAN OF PMT-MRD TIME DIFFERENCE LAST LOOP: " << CurrentDriftMean << std::endl;
  for (int i=0;i<(NumPairsToMake); i++) {
    double TSDiff = (static_cast<double>(BeamTankTimestamps.at(i)/1E6) - 21600000.0) - static_cast<double>(BeamMRDTimestamps.at(i));
    if(verbosity>4){
      std::cout << "PAIRED TANK TIMESTAMP: " << BeamTankTimestamps.at(i) << std::endl;
      std::cout << "PAIRED MRD TIMESTAMP: " << BeamMRDTimestamps.at(i) << std::endl;
      std::cout << "DIFFERENCE BETWEEN PMT AND MRD TIMESTAMP (ms): " << 
      (((BeamTankTimestamps.at(i)/1E6) - 21600000) - BeamMRDTimestamps.at(i)) << std::endl;
    }
    if(std::abs(TSDiff-CurrentDriftMean) > MRDPMTTimeDiffTolerance){
      if(verbosity>3) std::cout << "DEVIATION OF " << MRDPMTTimeDiffTolerance << " ms DETECTED IN STREAMS!" << std::endl;
      if(TSDiff > 0) {
        if(verbosity>3) std::cout << "MOVING MRD TIMESTAMP TO ORPHANAGE" << std::endl;
        OrphanMRDTimestamps.push_back(BeamMRDTimestamps.at(i));
        NumOrphans +=1;
        BeamMRDTimestamps.erase(std::remove(BeamMRDTimestamps.begin(),BeamMRDTimestamps.end(),BeamMRDTimestamps.at(i)), 
             BeamMRDTimestamps.end());
      } else {
        if(verbosity>3) std::cout << "MOVING TANK TIMESTAMP TO ORPHANAGE" << std::endl;
        OrphanTankTimestamps.push_back(BeamTankTimestamps.at(i));
        NumOrphans +=1;
        BeamTankTimestamps.erase(std::remove(BeamTankTimestamps.begin(),BeamTankTimestamps.end(),BeamTankTimestamps.at(i)), 
             BeamTankTimestamps.end());
      }
      i-=1;
      NumPairsToMake-=1;
    } else {
      ThisPairingTSDiffs.push_back(TSDiff);
    }
  }

  //With the last set of pairs calculate what the mean drift is
  //TODO: Error handling for high drift and high orphan rates?
  //TODO: Try to pair up orphans at some point?
  double ThisPairingMean, ThisPairingVariance;
  if(ThisPairingTSDiffs.size()>2){
    ComputeMeanAndVariance(ThisPairingTSDiffs,CurrentDriftMean,CurrentDriftVariance);
    ComputeMeanAndVariance(ThisPairingTSDiffs,ThisPairingMean,ThisPairingVariance);
    if(std::abs(CurrentDriftMean-ThisPairingMean)>DriftWarningValue){
      std::cout << "ANNIEEventBuilder tool: WARNING! Shift in drift greater than " << DriftWarningValue << " since last pairings." << std::endl;
    }
    if(NumOrphans > OrphanWarningValue){
      std::cout << "ANNIEEventBuilder tool: WARNING! High orphan rate detected.  More than " << OrphanWarningValue << " this pairing sequence." << std::endl;
    }
    CurrentDriftMean = ThisPairingMean;
    CurrentDriftVariance = ThisPairingVariance;
  }
  if(verbosity>4) std::cout << "DOING OUR PAIR UP: " << std::endl;
  for (int i=0;i<(NumPairsToMake); i++) {
    BeamTankMRDPairs.emplace(BeamTankTimestamps.at(i),BeamMRDTimestamps.at(i));
    TankStampsToDelete.push_back(BeamTankTimestamps.at(i));
    MRDStampsToDelete.push_back(BeamMRDTimestamps.at(i));
  }

  if(verbosity>4) std::cout << "DELETE PAIRED TIMESTAMPS FROM THE UNPAIRED VECTORS: " << std::endl;
  //Delete all paired timestamps still in the unpaired unbuilt timestamps vectors 
  for (int i=0; i<(NumPairsToMake); i++){
    uint64_t BuiltTankTime = TankStampsToDelete.at(i);
    uint64_t BuiltMRDTime = MRDStampsToDelete.at(i);
    BeamTankTimestamps.erase(std::remove(BeamTankTimestamps.begin(),BeamTankTimestamps.end(),BuiltTankTime), 
               BeamTankTimestamps.end());
    BeamMRDTimestamps.erase(std::remove(BeamMRDTimestamps.begin(),BeamMRDTimestamps.end(),BuiltMRDTime), 
               BeamMRDTimestamps.end());
  }
  if(verbosity>4) std::cout << "FINISHED ERASING PAIRED TIMESTAMPS FROM FINISHED VECTORS" << std::endl;

  return;
}


void ANNIEEventBuilder::BuildANNIEEventMRD(std::vector<std::pair<unsigned long,int>> MRDHits, 
        unsigned long MRDTimeStamp, std::string MRDTriggerType)
{
  std::cout << "Building an ANNIE Event (MRD), ANNIEEventNum = "<<ANNIEEventNum << std::endl;
  TDCData = new std::map<unsigned long, std::vector<Hit>>;

  //TODO: Loop through MRDHits at this timestamp and form the Hit vector.
  for (unsigned int i_value=0; i_value< MRDHits.size(); i_value++){
    unsigned long channelkey = MRDHits.at(i_value).first;
    int hitTimeADC = MRDHits.at(i_value).second;
    double hitTime = 4000. - 4.*(double)hitTimeADC;
    if (verbosity > 4) std::cout <<"creating hit with ADC value "<<hitTimeADC<<"and chankey "<<channelkey<<std::endl;
    if (verbosity > 3) std::cout <<"creating hit with time vlaue "<<hitTime<<"and chankey "<<channelkey<<std::endl;
    if (TDCData->count(channelkey)==0){
      std::vector<Hit> newhitvector;
      newhitvector.push_back(Hit(0,hitTime,1.));    //Hit(tubeid, time, charge). 1 TDC tick corresponds to 4ns, no charge information (set to 1)
      TDCData->emplace(channelkey,newhitvector);
    } else {
      TDCData->at(channelkey).push_back(Hit(0,hitTime,1.));
    }
  }

  Log("ANNIEEventBuilder: TDCData size: "+std::to_string(TDCData->size()),v_debug,verbosity);

  ANNIEEvent->Set("TDCData",TDCData,true);
  TimeClass timeclass_timestamp((uint64_t)MRDTimeStamp*1000);  //in microseconds
  ANNIEEvent->Set("EventTime",timeclass_timestamp); //not sure if EventTime is also in UTC or defined differently
  ANNIEEvent->Set("MRDTriggerType",MRDTriggerType);
  return;
}

void ANNIEEventBuilder::BuildANNIEEventRunInfo(int RunNumber, int SubRunNumber,
        int RunType, uint64_t StartTime)
{
  if(verbosity>v_message)std::cout << "Building an ANNIE Event Run Info" << std::endl;
  ANNIEEvent->Set("EventNumber",ANNIEEventNum);
  ANNIEEvent->Set("RunNumber",RunNumber);
  ANNIEEvent->Set("SubrunNumber",SubRunNumber);
  ANNIEEvent->Set("RunType",RunType);
  ANNIEEvent->Set("RunStartTime",StartTime);
  //TODO: Things missing from ANNIEEvent that should be in before this tool finishes:
  //  - BeamStatus?  
  return;
}

void ANNIEEventBuilder::BuildANNIEEventCTC(uint64_t CTCTime, uint32_t CTCWord)
{
  if(verbosity>v_message)std::cout << "Building an ANNIE Event CTC Info" << std::endl;
  ANNIEEvent->Set("CTCTimestamp",CTCTime);
  ANNIEEvent->Set("TriggerWord",CTCWord);
  return;
}

void ANNIEEventBuilder::BuildANNIEEventTank(uint64_t ClockTime, 
        std::map<std::vector<int>, std::vector<uint16_t>> WaveMap)
{
  if(verbosity>v_message)std::cout << "Building an ANNIE Event" << std::endl;

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
  std::cout << "Setting ANNIE Event information" << std::endl;
  ANNIEEvent->Set("RawADCData",RawADCData);
  ANNIEEvent->Set("RawADCAuxData",RawADCAuxData);
  ANNIEEvent->Set("EventTimeTank",ClockTime);
  //TODO: Things missing from ANNIEEvent that should be in before this tool finishes:
  //  - EventTime
  //  - TriggerData
  //  - RawLAPPDData
  if(verbosity>v_debug) std::cout << "ANNIEEventBuilder: ANNIE Event "+
      to_string(ANNIEEventNum)+" built." << std::endl;
  return;
}


void ANNIEEventBuilder::SaveEntryToFile(int RunNum, int SubRunNum)
{
  /*if(verbosity>4)*/ std::cout << "ANNIEEvent: Saving ANNIEEvent entry"+to_string(ANNIEEventNum) << std::endl;
  std::string Filename = SavePath + ProcessedFilesBasename + "R" + to_string(RunNum) + 
      "S" + to_string(SubRunNum);
  ANNIEEvent->Save(Filename);
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

void ANNIEEventBuilder::OpenNewANNIEEvent(int RunNum, int SubRunNum,uint64_t StarT, int RunT){
  uint64_t StarTime;
  int RunType;
  if(verbosity>v_warning) std::cout << "ANNIEEventBuilder: New run or subrun encountered. Opening new BoostStore" << std::endl;
  if(verbosity>v_debug) std::cout << "ANNIEEventBuilder: Current run,subrun:" << CurrentRunNum << "," << CurrentSubRunNum << std::endl;
  if(verbosity>v_debug) std::cout << "ANNIEEventBuilder: Encountered run,subrun:" << RunNum << "," << SubRunNum << std::endl;
  ANNIEEvent->Close();
  ANNIEEvent->Delete();
  delete ANNIEEvent; ANNIEEvent = new BoostStore(false,2);
  CurrentRunNum = RunNum;
  CurrentSubRunNum = SubRunNum;
  CurrentRunType = RunT;
  CurrentStarTime = StarT;
  if((CurrentRunNum != RunNum)) CurrentDriftMean = 0;
}
