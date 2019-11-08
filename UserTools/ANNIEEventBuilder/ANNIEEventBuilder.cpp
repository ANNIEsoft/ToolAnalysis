#include "ANNIEEventBuilder.h"

ANNIEEventBuilder::ANNIEEventBuilder():Tool(){}


bool ANNIEEventBuilder::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer

  RunNum = 0;
  SubrunNum = 0;
  SavePath = "/ToolAnalysis/";
  ProcessedFilesBasename = "ProcessedRawData";
  isTankData = 0;
  isMRDData = 0;

  /////////////////////////////////////////////////////////////////
  //FIXME: Eventually, RunNumber should be loaded from A run database
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("RunNumber",RunNum);
  m_variables.Get("RunNumber",SubrunNum);
  m_variables.Get("EntriesPerSubrun",EntriesPerSubrun);
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
  SubrunNum = 0;

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
    m_data->CStore.Get("FinishedPMTWaves",FinishedPMTWaves);
    //Assume a whole processed file will have all it's PMT data finished
    for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>>> apair : FinishedPMTWaves){
      uint64_t PMTCounterTime = apair.first;
      if(verbosity>4) std::cout << "Finished waveset with time " << PMTCounterTime << std::endl;
      std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = apair.second;
      if(verbosity>4) std::cout << "Number of waves for timestamp: " << aWaveMap.size() << std::endl;
      this->BuildANNIEEvent(PMTCounterTime, aWaveMap);
      if(verbosity>4) std::cout << "Built event, saving to ANNIEEvent booststore" << std::endl;
      this->SaveEntryToFile();
      //Erase this entry from the FinishedPMTWavesMap
      FinishedPMTWaves.erase(PMTCounterTime);
    }
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
      this->BuildANNIEEventMRD(MRDHits, MRDTimeStamp, MRDTriggerType);
      this->SaveEntryToFile();
      //Erase this entry from the FinishedPMTWavesMap
      //FIXME: Check that the erase doesn't mess up looping through all entries somehow
      MRDEvents.erase(MRDTimeStamp);
      TriggerTypeMap.erase(MRDTimeStamp);
    }
  }

  if(verbosity>4) std::cout << "All ANNIEEvents built.  Set total entries" << std::endl;
  ANNIEEvent->Header->Set("TotalEntries",(long)ANNIEEventNum);
  if(verbosity>2) std::cout << "ANNIEEventBuilder: Saving and closing file." << std::endl;
  ANNIEEvent->Close();
  ANNIEEvent->Delete();
  delete ANNIEEvent; ANNIEEvent = new BoostStore(false,2);
  SubrunNum+=1;

  return true;
}


bool ANNIEEventBuilder::Finalise(){
  //Save the current subrun and delete ANNIEEvent
  std::cout << "ANNIEEventBuilder Exitting" << std::endl;
  return true;
}


void ANNIEEventBuilder::BuildANNIEEventMRD(std::vector<std::pair<unsigned long,int>> MRDHits, 
        unsigned long MRDTimeStamp, std::string MRDTriggerType)
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
  ANNIEEvent->Set("EventNumber",ANNIEEventNum);
  TimeClass timeclass_timestamp((uint64_t)MRDTimeStamp*1000);  //in ns
  ANNIEEvent->Set("EventTime",timeclass_timestamp); //not sure if EventTime is also in UTC or defined differently

  ANNIEEventNum+=1;
  return;
}

void ANNIEEventBuilder::BuildANNIEEvent(uint64_t ClockTime, 
        std::map<std::vector<int>, std::vector<uint16_t>> WaveMap)
{
  std::cout << "Building an ANNIE Event" << std::endl;
  ANNIEEvent->GetEntry(ANNIEEventNum);

  ///////////////LOAD RAW PMT DATA INTO ANNIEEVENT///////////////
  std::map<unsigned long, std::vector<Waveform<uint16_t>> > RawADCData;
  std::cout << "Looping through wavemap" << std::endl;
  for(std::pair<std::vector<int>, std::vector<uint16_t>> apair : WaveMap){
    int CardID = apair.first.at(0);
    int ChannelID = apair.first.at(1);
    int CrateNum=-1;
    int SlotNum=-1;
    this->CardIDToElectronicsSpace(CardID, CrateNum, SlotNum);
    std::vector<uint16_t> TheWaveform = apair.second;
    std::vector<int> CrateSpace{CrateNum,SlotNum,ChannelID};
    std::cout << "Getting Channel Key" << std::endl;
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
    std::cout << "Initializing waveform for channel_key" <<ChannelKey << std::endl;
    Waveform<uint16_t> TheWave(ClockTime, TheWaveform);
    //Placing waveform in a vector in case we want a hefty-mode minibuffer storage eventually
    std::vector<Waveform<uint16_t>> WaveVec{TheWave}; 
    std::cout << "Emplacing waveform in the Raw ADC Data map" << std::endl;
    RawADCData.emplace(ChannelKey,WaveVec);
  }
  ANNIEEvent->Set("RawADCData",RawADCData);
  ANNIEEvent->Set("RunNumber",RunNum);
  ANNIEEvent->Set("SubrunNumber",SubrunNum);
  //TODO: Things missing from ANNIEEvent that should be in before this tool finishes:
  //  - EventTime
  //  - TriggerData
  //  - BeamStatus?  
  //  - RawLAPPDData
  ANNIEEventNum+=1;
  return;
}

void ANNIEEventBuilder::SaveEntryToFile()
{
  //TODO: Build the Filename out of SavePath_ProcessedFileBasename_Runnum_Subrun_Passnum
  std::string Filename = SavePath + ProcessedFilesBasename + "_" + to_string(RunNum) + 
      "_" + to_string(SubrunNum);
  ANNIEEvent->Save(Filename);
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
