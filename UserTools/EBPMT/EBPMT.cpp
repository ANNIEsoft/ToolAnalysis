#include "EBPMT.h"

EBPMT::EBPMT() : Tool() {}

bool EBPMT::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if (configfile != "")
    m_variables.Initialise(configfile); // loading config file
  // m_variables.Print();

  m_data = &data; // assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("verbosityEBPMT", verbosityEBPMT);
  matchTargetTrigger = 5;
  m_variables.Get("matchTargetTrigger", matchTargetTrigger);
  matchTolerance_ns = 200; // default 200ns
  m_variables.Get("matchTolerance_ns", matchTolerance_ns);
  exePerMatch = 500;
  m_variables.Get("exePerMatch", exePerMatch);
  matchToAllTriggers = false;
  m_variables.Get("matchToAllTriggers", matchToAllTriggers);

  NumWavesInCompleteSet = 140;

  FinishedHits = new std::map<uint64_t, std::map<unsigned long, std::vector<Hit>> *>();
  RWMRawWaveforms = new std::map<uint64_t, std::vector<uint16_t>>();
  BRFRawWaveforms = new std::map<uint64_t, std::vector<uint16_t>>();

  saveRWMWaveforms = true;
  saveBRFWaveforms = true;
  m_variables.Get("saveRWMWaveforms", saveRWMWaveforms);
  m_variables.Get("saveBRFWaveforms", saveBRFWaveforms);

  return true;
}

