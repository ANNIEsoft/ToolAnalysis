#include "ANNIEEventBuilder.h"

ANNIEEventBuilder::ANNIEEventBuilder():Tool(){}


bool ANNIEEventBuilder::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer

  SavePath = "/ToolAnalysis/";
  ProcessedFilesBasename = "ProcessedRawData";
  isTankData = 0;
  isMRDData = 0;

  /////////////////////////////////////////////////////////////////
  //FIXME: Eventually, RunNumber should be loaded from A run database
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("SavePath",SavePath);
  m_variables.Get("ProcessedFilesBasename",ProcessedFilesBasename);
  m_variables.Get("IsTankData",isTankData);
  m_variables.Get("IsMRDData",isMRDData);

  if(isTankData && isMRDData){
    std::cout << "BuildANNIEEvent ERROR: No data stream matching " <<
        "implemented yet.  Please select either Tank or MRD Data only" << std::endl;
    return false;
  }

  if((isTankData || isMRDData) == 0){
    std::cout << "BuildANNIEEvent ERROR: No data file type chosen. " <<
        "Please select either Tank or MRD Data only" << std::endl;
    return false;
  }

  m_data->CStore.Get("TankPMTCrateSpaceToChannelNumMap",TankPMTCrateSpaceToChannelNumMap);
  m_data->CStore.Get("MRDCrateSpaceToChannelNumMap",MRDCrateSpaceToChannelNumMap);

  //////////////////////initialize subrun index//////////////
  ANNIEEvent = new BoostStore(false,2);
  ANNIEEventNum = 0;
  CurrentRunNum = -1;
  CurrentSubrunNum = -1;

  return true;
}


bool ANNIEEventBuilder::Execute(){

  if (isTankData){
    //Check to see if there's new PMT data
    m_data->CStore.Get("NewTankPMTDataAvailable",IsNewTankData);
    if(!IsNewTankData){
      Log("ANNIEEventBuilder:: No new Tank Data.  Not building ANNIEEvent: ",v_message, verbosity);
      return true;
    }
    //Get the current FinishedPMTWaves map
    if(verbosity>4) std::cout << "ANNIEEventBuilder: Getting waves and run info from CStore" << std::endl;
    m_data->CStore.Get("FinishedPMTWaves",FinishedPMTWaves);
    m_data->CStore.Get("RunInfoPostgress",RunInfoPostgress);
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
    if(CurrentSubrunNum == -1) CurrentSubrunNum = SubRunNumber;
    //If we're in a new run or subrun, make a new ANNIEEvent file. 
    if(isTankData && ((CurrentRunNum != RunNumber) || (CurrentSubrunNum != SubRunNumber))){
      if(verbosity>v_warning) std::cout << "New run or subrun encountered. Opening new BoostStore" << std::endl;
      ANNIEEvent->Header->Set("TotalEntries",(long)ANNIEEventNum);
      if(verbosity>v_warning) std::cout << "ANNIEEventBuilder: Saving and closing file." << std::endl;
      ANNIEEvent->Close();
      ANNIEEvent->Delete();
      delete ANNIEEvent; ANNIEEvent = new BoostStore(false,2);
      CurrentRunNum = RunNumber;
      CurrentSubrunNum = SubRunNumber;
    }

    //Assume a whole processed file will have all it's PMT data finished
    std::vector<uint64_t> PMTEventsToDelete;
    for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>>> apair : FinishedPMTWaves){
      if(verbosity>4) std::cout << "Accessing next PMT counter?" << std::endl;
      uint64_t PMTCounterTime = apair.first;
      if(verbosity>4) std::cout << "Finished waveset has clock counter: " << PMTCounterTime << std::endl;
      std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = apair.second;
      if(verbosity>4) std::cout << "Number of waves for this counter: " << aWaveMap.size() << std::endl;
      this->BuildANNIEEvent(PMTCounterTime, aWaveMap,RunNumber,SubRunNumber,RunType,StarTime);
      this->SaveEntryToFile(CurrentRunNum,CurrentSubrunNum);
      //Erase this entry from the FinishedPMTWavesMap
      if(verbosity>4) std::cout << "Counter time will be erased from FinishedPMTWaves: " << PMTCounterTime << std::endl;
      PMTEventsToDelete.push_back(PMTCounterTime);
    }

    for(int i=0; i< PMTEventsToDelete.size(); i++) FinishedPMTWaves.erase(PMTEventsToDelete.at(i));
    //Update the current FinishedPMTWaves map
    m_data->CStore.Set("FinishedPMTWaves",FinishedPMTWaves);

  } else if (isMRDData){
    m_data->CStore.Get("NewMRDDataAvailable",IsNewMRDData);
    if(!IsNewTankData){
      Log("ANNIEEventBuilder:: No new Tank Data.  Not building ANNIEEvent: ",v_message, verbosity);
      return true;
    }
    m_data->CStore.Get("MRDEvents",MRDEvents);
    m_data->CStore.Get("MRDEventTriggerTypes",TriggerTypeMap);
    //Loop through MRDEvents and process each into ANNIEEvent.
    for(std::pair<unsigned long,std::vector<std::pair<unsigned long,int>>> apair : MRDEvents){
      unsigned long MRDTimeStamp = apair.first;
      std::vector<std::pair<unsigned long,int>> MRDHits = apair.second;
      std::string MRDTriggerType = TriggerTypeMap[MRDTimeStamp];
      //FIXME: Once we fuse PMT data and MRD data streams, need to put in 
      //run number, subrun number, run type, etc. from Postgress DB
      this->BuildANNIEEventMRD(MRDHits, MRDTimeStamp, MRDTriggerType, 0, 0, 0);
      this->SaveEntryToFile(0,0);
      //Erase this entry from the FinishedPMTWavesMap
      //FIXME: Check that the erase doesn't mess up looping through all entries somehow
      MRDEvents.erase(MRDTimeStamp);
      TriggerTypeMap.erase(MRDTimeStamp);
    }
  }

  return true;
}


