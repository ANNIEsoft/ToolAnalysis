#include "PMTDataDecoder.h"

PMTDataDecoder::PMTDataDecoder():Tool(){}


bool PMTDataDecoder::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  verbosity = 0;
  CardDataEntriesPerExecute = 5;
  LockStep = 0;

  m_variables.Get("verbosity",verbosity);
  m_variables.Get("InputFile",InputFile);
  m_variables.Get("LockStep",LockStep);
  m_variables.Get("CardDataEntriesPerExecute",CardDataEntriesPerExecute);

  std::cout << "Initializing BoostStore" << std::endl;
  // Initialize RawData
  RawData = new BoostStore(false,0);
  RawData->Initialise(InputFile.c_str());
  RawData->Print(false);

  std::cout << "RawData BoostStore Initialized.  Now, the PMTData" << std::endl;
  /////////////////// getting PMT Data ////////////////////
  PMTData = new BoostStore(false,2);
  RawData->Get("PMTData",*PMTData);

  std::cout << "Next, set print to false and get total entries" << std::endl;
  PMTData->Print(false);
  
  long entries=0;
  PMTData->Header->Get("TotalEntries",entries);
  std::cout<<"Total entries in PMTData store: "<<entries<<std::endl;
  
  std::cout << "PMTDataDecoder Tool: Initialized successfully" << std::endl;
  return true;
}


bool PMTDataDecoder::Execute(){
  int NumPMTDataProcessed = 0;
  while(NumPMTDataProcessed<CardDataEntriesPerExecute){
    std::cout<<"entry "<<CDEntryNum<<std::endl;
    PMTData->GetEntry(CDEntryNum);
    PMTData->Get("CardData",Cdata);
    std::cout<<"Cdata size="<<Cdata.size()<<std::endl;
    std::cout<<"CardData in Cdata's 0th index CardID="<<Cdata.at(0).CardID<<std::endl;
    std::cout<<"CardData in Cdata's 0th index data size="<<Cdata.at(0).Data.size()<<std::endl;
    
    //###### MOCK-UP FOR HOW TO LOOP THROUGH AN ENTRY's VECTOR OF CARD DATA
    for (unsigned int CardDataIndex=0; CardDataIndex<Cdata.size(); CardDataIndex++){
      CardData aCardData = Cdata.at(CardDataIndex);
      bool IsNextInSequence = this->CheckIfCardNextInSequence(aCardData);
      if (IsNextInSequence) {
        std::cout << "CardData is next in it's sequence!  Decoding..." << std::endl;
        std::cout << "Decoding CardID " << aCardData.CardID << ", SequenceID " <<
            aCardData.SequenceID << std::endl;
        //For this CardData entry, decode raw binary frames
        std::vector<DecodedFrame> ThisCardDFs;
        ThisCardDFs = this->DecodeFrames(aCardData.Data);
        //Now, loop through each frame and Parse their information
        for (unsigned int i=0; i < ThisCardDFs.size(); i++){
          this->ParseFrame(aCardData.CardID,ThisCardDFs.at(i));
        }
      } else {
        std::cout << "Card is out of sequence.  Store it for later." << std::endl;
        //This CardData will be needed later when it's next in sequence.  
        //Log it in the Out-Of-Order (OOO) Data seen so far.
        int OOOCardID = aCardData.CardID; 
        int OOOSequenceID = aCardData.SequenceID;
        int OOOBoostEntry = CDEntryNum;
        int OOOCardDataIndex = CardDataIndex;
        std::vector<int> OOOProperties{OOOSequenceID, OOOBoostEntry, OOOCardDataIndex};
        std::cout << "Storing CardID" << OOOCardID << " of sequenceID " << 
            OOOSequenceID << "into UnprocessedEntries" << std::endl;
        if(UnprocessedEntries.count(OOOCardID)==0){
          std::vector<std::vector<int>> OOOVector;
          OOOVector.push_back(OOOProperties);
          UnprocessedEntries.emplace(OOOCardID,OOOVector);
        } else {
          UnprocessedEntries.at(OOOCardID).push_back(OOOProperties);
        }
      } 
    }
    CDEntryNum+=1;
    NumPMTDataProcessed+=1;
  }

  ///////////////Search through Out-Of-Order Cards;/////////////
  ///////////////See if they are in order now ///////////////// 
  this->ParseOOOsNowInOrder();
   
  //PMT Data done being processed this loop.  Push pointer to the
  //CStore and use it in the ANNIEEventBuilder tool.
  //Any of the MTCCounters have all their PMT data.
  m_data->CStore.Set("FinishedPMTWaves",FinishedPMTWaves);
  //###### END MOCK-UP ######
  //
  //Check the size of the WaveBank to see if things are bloating
  std::cout << "Size of WaveBank (# events in building progress): " << WaveBank.size() << std::endl;
  std::cout << "Size of FinishedPMTWaves (# triggers with at least one wave built): " << FinishedPMTWaves.size() << std::endl;
  //TODO: Print out, if debugging, the size of each entry in FinishedPMTWaves

  
  ///////////////////////////////////////////

  return true;
}


