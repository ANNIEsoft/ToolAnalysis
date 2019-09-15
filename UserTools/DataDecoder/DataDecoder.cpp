#include "DataDecoder.h"

DataDecoder::DataDecoder():Tool(){}


bool DataDecoder::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  verbosity = 0;

  m_variables.Get("verbosity",verbosity);
  m_variables.Get("InputFile",InputFile);
  m_variables.Get("LockStep",LockStepRunning);
  m_variables.Get("ParsesPerExecute",ParsesPerExecute);

  // Initialize RawData
  RawData.Initialise(InputFile.c_str());
  RawData.Print(false);

  /////////////////// getting PMT Data ////////////////////
  RawData.Get("PMTData",PMTData);

  PMTData.Print(false);
  
  long entries=0;
  PMTData.Header->Get("TotalEntries",entries);
  std::cout<<"Total entries in PMTData store: "<<entries<<std::endl;
  
  return true;
}


bool DataDecoder::Execute(){
  int NumPMTDataProcessed = 0;
  while(NumPMTDataProcessed<ParsesPerLoop){
    std::cout<<"entry "<<CDEntryNum<<" of "<<entries<<std::endl;
    PMTData.GetEntry(CDEntryNum);
    PMTData.Get("CardData",Cdata);
    std::cout<<"Cdata size="<<Cdata.size()<<std::endl;
    std::cout<<"CardData in Cdata's 0th index CardID="<<Cdata.at(0).CardID<<std::endl;
    std::cout<<"CardData in Cdata's 0th index data size="<<Cdata.at(0).Data.size()<<std::endl;
    //###### MOCK-UP FOR HOW TO LOOP THROUGH AN ENTRY's VECTOR OF CARD DATA
    for (int k=0; k<Cdata.size(); k++){
      aCardData = Cdata.at(k);
      //For this CardData entry, decode raw binary frames
      std::vector<DecodedFrame> ThisCardDFs;
      ThisCardDFs = this->DecodeFrames(aCardData.Data);
      //Now, loop through each frame and Parse their information
      for (int i=0; i < ThisCardDFs.size(); i++){
        this->ParseFrame(aCardData.CardID,ThisCardDFs.at(i));
      }
    }
    CDEntryNum+=1;
    NumPMTDataProcessed+=1;
  }
  //PMT Data done being processed this loop.  Push pointer to the
  //CStore and use it in the TriggerData Parser tool.
  //Any of the MTCCounters have all their PMT data.
  m_data->CStore.Set("FinishedWaves",*FinishedWaves);
  this->BuildReadyEvents();
  //###### END MOCK-UP ######
  //
  //Check the size of the WaveBank to see if things are bloating
  std::cout << "Size of WaveBank (# events in building progress): " << WaveBank.size() << std::endl;
  std::cout << "Size of FinishedWaves (# triggers with at least one wave built): " << FinishedWaves.size() << std::endl;
  //TODO: Print out, if debugging, the size of each entry in FinishedWaves

  
  ///////////////////////////////////////////

  return true;
}


bool DataDecoder::Finalise(){
  std::cout << "DataDecoder tool exitting" << std::endl;
  return true;
}

std::vector<DecodedFrame> DataDecoder::UnpackFrames(std::vector<uint32_t> bank)
{
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
        std::cout << "TEMP WORD BEFORE be32toh AN LEFT SHIFT: " << std::hex << "0x" << tempword << std::endl;
        std::bitset<64> tempbin(tempword);
        std::cout << "TEMP WORD BEFORE be32toh AN LEFT SHIFT: " << tempbin << std::endl;
        tempword += ((uint64_t)be32toh(bank[wordindex]))<<bitsleft;  //TODO: I'm worried this isn't the right shift.  We should do some before/after prints here with real data.
        std::cout << "TEMP WORD AFTER be32toh AND LEFT SHIFT: " << std::hex << "0x" << tempword << std::endl;
        std::bitset<64> tempbin(tempword);
        std::cout << "TEMP WORD BEFORE be32toh AN LEFT SHIFT: " << tempbin << std::endl;
        bitsleft += 32;
        wordindex += 1;
      }
      samples[sampleindex] = tempword&0xfff;  //You're only taking the first 12 bits of the tempword
      if((tempword&0xfff)==RECORD_LABEL_HEADERPART2) haverecheader_part2 = true;
      else if (haverecheader_part2 && (tempword&0xfff)==RECORD_LABEL_HEADERPART1){
        thisframe.has_recordheader=true;
        thisframe.recordheader_000indices.push_back(sampleindex);
      }
      else haverecheader_part2 = false;
      tempword = tempword>>12;  //TODO: also check this shift is the right direction
      bitsleft -= 12;
      sampleindex += 1;
    } //END parse out this frame
    thisframe.frameheader = be32toh(bank[16*frame+15]);  //Frameid is held in the frame's last 32-bit word
    thisframe.samples = samples;
    frames.push_back(thisframe);
  }
  return frames;
}

