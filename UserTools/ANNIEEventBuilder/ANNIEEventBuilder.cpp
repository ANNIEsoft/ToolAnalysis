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
    m_data->CStore.Get("MRDEventTriggerTypes",TriggerTypeMap);
    m_data->CStore.Get("MRDBeamLoopback",MRDBeamLoopbackMap);
    m_data->CStore.Get("MRDCosmicLoopback",MRDCosmicLoopbackMap);
    
   //Loop through MRDEvents and process each into ANNIEEvent.
    for(std::pair<unsigned long,std::vector<std::pair<unsigned long,int>>> apair : MRDEvents){
      unsigned long MRDTimeStamp = apair.first;
      std::vector<std::pair<unsigned long,int>> MRDHits = apair.second;
      std::string MRDTriggerType = TriggerTypeMap[MRDTimeStamp];
      int beam_tdc = MRDBeamLoopbackMap[MRDTimeStamp];
      int cosmic_tdc = MRDCosmicLoopbackMap[MRDTimeStamp];
      this->BuildANNIEEventRunInfo(RunNumber,SubRunNumber,RunType,StarTime);
      this->BuildANNIEEventMRD(MRDHits, MRDTimeStamp, MRDTriggerType, beam_tdc, cosmic_tdc);
      this->SaveEntryToFile(RunNumber,SubRunNumber);
      //Erase this entry from the InProgressTankEventsMap
      MRDEventsToDelete.push_back(MRDTimeStamp);
    }
    for (unsigned int i=0; i< MRDEventsToDelete.size(); i++){
      MRDEvents.erase(MRDEventsToDelete.at(i));
      TriggerTypeMap.erase(MRDEventsToDelete.at(i));
      MRDBeamLoopbackMap.erase(MRDEventsToDelete.at(i));
      MRDCosmicLoopbackMap.erase(MRDEventsToDelete.at(i));
    }
    m_data->CStore.Set("MRDEvents",MRDEvents);
    m_data->CStore.Set("MRDEventTriggerTypes",TriggerTypeMap);
    m_data->CStore.Set("MRDBeamLoopback",MRDBeamLoopbackMap);
    m_data->CStore.Set("MRDCosmicLoopback",MRDCosmicLoopbackMap);
  }

  else if (BuildType == "TankAndMRD"){

  
    //If there's too many completed timestamps in either data type, pause it's decoding
    //FIXME: I think you could get into a state where the above logic and this method
    //Pause both streams indefinitely...
    //this->PauseDecodingOnAheadStream();

    //Check if any In-progress tank events now have all waveforms
    m_data->CStore.Get("InProgressTankEvents",InProgressTankEvents);
    std::vector<uint64_t> InProgressEventsToDelete;
    m_data->CStore.Get("NewTankPMTDataAvailable",IsNewTankData);
    if(IsNewTankData){
      if(verbosity>3) std::cout << "ANNIEEventBuilder Tool: Processing new tank data " << std::endl;
      for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>>> apair : *InProgressTankEvents){
        uint64_t PMTCounterTimeNs = apair.first;
        std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = apair.second;
        if(verbosity>4) std::cout << "Number of waves for this counter: " << aWaveMap.size() << std::endl;
        
        //Push back any new timestamps, then remove duplicates in the end
        //FIXME: Should be able to speed this up with sets rather than vectors?
        UnpairedTankTimestamps.push_back(PMTCounterTimeNs);
        if(PMTCounterTimeNs>NewestTimestamp){
          NewestTimestamp = PMTCounterTimeNs;
          if(verbosity>3)std::cout << "TANKTIMESTAMP," << PMTCounterTimeNs << std::endl;
        }
        //if(std::find(UnpairedTankTimestamps.begin(),UnpairedTankTimestamps.end(), PMTCounterTimeNs) == 
         //            UnpairedTankTimestamps.end()){
         // UnpairedTankTimestamps.push_back(PMTCounterTimeNs);  //Units in ns
        //}
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

        //If this InProgressTankEvent is too old, clear it
        //out from all TankTimestamp maps
        if(OrphanOldTankTimestamps && ((NewestTimestamp - PMTCounterTimeNs) > OldTimestampThreshold*1E9)){
          InProgressEventsToDelete.push_back(PMTCounterTimeNs);
        }
      }
      RemoveDuplicates(UnpairedTankTimestamps);
    }
    
    //Look through our MRD data for any new timestamps
    m_data->CStore.Get("MRDEventTriggerTypes",TriggerTypeMap);
    m_data->CStore.Get("NewMRDDataAvailable",IsNewMRDData);
    m_data->CStore.Get("MRDEvents",MRDEvents);
    m_data->CStore.Get("MRDBeamLoopback",MRDBeamLoopbackMap);
    m_data->CStore.Get("MRDCosmicLoopback",MRDCosmicLoopbackMap);
    if(IsNewMRDData){
      for(std::pair<unsigned long,std::vector<std::pair<unsigned long,int>>> apair : MRDEvents){
        unsigned long MRDTimeStamp = apair.first;
        std::vector<std::pair<unsigned long,int>> MRDHits = apair.second;
        //See if this MRD timestamp has been encountered before; if not,
        //add it to our MRD timestamp record
        //FIXME: should be able to speed up using sets?
        UnpairedMRDTimestamps.push_back(MRDTimeStamp);
        //if(std::find(UnpairedMRDTimestamps.begin(),UnpairedMRDTimestamps.end(), MRDTimeStamp) == 
        //             UnpairedMRDTimestamps.end()){
        //  UnpairedMRDTimestamps.push_back(MRDTimeStamp);
        //}
        if(verbosity>3)std::cout << "MRDTIMESTAMPTRIGTYPE," << MRDTimeStamp << "," << TriggerTypeMap.at(MRDTimeStamp) << std::endl;
      }
      RemoveDuplicates(UnpairedMRDTimestamps);
    }
    //Since timestamp pairing has been done for finished Tank Events,
    //Erase the finished Tank Events from the InProgressTankEventsMap
    if(verbosity>3)std::cout << "Now erasing finished timestamps from InProgressTankEvents" << std::endl;
    for (unsigned int j=0; j< InProgressEventsToDelete.size(); j++){
      InProgressTankEvents->erase(InProgressEventsToDelete.at(j));
    }
    if(verbosity>3) std::cout << "Current number of unfinished PMT Waveforms: " << InProgressTankEvents->size() << std::endl;

    
    //Now, pair up PMT and MRD events...
    int NumTankTimestamps = UnpairedTankTimestamps.size();
    int NumMRDTimestamps = UnpairedMRDTimestamps.size();
    int MinStamps = std::min(NumTankTimestamps,NumMRDTimestamps);
    std::vector<uint64_t> MRDStampsToDelete;

    if(MinStamps > (EventsPerPairing*10)){
      this->RemoveCosmics();
      this->PairTankPMTAndMRDTriggers();

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
        int beam_tdc = MRDBeamLoopbackMap[MRDTimeStamp];
        int cosmic_tdc = MRDCosmicLoopbackMap[MRDTimeStamp];
        if(verbosity>4) std::cout << "BUILDING AN ANNIE EVENT" << std::endl;
        this->BuildANNIEEventRunInfo(CurrentRunNum, CurrentSubRunNum, CurrentRunType,CurrentStarTime);
        this->BuildANNIEEventTank(TankCounterTime, aWaveMap);
        this->BuildANNIEEventMRD(MRDHits, MRDTimeStamp, MRDTriggerType, beam_tdc, cosmic_tdc);
        this->SaveEntryToFile(CurrentRunNum,CurrentSubRunNum);
        if(verbosity>4) std::cout << "BUILT AN ANNIE EVENT SUCCESSFULLY" << std::endl;
        //Erase this entry from maps/vectors used when pairing completed events 
        BuiltTankTimes.push_back(TankCounterTime);
        FinishedTankEvents.erase(TankCounterTime);
        MRDEvents.erase(MRDTimeStamp);
        TriggerTypeMap.erase(MRDTimeStamp);
        MRDBeamLoopbackMap.erase(MRDTimeStamp);
        MRDCosmicLoopbackMap.erase(MRDTimeStamp);
        FinishedTankTimestamps.erase(std::remove(FinishedTankTimestamps.begin(),FinishedTankTimestamps.end(),TankCounterTime), 
                   FinishedTankTimestamps.end());
      }
      for(int i=0; i<BuiltTankTimes.size(); i++){
        UnbuiltTankMRDPairs.erase(BuiltTankTimes.at(i));
      }
    }
  
  }
  
  //m_data->CStore.Set("InProgressTankEvents",InProgressTankEvents);
  m_data->CStore.Set("MRDEvents",MRDEvents);
  m_data->CStore.Set("MRDEventTriggerTypes",TriggerTypeMap);
  m_data->CStore.Set("MRDBeamLoopback",MRDBeamLoopbackMap);
  m_data->CStore.Set("MRDCosmicLoopback",MRDCosmicLoopbackMap);
  //InProgressTankEvents.clear();
  MRDEvents.clear();
  TriggerTypeMap.clear();
  MRDBeamLoopbackMap.clear();
  MRDCosmicLoopbackMap.clear();

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
    MRDEvents.erase(MRDStampsToDelete.at(j));
    TriggerTypeMap.erase(MRDStampsToDelete.at(j));
    MRDBeamLoopbackMap.erase(MRDStampsToDelete.at(j));
    MRDCosmicLoopbackMap.erase(MRDStampsToDelete.at(j));
  }
  return;
}


