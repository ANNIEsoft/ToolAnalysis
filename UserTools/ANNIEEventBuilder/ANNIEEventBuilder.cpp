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
  //FIXME: Eventually, RunNumber should be loaded from
  //A run database
  m_variables.Get("RunNumber",RunNum);
  m_variables.Get("PassNumber",PassNum);
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

  m_data->CStore.Get("TankPMTCrateSpaceToChannelNumMap",TankPMTCrateSpaceToChannelNumMap);
  m_data->CStore.Get("MRDCrateSpaceToChannelNumMap",MRDCrateSpaceToChannelNumMap);

  ///////////////////////Initialise ANNIEEvent BoostStore////
  ANNIEEvent = new BoostStore(false,2);

  //////////////////////initialize subrun index//////////////
  SubrunNum = 0;
  ANNIEEventNum = 0;

  return true;
}


bool ANNIEEventBuilder::Execute(){

  if (isTankData){
    //Get the current FinishedPMTWaves map
    m_data->CStore.Get("FinishedPMTWaves",FinishedPMTWaves);
    //Assume a whole processed file will have all it's PMT data finished
    for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>>> apair : FinishedPMTWaves){
      uint64_t PMTCounterTime = apair.first;
      std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = apair.second;
      this->BuildANNIEEvent(PMTCounterTime, aWaveMap);
      //Erase this entry from the FinishedPMTWavesMap
      FinishedPMTWaves.erase(PMTCounterTime);
    }
    //Update the current FinishedPMTWaves map
    m_data->CStore.Set("FinishedPMTWaves",FinishedPMTWaves);

  } else if (isMRDData){
    m_data->CStore.Get("MRDEvents",MRDEvents);
    m_data->CStore.Get("MRDEventTriggerTypes",TriggerTypeMap);
    //Loop through MRDEvents and process each into ANNIEEvent.
    for(std::pair<unsigned long,std::vector<std::pair<unsigned long,int>>> apair : MRDEvents){
      unsigned long MRDTimeStamp = apair.first;
      std::vector<std::pair<unsigned long,int>> MRDHits = apair.second;
      std::string MRDTriggerType = TriggerTypeMap[MRDTimeStamp];
      this->BuildANNIEEventMRD(MRDHits, MRDTimeStamp, MRDTriggerType);
      //Erase this entry from the FinishedPMTWavesMap
      //FIXME: Check that the erase doesn't mess up looping through all entries somehow
      MRDEvents.erase(MRDTimeStamp);
      TriggerTypeMap.erase(MRDTimeStamp);
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


void ANNIEEventBuilder::BuildANNIEEventMRD(std::vector<std::pair<unsigned long,int>> MRDHits, 
        unsigned long MRDTimeStamp, std::string MRDTriggerType)
{
  std::cout << "Building an ANNIE Event (MRD), ANNIEEventNum = "<<ANNIEEventNum << std::endl;
  m_data->Stores["ANNIEEvent"]->GetEntry(ANNIEEventNum);

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

  m_data->Stores["ANNIEEvent"]->Set("TDCData",TDCData,true);
  m_data->Stores["ANNIEEvent"]->Set("RunNumber",RunNum);
  m_data->Stores["ANNIEEvent"]->Set("SubrunNumber",SubrunNum);
  m_data->Stores["ANNIEEvent"]->Set("EventNumber",ANNIEEventNum);
  TimeClass timeclass_timestamp((uint64_t)MRDTimeStamp*1000);  //in ns
  m_data->Stores["ANNIEEvent"]->Set("EventTime",timeclass_timestamp); //not sure if EventTime is also in UTC or defined differently

  ANNIEEventNum+=1;
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
    //FIXME: We're feeding Waveform class expects a double, not a uint64_t (?)
    Waveform<uint16_t> TheWave(ClockTime, TheWaveform);
    RawADCData.emplace(ChannelKey,TheWave);
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
