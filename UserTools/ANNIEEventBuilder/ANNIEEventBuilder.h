#ifndef ANNIEEventBuilder_H
#define ANNIEEventBuilder_H

#include <string>
#include <iostream>
#include <set>
#include <unordered_set>

#include "Tool.h"
#include "TimeClass.h"
#include "TriggerClass.h"
#include "Waveform.h"
#include "ANNIEalgorithms.h"
/**
 * \class ANNIEEventBuilder
 *
*
* $Author: T.Pershing $
* $Date: 2020/01/18 $
* Contact: tjpershing@ucdavis.edu 
*/
class ANNIEEventBuilder: public Tool {


 public:

  ANNIEEventBuilder(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
  
  void PairTankPMTAndMRDTriggers();  // Put together timestamps of finished decoding Tank Triggers and MRD Triggers 
  void RemoveCosmics();             // Removes events from MRD stream labeled as a cosmic trigger only
  void BuildANNIEEventRunInfo(int RunNum, int SubRunNum, int RunType, uint64_t RunStartTime);  //Loads run level information, as well as the entry number
  void BuildANNIEEventTank(uint64_t CounterTime, std::map<std::vector<int>, std::vector<uint16_t>> WaveMap);
  void BuildANNIEEventCTC(uint64_t CTCTime, uint32_t TriggerWord);
  void BuildANNIEEventMRD(std::vector<std::pair<unsigned long,int>> MRDHits, 
        unsigned long MRDTimeStamp, std::string MRDTriggerType);
  void CardIDToElectronicsSpace(int CardID, int &CrateNum, int &SlotNum);
  void SaveEntryToFile(int RunNum, int SubRunNum);
  void OpenNewANNIEEvent(int RunNum, int SubRunNum,uint64_t StarT, int RunT);
  std::vector<uint64_t> ProcessNewTankPMTData();
  void ProcessNewMRDData();
  void ProcessNewCTCData();
  void MergeStreams();
  
  template<typename T> void RemoveDuplicates(std::vector<T> &v){
    typename std::vector<T>::iterator itr = v.begin();
    typename std::unordered_set<T> s;

    for (auto curr = v.begin(); curr != v.end(); ++curr) {
      if (s.insert(*curr).second) *itr++ = *curr;
    }

    v.erase(itr, v.end());
  }

 private:

  //####### MAPS THAT ARE LOADED FROM OR CONTAIN INFO FROM THE CSTORE (FROM MRD/PMT DECODING) #########
  std::map<uint64_t, std::vector<std::pair<unsigned long, int> > > MRDEvents;  //Key: {MTCTime}, value: "WaveMap" with key (CardID,ChannelID), value FinishedWaveform
  std::map<uint64_t, std::string>  MRDTriggerTypeMap;  //Key: {MTCTime}, value: string noting what loopback channels were fired in the MRD in this event
  std::map<uint64_t, std::map<std::vector<int>, std::vector<uint16_t> > >* InProgressTankEvents;  //Key: {MTCTime}, value: map of in-progress PMT trigger decoding from WaveBank
  std::map<uint64_t, std::map<std::vector<int>, std::vector<uint16_t> > > FinishedTankEvents;  //Key: {MTCTime}, value: map of fully-built waveforms from WaveBank
  std::map<uint64_t,uint32_t>* TimeToTriggerWordMap;  // Key: CTCTimestamp, value: Trigger Mask ID;
  Store RunInfoPostgress;   //Has Run number, subrun number, etc...

  std::map<std::vector<int>,int> TankPMTCrateSpaceToChannelNumMap;
  std::map<std::vector<int>,int> AuxCrateSpaceToChannelNumMap;
  std::map<std::vector<int>,int> MRDCrateSpaceToChannelNumMap;

  //######### INFORMATION USED FOR PAIRING UP TANK AND MRD DATA TRIGGERS ########
  int EventsPerPairing;  //Determines how many Tank, MRD, and CTC events are paired per event building cycle (10* this number needed to do pairing)
  
  std::vector<uint64_t> BeamTankTimestamps;  //Contains beam timestamps for all PMT events that haven't been paired to an MRD or CTC TS
  std::vector<uint64_t> BeamMRDTimestamps;  //Contains beam timestamps for all MRD events that haven't been paired to a PMT or CTC TS
  std::vector<uint64_t> CTCTimestamps;  //Contains CTC timestamps encountered so far
  std::map<uint64_t,uint64_t> BeamTankMRDPairs; //Pairs of beam-triggered Tank PMT/MRD counters ready to be built if all PMT waveforms are ready (TankAndMRD mode only)
  std::map<uint64_t,std::map<std::string,uint64_t>> BuildMap; //key: CTC timestamp, value: vector of maps with key: "Tank", "MRD", or "LAPPD" and value of timestamp for that data stream,
                                                                           //or "CTC" for the trigger word of that timestamp
  
  
  bool OrphanOldTankTimestamps;  // If a timestamp in the InProgressTankEvents gets too old, clear it and move to orphanage
  int OldTimestampThreshold;  // Threshold where a timestamp relative to the newest timestamp crosses before moving to the orphanage
  int OrphanWarningValue;    //Number of orphanage placements in a pairing event to print a warning
  std::vector<uint64_t> OrphanTankTimestamps;  //Contains timestamps for all PMT events that were out of step with the rest of the stream
  std::map<uint64_t, uint32_t> OrphanCTCTimeWordPairs;  //CTC timestamps with no PMT/MRD pair.  key: CTC time in ns, value: CTC word
  std::vector<uint64_t> OrphanMRDTimestamps;  //Contains timestamps for all MRD events that were out of step with the rest of the stream
  

  BoostStore *ANNIEEvent = nullptr;
  std::map<unsigned long, std::vector<Hit>> *TDCData = nullptr;


  // Number of PMTs that must be found in a WaveSet to build the event
  //
  unsigned int NumWavesInCompleteSet = 140; 

  int ExecutesPerBuild;          // Number of execute loops to pass through before running the execute loop
  int ExecuteCount = 0;

  std::string InputFile;
  std::string BuildType;

  uint64_t NewestTimestamp = 0;
  double CurrentDriftMean = 0;
  double CurrentDriftVariance = 0;

  int MRDPMTTimeDiffTolerance;   //Threshold relative to current drift mean where an event will be put to the orphanage
  int DriftWarningValue;
  bool IsNewMRDData;
  bool IsNewTankData;
  bool IsNewCTCData;

  //Run Number defined in config, others iterated over as ANNIEEvent filled
  uint32_t ANNIEEventNum;
  int CurrentRunNum;
  int CurrentSubRunNum;
  int CurrentRunType;
  int CurrentStarTime;

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
