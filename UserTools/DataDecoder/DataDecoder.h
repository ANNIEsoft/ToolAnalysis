#ifndef DataDecoder_H
#define DataDecoder_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "CardData.h"
#include "TriggerData.h"

/**
 * \class DataDecoder
 *
 This tool is used to decode the binary raw data and construct the ANNIEEvent
 boost stores.
 
 *
* $Author: Teal Pershing $
* $Date: 2019/05/28 10:44:00 $
* Contact: tjpershing@ucdavis.edu 
*/


struct DecodedFrame{
  bool has_recordheader;
  uint32_t frameheader;
  std::vector<uint16_t> samples;
  std::vector<int> recordheader_000indices; //Holds indices where a record header starts in samples
  ~DecodedFrame(){
  }
};

class DataDecoder: public Tool {


 public:

  DataDecoder(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
  std::vector<DecodedFrame> UnpackFrames(std::vector<uint32_t> bank);

  void ParseFrame(int CardID, DecodedFrame DF);
  void ParseRecordHeader(int CardID, int ChannelID, std::vector<uint16_t> RH);
  void StoreWaveform(int CardID, int ChannelID);
  void AddSamplesToWaveBank(int CardID, int ChannelID, std::vector<uint16_t> WaveSlice);
  void BuildReadyEvents()
  void BuildANNIEEvent(std::map<std::vector<int>, std::vector<uint16_t> > WaveMap)

 private:

  int RECORD_HEADER_SAMPLENUMS = 8;


  //A Record header's first 48-bit word has least significant bits of 0xFFF000.  
  //So, check for this structure within the 12-bit samples being parsed.
  int RECORD_HEADER_LABELPART1 = 0x000;
  int RECORD_HEADER_LABELPART2 = 0xFFF;

  //A record header is made of two 48-bit words, each word in little endian order.  The
  //end of the first 48-bit word has the 0x000 of the Record Header.  Given each 12-bit
  //chunk is stored in a 16-bit word, you want to grab the 3 samples  left of 0x000 and
  //4 samples right of the 0x000.
  int SAMPLES_LEFTOF_000 = 3;
  int SAMPLES_RIGHTOF_000 = 4;

  BoostStore RawData(false,0);
  BoostStore PMTData(false,2);
  BoostStore TrigData(false,2);
  std::vector<CardData> Cdata;
  TriggerData Tdata;

  std::string InputFile;


  //Maps for keeping track of what CardData classes have been and need to be processed
  std::map<int, int> SequenceMap;  //Key is CardID, Value is what sequence # is next
  std::map<int, std::vector<int>> UnprocessedEntries; //Key is CardID, Value is vector of boost entry #s with an unprocessed entry

  //Maps used in decoding frames; specifically, holds record header and record waveform info
  std::map<std::vector<int>, uint64_t> TriggerTimeBank;  //Key: {cardID, channelID}. Value: trigger time associated with wave in WaveBank 
  std::map<std::vector<int>, std::vector<uint16_t>> WaveBank;  //Key: {cardID, channelID}.  If you're in sequence, the MTCTime doesn't matter for mapping

  //Maps that store completed waveforms from cards
  std::map<uint64_t, std::map<std::vector<int>, std::vector<uint16_t> > > FinishedWaves;  //Key: {MTCTime}, value: map of fully-built waveforms from WaveBank


  // Number of PMTs that must be found in a WaveSet to build the event
  int NumWavesInSet = 131;  
  /// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;

};


#endif