bool EBPMT::Execute()
{
  m_data->CStore.Get("RunCode", currentRunCode);
  bool gotHits = m_data->CStore.Get("InProgressHits", InProgressHits);
  bool gotChkey = m_data->CStore.Get("InProgressChkey", InProgressChkey);

  m_data->CStore.Get("RWMRawWaveforms", RWMRawWaveforms);
  m_data->CStore.Get("BRFRawWaveforms", BRFRawWaveforms);
  Log("EBPMT: Got RWMRawWaveforms size: " + std::to_string(RWMRawWaveforms->size()), v_message, verbosityEBPMT);
  Log("EBPMT: Got BRFRawWaveforms size: " + std::to_string(BRFRawWaveforms->size()), v_message, verbosityEBPMT);

  if (exeNum % 80 == 0 && exeNum != 0)
  {
    // 80 is arbitrary, because 6*80 = 480 around, close, and smaller than the pairing exe number, exePerMatch default 500
    Log("EBPMT: exeNum: " + std::to_string(exeNum) + " Before loading, apply a VMEOffset correction", v_message, verbosityEBPMT);
    Log("EBPMT: before apply the VME offset correction, the size of FinishedHits is " + std::to_string(FinishedHits->size()), v_message, verbosityEBPMT);
    m_data->CStore.Get("InProgressHitsAux", InProgressHitsAux);
    m_data->CStore.Get("InProgressRecoADCHits", InProgressRecoADCHits);
    m_data->CStore.Get("InProgressRecoADCHitsAux", InProgressRecoADCHitsAux);

    CorrectVMEOffset();
    m_data->CStore.Set("InProgressHits", InProgressHits);
    m_data->CStore.Set("InProgressChkey", InProgressChkey);
    m_data->CStore.Set("InProgressHitsAux", InProgressHitsAux);
    m_data->CStore.Set("InProgressRecoADCHits", InProgressRecoADCHits);
    m_data->CStore.Set("InProgressRecoADCHitsAux", InProgressRecoADCHitsAux);
    m_data->CStore.Set("RWMRawWaveforms", RWMRawWaveforms);
    m_data->CStore.Set("BRFRawWaveforms", BRFRawWaveforms);
  }

  m_data->CStore.Get("PairedPMTTriggerTimestamp", PairedCTCTimeStamps);
  m_data->CStore.Get("PairedPMTTimeStamps", PairedPMTTimeStamps);
  Log("EBPMT: Got PairedPMTTimeStamps size: " + std::to_string(PairedPMTTimeStamps.size()), v_message, verbosityEBPMT);
  Log("EBPMT: Got PairedPMTTriggerTimestamp size: " + std::to_string(PairedCTCTimeStamps.size()), v_message, verbosityEBPMT);

  Log("EBPMT: gotHits = " + std::to_string(gotHits) + " gotChkey = " + std::to_string(gotChkey), v_message, verbosityEBPMT);
  if (!gotHits || !gotChkey)
  {
    Log("EBPMT: No InProgressHits or InProgressChkey found", v_message, verbosityEBPMT);
    return true;
  }

  Log("EBPMT: got inprogress hits and chkey with size " + std::to_string(InProgressHits->size()) + " and " + std::to_string(InProgressChkey->size()), v_message, verbosityEBPMT);

  vector<uint64_t> PMTEmplacedHitTimes;
  vector<uint64_t> RWMEmplacedTimes;
  vector<uint64_t> BRFEmplacedTimes;

  for (std::pair<uint64_t, std::map<unsigned long, std::vector<Hit>> *> p : *InProgressHits)
  {
    uint64_t PMTCounterTimeNs = p.first;
    std::map<unsigned long, std::vector<Hit>> *hitMap = p.second;
    vector<unsigned long> ChannelKey = InProgressChkey->at(PMTCounterTimeNs);

    Log("EBPMT: PMTCounterTimeNs: " + std::to_string(PMTCounterTimeNs), v_debug, verbosityEBPMT);
    Log("EBPMT: hitMap size: " + std::to_string(hitMap->size()), v_debug, verbosityEBPMT);
    Log("EBPMT: ChannelKey vector size: " + std::to_string(ChannelKey.size()), v_debug, verbosityEBPMT);
    Log("EBPMT: Current unfinished InProgressHits size: " + std::to_string(InProgressHits->size()), v_debug, verbosityEBPMT);

    if (static_cast<int>(ChannelKey.size()) > MaxObservedNumWaves)
      MaxObservedNumWaves = ChannelKey.size();
    if (InProgressHits->size() > 500 && MaxObservedNumWaves < NumWavesInCompleteSet && !max_waves_adapted && MaxObservedNumWaves >= 130)
    {
      NumWavesInCompleteSet = MaxObservedNumWaves;
      max_waves_adapted = true;
      Log("EBPMT: MaxObservedNumWaves = " + std::to_string(MaxObservedNumWaves), v_message, verbosityEBPMT);
      Log("EBPMT: NumWavesInCompleteSet = " + std::to_string(NumWavesInCompleteSet), v_message, verbosityEBPMT);
    }

    if (MaxObservedNumWaves > NumWavesInCompleteSet)
      NumWavesInCompleteSet = MaxObservedNumWaves;

    if (ChannelKey.size() == (NumWavesInCompleteSet - 1))
    {
      Log("EBPMT: ChannelKey.size() == (NumWavesInCompleteSet - 1), ChannelKey.size() = " + std::to_string(ChannelKey.size()) + " == NumWavesInCompleteSet - 1 = " + std::to_string(NumWavesInCompleteSet - 1), v_debug, verbosityEBPMT);
      if (AlmostCompleteWaveforms.find(PMTCounterTimeNs) != AlmostCompleteWaveforms.end())
      {
        Log("EBPMT: AlmostCompleteWaveforms size = " + std::to_string(AlmostCompleteWaveforms.size()), v_debug, verbosityEBPMT);
        AlmostCompleteWaveforms[PMTCounterTimeNs]++;
      }
      else
        AlmostCompleteWaveforms.emplace(PMTCounterTimeNs, 0);
      Log("EBPMT: AlmostCompleteWaveforms adding PMTCounterTimeNs = " + std::to_string(PMTCounterTimeNs) + " to " + std::to_string(AlmostCompleteWaveforms[PMTCounterTimeNs]), v_debug, verbosityEBPMT);
    }

    Log("EBPMT: ChannelKey.size() = " + std::to_string(ChannelKey.size()) + " >= NumWavesInCompleteSet = " + std::to_string(NumWavesInCompleteSet) + " or AlmostCompleteWaveforms.at(PMTCounterTimeNs) = " + std::to_string(AlmostCompleteWaveforms[PMTCounterTimeNs] >= 5), v_debug, verbosityEBPMT);


    // print all elements in vector<unsigned long> ChannelKey
//    cout<<"EBPMT: ChannelKey: ";
//    for (unsigned long ch : ChannelKey)
//    {
//      cout<<ch<<", ";
//    }
//    cout<<endl;


    if (ChannelKey.size() >= NumWavesInCompleteSet || ((ChannelKey.size() == NumWavesInCompleteSet - 1) && (AlmostCompleteWaveforms[PMTCounterTimeNs] >= 5)))
    {
      Log("EBPMT: Emplace hit map to FinishedHits, ChannelKey.size() = " + std::to_string(ChannelKey.size()) + " >= NumWavesInCompleteSet = " + std::to_string(NumWavesInCompleteSet) + " or AlmostCompleteWaveforms.at(PMTCounterTimeNs) = " + std::to_string(AlmostCompleteWaveforms.at(PMTCounterTimeNs) >= 5), v_debug, verbosityEBPMT);

      // check if the PMTCounterTimeNs is already in the FinishedHits, if not then add it

      if (FinishedHits->find(PMTCounterTimeNs) != FinishedHits->end())
      {
        Log("EBPMT: PMTCounterTimeNs: " + std::to_string(PMTCounterTimeNs) + " already in FinishedHits", v_debug, verbosityEBPMT);
        continue;
      }
      // if find PMTCounterTimeNs in PairedPMTTimeStamps, then skip this hit
      if (PairedPMTTimeStamps.size() > 0)
      {
        bool skip = false;
        for (std::pair<int, vector<uint64_t>> p : PairedPMTTimeStamps)
        {
          vector<uint64_t> PMTTimeStamps = p.second;
          for (uint64_t PMTTimeStamp : PMTTimeStamps)
          {
            if (PMTCounterTimeNs == PMTTimeStamp)
            {
              skip = true;
              break;
            }
          }
          if (skip)
            break;
        }
        if (skip)
          continue;
      }
      FinishedHits->emplace(PMTCounterTimeNs, hitMap);

      PMTEmplacedHitTimes.push_back(PMTCounterTimeNs);

      std::map<std::vector<int>, int> aWaveMapSampleSize;

      // Put PMT timestamp into the timestamp set for this run.
      Log("EBPMT: waveset has clock counter: " + std::to_string(PMTCounterTimeNs), v_debug, verbosityEBPMT);
    }

    if ((ChannelKey.size() == NumWavesInCompleteSet - 1))
    {
      if (AlmostCompleteWaveforms.at(PMTCounterTimeNs) >= 5)
        AlmostCompleteWaveforms.erase(PMTCounterTimeNs);
    }
  }

  Log("EBPMT: InProgressHits size: " + std::to_string(InProgressHits->size()) + " InProgressChkey size: " + std::to_string(InProgressChkey->size()), v_message, verbosityEBPMT);
  Log("EBPMT: PMTEmplacedHitTimes size: " + std::to_string(PMTEmplacedHitTimes.size()), v_message, verbosityEBPMT);
  /*for (uint64_t PMTCounterTimeNs : PMTEmplacedHitTimes)
  {
    InProgressHits->erase(PMTCounterTimeNs);
    InProgressChkey->erase(PMTCounterTimeNs);
  }*/
  Log("EBPMT: InProgressHits size: " + std::to_string(InProgressHits->size()) + " InProgressChkey size: " + std::to_string(InProgressChkey->size()), v_message, verbosityEBPMT);

  // If this InProgressTankEvent is too old, clear it
  Log("EBPMT: Current number of unfinished hitmaps in InProgressHits: " + std::to_string(InProgressHits->size()), v_debug, verbosityEBPMT);

  Log("EBPMT: All finished, left size are, matchedHitNumber: " + std::to_string(matchedHitNumber) + " InProgressHits: " + std::to_string(InProgressHits->size()) + " FinishedHits: " + std::to_string(FinishedHits->size()), v_message, verbosityEBPMT);

  if (exeNum % 80 == 0)
  {
    Log("EBPMT: after apply the VME offset correction and loading, the size of FinishedHits is " + std::to_string(FinishedHits->size()), v_message, verbosityEBPMT);
  }

  exeNum++;

  if (exeNum % 50 == 0)
  {
    Log("EBPMT: exeNum: " + std::to_string(exeNum), v_message, verbosityEBPMT);
  }

  bool stopLoop = false;
  m_data->vars.Get("StopLoop", stopLoop);
  int runNum = thisRunNum; // run number saved in buffer as the previous run number
  // m_data->vars.Get("RunNumber", thisRunNum);
  m_data->CStore.Get("runNumber", thisRunNum);

  bool ForcePMTMatching = false;
  m_data->CStore.Get("ForcePMTMatching", ForcePMTMatching);

  if (exeNum % exePerMatch == 0 || runNum != thisRunNum || stopLoop || ForcePMTMatching)
  {
    Log("EBPMT: exeNum: " + std::to_string(exeNum) + " Doing Matching", v_message, verbosityEBPMT);
    if (matchToAllTriggers)
    {
      Matching(0, 0);
    }
    else
    {
      bool BeamTriggerGroupped = false;
      m_data->CStore.Get("BeamTriggerGroupped", BeamTriggerGroupped);
      if (BeamTriggerGroupped)
        Matching(5, 14);
      else
        Log("EBPMT: BeamTriggerGroupped is false, no beam trigger groupped in the grouper, stop matching", v_message, verbosityEBPMT);
    }
  }

  m_data->CStore.Set("PairedPMTTriggerTimestamp", PairedCTCTimeStamps);
  m_data->CStore.Set("PairedPMTTimeStamps", PairedPMTTimeStamps);
  m_data->CStore.Set("PairedPMT_TriggerIndex", PairedPMT_TriggerIndex);
  m_data->CStore.Set("PMTHitmapRunCode", PMTHitmapRunCode);
  // everytime change the trigger group, also update the trigger index.
  Log("EBPMT: Set PairedPMTTimeStamps size: " + std::to_string(PairedPMTTimeStamps.size()), v_message, verbosityEBPMT);
  Log("EBPMT: Set PairedPMTTriggerTimestamp size: " + std::to_string(PairedCTCTimeStamps.size()), v_message, verbosityEBPMT);

  return true;
}

