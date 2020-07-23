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

//########## MAPS USED TO HOLD MRD DATA.  EACH MAP'S KEYS ARE THE MTC TIME  ########
struct MRDEventMaps{
  std::map<uint64_t, std::vector<std::pair<unsigned long, int> > > MRDEvents;  //Key: {MTCTime}, value: "WaveMap" with key (CardID,ChannelID), value FinishedWaveform
  std::map<uint64_t, std::string>  MRDTriggerTypeMap;  //Key: {MTCTime}, value: string noting what loopback channels were fired in the MRD in this event
  std::map<uint64_t, int> MRDBeamLoopbackMap;  //Key: {MTCTime}, value: beam loopback TDC
  std::map<uint64_t, int> MRDCosmicLoopbackMap;  //Key: {MTCTime}, value: cosmic loopback TDC
  ~MRDEventMaps(){}
};

//########## VECTORS AND MAPS USED TO HOLD TIMESTAMPS OF ORPHANED DATA  ########
struct Orphanage{
  std::map<uint64_t, std::map<std::string,std::string>> OrphanTankTimestamps;  //Contains timestamps for all PMT events that were out of step with the rest of the stream
  std::map<uint64_t, std::map<std::string,std::string>> OrphanCTCTimestamps;  //CTC timestamps with no PMT/MRD pair.
  std::map<uint64_t, std::map<std::string,std::string>> OrphanMRDTimestamps;  //Contains timestamps for all MRD events that were out of step with the rest of the stream
  ~Orphanage(){}
};

//########## TIMESTAMP STREAMS USED WHEN PAIRING DATA TO BUILD ANNIE EVENTS ########
struct TimeStream{
  std::vector<uint64_t> BeamTankTimestamps;  //Contains beam timestamps for all PMT events that haven't been paired to an MRD or CTC TS (keys in FinishedTankEvents)
  std::vector<uint64_t> BeamMRDTimestamps;  //Contains beam timestamps for all MRD events that haven't been paired to a PMT or CTC TS (keys in MRDEvents) - name is a misnomer, this is not just beam but also MRD cosmic triggers
  std::vector<uint64_t> CTCTimestamps;  //Contains CTC timestamps encountered so far (keys in TimeToTriggerWordMap)
  ~TimeStream(){}
};

class ANNIEEventBuilder: public Tool {


 public:

