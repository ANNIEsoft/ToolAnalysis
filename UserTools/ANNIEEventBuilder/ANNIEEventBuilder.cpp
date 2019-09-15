#include "ANNIEEventBuilder.h"

ANNIEEventBuilder::ANNIEEventBuilder():Tool(){}


bool ANNIEEventBuilder::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("InputFile",InputFile);
  m_variables.Get("RunNumber",RunNum);
  m_variables.Get("SubRunNumber",SubRunNum);

  // Initialize RawData
  RawData.Initialise(InputFile.c_str());
  RawData.Print(false);
  
  ////////////////////////getting trigger data ////////////////
  RawData.Get("TrigData",TrigData);

  TrigData.Print(false);
  long trigentries=0;
  TrigData.Header->Get("TotalEntries",trigentries);
  std::cout<<"Total entries in TriggerData store: "<<entries<<std::endl;

  return true;
}


bool ANNIEEventBuilder::Execute(){
  //Get the current FinishedWaves map
  m_data->CStore.Get("FinishedWaves",FinishedWaves);

  //Check to see if any trigger times have all their PMTs
  for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>> && apair : *FinishedWaves){
    uint64_t aTrigTime = apair.First;
    std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = apair.second;
    if(aWaveMap.size()>NumWavesInSet){
      std::cout << "TriggerTime " << aTrigTime << " has more than " << NumWavesInSet << ". " <<
          "beginning to build ANNIEEvent." << std::endl;
      int MatchingEntryNum=-1;
      int MatchingIndexNum=-1;
      this->SearchTriggerData(aTrigTime,MatchingEntryNum, MatchingIndexNum);
      if(MatchinEntryNum == -1 || MatchingIndexNum == -1){
        std::cout << "ANNIEEventBuilder TODO: How do we handle wave maps with no matching trigger time?" << std::endl;
      }
      //If here, a match was found.  Start to build the ANNIEEvent.
      this->BuildANNIEEvent(aWaveMap, MatchingEntryNum, MatchingIndexNum);
    }
  }

  return true;
}


bool ANNIEEventBuilder::Finalise(){
  // Make the ANNIEEvent Store if it doesn't exist
  // =============================================
  int annieeventexists = m_data->Stores.count("ANNIEEvent");
  if(annieeventexists==0) m_data->Stores["ANNIEEvent"] = new BoostStore(false,2);
  return true;
}

void ANNIEEventBuilder::SearchTriggerData(uint64_t aTrigTime, int &MatchEntry, int &MatchIndex)
{
  // Search the TriggerData for a matching trigger time
  for( int i=0;i<trigentries;i++){
    std::cout<<"Checking entry "<<i<<" of "<<trigentries<<std::endl;
    TrigData.GetEntry(i);
    TriggerData Tdata;
    TrigData.Get("TrigData",Tdata);
    int EventSize=Tdata.EventSize;
    std::cout<<"EventSize="<<EventSize<<std::endl;
    std::cout<<"SequenceID="<<Tdata.SequenceID<<std::endl;
    std::cout<<"EventTimes: " << std::endl;
    for(int j=0;j<Tdata.EventTimes.size();j++){
      std::cout<< Tdata.EventTimes.at(j)<<" , ";
      if(Tdata.EventTimes.at(j) == aTrigTime){
        std::cout << "Trigger Match found!" << std::endl;
        MatchEntry = i;
        MatchIndex = j;
        return;
      }
    }
    std::cout<<"EventIDs: " << std::endl;
    for(int j=0;j<Tdata.EventIDs.size();j++){
     std::cout<< Tdata.EventIDs.at(j)<<" , ";
    }
    std::cout<<std::endl;
  }
  std::cout << "ANNIEEventBuilder: Reached end of trigger data.  No match for trigger time " << aTrigTime << std::endl;
  //TODO: What do we do here?  Delete the elements in the CStore?
  return;
}

void ANNIEEventBuilder::BuildANNIEEvent(std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap,
        int MatchEntry, int MatchIndex)
{
  TrigData.GetEntry(MatchEntry);
  TriggerData Tdata;
  TrigData.Get("TrigData",Tdata);
  //Get the Trigger Data associated with this WaveMap's trigger time
  uint16_t EventID = Tdata.EventIDs.at(MatchIndex);
  uint64_t EventTime = Tdata.EventTimes.at(MatchIndex);
  TimeClass TrigTime(EventTime);
  uint32_t TriggerMask = Tdata.TriggerMasks.at(MatchIndex);
  uint32_t TriggerCounter = Tdata.TriggerCounters.at(MatchIndex);
  //A couple TODOs here:
  //RunNumber and SubrunNumber are missing.  Let's define them for now using the
  //ConfigFile, but they'll   
