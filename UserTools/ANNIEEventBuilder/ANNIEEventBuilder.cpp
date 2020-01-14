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

  /////////////////////////////////////////////////////////////////
  //FIXME: Eventually, RunNumber should be loaded from A run database
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("SavePath",SavePath);
  m_variables.Get("ProcessedFilesBasename",ProcessedFilesBasename);
  m_variables.Get("BuildType",BuildType);

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

  //////////////////////initialize subrun index//////////////
  ANNIEEvent = new BoostStore(false,2);
  ANNIEEventNum = 0;
  CurrentRunNum = -1;
  CurrentSubRunNum = -1;

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
      if(verbosity>4) std::cout << "Accessing next PMT counter" << std::endl;
      uint64_t PMTCounterTime = apair.first;
      if(verbosity>4) std::cout << "Finished waveset has clock counter: " << PMTCounterTime << std::endl;
      std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = apair.second;
      if(verbosity>4) std::cout << "Number of waves for this counter: " << aWaveMap.size() << std::endl;
      //For this counter, need to have the number of TankPMT channels plus number of aux channels
      int NumTankPMTChannels = TankPMTCrateSpaceToChannelNumMap.size();
      int NumAuxChannels = AuxCrateSpaceToChannelNumMap.size();
      if(aWaveMap.size() >= (NumTankPMTChannels + NumAuxChannels)){
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
    if (TankRunNumber != MRDRunNumber || TankSubRunNumber != MRDSubRunNumber){
      if((TankRunNumber*10000 + TankSubRunNumber) > (MRDRunNumber*10000 + MRDSubRunNumber)){
        LowestRunNum = TankRunNumber;
        LowestSubRunNum = TankSubRunNumber;
        LowestRunType = TankRunType;
        LowestStarTime = TankStarTime;
        m_data->CStore.Set("PauseMRDDecoding",true);
      } else {
        LowestRunNum = MRDRunNumber;
        LowestSubRunNum = MRDSubRunNumber;
        LowestRunType = MRDRunType;
        LowestStarTime = MRDStarTime;
        m_data->CStore.Set("PauseTankDecoding",true);
      }
    } else {
      m_data->CStore.Set("PauseMRDDecoding",false);
      m_data->CStore.Set("PauseTankDecoding",false);
    }

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
    }

    //Check if any In-progress tank events now have all waveforms
    m_data->CStore.Get("InProgressTankEvents",InProgressTankEvents);
    std::vector<uint64_t> InProgressEventsToDelete;
    m_data->CStore.Get("NewTankPMTDataAvailable",IsNewTankData);
    if(!IsNewTankData){
      for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>>> apair : InProgressTankEvents){
        uint64_t PMTCounterTime = apair.first;
        std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = apair.second;
        
        //See if this timestamp has been encountered before; if not,
        //add it to our Tank timestamp record
        if(std::find(RunTankTimestamps.begin(),RunTankTimestamps.end(), PMTCounterTime) == 
                     RunTankTimestamps.end()){
          RunTankTimestamps.push_back(PMTCounterTime);
        }

        //If this trigger has all of it's waveforms, add it to the finished
        //Events and delete it from the in-progress events
        int NumTankPMTChannels = TankPMTCrateSpaceToChannelNumMap.size();
        int NumAuxChannels = AuxCrateSpaceToChannelNumMap.size();
        if(aWaveMap.size() >= (NumTankPMTChannels + NumAuxChannels)){
          FinishedTankEvents.emplace(PMTCounterTime,aWaveMap);
          FinishedTankTimestamps.push_back(PMTCounterTime);
          //Put PMT timestamp into the timestamp set for this run.
          if(verbosity>4) std::cout << "Finished waveset has clock counter: " << PMTCounterTime << std::endl;
          if(verbosity>4) std::cout << "Number of waves for this counter: " << aWaveMap.size() << std::endl;
          InProgressEventsToDelete.push_back(PMTCounterTime);
        }
        //Erase this entry from the InProgressTankEventsMap
        if(verbosity>4) std::cout << "Counter time will be erased from InProgressTankEvents: " << PMTCounterTime << std::endl;
        for (unsigned int i=0; i< InProgressEventsToDelete.size(); i++){
          InProgressTankEvents.erase(InProgressEventsToDelete.at(i));
        }
      }
    }

    //Look through our MRD data for any new timestamps
    m_data->CStore.Get("NewMRDDataAvailable",IsNewMRDData);
    m_data->CStore.Get("MRDEvents",MRDEvents);
    std::vector<unsigned long> MRDEventsToDelete;
    if(IsNewMRDData){
      for(std::pair<unsigned long,std::vector<std::pair<unsigned long,int>>> apair : MRDEvents){
        unsigned long MRDTimeStamp = apair.first;
        std::vector<std::pair<unsigned long,int>> MRDHits = apair.second;
        //See if this MRD timestamp has been encountered before; if not,
        //add it to our MRD timestamp record
        if(std::find(RunMRDTimestamps.begin(),RunMRDTimestamps.end(), MRDTimeStamp) == 
                     RunMRDTimestamps.end()){
          RunMRDTimestamps.push_back(MRDTimeStamp);
        }
      }
    }

    //Now, come up with an algorithm to pair up PMT and MRD events...
    this->PairTankPMTAndMRDTriggers();
    
    //Finally, Build the ANNIEEvent of any PMT/MRD data that is fully decoded 
    //and has been paired
    std::vector<unsigned long> CompleteEventsToDelete;
    m_data->CStore.Get("MRDEventTriggerTypes",TriggerTypeMap);
    for(std::pair<uint64_t,uint64_t> cpair : FinishedTankMRDPairs){
      uint64_t TankCounterTime = cpair.first;
      std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = FinishedTankEvents.at(TankCounterTime);
      
      uint64_t MRDTimeStamp = cpair.second;
      std::vector<std::pair<unsigned long,int>> MRDHits = MRDEvents.at(MRDTimeStamp);
      std::string MRDTriggerType = TriggerTypeMap[MRDTimeStamp];
      

      this->BuildANNIEEventRunInfo(CurrentRunNum, CurrentSubRunNum, CurrentRunType,CurrentStarTime);
      this->BuildANNIEEventTank(TankCounterTime, aWaveMap);
      this->BuildANNIEEventMRD(MRDHits, MRDTimeStamp, MRDTriggerType);
      this->SaveEntryToFile(CurrentRunNum,CurrentSubRunNum);
      //Erase this entry from the InProgressTankEventsMap
      CompleteEventsToDelete.push_back(TankCounterTime);
    }
    for (unsigned int i=0; i< CompleteEventsToDelete.size(); i++){
      //FIXME: Erase timestamps from RunTankTimestamp, FinishedTankTimestamps, and RunMRDTimestamp
      FinishedTankMRDPairs.erase(CompleteEventsToDelete.at(i));
    }
  }
  
  return true;
}


