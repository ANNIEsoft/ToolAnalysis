#ifndef MRDDataDecoder_H
#define MRDDataDecoder_H

#include <string>
#include <iostream>
#include <bitset>
#include <deque>

#include "Tool.h"
#include "CardData.h"
#include "TriggerData.h"
#include "BoostStore.h"
#include "Store.h"

/**
 * \class MRDDataDecoder
 *
 This tool is used to decode the binary raw data and construct the ANNIEEvent
 boost stores.
 
 *
* $Author: Teal Pershing $
* $Date: 2019/05/28 10:44:00 $
* Contact: tjpershing@ucdavis.edu 
*/


class MRDDataDecoder: public Tool {


 public:

  MRDDataDecoder(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

 private:
  long totalentries=0;
  int EntryNum = 0;


  BoostStore *MRDData;

  std::string InputFile;
  std::string Mode;
  bool SingleFileLoaded = false;
 
  //Counter used to track the number of entries processed in a PMT file
  int NumMRDDataProcessed = 0;



  std::map<int, deque<std::vector<int>>> UnprocessedEntries; //Key is CardID, Value is vector of vector{SequenceID, BoostEntry, CdataVectorIndex}

  //Map used to relate MRD Crate Space value to channel key
  std::map<std::vector<int>,int> MRDCrateSpaceToChannelNumMap;

  //Maps that store completed waveforms from cards
  std::map<uint64_t, std::vector<std::pair<unsigned long, int> > > MRDEvents;  //Key: {MTCTime}, value: "WaveMap" with key (CardID,ChannelID), value FinishedWaveform
  std::map<uint64_t, std::vector<std::pair<unsigned long, int> > > CStoreMRDEvents;  //Key: {MTCTime}, value: "WaveMap" with key (CardID,ChannelID), value FinishedWaveform
  std::map<uint64_t, std::string>  TriggerTypeMap;  //Key: {MTCTime}, value: string noting what type of trigger occured for the event 
  std::map<uint64_t, std::string>  CStoreTriggerTypeMap;  //Same as TriggerTypeMap, but loaded from CStore for appending new data to

  // Notes whether DAQ is in lock step running
  // Number of PMTs that must be found in a WaveSet to build the event
  /// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;

};


#endif
