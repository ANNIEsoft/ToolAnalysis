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
    std::cout<<"Cdata entry 0 CardID="<<Cdata.at(0).CardID<<std::endl;
    std::cout<<"Cdata entry 0 Data vector of size="<<Cdata.at(0).Data.size()<<std::endl;
    for (int j=0;j<Cdata.at(0).Data.size();j++){
      std::cout<<Cdata.at(0).Data.at(j)<<" , ";
    }
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

std::vector<DecodedFrame> DataDecoder::UnpackFrames(std::vector<uint32_t> bank) {
  uint64_t tempword;
  std::vector<DecodedFrame> frames;  //What we will return
  std::vector<uint16_t> samples;
  samples.resize(40); //Well, if there's 480 bits per frame of samples max, this fits it
  for (unsigned int frame = 0; frame<bank.size()/16; ++frame) {  // if each frame has 16 32-bit ints, nframes = bank_size/16
    bool hasheader = false;
    int sampleindex = 0;
    int wordindex = 16*frame;  //Index of first 32-bit int in this frame
    int bitsleft = 0;  //# bits to shift to the left in the temp word
    while (sampleindex < 40) {  //Parse out this whole frame
      if (bitsleft < 12) {
        tempword += ((uint64_t)be32toh(bank[wordindex]))<<bitsleft;  //TODO: I'm worried this isn't the right shift.  We should do some before/after prints here with real data.
        bitsleft += 32;
        wordindex += 1;
      }
      samples[sampleindex] = tempword&0xfff;  //You're only taking the first 12 bits of the tempword
      if((tempword&0xfff)==0xfff) first_headword = true;
      else if (first_headword && (tempword&0xfff)==0x000) has_header=true;
      else first_headword = false;
      tempword = tempword>>12;  //TODO: also check this shift is the right direction
      bitsleft -= 12;
      sampleindex += 1;
    } //END parse out this frame
    struct DecodedFrame tempframe;
    tempframe.frameid = be32toh(bank[16*frame+15]);  //Frameid is held in the frame's last 32-bit word
    tempframe.samples = samples;
    tempframe.has_recordheader = has_header;
    frames.push_back(tempframe);
  }
  return frames;
}


