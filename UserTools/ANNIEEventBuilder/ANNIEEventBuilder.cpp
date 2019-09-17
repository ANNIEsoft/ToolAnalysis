#include "ANNIEEventBuilder.h"

ANNIEEventBuilder::ANNIEEventBuilder():Tool(){}


bool ANNIEEventBuilder::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer

  RunNum = 0;
  PassNum = 0;
  SavePath = "./";
  ProcessedFilesBasename = "ProcessedRawData";
  /////////////////////////////////////////////////////////////////
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("InputFile",InputFile);
  //FIXME: Eventually, RunNumber should be loaded from
  //A run database
  m_variables.Get("RunNumber",RunNum);
  m_variables.Get("PassNumber",PassNum);
  m_variables.Get("EntriesPerSubrun",EntriesPerSubrun);
  m_variables.Get("SavePath",SavePath);
  m_variables.Get("ProcessedFilesBasename",ProcessedFilesBasename);

  // Initialize RawData
  RawData = new BoostStore(false,0);
  RawData->Initialise(InputFile.c_str());
  RawData->Print(false);
  
  ////////////////////////getting trigger data ////////////////
  TrigData = new BoostStore(false,2);
  RawData->Get("TrigData",TrigData);
  TrigData->Print(false);
  TrigData->Header->Get("TotalEntries",trigentries);
  std::cout<<"Total entries in TriggerData store: "<<trigentries<<std::endl;

  m_data->CStore.Get("TankPMTCrateSpaceToChannelNumMap",TankPMTCrateSpaceToChannelNumMap);
  ///////////////////////Initialise ANNIEEvent BoostStore////
  ANNIEEvent = new BoostStore(false,2);

  //////////////////////initialize subrun index//////////////
  SubrunNum = 0;
  ANNIEEventNum = 0;

  return true;
}


bool ANNIEEventBuilder::Execute(){

  //Get the current FinishedPMTWaves map
  m_data->CStore.Get("FinishedPMTWaves",FinishedPMTWaves);
  //Check to see if any trigger times have all their PMTs
  for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>>> apair : FinishedPMTWaves){
    uint64_t PMTCounterTime = apair.first;
    std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = apair.second;
    if(aWaveMap.size()>NumWavesInSet){
      std::cout << "PMTCounterTime " << PMTCounterTime << " has more than " << NumWavesInSet << ". " <<
          "beginning to build ANNIEEvent." << std::endl;
      //TODO: 
      //  - Sync the TriggerData with the PMT Times 
      //  - Check LAPPDData and TDCData for addition to ANNIEEvent 
      //  - Additional sanity checks on data prior to entry addition? 
      //}
      //If here, a match was found.  Start to build the ANNIEEvent.
      this->BuildANNIEEvent(PMTCounterTime, aWaveMap);
    }
  }

  return true;
}


bool ANNIEEventBuilder::Finalise(){
  delete RawData;
  //Save the current subrun and delete ANNIEEvent
  this->SaveSubrun();
  delete ANNIEEvent;
  delete TrigData;
  std::cout << "ANNIEEventBuilder Exitting" << std::endl;
  return true;
}

void ANNIEEventBuilder::SearchTriggerData(uint64_t aTrigTime, int &MatchEntry, int &MatchIndex)
{
  // Right now, this method just prints stuff for debugging.
  //TODO: Eventually, as trigger data is matched with the correct electronics
  //      data, start a map that tracks what TriggerData entries are finished?
  //      Could save time avoiding looping through the TriggerData over and
  //      over again when most entries are already paired with an ANNIEEvent. 
  for( int i=0;i<trigentries;i++){
    std::cout<<"Checking entry "<<i<<" of "<<trigentries<<std::endl;
    TrigData->GetEntry(i);
    TriggerData Tdata;
    TrigData->Get("TrigData",Tdata);
    int EventSize=Tdata.EventSize;
    std::cout<<"EventSize="<<EventSize<<std::endl;
    std::cout<<"SequenceID="<<Tdata.SequenceID<<std::endl;
    std::cout<<"EventTimes: " << std::endl;
    for(unsigned int j=0;j<Tdata.EventTimes.size();j++){
      std::cout<< Tdata.EventTimes.at(j)<<" , ";
    }
    std::cout<<"EventIDs: " << std::endl;
    for(unsigned int j=0;j<Tdata.EventIDs.size();j++){
     std::cout<< Tdata.EventIDs.at(j)<<" , ";
    }
    std::cout<<"TriggerMasks: " << std::endl;
    for(unsigned int j=0;j<Tdata.TriggerMasks.size();j++){
     std::cout<< Tdata.TriggerMasks.at(j)<<" , ";
    }
    std::cout<<"TriggerCounters: " << std::endl;
    for(unsigned int j=0;j<Tdata.TriggerCounters.size();j++){
     std::cout<< Tdata.TriggerCounters.at(j)<<" , ";
    }
    std::cout<<std::endl;
  }
  return;
}

void ANNIEEventBuilder::BuildANNIEEvent(uint64_t ClockTime, 
        std::map<std::vector<int>, std::vector<uint16_t>> WaveMap)
{
  std::cout << "Building an ANNIE Event" << std::endl;
  ANNIEEvent->GetEntry(ANNIEEventNum);

  ///////////////LOAD RAW PMT DATA INTO ANNIEEVENT///////////////
  std::map<unsigned long, Waveform<uint16_t> > RawADCData;
  for(std::pair<std::vector<int>, std::vector<uint16_t>> apair : WaveMap){
    int CardID = apair.first.at(0);
    int ChannelID = apair.first.at(1);
    int CrateNum, SlotNum;
    this->CardIDToElectronicsSpace(CardID, CrateNum, SlotNum);
    std::vector<uint16_t> TheWaveform = apair.second;
    std::vector<int> CrateSpace{CrateNum,SlotNum,ChannelID};
    unsigned long ChannelKey = TankPMTCrateSpaceToChannelNumMap.at(CrateSpace);
    //FIXME: We're feeding Waveform class expects a double, not a uint64_t
    Waveform<uint16_t> TheWave(ClockTime, TheWaveform);
    RawADCData.emplace(ChannelKey,TheWave);
  }
  ANNIEEvent->Set("RawADCData",RawADCData);
  ANNIEEvent->Set("RunNumber",RunNum);
  ANNIEEvent->Set("SubrunNumber",RunNum);
  //TODO: Things missing from ANNIEEvent that should be in before this tool finishes:
  //  - EventTime
  //  - TriggerData
  //  - BeamStatus?  
  //  - TDCData
  //  - RawLAPPDData


  ANNIEEventNum+=1;
  //Check if we have enough ANNIEEvents to constitute a subrun
  if((ANNIEEventNum/EntriesPerSubrun - 1) == SubrunNum){
    this->SaveSubrun();
    SubrunNum+=1;
  }
  return;
}

void ANNIEEventBuilder::SaveSubrun()
{
  //TODO: Build the Filename out of SavePath_ProcessedFileBasename_Runnum_Subrun_Passnum
  std::string Filename = "/ToolAnalysis/TheTestBoost";
  ANNIEEvent->Save(Filename);
  ANNIEEvent->Close();
  delete ANNIEEvent;
  ANNIEEvent = new BoostStore(false,2);
  SubrunNum +=1;
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
