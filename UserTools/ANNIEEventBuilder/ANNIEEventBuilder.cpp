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
  m_variables.Get("EntriesPerSubrun",EntriesPerSubrun);

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

  //////////////////////initialize subrun index///////////////
  SubrunNum = 0;
  ANNIEEventNum = 0;

  return true;
}


bool ANNIEEventBuilder::Execute(){
  if((ANNIEEventNum/EntriesPerSubrun - 1) == SubrunNum) SubrunNum+=1;

  //Get the current FinishedPMTWaves map
  m_data->CStore.Get("FinishedPMTWaves",FinishedPMTWaves);
  //Check to see if any trigger times have all their PMTs
  for(std::pair<uint64_t,std::map<std::vector<int>, std::vector<uint16_t>>> apair : FinishedPMTWaves){
    uint64_t PMTClockTime = apair.first;
    std::map<std::vector<int>, std::vector<uint16_t>> aWaveMap = apair.second;
    if(aWaveMap.size()>NumWavesInSet){
      std::cout << "PMTClockTime " << PMTClockTime << " has more than " << NumWavesInSet << ". " <<
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
      this->BuildANNIEEvent(PMTClockTime, aWaveMap);
    }
  }

  return true;
}


bool ANNIEEventBuilder::Finalise(){
  // Make the ANNIEEvent Store if it doesn't exist
  // =============================================
  int annieeventexists = m_data->Stores.count("ANNIEEvent");
  if(annieeventexists==0) m_data->Stores["ANNIEEvent"] = new BoostStore(false,2);

  //FIXME: Have the ANNIEEvent booststore we made be saved in "The Store"
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

void ANNIEEventBuilder::BuildANNIEEvent(uint64_t PMTClockTime, 
        std::map<std::vector<int>, std::vector<uint16_t>> WaveMap)
{
  std::cout << "Building an ANNIE Event" << std::endl;
  for(std::pair<std::vector<int>, std::vector<uint16_t>> apair : WaveMap){
    int CardID = apair.first.at(0);
    int ChannelID = apair.first.at(1);
    std::vector<uint16_t> TheWaveform = apair.second;
    unsigned long ChannelKey = this->GetWavesChannelKey(CardID,ChannelID);
    //Then, convert the wave into a RawWaveform class.
    //FIXME: Waveform class expects a double, not a uint64_t
    Waveform TheWave(PMTClockTime, TheWaveform);
    //Lastly, we need to put these into an ANNIEEvent BoostStore!
  }
  //TODO: Trigger information needs to be identified with the PMT Clock Time and
  //also added to the ANNIEEvent.  For now, just push the Clock Time as the
  //Trigger Time.
  
  ////////////////////// TRIGGER DATA ACCESSING EXAMPLE  /////////////////////// 
  //TrigData->GetEntry(MatchEntry);
  //TriggerData Tdata;
  //TrigData->Get("TrigData",Tdata);
  //uint16_t EventID = Tdata.EventIDs.at(MatchIndex);
  //uint64_t EventTime = Tdata.EventTimes.at(MatchIndex);
  //TimeClass TrigTime(EventTime);
  //uint32_t TriggerMask = Tdata.TriggerMasks.at(MatchIndex);
  //uint32_t TriggerCounter = Tdata.TriggerCounters.at(MatchIndex);
  return;
}

unsigned long ANNIEEventBuilder::GetWavesChannelKey(int CardID, int ChannelID)
{
  std::cout << "Getting channel key for given CardID and ChannelID" << std::endl;
  //TODO: We need a function that converts CardID and ChannelID into
  //our ChannelKey.
  //Use Geometry information to get the right ChannelKey given this info.
  unsigned long ChannelKey = 42;
  return ChannelKey;
}


