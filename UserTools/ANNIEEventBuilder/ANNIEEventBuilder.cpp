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
  MRDPMTTimeDiffTolerance = 10;   //ms
  RoughScanMean = 200;
  RoughScanVariance = 50000;

  /////////////////////////////////////////////////////////////////
  //FIXME: Need rough scan and tolerances in variable settings
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("SavePath",SavePath);
  m_variables.Get("ProcessedFilesBasename",ProcessedFilesBasename);
  m_variables.Get("BuildType",BuildType);
  m_variables.Get("NumEventsPerPairing",EventsPerPairing);

  if(BuildType == "TankAndMRD"){
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

  if(BuildType == "Tank" || BuildType == "TankAndMRD"){
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
  DataStreamsSynced = false;

  return true;
}


bool ANNIEEventBuilder::Execute(){

  if (BuildType == "Tank"){
    //Check to see if there's new PMT data
    m_data->CStore.Get("NewTankPMTDataAvailable",IsNewTankData);
    if(!IsNewTankData){
      Log("ANNIEEventBuilder:: No new Tank Data.  Not building ANNIEEvent. ",v_message, verbosity);
      return true;
    }
    //Get the current InProgressTankEvents map
    if(verbosity>4) std::cout << "ANNIEEventBuilder: Getting waves and run info from CStore" << std::endl;
    m_data->CStore.Get("InProgressTankEvents",InProgressTankEvents);
    m_data->CStore.Get("TankRunInfoPostgress",RunInfoPostgress);
    int RunNumber;
    int SubRunNumber;
    uint64_t StarTime;
    int RunType;
    RunInfoPostgress.Get("RunNumber",RunNumber);
    RunInfoPostgress.Get("SubRunNumber",SubRunNumber);
    RunInfoPostgress.Get("RunType",RunType);
    RunInfoPostgress.Get("StarTime",StarTime);

    //Initialize Current RunNum and SubrunNum. New ANNIEEvent for any New Run or Subrun
    if(CurrentRunNum == -1) CurrentRunNum = RunNumber;
    if(CurrentSubRunNum == -1) CurrentSubRunNum = SubRunNumber;
    //If we're in a new run or subrun, make a new ANNIEEvent file. 
    if((CurrentRunNum != RunNumber) || (CurrentSubRunNum != SubRunNumber)){
      if(verbosity>v_warning) std::cout << "New run or subrun encountered. Opening new BoostStore" << std::endl;
      if(verbosity>v_warning) std::cout << "ANNIEEventBuilder: Saving and closing file." << std::endl;
      ANNIEEvent->Close();
      ANNIEEvent->Delete();
      delete ANNIEEvent; ANNIEEvent = new BoostStore(false,2);
      CurrentRunNum = RunNumber;
      CurrentSubRunNum = SubRunNumber;
    }

    //Assume a whole processed file will have all it's PMT data finished
    std::vector<uint64_t> PMTEventsToDelete;
    for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>>> apair : InProgressTankEvents){
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

    for(unsigned int i=0; i< PMTEventsToDelete.size(); i++) InProgressTankEvents.erase(PMTEventsToDelete.at(i));
    //Update the current InProgressTankEvents map
    if(verbosity>3) std::cout << "Current number of unfinished PMT Waveforms: " << InProgressTankEvents.size() << std::endl;
    m_data->CStore.Set("InProgressTankEvents",InProgressTankEvents);

  } else if (BuildType == "MRD"){
    m_data->CStore.Get("NewMRDDataAvailable",IsNewMRDData);
    std::vector<unsigned long> MRDEventsToDelete;
    if(!IsNewMRDData){
      Log("ANNIEEventBuilder:: No new MRD Data.  Not building ANNIEEvent: ",v_message, verbosity);
      return true;
    }
    m_data->CStore.Get("MRDEvents",MRDEvents);
    m_data->CStore.Get("MRDEventTriggerTypes",TriggerTypeMap);
    
    m_data->CStore.Get("MRDRunInfoPostgress",RunInfoPostgress);
    int RunNumber;
    int SubRunNumber;
    uint64_t StarTime;
    int RunType;
    RunInfoPostgress.Get("RunNumber",RunNumber);
    RunInfoPostgress.Get("SubRunNumber",SubRunNumber);
    RunInfoPostgress.Get("RunType",RunType);
    RunInfoPostgress.Get("StarTime",StarTime);

   //Loop through MRDEvents and process each into ANNIEEvent.
    for(std::pair<unsigned long,std::vector<std::pair<unsigned long,int>>> apair : MRDEvents){
      unsigned long MRDTimeStamp = apair.first;
      std::vector<std::pair<unsigned long,int>> MRDHits = apair.second;
      std::string MRDTriggerType = TriggerTypeMap[MRDTimeStamp];
      this->BuildANNIEEventRunInfo(RunNumber,SubRunNumber,RunType,StarTime);
      this->BuildANNIEEventMRD(MRDHits, MRDTimeStamp, MRDTriggerType);
      this->SaveEntryToFile(RunNumber,SubRunNumber);
      //Erase this entry from the InProgressTankEventsMap
      MRDEventsToDelete.push_back(MRDTimeStamp);
    }
    for (unsigned int i=0; i< MRDEventsToDelete.size(); i++){
      MRDEvents.erase(MRDEventsToDelete.at(i));
      TriggerTypeMap.erase(MRDEventsToDelete.at(i));
    }
    m_data->CStore.Set("MRDEvents",MRDEvents);
    m_data->CStore.Set("MRDEventTriggerTypes",TriggerTypeMap);
  }

  else if (BuildType == "TankAndMRD"){

    //See if the MRD and Tank are at the same run/subrun for building
    m_data->CStore.Get("TankRunInfoPostgress",RunInfoPostgress);
    int TankRunNumber;
    int TankSubRunNumber;
    uint64_t TankStarTime;
    int TankRunType;
    RunInfoPostgress.Get("RunNumber",TankRunNumber);
    RunInfoPostgress.Get("SubRunNumber",TankSubRunNumber);
    RunInfoPostgress.Get("RunType",TankRunType);
    RunInfoPostgress.Get("StarTime",TankStarTime);
    
    m_data->CStore.Get("MRDRunInfoPostgress",RunInfoPostgress);
    int MRDRunNumber;
    int MRDSubRunNumber;
    uint64_t MRDStarTime;
    int MRDRunType;
    RunInfoPostgress.Get("RunNumber",MRDRunNumber);
    RunInfoPostgress.Get("SubRunNumber",MRDSubRunNumber);
    RunInfoPostgress.Get("RunType",MRDRunType);
    RunInfoPostgress.Get("StarTime",MRDStarTime);
    
    //If our first execute loop, Initialize Current run information with Tank decoding progress
    if(CurrentRunNum == -1 || CurrentSubRunNum == -1){
      CurrentRunNum = TankRunNumber;
      CurrentSubRunNum = TankSubRunNumber;
      CurrentStarTime = TankStarTime;
      CurrentRunType = TankRunType;
      LowestRunNum = TankRunNumber;
      LowestSubRunNum = TankSubRunNumber;
    }
  
    // Check that Tank and MRD decoding are on the same run/subrun.  If not, take the
    // lowest Run Number and Subrun number to keep building events
    if(verbosity>4) {
      std::cout << "TANK RUN,SUBRUN: " << TankRunNumber << "," << TankSubRunNumber << std::endl;
      std::cout << "MRD RUN,SUBRUN: " << MRDRunNumber << "," << MRDSubRunNumber << std::endl;
    }
    if (TankRunNumber != MRDRunNumber || TankSubRunNumber != MRDSubRunNumber){
      if((TankRunNumber*10000 + TankSubRunNumber) < (MRDRunNumber*10000 + MRDSubRunNumber)){
        LowestRunNum = TankRunNumber;
        LowestSubRunNum = TankSubRunNumber;
        LowestRunType = TankRunType;
        LowestStarTime = TankStarTime;
        m_data->CStore.Set("PauseMRDDecoding",true);
        m_data->CStore.Set("PauseTankDecoding",false);
      } else {
        LowestRunNum = MRDRunNumber;
        LowestSubRunNum = MRDSubRunNumber;
        LowestRunType = MRDRunType;
        LowestStarTime = MRDStarTime;
        m_data->CStore.Set("PauseTankDecoding",true);
        m_data->CStore.Set("PauseMRDDecoding",false);
      }
    } else {
      m_data->CStore.Set("PauseMRDDecoding",false);
      m_data->CStore.Set("PauseTankDecoding",false);
    }

    //If there's too many completed timestamps in either data type, pause it's decoding
    //FIXME: I think you could get into a state where the above logic and this method
    //Pause both streams indefinitely...
    this->PauseDecodingOnAheadStream();

    //If the Lowest Run/Subrun is new,  make a new ANNIEEvent file. 
    if((CurrentRunNum != LowestRunNum) || (CurrentSubRunNum != LowestSubRunNum)){
      if(verbosity>v_warning) std::cout << "PMT and MRD data have both finished run " <<
         CurrentRunNum << "," << CurrentSubRunNum << ".  Opening new BoostStore" << std::endl;
      if(verbosity>v_warning) std::cout << "ANNIEEventBuilder: Saving and closing file." << std::endl;
      ANNIEEvent->Close();
      ANNIEEvent->Delete();
      delete ANNIEEvent; ANNIEEvent = new BoostStore(false,2);
      CurrentRunNum = LowestRunNum;
      CurrentSubRunNum = LowestSubRunNum;
      CurrentRunType = LowestRunType;
      CurrentStarTime = LowestStarTime;
      DataStreamsSynced = false;
    }


    //Check if any In-progress tank events now have all waveforms
    m_data->CStore.Get("InProgressTankEvents",InProgressTankEvents);
    std::vector<uint64_t> InProgressEventsToDelete;
    m_data->CStore.Get("NewTankPMTDataAvailable",IsNewTankData);
    if(IsNewTankData){
      if(verbosity>3) std::cout << "DataDecoder Tool: Processing new tank data " << std::endl;
      for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>>> apair : InProgressTankEvents){
        uint64_t PMTCounterTimeNs = apair.first;
        std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = apair.second;
        if(verbosity>4) std::cout << "Number of waves for this counter: " << aWaveMap.size() << std::endl;
        
        //See if this timestamp has been encountered before; if not,
        //add it to our Tank timestamp record and our in-progress timestamps
        if(std::find(IPTankTimestamps.begin(),IPTankTimestamps.end(), PMTCounterTimeNs) == 
                     IPTankTimestamps.end()){
          IPTankTimestamps.push_back(PMTCounterTimeNs);  //Units in ns
        }
        if(std::find(AllTankTimestamps.begin(),AllTankTimestamps.end(), PMTCounterTimeNs) == 
                     AllTankTimestamps.end()){
          if(verbosity>3)std::cout << "TANKTIMESTAMP," << PMTCounterTimeNs << std::endl;
          AllTankTimestamps.push_back(PMTCounterTimeNs);  //Units in ns
          UnpairedTankTimestamps.push_back(PMTCounterTimeNs);  //Units in ns
        }

        //If this trigger has all of it's waveforms, add it to the finished
        //Events and delete it from the in-progress events
        int NumTankPMTChannels = TankPMTCrateSpaceToChannelNumMap.size();
        int NumAuxChannels = AuxCrateSpaceToChannelNumMap.size();
        if(aWaveMap.size() >= (NumWavesInCompleteSet)){
          FinishedTankEvents.emplace(PMTCounterTimeNs,aWaveMap);
          FinishedTankTimestamps.push_back(PMTCounterTimeNs);
          //Put PMT timestamp into the timestamp set for this run.
          if(verbosity>4) std::cout << "Finished waveset has clock counter: " << PMTCounterTimeNs << std::endl;
          InProgressEventsToDelete.push_back(PMTCounterTimeNs);
        }
      }
    }
    
    //Look through our MRD data for any new timestamps
    m_data->CStore.Get("MRDEventTriggerTypes",TriggerTypeMap);
    m_data->CStore.Get("NewMRDDataAvailable",IsNewMRDData);
    m_data->CStore.Get("MRDEvents",MRDEvents);
    if(IsNewMRDData){
      for(std::pair<unsigned long,std::vector<std::pair<unsigned long,int>>> apair : MRDEvents){
        unsigned long MRDTimeStamp = apair.first;
        std::vector<std::pair<unsigned long,int>> MRDHits = apair.second;
        //See if this MRD timestamp has been encountered before; if not,
        //add it to our MRD timestamp record
        if(std::find(AllMRDTimestamps.begin(),AllMRDTimestamps.end(), MRDTimeStamp) == 
                     AllMRDTimestamps.end()){
          AllMRDTimestamps.push_back(MRDTimeStamp);
          UnpairedMRDTimestamps.push_back(MRDTimeStamp);
          if(verbosity>3)std::cout << "MRDTIMESTAMPTRIGTYPE," << MRDTimeStamp << "," << TriggerTypeMap.at(MRDTimeStamp) << std::endl;
        }
      }
    }
    //Since timestamp pairing has been done for finished Tank Events,
    //Erase the finished Tank Events from the InProgressTankEventsMap
    for (unsigned int j=0; j< InProgressEventsToDelete.size(); j++){
      IPTankTimestamps.erase(std::remove(IPTankTimestamps.begin(),IPTankTimestamps.end(),InProgressEventsToDelete.at(j)), 
                 IPTankTimestamps.end());
      InProgressTankEvents.erase(InProgressEventsToDelete.at(j));
    }
    if(verbosity>3) std::cout << "Current number of unfinished PMT Waveforms: " << InProgressTankEvents.size() << std::endl;

    
    //Now, pair up PMT and MRD events...
    int NumTankTimestamps = UnpairedTankTimestamps.size();
    int NumMRDTimestamps = UnpairedMRDTimestamps.size();
    int MinStamps = std::min(NumTankTimestamps,NumMRDTimestamps);
    std::vector<uint64_t> MRDStampsToDelete;

    if(MinStamps > (EventsPerPairing*5)){
      this->RemoveCosmics();
      //FIXME: Before we use this, two things are needed:
      // SyncDataStreams should probably move events to the orphanage rather than just delete
      // SyncDataStreams needs to be smarter; look at the current mean with no shift to
      // Predict the way the rough shift should be done
      //if(DataStreamsSynced)  this->CheckDataStreamsAreSynced();
      if(!DataStreamsSynced) this->SyncDataStreams();
      if(DataStreamsSynced)  this->PairTankPMTAndMRDTriggers();
  

      std::vector<uint64_t> BuiltTankTimes;
      for(std::pair<uint64_t,uint64_t> cpair : UnbuiltTankMRDPairs){
        uint64_t TankCounterTime = cpair.first;
        if(std::find(FinishedTankTimestamps.begin(),FinishedTankTimestamps.end(), TankCounterTime) == 
                     FinishedTankTimestamps.end()){
          continue;
        }
        if(verbosity>4) std::cout << "TANK EVENT WITH TIMESTAMP " << TankCounterTime << "HAS REQUIRED MINIMUM NUMBER OF WAVES TO BUILD" << std::endl;
        std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = FinishedTankEvents.at(TankCounterTime);
        uint64_t MRDTimeStamp = cpair.second;
        if(verbosity>4) std::cout << "MRD TIMESTAMP: " << MRDTimeStamp << std::endl;
        std::vector<std::pair<unsigned long,int>> MRDHits = MRDEvents.at(MRDTimeStamp);
        std::string MRDTriggerType = TriggerTypeMap[MRDTimeStamp];
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
        TriggerTypeMap.erase(MRDTimeStamp);
        FinishedTankTimestamps.erase(std::remove(FinishedTankTimestamps.begin(),FinishedTankTimestamps.end(),TankCounterTime), 
                   FinishedTankTimestamps.end());
      }
      for(int i=0; i<BuiltTankTimes.size(); i++){
        UnbuiltTankMRDPairs.erase(BuiltTankTimes.at(i));
      }
    }
  
  }
  
  m_data->CStore.Set("InProgressTankEvents",InProgressTankEvents);
  m_data->CStore.Set("MRDEvents",MRDEvents);
  m_data->CStore.Set("MRDEventTriggerTypes",TriggerTypeMap);
  InProgressTankEvents.clear();
  MRDEvents.clear();
  TriggerTypeMap.clear();
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

void ANNIEEventBuilder::PauseDecodingOnAheadStream(){
  int NumTankTimestamps = FinishedTankTimestamps.size();
  int NumMRDTimestamps = UnpairedMRDTimestamps.size(); 
  if((NumTankTimestamps - NumMRDTimestamps) > 10) m_data->CStore.Set("PauseTankDecoding",true);
  else if((NumMRDTimestamps - NumTankTimestamps) > 10) m_data->CStore.Set("PauseMRDDecoding",true);
  else {
    m_data->CStore.Set("PauseTankDecoding",false);
    m_data->CStore.Set("PauseMRDDecoding",false);
  }
  return;
}

void ANNIEEventBuilder::RemoveCosmics(){
  std::vector<uint64_t> MRDStampsToDelete;
  for (int i=0;i<(UnpairedMRDTimestamps.size()); i++) {
    std::string MRDTriggerType = TriggerTypeMap[UnpairedMRDTimestamps.at(i)];
    if(verbosity>4) std::cout << "THIS MRD TRIGGER TYPE IS: " << MRDTriggerType << std::endl;
    if(MRDTriggerType == "Cosmic"){
      MRDStampsToDelete.push_back(UnpairedMRDTimestamps.at(i));
      if(verbosity>3){
        std::cout << "NO BEAM COSMIC COSMIC IS: " << UnpairedMRDTimestamps.at(i) << std::endl;
        std::cout << "DELETING FROM TIMESTAMPS TO PAIR/BUILD" << std::endl;
      }
    }
  }
  for (int j=0; j<MRDStampsToDelete.size(); j++){
    UnpairedMRDTimestamps.erase(std::remove(UnpairedMRDTimestamps.begin(),UnpairedMRDTimestamps.end(),MRDStampsToDelete.at(j)), 
               UnpairedMRDTimestamps.end());
    TriggerTypeMap.erase(MRDStampsToDelete.at(j));
  }
  return;
}

void ANNIEEventBuilder::SyncDataStreams(){
  //The MRD and PMT datastreams can start at different times, and have different offsets.  
  //Let's try to find the offset needed to put the data streams in sync for building.
  //We'll write the logic assuming the MRD acquisition started prior to the PMT
  //acquisition.
  int SyncIndex = -9999;
  //int NumTankTimestamps = FinishedTankTimestamps.size();
  int NumTankTimestamps = UnpairedTankTimestamps.size();
  int NumMRDTimestamps = UnpairedMRDTimestamps.size();
  int MinStamps = std::min(NumTankTimestamps,NumMRDTimestamps);
  
  if(verbosity>4) std::cout << "ANNIEEventBuilder Tool: Trying to sync data streams" << std::endl;
  std::sort(UnpairedMRDTimestamps.begin(),UnpairedMRDTimestamps.end());
  //std::sort(UnpairedTankTimestamps.begin(),UnpairedTankTimestamps.end());
  std::sort(UnpairedTankTimestamps.begin(),UnpairedTankTimestamps.end());

  //Build offsets vectors used to sync streams
  std::vector<uint64_t> PMTOffsets;
  std::vector<uint64_t> MRDOffsets;
  for (int j=1; j<EventsPerPairing*2; j++){
    PMTOffsets.push_back((UnpairedTankTimestamps.at(j)/1E6) - (UnpairedTankTimestamps.at(j-1)/1E6));
    MRDOffsets.push_back(UnpairedMRDTimestamps.at(j)- UnpairedMRDTimestamps.at(j-1));
  }
  double BestMean = 1E100;
  double BestVar = 1E100;
  double BestIndex = -9999;
 
  //TO-DO: We should look at the overall mean of the unshifted differnces.
  //Depending on the sign, choose whether to slide the MRD or PMT dataset
  for (int i=0; i<EventsPerPairing; i++){
    //slide earliest times in MRD offset off the left
    std::vector<uint64_t> MRDTS_copy = UnpairedMRDTimestamps;
    std::vector<uint64_t> PMTTS_copy = UnpairedTankTimestamps;
    std::vector<uint64_t> SlidMRDOffsets = MRDOffsets;
    std::vector<uint64_t> SlidPMTOffsets = PMTOffsets;
    //std::cout << "SLIDING OFFSETS" << std::endl;
    SlidMRDOffsets.erase(SlidMRDOffsets.begin(),SlidMRDOffsets.begin()+i);
    SlidPMTOffsets.erase(SlidPMTOffsets.end()-i,SlidPMTOffsets.end());
    MRDTS_copy.erase(MRDTS_copy.begin(),MRDTS_copy.begin()+i);
    PMTTS_copy.erase(PMTTS_copy.end()-i,PMTTS_copy.end());
    std::vector<double> TSDifferences;
    std::vector<double> OffsetDifferences;
    for (int k=0; k<SlidPMTOffsets.size(); k++){
      if(verbosity>4){ 
        std::cout << "PMT-MRD TIMESTAMP: " << static_cast<double>((PMTTS_copy.at(k)/1E6) - 21600000) - 
                static_cast<double>(MRDTS_copy.at(k))  << std::endl;
        std::cout << "OFFSET DIFF: " << static_cast<double>(SlidPMTOffsets.at(k)) - 
                static_cast<double>(SlidMRDOffsets.at(k))  << std::endl;
      }
      OffsetDifferences.push_back(static_cast<double>(SlidPMTOffsets.at(k)) - static_cast<double>(SlidMRDOffsets.at(k)));
      TSDifferences.push_back(static_cast<double>((PMTTS_copy.at(k)/1E6) - 21600000) - static_cast<double>(MRDTS_copy.at(k)));
    }
    double tmean,tvar;
    double omean,ovar;
    ComputeMeanAndVariance(TSDifferences,tmean,tvar);
    ComputeMeanAndVariance(OffsetDifferences,omean,ovar);
    if(verbosity>4){
      std::cout << "SHIFTING MRD ARRAY BACK BY " << i << " INDICES..." << std::endl;
      std::cout << "MEAN TS SHIFT: " << tmean << std::endl;
      std::cout << "VARIANCE TS SHIFT: " << tvar << std::endl;
      std::cout << "MEAN OF OFFSETS: " << omean << std::endl;
      std::cout << "VARIANCE OF OFFSETS: " << ovar << std::endl;
    }
    if(tvar<BestVar){
      BestVar = tvar;
      BestMean = tmean;
      BestIndex = i;
    }
  }
  if(verbosity>3){
    std::cout << "BEST MEAN OF OFFSETS: " << BestMean << std::endl;
    std::cout << "BEST VARIANCE OF OFFSETS: " << BestVar << std::endl;
    std::cout << "BEST INDEX OF OFFSETS: " << BestIndex << std::endl;
  }
  if(BestVar < RoughScanVariance && BestMean < RoughScanMean){
    if(verbosity>3) std::cout << "VARIANCE AND MEAN MEET ROUGH SCAN CRITERIA.  CONSIDERED SYNCED" << std::endl;
    UnpairedMRDTimestamps.erase(UnpairedMRDTimestamps.begin(),(UnpairedMRDTimestamps.begin()+BestIndex));
    CurrentDriftMean = BestMean;
    CurrentDriftVariance = BestVar;
    DataStreamsSynced = true;
  }
  else {
    if(verbosity>3) std::cout << "NO MRD STREAM SHIFT BEATS MEAN/VARIANCE CRITERIA OF  " << RoughScanMean << ","<< RoughScanVariance << std::endl;
    UnpairedMRDTimestamps.erase(UnpairedMRDTimestamps.begin(),(UnpairedMRDTimestamps.begin()+EventsPerPairing));
    DataStreamsSynced = false;
  }
  return;
}

void ANNIEEventBuilder::CheckDataStreamsAreSynced(){
  //The MRD and PMT datastreams can start at different times, and have different offsets.  
  //Let's try to find the offset needed to put the data streams in sync for building.
  //We'll write the logic assuming the MRD acquisition started prior to the PMT
  //acquisition.
  
  if(verbosity>4) std::cout << "ANNIEEventBuilder Tool: Checking sync quality of stream" << std::endl;
  std::sort(UnpairedMRDTimestamps.begin(),UnpairedMRDTimestamps.end());
  //std::sort(FinishedTankTimestamps.begin(),FinishedTankTimestamps.end());
  std::sort(UnpairedTankTimestamps.begin(),UnpairedTankTimestamps.end());

  double BestMean = 1E100;
  double BestVar = 1E100;
  double BestIndex = -9999;
  //slide earliest times in MRD offset off the left
  std::vector<uint64_t> MRDTS_copy = UnpairedMRDTimestamps;
  std::vector<uint64_t> PMTTS_copy = UnpairedTankTimestamps;
  std::vector<double> TSDifferences;
  //std::cout << "CALCULATING MEAN AND VARIANCE OFFSETS" << std::endl;
  for (int k=0; k<(EventsPerPairing/2); k++){
    std::cout << "PMT-MRD TIMESTAMP: " << static_cast<double>((PMTTS_copy.at(k)/1E6))-21600000.0 - 
            static_cast<double>(MRDTS_copy.at(k))  << std::endl;

    TSDifferences.push_back(static_cast<double>(PMTTS_copy.at(k)/1E6) - static_cast<double>(MRDTS_copy.at(k)));
  }
  double tmean,tvar;
  double omean,ovar;
  ComputeMeanAndVariance(TSDifferences,tmean,tvar);
  std::cout << "MEAN TS SHIFT: " << tmean << std::endl;
  std::cout << "VARIANCE TS SHIFT: " << tvar << std::endl;
  if(tvar < RoughScanVariance){
    DataStreamsSynced = true;
  }
  else {
    if(verbosity>3){
      std::cout << "VARIANCE OF TIMESTAMP DIFFERENCES HAS GROWN PAST " << RoughScanVariance << std::endl;
      std::cout << "STREAM IS CONSIDERED OUT OF SYNC " << std::endl;
    }
    DataStreamsSynced = false;
  }
  return;
}

void ANNIEEventBuilder::PairTankPMTAndMRDTriggers(){
  //FIXME: I'm noticing a ~ms drift in the main sync pattern per 1000 events.  Let's have
  //A variable updated with the mean from all paired events in the previous iteration.  Add
  //This correction to the difference check, then update it with this built set's mean for
  //the next iteration of the method.`
  if(verbosity>4) std::cout << "ANNIEEventBuilder Tool: Beginning to pair events" << std::endl;
  //Organize Unpaired timestamps chronologically
  std::sort(UnpairedMRDTimestamps.begin(),UnpairedMRDTimestamps.end());
  std::sort(UnpairedTankTimestamps.begin(),UnpairedTankTimestamps.end());
  
  std::vector<uint64_t> TankStampsToDelete;
  std::vector<uint64_t> MRDStampsToDelete;
  int NumPairsToMake = EventsPerPairing;
  std::vector<double> ThisPairingTSDiffs;

  if(verbosity>4) std::cout << "MEAN OF PMT-MRD TIME DIFFERENCE LAST LOOP: " << CurrentDriftMean << std::endl;
  for (int i=0;i<(NumPairsToMake); i++) {
    double TSDiff = (static_cast<double>(UnpairedTankTimestamps.at(i)/1E6) - 21600000.0) - static_cast<double>(UnpairedMRDTimestamps.at(i));
    if(verbosity>4){
      std::cout << "PAIRED TANK TIMESTAMP: " << UnpairedTankTimestamps.at(i) << std::endl;
      std::cout << "PAIRED MRD TIMESTAMP: " << UnpairedMRDTimestamps.at(i) << std::endl;
      std::cout << "DIFFERENCE BETWEEN PMT AND MRD TIMESTAMP (ms): " << 
      (((UnpairedTankTimestamps.at(i)/1E6) - 21600000) - UnpairedMRDTimestamps.at(i)) << std::endl;
    }
    if(std::abs(TSDiff-CurrentDriftMean) > MRDPMTTimeDiffTolerance){
      if(verbosity>3) std::cout << "DEVIATION OF " << MRDPMTTimeDiffTolerance << " ms DETECTED IN STREAMS!" << std::endl;
      if(TSDiff > 0) {
        if(verbosity>3) std::cout << "MOVING MRD TIMESTAMP TO ORPHANAGE" << std::endl;
        OrphanMRDTimestamps.push_back(UnpairedMRDTimestamps.at(i));
        UnpairedMRDTimestamps.erase(std::remove(UnpairedMRDTimestamps.begin(),UnpairedMRDTimestamps.end(),UnpairedMRDTimestamps.at(i)), 
             UnpairedMRDTimestamps.end());
      } else {
        if(verbosity>3) std::cout << "MOVING TANK TIMESTAMP TO ORPHANAGE" << std::endl;
        OrphanTankTimestamps.push_back(UnpairedTankTimestamps.at(i));
        UnpairedTankTimestamps.erase(std::remove(UnpairedTankTimestamps.begin(),UnpairedTankTimestamps.end(),UnpairedTankTimestamps.at(i)), 
             UnpairedTankTimestamps.end());
      }
      i-=1;
      NumPairsToMake-=1;
    } else {
      ThisPairingTSDiffs.push_back(TSDiff);
    }
  }

  //With the last set of pairs calculate what the mean drift is
  ComputeMeanAndVariance(ThisPairingTSDiffs,CurrentDriftMean,CurrentDriftVariance);

  if(verbosity>4) std::cout << "DOING OUR PAIR UP: " << std::endl;
  for (int i=0;i<(NumPairsToMake); i++) {
    UnbuiltTankMRDPairs.emplace(UnpairedTankTimestamps.at(i),UnpairedMRDTimestamps.at(i));
    TankStampsToDelete.push_back(UnpairedTankTimestamps.at(i));
    MRDStampsToDelete.push_back(UnpairedMRDTimestamps.at(i));
  }

  if(verbosity>4) std::cout << "DELETE PAIRED TIMESTAMPS FROM THE UNPAIRED VECTORS: " << std::endl;
  //Delete all paired timestamps still in the unpaired unbuilt timestamps vectors 
  for (int i=0; i<(NumPairsToMake); i++){
    uint64_t BuiltTankTime = TankStampsToDelete.at(i);
    uint64_t BuiltMRDTime = MRDStampsToDelete.at(i);
    UnpairedTankTimestamps.erase(std::remove(UnpairedTankTimestamps.begin(),UnpairedTankTimestamps.end(),BuiltTankTime), 
               UnpairedTankTimestamps.end());
    UnpairedMRDTimestamps.erase(std::remove(UnpairedMRDTimestamps.begin(),UnpairedMRDTimestamps.end(),BuiltMRDTime), 
               UnpairedMRDTimestamps.end());
    TriggerTypeMap.erase(BuiltMRDTime);
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
    if (TDCData->count(channelkey)==0){
      std::vector<Hit> newhitvector;
      if (verbosity > 3) std::cout <<"creating hit with time value "<<hitTimeADC*4<<"and chankey "<<channelkey<<std::endl;
      newhitvector.push_back(Hit(0,hitTimeADC*4.,1.));    //Hit(tubeid, time, charge). 1 TDC tick corresponds to 4ns, no charge information (set to 1)
      TDCData->emplace(channelkey,newhitvector);
    } else {
      if (verbosity > 3) std::cout <<"creating hit with time value "<<hitTimeADC*4<<"and chankey "<<channelkey<<std::endl;
      TDCData->at(channelkey).push_back(Hit(0,hitTimeADC*4.,1.));
    }
  }

  Log("ANNIEEventBuilder: TDCData size: "+std::to_string(TDCData->size()),v_debug,verbosity);

  ANNIEEvent->Set("TDCData",TDCData,true);
  TimeClass timeclass_timestamp((uint64_t)MRDTimeStamp*1000);  //in ns
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
    if(verbosity>v_debug) std::cout << "Converting card ID " << CardID << ", channel ID " <<
          ChannelID << " to electronics space" << std::endl;
    this->CardIDToElectronicsSpace(CardID, CrateNum, SlotNum);
    std::vector<uint16_t> TheWaveform = apair.second;
    //FIXME: We're feeding Waveform class expects a double, not a uint64_t (?)
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