bool ANNIEEventBuilder::Finalise(){
  if(verbosity>4) std::cout << "ANNIEEvent Finalising.  Closing any open ANNIEEvent Boostore" << std::endl;
  ANNIEEvent->Header->Set("TotalEntries",(long)ANNIEEventNum);
  if(verbosity>2) std::cout << "ANNIEEventBuilder: Saving and closing file." << std::endl;
  ANNIEEvent->Close();
  ANNIEEvent->Delete();
  delete ANNIEEvent;
  //Save the current subrun and delete ANNIEEvent
  std::cout << "ANNIEEventBuilder Exitting" << std::endl;
  return true;
}


void ANNIEEventBuilder::BuildANNIEEventMRD(std::vector<std::pair<unsigned long,int>> MRDHits, 
        unsigned long MRDTimeStamp, std::string MRDTriggerType, int RunNum, int SubrunNum, int
        RunType)
{
  std::cout << "Building an ANNIE Event (MRD), ANNIEEventNum = "<<ANNIEEventNum << std::endl;
  ANNIEEvent->GetEntry(ANNIEEventNum);

  if (ANNIEEventNum==0) TDCData = new std::map<unsigned long, std::vector<Hit>>;
  if (TDCData->size()>0){
    TDCData->clear();
  }


  //TODO: Loop through MRDHits at this timestamp and form the Hit vector.
  for (int i_value=0; i_value< MRDHits.size(); i_value++){
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


  std::cout <<"TDCData size: "<<TDCData->size()<<std::endl;

  ANNIEEvent->Set("TDCData",TDCData,true);
  ANNIEEvent->Set("RunNumber",RunNum);
  ANNIEEvent->Set("SubrunNumber",SubrunNum);
  ANNIEEvent->Set("RunType",RunType);
  ANNIEEvent->Set("EventNumber",ANNIEEventNum);
  TimeClass timeclass_timestamp((uint64_t)MRDTimeStamp*1000);  //in ns
  ANNIEEvent->Set("EventTime",timeclass_timestamp); //not sure if EventTime is also in UTC or defined differently
  return;
}

void ANNIEEventBuilder::BuildANNIEEvent(uint64_t ClockTime, 
        std::map<std::vector<int>, std::vector<uint16_t>> WaveMap, int RunNum, int SubrunNum,
        int RunType, uint64_t StartTime)
{
  if(verbosity>v_message)std::cout << "Building an ANNIE Event" << std::endl;
  ANNIEEvent->GetEntry(ANNIEEventNum);

  ///////////////LOAD RAW PMT DATA INTO ANNIEEVENT///////////////
  std::map<unsigned long, std::vector<Waveform<uint16_t>> > RawADCData;
  for(std::pair<std::vector<int>, std::vector<uint16_t>> apair : WaveMap){
    int CardID = apair.first.at(0);
    int ChannelID = apair.first.at(1);
    int CrateNum=-1;
    int SlotNum=-1;
    if(verbosity>v_debug) std::cout << "Converting " << CardID << " to electronics space" << std::endl;
    this->CardIDToElectronicsSpace(CardID, CrateNum, SlotNum);
    std::vector<uint16_t> TheWaveform = apair.second;
    std::vector<int> CrateSpace{CrateNum,SlotNum,ChannelID};
    if(TankPMTCrateSpaceToChannelNumMap.count(CrateSpace)==0){
      Log("ANNIEEventBuilder:: Cannot find channel key for crate space entry: ",v_error, verbosity);
      Log("ANNIEEventBuilder::CrateNum "+to_string(CrateNum),v_error, verbosity);
      Log("ANNIEEventBuilder::SlotNum "+to_string(SlotNum),v_error, verbosity);
      Log("ANNIEEventBuilder::SlotNum "+to_string(SlotNum),v_error, verbosity);
      Log("ANNIEEventBuilder::ChannelID "+to_string(ChannelID),v_error, verbosity);
      Log("ANNIEEventBuilder:: Passing over the wave; PMT DATA LOST",v_error, verbosity);
      continue;
    }
    unsigned long ChannelKey = TankPMTCrateSpaceToChannelNumMap.at(CrateSpace);
    //FIXME: We're feeding Waveform class expects a double, not a uint64_t (?)
    Waveform<uint16_t> TheWave(ClockTime, TheWaveform);
    //Placing waveform in a vector in case we want a hefty-mode minibuffer storage eventually
    std::vector<Waveform<uint16_t>> WaveVec{TheWave}; 
    RawADCData.emplace(ChannelKey,WaveVec);
  }
  if(RawADCData.size() == 0){
    std::cout << "No Raw ADC Data in entry.  Not putting to ANNIEEvent." << std::endl;
  }
  std::cout << "Setting ANNIE Event information" << std::endl;
  ANNIEEvent->Set("RawADCData",RawADCData);
  ANNIEEvent->Set("RunNumber",RunNum);
  ANNIEEvent->Set("SubrunNumber",SubrunNum);
  ANNIEEvent->Set("RunType",SubrunNum);
  ANNIEEvent->Set("RunStartTime",StartTime);
  //TODO: Things missing from ANNIEEvent that should be in before this tool finishes:
  //  - EventTime
  //  - TriggerData
  //  - BeamStatus?  
  //  - RawLAPPDData
  if(verbosity>v_debug) std::cout << "ANNIEEventBuilder: ANNIE Event "+
      to_string(ANNIEEventNum)+" built." << std::endl;
  return;
}

void ANNIEEventBuilder::SaveEntryToFile(int RunNum, int SubrunNum)
{
  if(verbosity>4) std::cout << "ANNIEEvent: Saving ANNIEEvent entry"+to_string(ANNIEEventNum) << std::endl;
  std::string Filename = SavePath + ProcessedFilesBasename + "R" + to_string(RunNum) + 
      "S" + to_string(SubrunNum);
  ANNIEEvent->Save(Filename);
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
