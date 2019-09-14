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

  // Initialize BoostStores
  BoostStore VMEData(false,0);
  //PMTData = new BoostStore(false,2);
  //TriggerData = new BoostStore(false,2);

  return true;
}


bool DataDecoder::Execute(){

  BoostStore in(false,0);

  in.Initialise(InputFile.c_str());
  in.Print(false);

  /////////////////// getting PMT Data ////////////////////
  BoostStore PMTData(false,2);
  in.Get("PMTData",PMTData);

  PMTData.Print(false);

  long entries=0;
  PMTData.Header->Get("TotalEntries",entries);
  std::cout<<"entries = "<<entries<<std::endl;

  for( int i=0;i<5;i++){
    std::cout<<"entry "<<i<<" of "<<entries<<std::endl;
    PMTData.GetEntry(i);
    std::vector<CardData> Cdata;
    PMTData.Get("CardData",Cdata);
    std::cout<<"Cdata size="<<Cdata.size()<<std::endl;
    std::cout<<"Cdata entry 0 CardID="<<Cdata.at(i).CardID<<std::endl;
    std::cout<<"Cdata entry 0 Data vector of size="<<Cdata.at(i).Data.size()<<std::endl;
    for (int j=0;j<Cdata.at(0).Data.size();j++){
      std::cout<<Cdata.at(0).Data.at(j)<<" , ";
    }
    //For this CardData entry, decode raw binary frames
    std::vector<DecodedFrame> ThisCardDFs;
    ThisCardDFs = this->DecodeFrames(Cdata.at(i).Data);
    //Now, loop through each frame and Parse their information
    for (int i=0; i < ThisCardDFs.size(); i++){
      this->ParseFrame(Cdata.at(i).CardID,ThisCardDFs.at(i)); 
    std::cout<<std::endl;

  }
  ///////////////////////////////////////////

  ////////////////////////getting trigger data ////////////////
 BoostStore TrigData(false,2);
  in.Get("TrigData",TrigData);

  TrigData.Print(false);
  long trigentries=0;
  TrigData.Header->Get("TotalEntries",trigentries);
  std::cout<<"entries = "<<entries<<std::endl;

  for( int i=0;i<5;i++){
    std::cout<<"entry "<<i<<" of "<<trigentries<<std::endl;
    TrigData.GetEntry(i);
    TriggerData Tdata;
    PMTData.Get("TrigData",Tdata);
    int EventSize=Tdata.EventSize;
    std::cout<<"EventSize="<<EventSize<<std::endl;
    std::cout<<"SequenceID="<<Tdata.SequenceID<<std::endl;
    std::cout<<"EventTimes: " << std::endl;
    for(int j=0;j<Tdata.EventTimes.size();j++){

     std::cout<< Tdata.EventTimes.at(j)<<" , ";
    }
    std::cout<<"EventIDs: " << std::endl;
    for(int j=0;j<Tdata.EventIDs.size();j++){

     std::cout<< Tdata.EventIDs.at(j)<<" , ";
    }

    std::cout<<std::endl;
 }
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
        tempword += ((uint64_t)be32toh(bank[wordindex]))<<bitsleft;  //TODO: I'm worried this isn't the right shift.  We should do some before/after prints here with real data.
        bitsleft += 32;
        wordindex += 1;
      }
      samples[sampleindex] = tempword&0xfff;  //You're only taking the first 12 bits of the tempword
      if((tempword&0xfff)==RECORD_LABEL_HEADERPART2) haverecheader_part2 = true;
      else if (haverecheader_part2 && (tempword&0xfff)==RECORD_LABEL_HEADERPART1){
        thisframe.has_recordheader=true;
        thisframe.recordheader_indices.push_back(sampleindex-1);
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
    //We need to get the rest of a wave from WaveSecBegin to where the next frame starts
    for (int j = 0; j<DF.recordheader_indices.size; j++){
      std::vector<uint16_t> WaveSlice(DF.samples.begin()+WaveSecBegin, 
              DF.samples.begin()+DF.recordheader_indices.at(j)-1);
      //All samples are waveforms for channel record that already exists in the WaveBank.
      this->AddSamplesToWaveBank(CardID, ChannelID, WaveSlice);
      this->StoreWaveform(CardID, ChannelID);
      //Now, we have a header coming next.  Get it and parse it, starting whatever
      //Entries in maps are needed.  
      std::vector<uint16_t> RecordHeader(DF.samples.begin()+
              DF.recordheader_indices.at(j), DF.samples.begin()+
              DF.recordheader_indices.at(j)+ RECORD_HEADER_SAMPLENUMS - 1);
      this->ParseRecordHeader(CardID, ChannelID, RecordHeader);
      WaveSecBegin = DF.recordheader_indices.at(j)+RECORD_HEADER_SAMPLENUMS;
    }
    // No more record headers from here; just parse the rest of whatever 
    // waveform is being looked at
    WaveSecEnd = DF.samples.size()-1;
    std::vector<uint16_t> WaveSlice(WaveSecBegin, WaveSecEnd);
    this->AddSamplesToWaveBank(CardID, ChannelID, WaveSlice);
  }
  return;
}
 
void DataDecoder::StoreWaveform(int CardID, int ChannelID)
{
  //Get the waveform
  std::vector<int> wave_key{CardID,ChannelID};
  std::vector<uint16_t> FinishedWave = WaveBank.at(wave_key);
  int FinishedWaveTrigTime = TriggerTimeBank.at(wave_key);

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
