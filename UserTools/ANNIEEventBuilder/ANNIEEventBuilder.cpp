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
      //TODO: Eventually, we'll need to sync the TriggerData with the PMT
      //Times (and LAPPD times when an LAPPDDataDecoder tool comes along).
      //int MatchingEntryNum=-1;
      //int MatchingIndexNum=-1;
      //this->SearchTriggerData(aTrigTime,MatchingEntryNum, MatchingIndexNum);
      //if(MatchingEntryNum == -1 || MatchingIndexNum == -1){
      //  std::cout << "ANNIEEventBuilder TODO: How do we handle wave maps with no matching trigger time?" << std::endl;
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
  // Search the TriggerData for a matching trigger time
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
      if(Tdata.EventTimes.at(j) == aTrigTime){
        std::cout << "Trigger Match found!" << std::endl;
        MatchEntry = i;
        MatchIndex = j;
        return;
      }
    }
    std::cout<<"EventIDs: " << std::endl;
    for(unsigned int j=0;j<Tdata.EventIDs.size();j++){
     std::cout<< Tdata.EventIDs.at(j)<<" , ";
    }
    std::cout<<std::endl;
  }
  std::cout << "ANNIEEventBuilder: Reached end of trigger data.  No match for trigger time " << aTrigTime << std::endl;
  //TODO: What do we do here?  Delete the elements in the CStore?
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
  //FIXME: add other obvious ones here too...


  //TODO: Trigger information needs to be identified with the PMT Clock Time and
  //also added to the ANNIEEvent.  For now, just push the Clock Time as the
  //Trigger Time.
  
  ////////////////////// LOAD TRIGGER DATA INTO ANNIEEVENT  /////////////////////// 
  //TrigData->GetEntry(MatchEntry);
  //TriggerData Tdata;
  //TrigData->Get("TrigData",Tdata);
  //uint16_t EventID = Tdata.EventIDs.at(MatchIndex);
  //uint64_t EventTime = Tdata.EventTimes.at(MatchIndex);
  //TimeClass TrigTime(EventTime);
  //uint32_t TriggerMask = Tdata.TriggerMasks.at(MatchIndex);
  //uint32_t TriggerCounter = Tdata.TriggerCounters.at(MatchIndex);
  
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
  std::cout << "FIXME: We need to build the map of VME CardIDs to the electronics space position." << std::endl;
  std::cout << "In the meantime, the answer is 42." << std::endl;
  CrateNum = 42;
  SlotNum = 42;
  return;
}