bool EBPMT::Finalise()
{

  Log("\033[1;34mEBPMT: Finalising\033[0m", v_message, verbosityEBPMT);
  Log("EBPMT: Matched Hit Map Entry Number: " + std::to_string(matchedHitNumber), v_message, verbosityEBPMT);
  Log("EBPMT: Unmatched Hit Map Entry Number left: " + std::to_string(FinishedHits->size()), v_message, verbosityEBPMT);

  return true;
}

bool EBPMT::Matching(int targetTrigger, int matchToTrack)
{
  cout << "\033[1;34m******* EBPMT : Matching *******\033[0m" << endl;
  std::map<int, std::vector<std::map<uint64_t, uint32_t>>> GroupedTriggersInTotal; // each map is a group of triggers, with the key is the target trigger word
  m_data->CStore.Get("GroupedTriggersInTotal", GroupedTriggersInTotal);

  vector<uint64_t> matchedHitTimes;
  // loop over all the Hits, for each FinishedHits, loop all grouped triggers, if the time differencs<100 for trigger 5,
  // matchedHitNumber ++, then remove the hit from the map
  int loopNum = 0;
  for (std::pair<uint64_t, std::map<unsigned long, std::vector<Hit>> *> pmtpair : *FinishedHits)
  {
    if (verbosityEBPMT > 11)
      cout << "******************EBPMT: new hit" << endl;
    uint64_t PMTCounterTimeNs = pmtpair.first;
    std::map<unsigned long, std::vector<Hit>> *hitMap = pmtpair.second;
    // set minDT to 5 min
    uint64_t minDT = 5 * 60 * 1e9;
    uint64_t minDTTrigger = 0;
    uint64_t dt = 0;
    uint32_t matchedTrigWord = 0;
    int matchedTrack = 0;
    int matchedIndex = 0;
    // loop all tracks of GroupedTriggersInTotal
    for (std::pair<int, std::vector<std::map<uint64_t, uint32_t>>> pair : GroupedTriggersInTotal)
    {
      int TrackTriggerWord = pair.first;
      if (TrackTriggerWord != matchToTrack && !matchToAllTriggers)
        continue;

      vector<std::map<uint64_t, uint32_t>> groupedTriggers = pair.second;

      // loop all the grouped triggers, if the value is target trigger, then calculate the time difference
      for (int i = 0; i < groupedTriggers.size(); i++)
      {
        map<uint64_t, uint32_t> groupedTrigger = groupedTriggers.at(i);
        // itearte over all the grouped triggers, if the value is target trigger, then calculate the time difference
        for (std::pair<uint64_t, uint32_t> p : groupedTrigger)
        {
          if (matchToAllTriggers || p.second == targetTrigger)
          {
            if (PMTCounterTimeNs > p.first)
            {
              dt = PMTCounterTimeNs - p.first;
            }
            else
            {
              dt = p.first - PMTCounterTimeNs;
            }
            if (dt < minDT)
            {
              minDT = dt;
              minDTTrigger = p.first;
              matchedTrigWord = p.second;
              matchedTrack = TrackTriggerWord;
              matchedIndex = i;
            }
          }
        }
      }
    }

    Log("EBPMT: looping hit " + std::to_string(loopNum) + ", minDT: " + std::to_string(minDT) + ", minDTTrigger time: " + std::to_string(minDTTrigger) + " with word " + std::to_string(matchedTrigWord) + ", in trigger track " + std::to_string(matchedTrack), v_warning, verbosityEBPMT);
    if (minDT < matchTolerance_ns)
    {
      PairedCTCTimeStamps[matchedTrack].push_back(minDTTrigger);
      PairedPMTTimeStamps[matchedTrack].push_back(PMTCounterTimeNs);
      PairedPMT_TriggerIndex[matchedTrack].push_back(matchedIndex);

      // the pmt hit map with timestmap PMTCounterTimeNs, match to trigger with timestamp minDTTrigger
      // the matched trigger is at matchedIndex of that trigger track

      matchedHitNumber++;
      matchedHitTimes.push_back(PMTCounterTimeNs);
      // FinishedHits->erase(PMTCounterTimeNs);
      Log("EBPMT: Matched Hit to trigger " + std::to_string(matchedTrigWord) + " at PMTCounterTimeNs: " + std::to_string(PMTCounterTimeNs) + " to TriggerTime: " + std::to_string(minDTTrigger) + " with minDT: " + std::to_string(minDT) + ", in trigger track " + std::to_string(matchedTrack) + ", in trigger index " + std::to_string(matchedIndex), v_message, verbosityEBPMT);
    }
    else
    {
      Log("EBPMT: Match failed, found min diff Hit to trigger " + std::to_string(matchedTrigWord) + " at PMTCounterTimeNs: " + std::to_string(PMTCounterTimeNs) + " to TriggerTime: " + std::to_string(minDTTrigger) + " with minDT: " + std::to_string(minDT) + ", in trigger track " + std::to_string(matchedTrack) + ", in trigger index " + std::to_string(matchedIndex), v_message, verbosityEBPMT);
    }
    loopNum++;
  }
  Log("EBPMT: total matchedHitNumber: " + std::to_string(matchedHitNumber), v_message, verbosityEBPMT);
  Log("EBPMT: Current number of unfinished hitmaps after match in InProgressHits: " + std::to_string(InProgressHits->size()), v_message, verbosityEBPMT);

  Log("EBPMT: Found matched hits: " + std::to_string(matchedHitTimes.size()), v_message, verbosityEBPMT);
  Log("EBPMT: before erase, left number of unfinished hitmaps in FinishedHits: " + std::to_string(FinishedHits->size()), v_message, verbosityEBPMT);
  for (uint64_t PMTCounterTimeNs : matchedHitTimes)
  {
    FinishedHits->erase(PMTCounterTimeNs);

    if (saveRWMWaveforms)
    {
      // check does RWMRawWaveforms have key PMTCounterTimeNs, if not, print a log waring and skip
      if (RWMRawWaveforms->find(PMTCounterTimeNs) == RWMRawWaveforms->end())
      {
        Log("EBPMT: After matching, RWMRawWaveforms does not have key PMTCounterTimeNs: " + std::to_string(PMTCounterTimeNs), v_warning, verbosityEBPMT);
      }
    }

    if (saveBRFWaveforms)
    {
      // check does BRFRawWaveforms have key PMTCounterTimeNs, if not, print a log waring and skip
      if (BRFRawWaveforms->find(PMTCounterTimeNs) == BRFRawWaveforms->end())
      {
        Log("EBPMT: After matching, BRFRawWaveforms does not have key PMTCounterTimeNs: " + std::to_string(PMTCounterTimeNs), v_warning, verbosityEBPMT);
      }
    }
  }
  Log("EBPMT: after erase, left number of unfinished hitmaps in FinishedHits: " + std::to_string(FinishedHits->size()), v_message, verbosityEBPMT);

  return true;
}

