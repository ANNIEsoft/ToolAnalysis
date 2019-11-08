#ifndef ANNIEEventBuilder_H
#define ANNIEEventBuilder_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TimeClass.h"
#include "TriggerClass.h"
#include "Waveform.h"

/**
 * \class ANNIEEventBuilder
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class ANNIEEventBuilder: public Tool {


 public:

  ANNIEEventBuilder(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  void BuildANNIEEvent(uint64_t CounterTime, std::map<std::vector<int>, std::vector<uint16_t>> WaveMap);
  void BuildANNIEEventMRD(std::vector<std::pair<unsigned long,int>> MRDHits, 
        unsigned long MRDTimeStamp, std::string MRDTriggerType);
  void CardIDToElectronicsSpace(int CardID, int &CrateNum, int &SlotNum);
  void SaveEntryToFile();

 private:


  std::map<uint64_t, std::vector<std::pair<unsigned long, int> > > MRDEvents;  //Key: {MTCTime}, value: "WaveMap" with key (CardID,ChannelID), value FinishedWaveform
  std::map<uint64_t, std::string>  TriggerTypeMap;  //Key: {MTCTime}, value: string noting what type of trigger occured for the event 
  std::map<uint64_t, std::map<std::vector<int>, std::vector<uint16_t> > > FinishedPMTWaves;  //Key: {MTCTime}, value: map of fully-built waveforms from WaveBank

  std::map<std::vector<int>,int> TankPMTCrateSpaceToChannelNumMap;
  std::map<std::vector<int>,int> MRDCrateSpaceToChannelNumMap;
  BoostStore *RawData;
  BoostStore *TrigData;

  bool isMRDData;
  bool isTankData;

  BoostStore *ANNIEEvent = nullptr;

  std::map<unsigned long, std::vector<Hit>> *TDCData = nullptr;

  std::string InputFile;

  // Number of trigger entries from TriggerData loaded here
  long trigentries=0;

  // Number of PMTs that must be found in a WaveSet to build the event
  //
  unsigned int NumWavesInSet = 131;  
  int EntriesPerSubrun;
  
  bool IsNewMRDData;
  bool IsNewTankData;

  //Run Number defined in config, others iterated over as ANNIEEvent filled
  uint32_t RunNum;
  uint32_t SubrunNum;
  uint32_t ANNIEEventNum;
 
  bool SaveToFile; 
  std::string SavePath;
  std::string ProcessedFilesBasename;


  /// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;
};


#endif