bool PMTDataDecoder::Finalise(){
  delete RawData;
  delete PMTData;
  std::cout << "PMTDataDecoder tool exitting" << std::endl;
  return true;
}

bool PMTDataDecoder::CheckIfCardNextInSequence(CardData aCardData)
{
  bool IsNextInSequence = false;
  //Check if this CardData is next in it's sequence for processing
  std::map<int, int>::iterator it = SequenceMap.find(aCardData.CardID);
  if(it != SequenceMap.end()){ //Data from this Card has been seen before
    if (it->second == aCardData.CardID){ //This CardData is expected next
      IsNextInSequence = true;
      it->second+=1;
    }
  } else if ((it == SequenceMap.end()) && (aCardData.SequenceID==0)){  //This is the first CardData seen by this CardID
    SequenceMap.emplace(aCardData.CardID, 1); //Next in sequence is SequenceID == 1
    IsNextInSequence = true;
  }
  return IsNextInSequence;
}

void PMTDataDecoder::ParseOOOsNowInOrder()
{
  //Now, we want to loop through the Out-Of-Order card data and see if any of it
  //is in order.  Each time a CardData is actually in order now, look again after
  //parsing the In-Order to see if any other cards are also now in order.
  bool OOOResolved = false;
  for (std::pair<int, std::vector<std::vector<int>>> OOOpair: UnprocessedEntries) {
    bool IsNextInSequence = false;
    //Check if any of this Card's unprocessed entries are next in sequence
    int OOOCardID = OOOpair.first;
    int NextInCardsSequence = SequenceMap.at(OOOCardID);
    std::vector<std::vector<int>> OOOProperties = OOOpair.second;
    for (unsigned int i=0; i < OOOProperties.size();i++){
      std::vector<int> OOOProperty = OOOProperties.at(i);
      int OOOSequenceID = OOOProperty.at(0);
      int OOOBoostEntry = OOOProperty.at(1);
      int OOOCardVectorInd = OOOProperty.at(2);
      if(OOOSequenceID == NextInCardsSequence){
        //This OOO CardData is now next in order.  Parse it.
        PMTData->GetEntry(OOOBoostEntry);
        PMTData->Get("CardData",Cdata);
        CardData aCardData = Cdata.at(OOOCardVectorInd);
        std::vector<DecodedFrame> ThisCardDFs;
        ThisCardDFs = this->DecodeFrames(aCardData.Data);
        //Now, loop through each frame and Parse their information
        for (unsigned int i=0; i < ThisCardDFs.size(); i++){
          this->ParseFrame(aCardData.CardID,ThisCardDFs.at(i));
        }
        OOOResolved = true;
      }
    }
  }
  if (OOOResolved) this->ParseOOOsNowInOrder();
  else return;
}