void EBPMT::CorrectVMEOffset()
{
  Log("EBPMT: Correcting VME Offset", v_message, verbosityEBPMT);
  vector<uint64_t> timestamps;                      // all current timestamps
  std::map<uint64_t, uint64_t> timestamps_to_shift; // timestamps need to be shifted

  // if InProgressHits size is 0, return
  if (InProgressHits->size() == 0)
  {
    Log("EBPMT: InProgressHits size is 0, return", v_message, verbosityEBPMT);
    return;
  }

  // insert the key of std::map<uint64_t, std::map<unsigned long, std::vector<Hit>> *> *InProgressHits; to timestamps
  for (std::pair<uint64_t, std::map<unsigned long, std::vector<Hit>> *> p : *InProgressHits)
  {
    timestamps.push_back(p.first);
  }

  Log("EBPMT: Found " + std::to_string(timestamps.size()) + " timestamps", v_message, verbosityEBPMT);

  // loop timestamps，对于每一个时间戳，检查它与它之前的时间戳的差值是否是8或者16
  // 如果是，获得InProgressHits在这两个时间戳上的map的size
  // 在timestamps_to_shift中记录pair，第一个时间戳是size较小的那个，第二个是较大的那个
  for (int i = 1; i < timestamps.size(); i++)
  {
    uint64_t dt = (timestamps[i] > timestamps[i - 1]) ? (timestamps[i] - timestamps[i - 1]) : (timestamps[i - 1] - timestamps[i]);
    Log("EBPMT: Found two timestamps with difference  = " + std::to_string(dt) + "ns", v_message, verbosityEBPMT);
    Log("EBPMT: timestamps[i - 1] = " + std::to_string(timestamps[i - 1]) + " timestamps[i] = " + std::to_string(timestamps[i]), v_message, verbosityEBPMT);
    if (dt == 8 || dt == 16)
    {
      uint64_t FirstMapSize = InProgressHits->at(timestamps[i - 1])->size();
      uint64_t SecondMapSize = InProgressHits->at(timestamps[i])->size();
      Log("EBPMT: Found two timestamps with 8 or 16ns difference, FirstMapSize: " + std::to_string(FirstMapSize) + " SecondMapSize: " + std::to_string(SecondMapSize), v_message, verbosityEBPMT);
      if (FirstMapSize < SecondMapSize)
      {
        timestamps_to_shift.emplace(timestamps[i - 1], timestamps[i]);
      }
      else
      {
        timestamps_to_shift.emplace(timestamps[i], timestamps[i - 1]);
      }
    }
    else if (dt < 1600)
    {
      Log("EBPMT: Found two timestamps with difference  = " + std::to_string(dt) + "ns", v_message, verbosityEBPMT);
      continue;
    }
  }

  int correctionApplied = 0;
  Log("EBPMT: Found " + std::to_string(timestamps_to_shift.size()) + " timestamps to shift", v_message, verbosityEBPMT);
  // go through timestamps_to_shift, for each pair, shift the hit map in the first smaller timestamp to the second larger timestamp
  // apply on InProgressHits, InProgressHitsAux, InProgressChkey, InProgressRecoADCHits, InProgressRecoADCHitsAux
  // also need to apply on RWMRawWaveforms and BRFRawWaveforms
  if (InProgressHitsAux != NULL && InProgressRecoADCHitsAux != NULL)
  { // InProgressHitsAux,InProgressRecoADCHitsAux may not exist yet
    for (std::map<uint64_t, uint64_t>::iterator it = timestamps_to_shift.begin(); it != timestamps_to_shift.end(); it++)
    {
      uint64_t SmallerMapTS = it->first;
      uint64_t LargerMapTS = it->second;
      Log("EBPMT::CorrectVMEOffset: Map Timestamp " + std::to_string(SmallerMapTS) + " to timestamp " + std::to_string(LargerMapTS), v_debug, verbosityEBPMT);
      if (InProgressHits->count(SmallerMapTS) == 0 || InProgressHits->count(LargerMapTS) == 0)
      { // map object at FirstTS, SecondTS may not exist yet
        Log("EBPMT::CorrectVMEOffset: InProgressHits->count(FirstTS) == " + std::to_string(InProgressHits->count(SmallerMapTS)) + ", InProgressHits->count(SecondTS) == " + std::to_string(InProgressHits->count(LargerMapTS)), v_debug, verbosityEBPMT);
        break;
      }
      // Get InProgress* {Hits, Chkey, and RecoADCHits} objects
      std::map<unsigned long, std::vector<Hit>> *FirstTankHits = InProgressHits->at(SmallerMapTS);
      std::map<unsigned long, std::vector<Hit>> *SecondTankHits = InProgressHits->at(LargerMapTS);
      std::vector<unsigned long> FirstChankey = InProgressChkey->at(SmallerMapTS);
      std::vector<unsigned long> SecondChankey = InProgressChkey->at(LargerMapTS);
      std::map<unsigned long, std::vector<Hit>> *FirstTankHitsAux = InProgressHitsAux->at(SmallerMapTS);
      std::map<unsigned long, std::vector<Hit>> *SecondTankHitsAux = InProgressHitsAux->at(LargerMapTS);
      std::map<unsigned long, std::vector<std::vector<ADCPulse>>> FirstRecoADCHits = InProgressRecoADCHits->at(SmallerMapTS);
      std::map<unsigned long, std::vector<std::vector<ADCPulse>>> SecondRecoADCHits = InProgressRecoADCHits->at(LargerMapTS);
      std::map<unsigned long, std::vector<std::vector<ADCPulse>>> FirstRecoADCHitsAux = InProgressRecoADCHitsAux->at(SmallerMapTS);
      std::map<unsigned long, std::vector<std::vector<ADCPulse>>> SecondRecoADCHitsAux = InProgressRecoADCHitsAux->at(LargerMapTS);

      SecondTankHits->insert(FirstTankHits->begin(), FirstTankHits->end());
      SecondChankey.insert(SecondChankey.end(), FirstChankey.begin(), FirstChankey.end());
      SecondTankHitsAux->insert(FirstTankHitsAux->begin(), FirstTankHitsAux->end());
      SecondRecoADCHits.insert(FirstRecoADCHits.begin(), FirstRecoADCHits.end());
      SecondRecoADCHitsAux.insert(FirstRecoADCHitsAux.begin(), FirstRecoADCHitsAux.end());

      (*InProgressHits)[LargerMapTS] = SecondTankHits;
      InProgressHits->erase(SmallerMapTS);
      (*InProgressChkey)[LargerMapTS] = SecondChankey;
      InProgressChkey->erase(SmallerMapTS);
      (*InProgressHitsAux)[LargerMapTS] = SecondTankHitsAux;
      InProgressHitsAux->erase(SmallerMapTS);
      (*InProgressRecoADCHits)[LargerMapTS] = SecondRecoADCHits;
      InProgressRecoADCHits->erase(SmallerMapTS);
      (*InProgressRecoADCHitsAux)[LargerMapTS] = SecondRecoADCHitsAux;
      InProgressRecoADCHitsAux->erase(SmallerMapTS);

      if (saveRWMWaveforms)
      {
        // if found SmallerMapTS in RWMRawWaveforms, then change it to LargerMapTS
        if (RWMRawWaveforms->find(SmallerMapTS) != RWMRawWaveforms->end())
        {
          (*RWMRawWaveforms)[LargerMapTS] = (*RWMRawWaveforms)[SmallerMapTS];
          RWMRawWaveforms->erase(SmallerMapTS);
        }
      }
      if (saveBRFWaveforms)
      {
        // if found SmallerMapTS in BRFRawWaveforms, then change it to LargerMapTS
        if (BRFRawWaveforms->find(SmallerMapTS) != BRFRawWaveforms->end())
        {
          (*BRFRawWaveforms)[LargerMapTS] = (*BRFRawWaveforms)[SmallerMapTS];
          BRFRawWaveforms->erase(SmallerMapTS);
        }

        correctionApplied++;
      }
    }
  }
  Log("EBPMT: Corrected VME Offset for " + std::to_string(correctionApplied) + " timestamps", v_message, verbosityEBPMT);
}
