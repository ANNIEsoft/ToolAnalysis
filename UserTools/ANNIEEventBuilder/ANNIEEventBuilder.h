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

  void BuildANNIEEvent(uint64_t CounterTime, std::map<std::vector<int>, std::vector<uint16_t>> WaveMap, int RunNum, int SubrunNum, int RunType, uint64_t EventStartTime);
  void BuildANNIEEventMRD(std::vector<std::pair<unsigned long,int>> MRDHits, 
        unsigned long MRDTimeStamp, std::string MRDTriggerType, int RunNum, int SubrunNum, int RunType);
  void CardIDToElectronicsSpace(int CardID, int &CrateNum, int &SlotNum);
  void SaveEntryToFile(int RunNum, int SubrunNum);

 private:


  std::map<uint64_t, std::vector<std::pair<unsigned long, int> > > MRDEvents;  //Key: {MTCTime}, value: "WaveMap" with key (CardID,ChannelID), value FinishedWaveform
  std::map<uint64_t, std::string>  TriggerTypeMap;  //Key: {MTCTime}, value: string noting what type of trigger occured for the event 
  std::map<uint64_t, std::map<std::vector<int>, std::vector<uint16_t> > > FinishedPMTWaves;  //Key: {MTCTime}, value: map of fully-built waveforms from WaveBank
  Store RunInfoPostgress;   //Has Run number, subrun number, etc...

  std::map<std::vector<int>,int> TankPMTCrateSpaceToChannelNumMap;
  std::map<std::vector<int>,int> MRDCrateSpaceToChannelNumMap;
  BoostStore *RawData;
  BoostStore *TrigData;

  bool isMRDData;
  bool isTankData;
  bool TankFileComplete;

  BoostStore *ANNIEEvent = nullptr;

  std::map<unsigned long, std::vector<Hit>> *TDCData = nullptr;

  std::string InputFile;

  // Number of trigger entries from TriggerData loaded here
  long trigentries=0;

  // Number of PMTs that must be found in a WaveSet to build the event
  //
  unsigned int NumWavesInSet = 131;  
  
  bool IsNewMRDData;
  bool IsNewTankData;

  //Run Number defined in config, others iterated over as ANNIEEvent filled
  uint32_t ANNIEEventNum;
  int CurrentRunNum;
  int CurrentSubrunNum;

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