  ANNIEEventBuilder(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
 
  void CardIDToElectronicsSpace(int CardID, int &CrateNum, int &SlotNum);
  void RemoveCosmics();             // Removes events from MRD stream labeled as a cosmic trigger only (TankAndMRD only)

  //Methods to add info from different data streams to ANNIEEvent booststore
  void BuildANNIEEventRunInfo(int RunNum, int SubRunNum, int RunType, uint64_t RunStartTime);  //Loads run level information, as well as the entry number
  void BuildANNIEEventTank(uint64_t CounterTime, std::map<std::vector<int>, std::vector<uint16_t>> WaveMap);
  void BuildANNIEEventCTC(uint64_t CTCTime, uint32_t TriggerWord);
  void BuildANNIEEventMRD(std::vector<std::pair<unsigned long,int>> MRDHits, 
  uint64_t MRDTimeStamp, std::string MRDTriggerType, int beam_tdc, int cosmic_tdc);

  void SaveEntryToFile(int RunNum, int SubRunNum);
  void OpenNewANNIEEvent(int RunNum, int SubRunNum,uint64_t StarT, int RunT);

  //Methods for getting all timestamps encountered by decoder tools
  void ProcessNewTankPMTData();
  void ProcessNewMRDData();
  void ProcessNewCTCData();

  //Methods used to merge CTC/PMT/MRD streams
  std::map<uint64_t,uint64_t> PairTankPMTAndMRDTriggers();  // Return pairs of Tank and PMT timestamps
  std::map<uint64_t,std::map<std::string,uint64_t>> PairCTCCosmicPairs(std::map<uint64_t,std::map<std::string,uint64_t>> BuildMap, uint64_t max_timestamp, bool force_matching=false); //Pair Cosmics with Cosmic muon trigger words
  std::map<uint64_t,std::map<std::string,uint64_t>> MergeStreams(std::map<uint64_t,std::map<std::string,uint64_t>> BuildMap, uint64_t max_timestamp, bool force_matching=false);       // TankAndMRDAndCTC pairing mode;
  void ManageOrphanage();
  void MoveToOrphanage(std::map<uint64_t,std::string> TankOrphans,
                       std::map<uint64_t,std::string> MRDOrphans,
                       std::map<uint64_t,std::string> CTCOrphans);
  
  // store some info about orphaned events
  BoostStore *OrphanStore = nullptr;
  std::string OrphanFileBase="";
  
  template<typename T> void RemoveDuplicates(std::vector<T> &v){
    typename std::vector<T>::iterator itr = v.begin();
    typename std::unordered_set<T> s;

    for (auto curr = v.begin(); curr != v.end(); ++curr) {
      if (s.insert(*curr).second) *itr++ = *curr;
    }

    v.erase(itr, v.end());
  }

 private:

  std::map<std::vector<int>,int> TankPMTCrateSpaceToChannelNumMap;
  std::map<std::vector<int>,int> AuxCrateSpaceToChannelNumMap;
  std::map<std::vector<int>,int> MRDCrateSpaceToChannelNumMap;


  //####### MAPS THAT ARE LOADED FROM OR CONTAIN INFO FROM THE CSTORE (FROM MRD/PMT DECODING) #########
  std::map<uint64_t, std::map<std::vector<int>, std::vector<uint16_t> > >* InProgressTankEvents;  //Key: {MTCTime}, value: map of in-progress PMT trigger decoding from WaveBank
  std::map<uint64_t, std::map<std::vector<int>, std::vector<uint16_t> > > FinishedTankEvents;  //Key: {MTCTime}, value: map of fully-built waveforms from WaveBank
  std::map<uint64_t,uint32_t>* TimeToTriggerWordMap;  // Key: CTCTimestamp, value: Trigger Mask ID;
  MRDEventMaps myMRDMaps;


  //######### MAPS THAT HOLD PAIRED TANK/MRD/CTC TIMESTAMPS ########
  int EventsPerPairing;  //Determines how many Tank, MRD, and CTC events are paired per event building cycle (10* this number needed to do pairing)
  TimeStream myTimeStream;
  int64_t pause_threshold; // maximum difference between the most recent timestamps from each stream before we start throttling event reading - e.g. if one stream has much more frequent events than the others, don't read new events from it until the others catch up
  
  //######### INFORMATION USED FOR TRACKING ORPHAN DATA (EVENTS FROM STREAMS THAT HAVE NO OTHER PAIRS) ############
  bool OrphanOldTankTimestamps;  // If a timestamp in the InProgressTankEvents gets too old, clear it and move to orphanage
  int OldTimestampThreshold;  // Threshold where a timestamp relative to the newest timestamp crosses before moving to the orphanage
  int OrphanWarningValue;    //Number of orphanage placements in a pairing event to print a warning
  Orphanage myOrphanage;

  BoostStore* ProcessedStore = nullptr;
  BoostStore *ANNIEEvent = nullptr;
  std::map<unsigned long, std::vector<Hit>> *TDCData = nullptr;


  // Number of PMTs that must be found in a WaveSet to build the event
  unsigned int NumWavesInCompleteSet = 140; 

  int ExecutesPerBuild;          // Number of executions to pass through before running the execute loop
  int ExecuteCount = 0;

  std::string InputFile;
  std::string BuildType;

  uint64_t NewestTankTimestamp = 0;
  double CurrentDriftMean = 0;
  double CurrentDriftVariance = 0;

  int CTCTankTimeTolerance;   //Allowed time difference between CTC timestamp and Tank timestamp to pair data for event
  int CTCMRDTimeTolerance;   //Allowed time difference between CTC timestamp and MRD timestamp to pair data for event
  int MRDTankTimeTolerance;   //Threshold relative to current drift mean where an event will be put to the orphanage
  int DriftWarningValue;
  bool IsNewMRDData;
  bool IsNewTankData;
  bool IsNewCTCData;

  //Run Number defined in config, others iterated over as ANNIEEvent filled
  Store RunInfoPostgress;   //Has Run number, subrun number, etc...
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
