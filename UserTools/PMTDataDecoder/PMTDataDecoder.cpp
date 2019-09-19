#include "PMTDataDecoder.h"

PMTDataDecoder::PMTDataDecoder():Tool(){}


bool PMTDataDecoder::Initialise(std::string configfile, DataModel &data){
  std::cout << "Initializing PMTDataDecoder tool" << std::endl;
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

  Log("PMTDataDecoder Tool: Raw Data file as BoostStore",v_message,verbosity); 
  // Initialize RawData
  RawData = new BoostStore(false,0);
  RawData->Initialise(InputFile.c_str());
  RawData->Print(false);

  /////////////////// getting PMT Data ////////////////////
  Log("PMTDataDecoder Tool: Accessing PMT Data in raw data",v_message,verbosity); 
  PMTData = new BoostStore(false,2);
  RawData->Get("PMTData",*PMTData);

  PMTData->Print(false);
  
  long entries=0;
  PMTData->Header->Get("TotalEntries",entries);

  if(verbosity>v_message) std::cout<<"Total entries in PMTData store: "<<entries<<std::endl;
  
  std::cout << "PMTDataDecoder Tool: Initialized successfully" << std::endl;
  return true;
}


bool PMTDataDecoder::Execute(){
  int NumPMTDataProcessed = 0;
  while(NumPMTDataProcessed<CardDataEntriesPerExecute){
	Log("PMTDataDecoder Tool: Procesing PMTData Entry "+to_string(CDEntryNum),v_debug, verbosity);
    PMTData->GetEntry(CDEntryNum);
    PMTData->Get("CardData",Cdata);
	Log("PMTDataDecoder Tool: entry has #CardData classes = "+to_string(Cdata.size()),v_debug, verbosity);
    std::cout<<"CardData in Cdata's 0th index CardID="<<Cdata.at(0).CardID<<std::endl;
    std::cout<<"CardData in Cdata's 0th index data size="<<Cdata.at(0).Data.size()<<std::endl;
    
    for (unsigned int CardDataIndex=0; CardDataIndex<Cdata.size(); CardDataIndex++){
      CardData aCardData = Cdata.at(CardDataIndex);
      bool IsNextInSequence = this->CheckIfCardNextInSequence(aCardData);
      if (IsNextInSequence) {
	    Log("PMTDataDecoder Tool: CardData is next in sequence. Decoding... ",v_debug, verbosity);
	    Log("PMTDataDecoder Tool:  Decoding CardID... "+to_string(aCardData.CardID),v_debug, verbosity);
	    Log("PMTDataDecoder Tool:  CardData has SequenceID... "+to_string(aCardData.SequenceID),v_debug, verbosity);
        //For this CardData entry, decode raw binary frames.  Locates header markers
        //And separates the Frame Header from the data stream bits.
        std::vector<DecodedFrame> ThisCardDFs;
        ThisCardDFs = this->DecodeFrames(aCardData.Data);
        //Loop through each decoded frame and Parse their data stream and 
        //frame header.  Data stream infomration is moved to the
        //TriggerTimeBank and WaveBank, and any finished waveform is moved from 
        //the TriggerTimeBank/WaveBank to the FinishedPMTWaves
        for (unsigned int i=0; i < ThisCardDFs.size(); i++){
          this->ParseFrame(aCardData.CardID,ThisCardDFs.at(i));
        }
      } else {
	    Log("PMTDataDecoder Tool WARNING: CardData OUT OF SEQUENCE!!!",v_warning, verbosity);
        //This CardData will be needed later when it's next in sequence.  
        //Log it in the Out-Of-Order (OOO) Data seen so far.
        int OOOCardID = aCardData.CardID; 
        int OOOSequenceID = aCardData.SequenceID;
        int OOOBoostEntry = CDEntryNum;
        int OOOCardDataIndex = CardDataIndex;
        std::vector<int> OOOProperties{OOOSequenceID, OOOBoostEntry, OOOCardDataIndex};
	    Log("PMTDataDecoder Tool:  Storing CardID... " +
                to_string(aCardData.CardID),v_warning, verbosity);
	    Log("PMTDataDecoder Tool:  Storing Of SequenceID... " + 
                to_string(aCardData.SequenceID),v_warning, verbosity);
	    Log("PMTDataDecoder Tool:  Into UnprocessedEntries Map ",v_warning, verbosity);
        if(UnprocessedEntries.count(OOOCardID)==0){
          deque<std::vector<int>> OOOqueue;
          OOOqueue.push_back(OOOProperties);
          UnprocessedEntries.emplace(OOOCardID,OOOqueue);
        } else {
          if (OOOSequenceID < UnprocessedEntries.at(OOOCardID).at(0).at(0)){
            //This new out-of-order entry has the smallest SequenceID
            UnprocessedEntries.at(OOOCardID).push_front(OOOProperties);
          } else {
            //Not the earliest for now; just put it at the back
            UnprocessedEntries.at(OOOCardID).push_back(OOOProperties);
          }
        }
      } 
    }
	Log("PMTDataDecoder Tool: Decoding or Unprocessed logging complete",v_debug, verbosity);
    CDEntryNum+=1;
    NumPMTDataProcessed+=1;
  }

  ///////////////Search through All Out-Of-Order Cards;/////////////
  ///////////////See if they are in order now ///////////////// 
  this->ParseOOOsNowInOrder();
   
  //PMT Data done being processed this execute loop.  
  //Push the map of FinishedWaves to the CStore for ANNIEEvent to start 
  //Building ANNIEEvents. 
  std::cout << "SET FINISHED WAVES IN THE CSTORE" << std::endl;
  if(FinishedPMTWaves.empty()){
	Log("PMTDataDecoder Tool: No finished PMT waves available.  Not setting CStore.",v_debug, verbosity);
  } else {
	Log("PMTDataDecoder Tool: Saving Finished PMT waves to  CStore.",v_debug, verbosity);
    m_data->CStore.Set("FinishedPMTWaves",FinishedPMTWaves);
  }
  std::cout << "CSTORE SET" << std::endl;
  //Check the size of the WaveBank to see if things are bloating
  Log("PMTDataDecoder Tool: Size of WaveBank (# events in building progress): " + 
          to_string(WaveBank.size()),v_debug, verbosity);
  std::cout << "Size of WaveBank (# events in building progress): " << WaveBank.size() << std::endl;
  Log("PMTDataDecoder Tool: Size of FinishedPMTWaves (# triggers with at least one wave fully):" + 
          to_string(FinishedPMTWaves.size()),v_debug, verbosity);
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
    if (it->second == aCardData.SequenceID){ //This CardData is expected next
      IsNextInSequence = true;
      it->second+=1;
    }
  } else if ((it == SequenceMap.end()) && (aCardData.SequenceID==0)){  //This is the first CardData seen by this CardID
    SequenceMap.emplace(aCardData.CardID, 1); //Next in sequence is SequenceID == 1
    IsNextInSequence = true;
  } else if ((it == SequenceMap.end()) && (aCardData.SequenceID!=0)){  //This is the first CardData seen by this CardID, but is OOO
    Log("PMTDataDecoder Tool: WARNING! First data seen for this card is not SequenceID=0",v_warning,verbosity);
    SequenceMap.emplace(aCardData.CardID, 0);
  }
  return IsNextInSequence;
}

