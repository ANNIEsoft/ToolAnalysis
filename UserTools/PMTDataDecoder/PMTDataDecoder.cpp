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


  m_variables.Get("verbosity",verbosity);
  m_variables.Get("Mode",Mode);
  m_variables.Get("InputFile",InputFile);

  //Default mode of operation is the continuous flow of data for the live monitoring
  //The other possibility is reading in data from a specified list of files
  if (Mode != "Continuous" && Mode != "SingleFile") {
    if (verbosity > 0) std::cout <<"ERROR (PMTDataDecoder): Specified mode of operation ("<<Mode<<") is not an option [Continuous/SingleFile]. Setting default Continuous mode"<<std::endl;
    Mode = "Continuous";
  }

  m_data->CStore.Set("NewTankPMTDataAvailable",false);
  std::cout << "PMTDataDecoder Tool: Initialized successfully" << std::endl;
  return true;
}


bool PMTDataDecoder::Execute(){
  // Load 
  if(Mode=="SingleFile"){
    if(SingleFileLoaded){
      std::cout << "PMTDataDecoder tool: Single file has already been loaded" << std::endl;
      return true;
    } else {
      Log("PMTDataDecoder Tool: Raw Data file as BoostStore",v_message,verbosity); 
      // Initialize RawData
      RawData = new BoostStore(false,0);
      RawData->Initialise(InputFile.c_str());
      RawData->Print(false);

    }
  }

  else if (Mode == "Continuous"){
    std::string State;
    m_data->CStore.Get("State",State);
    std::cout << "PMTDataDecoder tool: checking CStore for status of data stream" << std::endl;
    if (State == "PMTSingle" || State == "Wait"){
      //Single event file available for monitoring; not relevant for this tool
      if (verbosity > 2) std::cout <<"PMTDataDecoder: State is "<<State<< ". No data file available" << std::endl;
      return true; 
    } else if (State == "DataFile"){
      // Full PMTData file ready to parse
      // FIXME: Not sure if the booststore or key are right, or even the DataFile State
      if (verbosity > 1) std::cout<<"PMTDataDecoder: New data file available."<<std::endl;
      m_data->Stores["CCData"]->Get("FileData",RawData);
      RawData->Print(false);
      
    }
  }
  /////////////////// getting PMT Data ////////////////////
  Log("PMTDataDecoder Tool: Accessing PMT Data in raw data",v_message,verbosity); 
  PMTData = new BoostStore(false,2);
  RawData->Get("PMTData",*PMTData);
  PMTData->Print(false);

  Log("PMTDataDecoder Tool: Accessing run information data",v_message,verbosity); 
  BoostStore RunInfo(false,0);
  RawData->Get("RunInformation",RunInfo);
  RunInfo.Print(false);

  Store Postgress;
  RunInfo.Get("Postgress",Postgress);
  Postgress.Print();

  int RunNumber;
  int SubRunNumber;
  uint64_t StarTime;
  int RunType;

  Postgress.Get("RunNumber",RunNumber);
  Postgress.Get("SubRunNumber",SubRunNumber);
  Postgress.Get("RunType",RunType);
  Postgress.Get("StarTime",StarTime);

  if(verbosity>v_message) std::cout<<"Processing RunNumber: "<<RunNumber<<std::endl;
  if(verbosity>v_message) std::cout<<"Processing SubRunNumber: "<<SubRunNumber<<std::endl;
  if(verbosity>v_message) std::cout<<"Run is of run type: "<<RunType<<std::endl;
  if(verbosity>v_message) std::cout<<"StartTime of Run: "<<StarTime<<std::endl;

  // Show the total entries in this file  
  PMTData->Header->Get("TotalEntries",totalentries);
  if(verbosity>v_message) std::cout<<"Total entries in PMTData store: "<<totalentries<<std::endl;

  NumPMTDataProcessed = 0;
  CDEntryNum = 0;
  Log("PMTDataDecoder Tool: Parsing entire PMTData booststore this loop",v_message,verbosity); 
  while(CDEntryNum < totalentries){
	Log("PMTDataDecoder Tool: Procesing PMTData Entry "+to_string(CDEntryNum),v_debug, verbosity);
    PMTData->GetEntry(CDEntryNum);
    PMTData->Get("CardData",Cdata);
	Log("PMTDataDecoder Tool: entry has #CardData classes = "+to_string(Cdata.size()),v_debug, verbosity);
    
    for (unsigned int CardDataIndex=0; CardDataIndex<Cdata.size(); CardDataIndex++){
      if(verbosity>3){
        std::cout<<"PMTDataDecoder Tool: Loading next CardData from entry's index " << CardDataIndex <<std::endl;
        std::cout<<"PMTDataDecoder Tool: CardData's CardID="<<Cdata.at(CardDataIndex).CardID<<std::endl;
        std::cout<<"PMTDataDecoder Tool: CardData's data vector size="<<Cdata.at(CardDataIndex).Data.size()<<std::endl;
      }
      CardData aCardData = Cdata.at(CardDataIndex);
      bool IsNextInSequence = this->CheckIfCardNextInSequence(aCardData);
      if (IsNextInSequence) {
	    Log("PMTDataDecoder Tool: CardData is next in sequence. Decoding... ",v_debug, verbosity);
	    Log("PMTDataDecoder Tool:  CardData has SequenceID... "+to_string(aCardData.SequenceID),v_debug, verbosity);
        //For this CardData entry, decode raw binary frames.  Locates header markers
        //And separates the Frame Header from the data stream bits.
        std::vector<DecodedFrame> ThisCardDFs;
        ThisCardDFs = this->DecodeFrames(aCardData.Data);
        //Loop through each decoded frame and Parse their data stream and 
        //frame header.  
        for (unsigned int i=0; i < ThisCardDFs.size(); i++){
          this->ParseFrame(aCardData.CardID,ThisCardDFs.at(i));
        }
        NumPMTDataProcessed+=1;
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
	    Log("PMTDataDecoder Tool:  Into UnprocessedEntries. Map ",v_warning, verbosity);
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
    ///////////////Search through All Out-Of-Order Cards;/////////////
    ///////////////See if they are in order now ///////////////// 
    this->ParseOOOsNowInOrder();
  }

   
  //PMT Data file fully processed.   
  //Push the map of FinishedWaves to the CStore for ANNIEEvent to start 
  //Building ANNIEEvents. 
  std::cout << "SET FINISHED WAVES IN THE CSTORE" << std::endl;
  if(FinishedPMTWaves.empty()){
	Log("PMTDataDecoder Tool: No finished PMT waves available.  Not setting CStore.",v_debug, verbosity);
  } else {
	Log("PMTDataDecoder Tool: Saving Finished PMT waves into CStore.",v_debug, verbosity);
    m_data->CStore.Get("FinishedPMTWaves",CStorePMTWaves);
    CStorePMTWaves.insert(FinishedPMTWaves.begin(),FinishedPMTWaves.end());
    m_data->CStore.Set("FinishedPMTWaves",CStorePMTWaves);
    m_data->CStore.Set("RunInfoPostgress",Postgress);
    m_data->CStore.Set("NewTankPMTDataAvailable",true);
  }
  //Check the size of the WaveBank to see if things are bloating
  Log("PMTDataDecoder Tool: Size of WaveBank (# waveforms partially built): " + 
          to_string(WaveBank.size()),v_debug, verbosity);
  Log("PMTDataDecoder Tool: Size of FinishedPMTWaves from this execution (# triggers with at least one wave fully):" + 
          to_string(FinishedPMTWaves.size()),v_debug, verbosity);
  Log("PMTDataDecoder Tool: Size of Finished waves in CStore:" + 
          to_string(CStorePMTWaves.size()),v_debug, verbosity);

  std::cout << "PMT WAVE CSTORE SET SUCCESSFULLY.  Clearing FinishedPMTWaves map from this file." << std::endl;
  FinishedPMTWaves.clear();

  SingleFileLoaded = true;
  ////////////// END EXECUTE LOOP ///////////////
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
  } else if ((it == SequenceMap.end())){  //This is the first CardData seen by this CardID
    if (aCardData.SequenceID!=0) Log("PMTDataDecoder Tool: WARNING! First data seen for this card is not SequenceID=0",v_warning,verbosity);
    SequenceMap.emplace(aCardData.CardID, aCardData.SequenceID+1); //Assume this is the first sequenceID even if not zero
    IsNextInSequence = true;
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
      NumPMTDataProcessed+=1;
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
  if(verbosity>2) std::cout << "DECODING A CARDDATA'S DATA BANK.  SIZE OF BANK: " << bank.size() << std::endl;
  if(verbosity>2) std::cout << "THIS SHOULD HOLD AN INTEGER NUMBER OF FRAMES.  EACH FRAME HAS" << std::endl;
  if(verbosity>2) std::cout << "512 BITs, split into 16 32-bit INTEGERS.  THIS SHOUDL BE DIVISIBLE BY 16" << std::endl;
  for (unsigned int frame = 0; frame<bank.size()/16; ++frame) {  // if each frame has 16 32-bit ints, nframes = bank_size/16
    struct DecodedFrame thisframe;
    int sampleindex = 0;
    int wordindex = 16*frame;  //Index of first 32-bit int for this frame
    int bitsleft = 0;  //# bits to shift to the left in the temp word
    bool haverecheader_part1 = false;
    while (sampleindex < 40) {  //Parse out this whole frame
      if (bitsleft < 12) {
        if(verbosity>4) std::cout << "DATA STREAM STEP AT SAMPLE INDEX" << sampleindex << std::endl;
        tempword += ((uint64_t)be32toh(bank[wordindex]))<<bitsleft;
        if(verbosity>4) std::cout << "DATA STREAM SNAPSHOT WITH NEXT 32-bit WORD FROM FRAME " << std::bitset<64>(tempword) << std::endl;
        bitsleft += 32;
        wordindex += 1;
      }
      //Logic to search for record headers
      if((tempword&0xfff)==RECORD_HEADER_LABELPART1) haverecheader_part1 = true;
      else if (haverecheader_part1 && ((tempword&0xfff)==RECORD_HEADER_LABELPART2)){
        if(verbosity>4) std::cout << "FOUND A RECORD HEADER. AT INDEX " << sampleindex << std::endl;
        thisframe.has_recordheader=true;
        thisframe.recordheader_starts.push_back(sampleindex-1);
        haverecheader_part1 = false;
      }
      else haverecheader_part1 = false;
     
      //Takie the first 12 bits of the tempword at each loop, and shift tempword
      samples[sampleindex] = tempword&0xfff;
      if(verbosity>4) std::cout << "FIRST 12 BITS IN THIS SNAPSHOT: " << std::bitset<16>(tempword&0xfff) << dec << std::endl;
      tempword = tempword>>12;
      bitsleft -= 12;
      sampleindex += 1;
    } //END parse out this frame
    thisframe.frameheader = be32toh(bank[16*frame+15]);  //Frameid is held in the frame's last 32-bit word
    if(verbosity>4) std::cout << "FRAMEHEADER last 8 bits: " << std::bitset<32>(thisframe.frameheader>>24) << std::endl;
    thisframe.samples = samples;
    if(verbosity>4) std::cout << "LENGTH OF SAMPLES AFTER DECODING A FRAME: " << dec << thisframe.samples.size() << std::endl;
    frames.push_back(thisframe);
  }
  Log("PMTDataDecoder Tool: Decoding frames complete ",v_debug, verbosity);
  return frames;
}

void PMTDataDecoder::ParseFrame(int CardID, DecodedFrame DF)
{ 
  //Decoded frame infomration is moved to the
  //TriggerTimeBank and WaveBank.  
  //Get the ID in the frame header.  Need to know if a channel, or sync signal
  unsigned channel_mask; 
  int ChannelID = DF.frameheader >> 24; //TODO: Use something more intricate?
                                  //Bitrange defined by Jonathan (511 downto 504)
  if(verbosity>3) std::cout << "Parsing frame with CardID and ChannelID-" << 
      CardID << "," << ChannelID << std::endl;
  if(!DF.has_recordheader && (ChannelID != SYNCFRAME_HEADERID)){
    //All samples are waveforms for channel record that already exists in the WaveBank.
    this->AddSamplesToWaveBank(CardID, ChannelID, DF.samples);
  } else if (ChannelID != SYNCFRAME_HEADERID){
    int WaveSecBegin = 0;
    //We need to get the rest of a wave from WaveSecBegin to where the header starts
    //FIXME: this works if there's already a wave being built.  You need to parse 
    //a record header in the wavebank first if it's the first thing in the frame though
    if(verbosity>3) {
      std::cout << "This decoded frame has headers at... " << std::endl;
      for (unsigned int j = 0; j<DF.recordheader_starts.size(); j++){
          std::cout << DF.recordheader_starts.at(j) << std::endl;
      }
    }
    for (unsigned int j = 0; j<DF.recordheader_starts.size(); j++){
      //TODO: More graceful way to handle this?  It's already happened once
      if(WaveSecBegin>DF.recordheader_starts.at(j)){
        std::cout << "WARNING: Record header label found inside another record header." << 
            "This is likely due to the counter.  Skipping record header and " <<
            "continuing" << std::endl;
        continue;
      }
      std::cout << "WE IN LOOP" << std::endl;
      std::cout << "RECORD HEADER INDEX" << DF.recordheader_starts.at(j) << std::endl;
      std::cout << "WAVESECBEGIN IS " << WaveSecBegin << std::endl;
      std::vector<uint16_t> WaveSlice(DF.samples.begin()+WaveSecBegin, 
              DF.samples.begin()+DF.recordheader_starts.at(j));
      Log("PMTDataDecoder Tool: Length of waveslice: "+to_string(WaveSlice.size()),v_debug, verbosity);
      //Add this WaveSlice to the wave bank
      this->AddSamplesToWaveBank(CardID, ChannelID, WaveSlice);
      //Since we have acquired the wave up to the next record header, the wave is done.
      //Store it in the FinishedWaves map.
      this->StoreFinishedWaveform(CardID, ChannelID);
      //Now, we have the header coming next.  Get it and parse it, starting whatever
      //Entries in maps are needed. 
      std::vector<uint16_t> RecordHeader(DF.samples.begin()+
              DF.recordheader_starts.at(j), DF.samples.begin()+
              DF.recordheader_starts.at(j)+SAMPLES_RIGHTOF_000+1);
      this->ParseRecordHeader(CardID, ChannelID, RecordHeader);
      WaveSecBegin = DF.recordheader_starts.at(j)+SAMPLES_RIGHTOF_000+1;
    }
    // No more record headers from here; just parse the rest of whatever 
    // waveform is being looked at
    int WaveSecEnd = DF.samples.size()-1;
    std::vector<uint16_t> WaveSlice(DF.samples.begin()+WaveSecBegin, 
            DF.samples.end());
    this->AddSamplesToWaveBank(CardID, ChannelID, WaveSlice);
  }
  else {
    this->ParseSyncFrame(CardID, DF);
  }
  return;
}

void PMTDataDecoder::ParseSyncFrame(int CardID, DecodedFrame DF)
{
  if(verbosity>4) std::cout << "PRINTING ALL DATA IN A SYNC FRAME FOR CARD" << CardID << std::endl;
  uint64_t SyncCounter = 0;
  for (int i=0; i < 6; i++){
    if(verbosity>4) std::cout << "SYNC FRAME DATA AT INDEX " << i << ": " << DF.samples.at(i) << std::endl;
    SyncCounter += ((uint64_t)DF.samples.at(i)) << (12*i);
    if(verbosity>4) std::cout << "SYNC COUNTER WITH CURRENT SAMPLE PUT AT LEFT: " << SyncCounter << std::endl;
  }
  std::map<int, std::vector<uint64_t>>::iterator it = SyncCounters.find(CardID);
  if(it != SyncCounters.end()) SyncCounters.at(CardID).push_back(SyncCounter);
  else {
    std::vector<uint64_t> SyncVec{SyncCounter};
    SyncCounters.emplace(CardID,SyncVec);
  }
  return;
}

void PMTDataDecoder::ParseRecordHeader(int CardID, int ChannelID, std::vector<uint16_t> RH)
{
  //We need to get the MTC count and make a new entry in TriggerTimeBank and WaveBank
  //First 4 samples; Just get the bits from 24 to 37 (is counter (61 downto 48)
  //Last 4 samples; All the first 48 bits of the MTC count.
  Log("PMTDataDecoder Tool: Parsing an encountered header ",v_debug, verbosity);
  if(verbosity>4){
    std::cout << "BIT WORDS IN RECORD HEADER: " << std::endl;
    for (unsigned int j=0; j<RH.size(); j++){
      std::cout << std::bitset<16>(RH.at(j)) << dec << std::endl;
    }
  }
  std::vector<uint16_t> CounterEnd(RH.begin()+2, RH.begin()+4);
  std::vector<uint16_t> CounterBegin(RH.begin()+4, RH.begin()+8);
  uint64_t ClockCount=0;
  int samplewidth=12;  //each uint16 really only holds 12 bits of info. (see DecodeFrame)
  //std::reverse(CounterBegin.begin(),CounterBegin.end());
  for (unsigned int j=0; j<CounterBegin.size(); j++){
    ClockCount += ((uint64_t)CounterBegin.at(j) << j*samplewidth);
  }
  for (unsigned int j=0; j<CounterEnd.size(); j++){
    ClockCount += ((uint64_t)CounterEnd.at(j) << ((CounterBegin.size() + j)*samplewidth));
  }
  std::vector<int> wave_key{CardID,ChannelID};
  std::vector<uint16_t> Waveform;
  
  //Update the TriggerTimeBank and WaveBank with new entries, since this channel's
  //Wave data is coming up next
  Log("PMTDataDecoder Tool: Parsed Clock time for header is "+to_string(ClockCount),v_debug, verbosity);
  TriggerTimeBank.emplace(wave_key,ClockCount);
  Log("PMTDataDecoder Tool: Placing empty waveform in WaveBank ",v_debug, verbosity);
  WaveBank.emplace(wave_key,Waveform);
  return;
}


void PMTDataDecoder::StoreFinishedWaveform(int CardID, int ChannelID)
{
  //Get the full waveform from the Wave Bank
  std::vector<int> wave_key{CardID,ChannelID};
  //Check there's a wave in the map
  if(WaveBank.count(wave_key)==0){
    Log("PMTDataDecoder::StoreFinishedWaveform: No waveform at wave key. ",v_message, verbosity);
    Log("PMTDataDecoder::StoreFinishedWaveForm: Continuing without saving any waves",v_message, verbosity);
    return;
  }
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
  TriggerTimeBank.erase(wave_key);
  return;
}
  
void PMTDataDecoder::AddSamplesToWaveBank(int CardID, int ChannelID, 
        std::vector<uint16_t> WaveSlice)
{
  Log("PMTDataDecoder Tool: Adding Waveslice to waveform.  Num. Samples: "+to_string(WaveSlice.size()),v_debug, verbosity);
  //TODO: Make sure the above is always divisible by 4!
  //Add the WaveSlice to the proper vector in the WaveBank.
  std::vector<int> wave_key{CardID,ChannelID};
  if(WaveBank.count(wave_key)==0){
    Log("PMTDataDecoder Tool: HAVE WAVE SLICE BUT NO WAVE BEING BUILT.: ",v_error, verbosity);
    Log("PMTDataDecoder Tool: WAVE SLICE WILL NOT BE SAVED, DATA LOST",v_error, verbosity);
    return;
  } else {
  WaveBank.at(wave_key).insert(WaveBank.at(wave_key).end(),
                               WaveSlice.begin(),
                               WaveSlice.end());
  }
  return;
}