bool ANNIEEventBuilder::Finalise(){
  if(verbosity>4) std::cout << "ANNIEEvent Finalising.  Closing any open ANNIEEvent Boostore" << std::endl;
  if(verbosity>2) std::cout << "ANNIEEventBuilder: Saving and closing file." << std::endl;
  ANNIEEvent->Close();
  ANNIEEvent->Delete();
  delete ANNIEEvent;
  //Save the current subrun and delete ANNIEEvent
  std::cout << "ANNIEEventBuilder Exitting" << std::endl;
  return true;
}

void ANNIEEventBuilder::PairTankPMTAndMRDTriggers(){
  //Couple things to do:
  // First, check the lengths of each; don't start pairing until we have at least 10 timestamps from the PMT and MRD
  // Once we have ten of them, organize them from earliest to latest.  Pair up the timestamps
  // Take the mean of these 10 as the baseline that other timestamp spreads should be close to in the run
  // If the deviation is large, or there's a jump anywhere in the run, print huge WARNINGS.
  std::cout << "WE SHALL PAIR" << std::endl;
  int NumTankTimestamps = RunTankTimestamps.size();
  int NumMRDTimestamps = RunMRDTimestamps.size();
  int MinStamps = std::min(NumTankTimestamps,NumMRDTimestamps);
  if(MinStamps > 10){
    std::cout << "We've got enough timestamps to start matching up" << std::endl;
    //Organize RunTankTimestamps and RunMRDTimestamps
    std::sort(RunTankTimestamps.begin(),RunTankTimestamps.end());
    std::sort(RunMRDTimestamps.begin(),RunMRDTimestamps.end());
    for (int i=0;i<MinStamps; i++){
      if (std::find(FinishedTankTimestamps.begin(),FinishedTankTimestamps.end(),RunTankTimestamps.at(i)) != FinishedTankTimestamps.end()){
        //This tank timestamp is finished decoding.  Pair up the Tank and MRD timestamp
        FinishedTankMRDPairs.emplace(RunTankTimestamps.at(i),RunMRDTimestamps.at(i));
      }
    }
  }
  //FIXME: Right now, the same timestamps would keep getting put in FinishedTankMRDPairs.  
  //Need to delete them from RunTankTimestamps...
  for (int i=0; i<MinStamps; i++){
    std::cout << "TANK TIMESTAMP: " << RunTankTimestamps.at(i) << ",MRD TIMESTAMP: " << RunMRDTimestamps.at(i) << std::endl;
  }
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