bool PMTDataDecoder::ParseOneCardOOOs(int CardID)
{
  //Now, we want to loop through the Out-Of-Order card data and see if any of it
  //is in order.  Each time a CardData is actually in order now, look again after
  //parsing the In-Order to see if any other cards are also now in order.
  bool ProcessedAnOOO = false;
  Log("PMTDataDecoder Tool: Parsing any in-order data for CardID "+to_string(CardID),v_debug,verbosity); 
  deque<std::vector<int>> OOOProperties = UnprocessedEntries.at(CardID);
  bool IsNextInSequence = false;
  Log("PMTDataDecoder Tool: Get this CardIDs next in sequence ",v_debug,verbosity); 
  int NextInCardsSequence = SequenceMap.at(CardID);
  std::vector<int> NowInOrderInds;
  Log("PMTDataDecoder Tool: Looping through CardID OOOProperties ",v_debug,verbosity); 
  for (unsigned int i=0; i < OOOProperties.size();i++){
    std::vector<int> OOOProperty = OOOProperties.at(i);
    int OOOSequenceID = OOOProperty.at(0);
    int OOOBoostEntry = OOOProperty.at(1);
    int OOOCardVectorInd = OOOProperty.at(2);
    if(OOOSequenceID == NextInCardsSequence){
      //This OOO CardData is now next in order.  Parse it.
      Log("PMTDataDecoder Tool: Out of order card is next in sequence!",v_debug,verbosity); 
      PMTData->GetEntry(OOOBoostEntry);
      PMTData->Get("CardData",Cdata);
      CardData aCardData = Cdata.at(OOOCardVectorInd);
      std::vector<DecodedFrame> ThisCardDFs;
      ThisCardDFs = this->DecodeFrames(aCardData.Data);
      //Now, loop through each frame and Parse their information
      for (unsigned int j=0; j < ThisCardDFs.size(); j++){
        this->ParseFrame(aCardData.CardID,ThisCardDFs.at(j));
      }
      NowInOrderInds.push_back(i);
      ProcessedAnOOO = true;
    }
  }
  //Delete all OOOProperties that are now in order.  Start from the
  //Back index and move forward  in deletion.
  std::sort(NowInOrderInds.begin(), NowInOrderInds.end());
  std::reverse(NowInOrderInds.begin(), NowInOrderInds.end());
  for (unsigned int j=0;j < NowInOrderInds.size();j++){
    int OrderedInd = NowInOrderInds.at(j);
    OOOProperties.erase(OOOProperties.begin()+OrderedInd);
  }
  UnprocessedEntries.at(CardID) = OOOProperties;
  //If any data for a card that was out-of-order was re-ordered, loop through again
  //And try to resolve any now-fixed OOOs
  return ProcessedAnOOO;
}