std::vector<DecodedFrame> PMTDataDecoder::DecodeFrames(std::vector<uint32_t> bank)
{
  std::cout << "LETS DECODE SOME FRAAAAAAMESSSSSSS" << std::endl;
  std::cout << "Bank size is: " << bank.size() << std::endl;
  uint64_t tempword;
  std::vector<DecodedFrame> frames;  //What we will return
  std::vector<uint16_t> samples;
  samples.resize(40); //Well, if there's 480 bits per frame of samples max, this fits it
  for (unsigned int frame = 0; frame<bank.size()/16; ++frame) {  // if each frame has 16 32-bit ints, nframes = bank_size/16
    struct DecodedFrame thisframe;
    int sampleindex = 0;
    int wordindex = 16*frame;  //Index of first 32-bit int in this frame
    int bitsleft = 0;  //# bits to shift to the left in the temp word
    bool haverecheader_part2 = false;
    while (sampleindex < 40) {  //Parse out this whole frame
      if (bitsleft < 12) {
        std::cout << "TEMP WORD BEFORE be32toh AND LEFT SHIFT: " << std::hex << "0x" << tempword << std::endl;
        std::bitset<64> tempbin(tempword);
        std::cout << "TEMP WORD BEFORE be32toh AND LEFT SHIFT: " << tempbin << std::endl;
        tempword += ((uint64_t)be32toh(bank[wordindex]))<<bitsleft;
        std::cout << "TEMP WORD AFTER be32toh AND LEFT SHIFT: " << std::hex << "0x" << tempword << std::endl;
        std::bitset<64> tempbin2(tempword);
        std::cout << "TEMP WORD BEFORE be32toh AND LEFT SHIFT: " << tempbin2 << std::endl;
        bitsleft += 32;
        wordindex += 1;
      }
      samples[sampleindex] = tempword&0xfff;  //You're only taking the first 12 bits of the tempword
      if((tempword&0xfff)==RECORD_HEADER_LABELPART2) haverecheader_part2 = true;
      else if (haverecheader_part2 && ((tempword&0xfff)==RECORD_HEADER_LABELPART1)){
        thisframe.has_recordheader=true;
        thisframe.recordheader_000indices.push_back(sampleindex);
      }
      else haverecheader_part2 = false;
      tempword = tempword>>12;
      bitsleft -= 12;
      sampleindex += 1;
    } //END parse out this frame
    thisframe.frameheader = be32toh(bank[16*frame+15]);  //Frameid is held in the frame's last 32-bit word
    thisframe.samples = samples;
    frames.push_back(thisframe);
  }
  std::cout << "FINISHED DECODING FRAMES " << std::endl;
  return frames;
}

void PMTDataDecoder::ParseFrame(int CardID, DecodedFrame DF)
{
  int ChannelID = DF.frameheader; //FIXME: We probably need a function that gets the
                                  //Bitrange defined by Jonathan (511 downto 504)
  if(!DF.has_recordheader){
    //All samples are waveforms for channel record that already exists in the WaveBank.
    this->AddSamplesToWaveBank(CardID, ChannelID, DF.samples);
  } else {
    int WaveSecBegin = 0;
    //We need to get the rest of a wave from WaveSecBegin to where the header starts
    for (unsigned int j = 0; j<DF.recordheader_000indices.size(); j++){
      std::vector<uint16_t> WaveSlice(DF.samples.begin()+WaveSecBegin, 
              DF.samples.begin()+DF.recordheader_000indices.at(j)- SAMPLES_LEFTOF_000-1);
      //Add this WaveSlice to the wave bank
      this->AddSamplesToWaveBank(CardID, ChannelID, WaveSlice);
      //Since we have acquired the wave up to the next record header, the wave is done.
      //Store it in the FinishedWaves map.
      this->StoreFinishedWaveform(CardID, ChannelID);
      //Now, we have the header coming next.  Get it and parse it, starting whatever
      //Entries in maps are needed.  
      std::vector<uint16_t> RecordHeader(DF.samples.begin()+
              DF.recordheader_000indices.at(j) - SAMPLES_LEFTOF_000, DF.samples.begin()+
              DF.recordheader_000indices.at(j)+SAMPLES_RIGHTOF_000);
      this->ParseRecordHeader(CardID, ChannelID, RecordHeader);
      WaveSecBegin = DF.recordheader_000indices.at(j)+SAMPLES_RIGHTOF_000+1;
    }
    // No more record headers from here; just parse the rest of whatever 
    // waveform is being looked at
    int WaveSecEnd = DF.samples.size()-1;
    std::vector<uint16_t> WaveSlice(WaveSecBegin, WaveSecEnd);
    this->AddSamplesToWaveBank(CardID, ChannelID, WaveSlice);
  }
  return;
}