void DataDecoder::ParseFrame(int CardID, DecodedFrame DF)
{
  int ChannelID = DF.frameheader; //FIXME: We probably need a function that gets the
                                  //Bitrange defined by Jonathan (511 downto 504)
  if(!DF.has_recordheader){
    //All samples are waveforms for channel record that already exists in the WaveBank.
    this->AddSamplesToWaveBank(CardID, ChannelID, DF.samples);
  } else {
    int WaveSecBegin = 0;
    //We need to get the rest of a wave from WaveSecBegin to where the header starts
    for (int j = 0; j<DF.recordheader_000indices.size; j++){
      std::vector<uint16_t> WaveSlice(DF.samples.begin()+WaveSecBegin, 
              DF.samples.begin()+DF.recordheader_000indices.at(j)- SAMPLES_LEFTOF_000-1);
      //All samples are waveforms for channel record that already exists in the WaveBank.
      this->AddSamplesToWaveBank(CardID, ChannelID, WaveSlice);
      this->StoreWaveform(CardID, ChannelID);
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
    WaveSecEnd = DF.samples.size()-1;
    std::vector<uint16_t> WaveSlice(WaveSecBegin, WaveSecEnd);
    this->AddSamplesToWaveBank(CardID, ChannelID, WaveSlice);
  }
  return;
}

void DataDecoder::ParseRecordHeader(int CardID, int ChannelID, std::vector<uint16_t> RH)
{
  //We need to get the MTC count and make a new entry in TriggerTimeBank and WaveBank
  //First 4 samples; Just get the bits from 24 to 37 (is counter (61 downto 48)
  //Last 4 samples; All the first 48 bits of the MTC count.
  std::vector<uint16_t> CounterEnd(RH.begin(), RH.begin()+2);
  std::vector<uint16_t> CounterBegin(RH.begin()+4, RH.begin()+7);
  std::vector<uint64_t> MTCCounter;
  int samplewidth=12;  //each uint16 really only holds 12 bits of info. (see DecodeFrame)
  for (int j=0; j<CounterBegin.size(); j++){
    MTCCounter += CounterBegin.at(j) << j*samplewidth;
  }
  for (int j=0; j<CounterEnd.size(); j++){
    MTCCounter += CounterEnd.at(j) << (CounterBegin.size() + j*samplewidth);
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


void DataDecoder::StoreWaveform(int CardID, int ChannelID)
{
  //Get the waveform
  std::vector<int> wave_key{CardID,ChannelID};
  std::vector<uint16_t> FinishedWave = WaveBank.at(wave_key);
  uint64_t FinishedWaveTrigTime = TriggerTimeBank.at(wave_key);

  std::map<std::vector<int>, std::vector<uint16_t> > TriggerTimeWaves;

  if(FinishedWaves.count(FinishedWaveTrigTime) == 0) {
    TriggerTimeWaves.emplace(wave_key,FinishedWave);
    FinishedWaves.emplace(FinishedWaveTrigTime,TriggerTimeWave);
  } else {
    FinishedWaves.at(FinishedWaveTrigTime).at(wave_key) = TriggerTimeWave;
  }

  //Clear the wave from WaveBank and TriggerTimeBank
  WaveBank.erase(wave_key);
  TriggerTimeBank.at(wave_key);
  return;
}
  
void DataDecoder::AddSamplesToWaveBank(int CardID, int ChannelID, 
        std::vector<uint16_t> WaveSlice)
{
  //Add the WaveSlice to the proper vector in the WaveBank.
  std::vector<int> wave_key{CardID,ChannelID};
  WaveBank.at(wave_key).insert(WaveBank.at(wave_key).end(),
                               WaveSlice.begin(),
                               WaveSlice.end());
  return;
}

void DataDecoder::BuildReadyEvents()
{
  //Well, I would say that we're ready to build an event if we have all the
  //expected PMT waves for a clock count.
  std::map<std::vector<int>, std::vector<uint16_t> > WaveMap;
  for(int i=0;i<FinishedWaves.size();i++){
    WaveMap = FinishedWaves.at(i);
    if (WaveMap.size() == NumWavesInSet){
      this->BuildANNIEEvent(WaveMap);
    }
  }
}

//FIXME: Maybe we should have a vector of these WaveMaps, then have another tool
//Downstream Named BuildANNIEEvent that actually uses the TriggerData paired with
//the WaveMap to build the event?
void DataDecoder::BuildANNIEEvent(std::map<std::vector<int>, std::vector<uint16_t> > WaveMap)
{
  std::cout << "We still need to actually write the ANNIEEvent builder..." << std::endl;
  return;
}