void PMTDataDecoder::ParseOOOsNowInOrder()
{
  //Now, we want to loop through the Out-Of-Order card data and see if any of it
  //is in order.  Each time a CardData is actually in order now, look again after
  //parsing the In-Order to see if any other cards are also now in order.
  std::vector<int> UnprocCardIDs;
  for (std::map<int, deque<std::vector<int>>>::iterator it=UnprocessedEntries.begin();
          it!=UnprocessedEntries.end(); ++it) {
    UnprocCardIDs.push_back(it->first);
  }
  for(int i=0;i<UnprocCardIDs.size(); i++){
    int UnprocCardID = UnprocCardIDs.at(i);
    bool OOOResolved = true;
    while(OOOResolved){ //Keep re-parsing out-of-order card data as long as one is found in order
      OOOResolved = this->ParseOneCardOOOs(UnprocCardID);
    }
  }
  return;
}

std::vector<DecodedFrame> PMTDataDecoder::DecodeFrames(std::vector<uint32_t> bank)
{
  Log("PMTDataDecoder Tool: Decoding frames now ",v_debug, verbosity);
  Log("PMTDataDecoder Tool: Bank size is "+to_string(bank.size()),v_debug, verbosity);
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
  Log("PMTDataDecoder Tool: Decoding frames complete ",v_debug, verbosity);
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
  Log("PMTDataDecoder Tool: Parsing an encountered header ",v_debug, verbosity);
  std::vector<uint16_t> CounterEnd(RH.begin(), RH.begin()+2);
  std::vector<uint16_t> CounterBegin(RH.begin()+4, RH.begin()+7);
  uint64_t ClockCount;
  int samplewidth=12;  //each uint16 really only holds 12 bits of info. (see DecodeFrame)
  for (unsigned int j=0; j<CounterBegin.size(); j++){
    ClockCount += (CounterBegin.at(j) << j*samplewidth);
  }
  for (unsigned int j=0; j<CounterEnd.size(); j++){
    ClockCount += (CounterEnd.at(j) << (CounterBegin.size() + j*samplewidth));
  }
  std::vector<int> wave_key{CardID,ChannelID};
  std::vector<uint16_t> Waveform;
  
  //Update the TriggerTimeBank and WaveBank with new entries, since this channel's
  //Wave data is coming up next
  Log("PMTDataDecoder Tool: Parsed Clock time for header is "+to_string(ClockCount),v_debug, verbosity);
  TriggerTimeBank.emplace(wave_key,ClockCount);
  WaveBank.emplace(wave_key,Waveform);
  return;
}


void PMTDataDecoder::StoreFinishedWaveform(int CardID, int ChannelID)
{
  //Get the full waveform from the Wave Bank
  std::vector<int> wave_key{CardID,ChannelID};
  std::vector<uint16_t> FinishedWave = WaveBank.at(wave_key);
  uint64_t FinishedWaveTrigTime = TriggerTimeBank.at(wave_key);
  Log("PMTDataDecoder Tool: Finished Wave Length"+to_string(WaveBank.size()),v_debug, verbosity);
  Log("PMTDataDecoder Tool: Finished Wave Clock time"+to_string(FinishedWaveTrigTime),v_debug, verbosity);

  if(FinishedPMTWaves.count(FinishedWaveTrigTime) == 0) {
    std::map<std::vector<int>, std::vector<uint16_t> > WaveMap;
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
  Log("PMTDataDecoder Tool: Adding Waveslice to waveform.  Num. Samples: "+to_string(WaveSlice.size()),v_debug, verbosity);
  //TODO: Make sure the above is always divisible by 4!
  //Add the WaveSlice to the proper vector in the WaveBank.
  std::vector<int> wave_key{CardID,ChannelID};
  WaveBank.at(wave_key).insert(WaveBank.at(wave_key).end(),
                               WaveSlice.begin(),
                               WaveSlice.end());
  return;
}