void PMTDataDecoder::ParseRecordHeader(int CardID, int ChannelID, std::vector<uint16_t> RH)
{
  //We need to get the MTC count and make a new entry in TriggerTimeBank and WaveBank
  //First 4 samples; Just get the bits from 24 to 37 (is counter (61 downto 48)
  //Last 4 samples; All the first 48 bits of the MTC count.
  std::vector<uint16_t> CounterEnd(RH.begin(), RH.begin()+2);
  std::vector<uint16_t> CounterBegin(RH.begin()+4, RH.begin()+7);
  uint64_t MTCCounter;
  int samplewidth=12;  //each uint16 really only holds 12 bits of info. (see DecodeFrame)
  for (unsigned int j=0; j<CounterBegin.size(); j++){
    MTCCounter += (CounterBegin.at(j) << j*samplewidth);
  }
  for (unsigned int j=0; j<CounterEnd.size(); j++){
    MTCCounter += (CounterEnd.at(j) << (CounterBegin.size() + j*samplewidth));
  }
  std::vector<int> wave_key{CardID,ChannelID};
  std::vector<uint16_t> Waveform;
  //FIXME: So the Counter is still little endian here; is that what we want?  I think so..
  //Update the TriggerTimeBank and WaveBank with new entries, since this channel's
  //Wave data is coming up next
  TriggerTimeBank.emplace(wave_key,MTCCounter);
  WaveBank.emplace(wave_key,Waveform);
  return;
}


void PMTDataDecoder::StoreFinishedWaveform(int CardID, int ChannelID)
{
  //Get the full waveform from the Wave Bank
  std::vector<int> wave_key{CardID,ChannelID};
  std::vector<uint16_t> FinishedWave = WaveBank.at(wave_key);
  uint64_t FinishedWaveTrigTime = TriggerTimeBank.at(wave_key);

  //TODO: Should we have a check the waveform is the length expected? 
  std::map<std::vector<int>, std::vector<uint16_t> > WaveMap;

  if(FinishedPMTWaves.count(FinishedWaveTrigTime) == 0) {
    WaveMap.emplace(wave_key,FinishedWave);
    FinishedPMTWaves.emplace(FinishedWaveTrigTime,WaveMap);
  } else {
    FinishedPMTWaves.at(FinishedWaveTrigTime).emplace(wave_key,FinishedWave);
  }
  //Clear the finished wave from WaveBank and TriggerTimeBank for the new wave
  //to start being put together
  WaveBank.erase(wave_key);
  TriggerTimeBank.at(wave_key);
  return;
}
  
void PMTDataDecoder::AddSamplesToWaveBank(int CardID, int ChannelID, 
        std::vector<uint16_t> WaveSlice)
{
  std::cout << "NUMBER OF SAMPLES IN WAVESLICE: " << WaveSlice.size() << std::endl;
  //TODO: Make sure the above is always divisible by 4!
  //Add the WaveSlice to the proper vector in the WaveBank.
  std::vector<int> wave_key{CardID,ChannelID};
  WaveBank.at(wave_key).insert(WaveBank.at(wave_key).end(),
                               WaveSlice.begin(),
                               WaveSlice.end());
  return;
}