void ANNIEEventBuilder::CalculateSlidWindows(std::vector<uint64_t> FirstTimestampSet,
        std::vector<uint64_t> SecondTimestampSet, int shift, double& tmean, double& tvar)
{
  //Build offsets vectors used to sync streams
  //std::vector<uint64_t> PMTOffsets;
  //std::vector<uint64_t> MRDOffsets;
  //for (int j=1; j<EventsPerPairing*2; j++){
  //  PMTOffsets.push_back((FirstTimestampSet.at(j)/1E6) - (FirstTimestampSet.at(j-1)/1E6));
  //  MRDOffsets.push_back(SecondTimestampSet.at(j)- SecondTimestampSet.at(j-1));
  //}
 
  //TO-DO: We should look at the overall mean of the unshifted differnces.
  //Depending on the sign, choose whether to slide the MRD or PMT dataset
  //slide earliest times in MRD offset off the left
  std::vector<uint64_t> MRDTS_copy = SecondTimestampSet;
  std::vector<uint64_t> PMTTS_copy = FirstTimestampSet;
  //std::vector<uint64_t> SlidMRDOffsets = MRDOffsets;
  //std::vector<uint64_t> SlidPMTOffsets = PMTOffsets;
  //std::cout << "SLIDING OFFSETS" << std::endl;
  //SlidMRDOffsets.erase(SlidMRDOffsets.begin(),SlidMRDOffsets.begin()+shift);
  //SlidPMTOffsets.erase(SlidPMTOffsets.end()-shift,SlidPMTOffsets.end());
  MRDTS_copy.erase(MRDTS_copy.begin(),MRDTS_copy.begin()+shift);
  PMTTS_copy.erase(PMTTS_copy.end()-shift,PMTTS_copy.end());
  std::vector<double> TSDifferences;
  //std::vector<double> OffsetDifferences;
  for (int k=0; k<EventsPerPairing*2; k++){
    if(verbosity>4){ 
      std::cout << "PMT-MRD TIMESTAMP: " << static_cast<double>(PMTTS_copy.at(k)) - 
              static_cast<double>(MRDTS_copy.at(k))  << std::endl;
      //std::cout << "OFFSET DIFF: " << static_cast<double>(SlidPMTOffsets.at(k)) - 
      //        static_cast<double>(SlidMRDOffsets.at(k))  << std::endl;
    }
    //OffsetDifferences.push_back(static_cast<double>(SlidPMTOffsets.at(k)) - static_cast<double>(SlidMRDOffsets.at(k)));
    TSDifferences.push_back(static_cast<double>(PMTTS_copy.at(k)) - static_cast<double>(MRDTS_copy.at(k)));
  }
  double mean,var;
  //double omean,ovar;
  ComputeMeanAndVariance(TSDifferences,mean,var);
  tmean = mean;
  tvar = var;
  //ComputeMeanAndVariance(OffsetDifferences,omean,ovar);
  if(verbosity>4){
    std::cout << "SHIFTING MRD ARRAY BACK BY " << shift << " INDICES..." << std::endl;
    std::cout << "MEAN TS SHIFT: " << mean << std::endl;
    std::cout << "VARIANCE TS SHIFT: " << var << std::endl;
    //std::cout << "MEAN OF OFFSETS: " << omean << std::endl;
    //std::cout << "VARIANCE OF OFFSETS: " << ovar << std::endl;
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
  //std::sort(UnpairedMRDTimestamps.begin(),UnpairedMRDTimestamps.end());
  //std::sort(UnpairedTankTimestamps.begin(),UnpairedTankTimestamps.end());
  
  std::vector<uint64_t> TankStampsToDelete;
  std::vector<uint64_t> MRDStampsToDelete;
  int NumPairsToMake = EventsPerPairing;
  std::vector<double> ThisPairingTSDiffs;

  int NumOrphans = 0;
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
        NumOrphans +=1;
        UnpairedMRDTimestamps.erase(std::remove(UnpairedMRDTimestamps.begin(),UnpairedMRDTimestamps.end(),UnpairedMRDTimestamps.at(i)), 
             UnpairedMRDTimestamps.end());
      } else {
        if(verbosity>3) std::cout << "MOVING TANK TIMESTAMP TO ORPHANAGE" << std::endl;
        OrphanTankTimestamps.push_back(UnpairedTankTimestamps.at(i));
        NumOrphans +=1;
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
  }
  if(verbosity>4) std::cout << "FINISHED ERASING PAIRED TIMESTAMPS FROM FINISHED VECTORS" << std::endl;

  return;
}


void ANNIEEventBuilder::BuildANNIEEventMRD(std::vector<std::pair<unsigned long,int>> MRDHits, 
        unsigned long MRDTimeStamp, std::string MRDTriggerType, int beam_tdc, int cosmic_tdc)
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

  std::map<std::string, int> mrd_loopback_tdc;
  mrd_loopback_tdc.emplace("BeamLoopbackTDC",beam_tdc);
  mrd_loopback_tdc.emplace("CosmicLoopbackTDC",cosmic_tdc);

  Log("ANNIEEventBuilder: TDCData size: "+std::to_string(TDCData->size()),v_debug,verbosity);

  ANNIEEvent->Set("TDCData",TDCData,true);
  TimeClass timeclass_timestamp((uint64_t)MRDTimeStamp*1000);  //in microseconds
  ANNIEEvent->Set("EventTime",timeclass_timestamp); //not sure if EventTime is also in UTC or defined differently
  ANNIEEvent->Set("MRDTriggerType",MRDTriggerType);
  ANNIEEvent->Set("MRDLoopbackTDC",mrd_loopback_tdc);
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
  ANNIEEvent->Set("EventTimeTank",ClockTime);
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
