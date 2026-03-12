#include "EBLAPPD.h"

EBLAPPD::EBLAPPD() : Tool() {}

bool EBLAPPD::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if (configfile != "")
    m_variables.Initialise(configfile); // loading config file
  // m_variables.Print();

  m_data = &data; // assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("verbosityEBLAPPD", verbosityEBLAPPD);
  matchTargetTrigger = 14;
  m_variables.Get("matchTargetTrigger", matchTargetTrigger);
  matchTolerance_ns = 400000; // default 400us
  m_variables.Get("matchTolerance_ns", matchTolerance_ns);
  matchToAllTriggers = false;
  m_variables.Get("matchToAllTriggers", matchToAllTriggers);
  exePerMatch = 500;
  m_variables.Get("exePerMatch", exePerMatch);

  return true;
}

bool EBLAPPD::Execute()
{
  m_data->CStore.Get("PairedLAPPDTriggerTimestamp", PairedCTCTimeStamps);
  m_data->CStore.Get("PairedLAPPDTimeStamps", PairedLAPPDTimeStamps);
  m_data->CStore.Get("PairedLAPPD_TriggerIndex", PairedLAPPD_TriggerIndex);
  m_data->CStore.Get("Buffer_LAPPDData", Buffer_LAPPDData);
  m_data->CStore.Get("Buffer_LAPPDTimestamp_ns", Buffer_LAPPDTimestamp_ns);
  m_data->CStore.Get("Buffer_LAPPDBeamgate_ns", Buffer_LAPPDBeamgate_ns);
  m_data->CStore.Get("Buffer_LAPPDOffset", Buffer_LAPPDOffset);
  m_data->CStore.Get("Buffer_LAPPDBeamgate_Raw", Buffer_LAPPDBeamgate_Raw);
  m_data->CStore.Get("Buffer_LAPPDTimestamp_Raw", Buffer_LAPPDTimestamp_Raw);
  m_data->CStore.Get("Buffer_LAPPDBGCorrection", Buffer_LAPPDBGCorrection);
  m_data->CStore.Get("Buffer_LAPPDTSCorrection", Buffer_LAPPDTSCorrection);
  m_data->CStore.Get("Buffer_LAPPDOffset_minus_ps", Buffer_LAPPDOffset_minus_ps);
  m_data->CStore.Get("Buffer_LAPPDRunCode", Buffer_RunCode);

  m_data->CStore.Get("Buffer_LAPPDBG_PPSBefore", Buffer_LAPPDBG_PPSBefore);
  m_data->CStore.Get("Buffer_LAPPDBG_PPSAfter", Buffer_LAPPDBG_PPSAfter);
  m_data->CStore.Get("Buffer_LAPPDBG_PPSDiff", Buffer_LAPPDBG_PPSDiff);
  m_data->CStore.Get("Buffer_LAPPDBG_PPSMissing", Buffer_LAPPDBG_PPSMissing);
  m_data->CStore.Get("Buffer_LAPPDTS_PPSBefore", Buffer_LAPPDTS_PPSBefore);
  m_data->CStore.Get("Buffer_LAPPDTS_PPSAfter", Buffer_LAPPDTS_PPSAfter);
  m_data->CStore.Get("Buffer_LAPPDTS_PPSDiff", Buffer_LAPPDTS_PPSDiff);
  m_data->CStore.Get("Buffer_LAPPDTS_PPSMissing", Buffer_LAPPDTS_PPSMissing);

  Log("EBLAPPD: Got pairing information from CStore, PairedLAPPDTimeStamps[14] size = " + std::to_string(PairedLAPPDTimeStamps[14].size()), v_message, verbosityEBLAPPD);

  CleanData();
  m_data->CStore.Get("RunCode", currentRunCode);

  bool IsNewLAPPDData = false;
  m_data->CStore.Get("NewLAPPDDataAvailable", IsNewLAPPDData);
  Log("EBLAPPD: NewLAPPDDataAvailable = " + std::to_string(IsNewLAPPDData), v_message, verbosityEBLAPPD);
  bool LoadingPPS = false;
  m_data->CStore.Get("LoadingPPS", LoadingPPS);

  if (IsNewLAPPDData && !LoadingPPS)
    LoadLAPPDData();

  Log("EBLAPPD: Finished Loading LAPPD data to buffer, Buffer_LAPPDData size is now " + std::to_string(Buffer_LAPPDData.size()), v_message, verbosityEBLAPPD);

  string storeFileName;
  m_data->CStore.Get("SaveToFileName", storeFileName);

  bool stopLoop = false;
  m_data->vars.Get("StopLoop", stopLoop);
  int runNum = thisRunNum;
  m_data->vars.Get("RunNumber", thisRunNum);
  bool ForceLAPPDMatching = false;
  m_data->CStore.Get("ForceLAPPDMatching", ForceLAPPDMatching);

  if (stopLoop || runNum != thisRunNum || exeNum % exePerMatch == 0 || ForceLAPPDMatching)
  {
    Log("EBLAPPD: exeNum = " + std::to_string(exeNum) + ". Doing matching", v_message, verbosityEBLAPPD);
    Log("EBLAPPD: Matching reason is stopLoop = " + std::to_string(stopLoop) + ", runNum = " + std::to_string(runNum) + ", exeNum = " + std::to_string(exeNum) + ", ForceLAPPDMatching = " + std::to_string(ForceLAPPDMatching), v_message, verbosityEBLAPPD);
    if (matchToAllTriggers)
    {
      Matching(0, 0);
    }
    else
    {
      bool BeamTriggerGroupped = false;
      m_data->CStore.Get("BeamTriggerGroupped", BeamTriggerGroupped);
      if (BeamTriggerGroupped)
        Matching(14, 14);
      else
        Log("EBLAPPD: BeamTriggerGroupped is false, no beam trigger groupped in the grouper, stop matching", v_message, verbosityEBLAPPD);

      bool LaserTriggerGroupped = false;
      m_data->CStore.Get("LaserTriggerGroupped", LaserTriggerGroupped);
      if (LaserTriggerGroupped)
        Matching(47, 47);
      else
        Log("EBLAPPD: LaserTriggerGroupped is false, no laser trigger groupped in the grouper, stop matching", v_message, verbosityEBLAPPD);

      bool CosmicTriggerGroupped = false;
      m_data->CStore.Get("CosmicTriggerGroupped", CosmicTriggerGroupped);
      if (CosmicTriggerGroupped)
        Matching(45, 46);
      else
        Log("EBLAPPD: CosmicTriggerGroupped is false, no cosmic trigger groupped in the grouper, stop matching", v_message, verbosityEBLAPPD);

      bool LEDTriggerGroupped = false;
      m_data->CStore.Get("LEDTriggerGroupped", LEDTriggerGroupped);
      if (LEDTriggerGroupped)
        Matching(31, 46);
      else
        Log("EBLAPPD: LEDTriggerGroupped is false, no LED trigger groupped in the grouper, stop matching", v_message, verbosityEBLAPPD);

      bool NuMITriggerGroupped = false;
      m_data->CStore.Get("NuMITriggerGroupped", NuMITriggerGroupped);
      if (NuMITriggerGroupped)
        Matching(42, 46);
      else
        Log("EBLAPPD: NuMITriggerGroupped is false, no NuMI trigger groupped in the grouper, stop matching", v_message, verbosityEBLAPPD);
    }
  }

  // Set all matching info to CStore
  m_data->CStore.Set("PairedLAPPDTriggerTimestamp", PairedCTCTimeStamps);
  m_data->CStore.Set("PairedLAPPDTimeStamps", PairedLAPPDTimeStamps);
  m_data->CStore.Set("PairedLAPPD_TriggerIndex", PairedLAPPD_TriggerIndex);
  Log("EBLAPPD: Set pairing information to CStore, PairedLAPPDTimeStamps[14] size = " + std::to_string(PairedLAPPDTimeStamps[14].size()), v_message, verbosityEBLAPPD);
  Log("EBLAPPD: Set pairing information to CStore, PairedLAPPDTimeStamps[47] size = " + std::to_string(PairedLAPPDTimeStamps[47].size()), v_message, verbosityEBLAPPD);

  // Set the indexing of buffer
  m_data->CStore.Set("Buffer_LAPPDTimestamp_ns", Buffer_LAPPDTimestamp_ns);

  // Also set all buffers to CStore
  m_data->CStore.Set("Buffer_LAPPDData", Buffer_LAPPDData);
  m_data->CStore.Set("Buffer_LAPPDBeamgate_ns", Buffer_LAPPDBeamgate_ns);
  m_data->CStore.Set("Buffer_LAPPDOffset", Buffer_LAPPDOffset);
  m_data->CStore.Set("Buffer_LAPPDBeamgate_Raw", Buffer_LAPPDBeamgate_Raw);
  m_data->CStore.Set("Buffer_LAPPDTimestamp_Raw", Buffer_LAPPDTimestamp_Raw);
  m_data->CStore.Set("Buffer_LAPPDBGCorrection", Buffer_LAPPDBGCorrection);
  m_data->CStore.Set("Buffer_LAPPDTSCorrection", Buffer_LAPPDTSCorrection);
  m_data->CStore.Set("Buffer_LAPPDOffset_minus_ps", Buffer_LAPPDOffset_minus_ps);
  m_data->CStore.Set("Buffer_LAPPDRunCode", Buffer_RunCode);

  m_data->CStore.Set("Buffer_LAPPDBG_PPSBefore", Buffer_LAPPDBG_PPSBefore);
  m_data->CStore.Set("Buffer_LAPPDBG_PPSAfter", Buffer_LAPPDBG_PPSAfter);
  m_data->CStore.Set("Buffer_LAPPDBG_PPSDiff", Buffer_LAPPDBG_PPSDiff);
  m_data->CStore.Set("Buffer_LAPPDBG_PPSMissing", Buffer_LAPPDBG_PPSMissing);
  m_data->CStore.Set("Buffer_LAPPDTS_PPSBefore", Buffer_LAPPDTS_PPSBefore);
  m_data->CStore.Set("Buffer_LAPPDTS_PPSAfter", Buffer_LAPPDTS_PPSAfter);
  m_data->CStore.Set("Buffer_LAPPDTS_PPSDiff", Buffer_LAPPDTS_PPSDiff);
  m_data->CStore.Set("Buffer_LAPPDTS_PPSMissing", Buffer_LAPPDTS_PPSMissing);

  exeNum++;

  return true;
}

bool EBLAPPD::Finalise()
{
  Log("\033[1;34mEBLAPPD: Finalising\033[0m", v_message, verbosityEBLAPPD);
  Log("EBLAPPD: Matched LAPPD number = " + std::to_string(matchedLAPPDNumber), v_message, verbosityEBLAPPD);
  Log("EBLAPPD: Unmatched LAPPD number = " + std::to_string(MatchBuffer_LAPPDTimestamp_ns.size()), v_message, verbosityEBLAPPD);
  return true;
}

bool EBLAPPD::CleanData()
{
  LAPPDBeamgate_ns = 0;
  LAPPDTimestamp_ns = 0;
  LAPPDOffset = 0;
  LAPPDBeamgate_Raw = 0;
  LAPPDTimestamp_Raw = 0;
  LAPPDBGCorrection = 0;
  LAPPDTSCorrection = 0;
  LAPPDOffset_minus_ps = 0;

  LAPPDBG_PPSBefore = 0;
  LAPPDBG_PPSAfter = 0;
  LAPPDBG_PPSDiff = 0;
  LAPPDBG_PPSMissing = 0;
  LAPPDTS_PPSBefore = 0;
  LAPPDTS_PPSAfter = 0;
  LAPPDTS_PPSDiff = 0;
  LAPPDTS_PPSMissing = 0;

  return true;
}

bool EBLAPPD::LoadLAPPDData()
{
  // get the LAPPD beamgate
  LAPPDBeamgate_Raw = 0;
  LAPPDTimestamp_Raw = 0;
  m_data->CStore.Get("LAPPDBeamgate_Raw", LAPPDBeamgate_Raw);
  m_data->CStore.Get("LAPPDTimestamp_Raw", LAPPDTimestamp_Raw);

  LAPPDBeamgate_ns = LAPPDBeamgate_Raw * 3.125;
  LAPPDTimestamp_ns = LAPPDTimestamp_Raw * 3.125;

  LAPPDBGCorrection = 0;
  LAPPDTSCorrection = 0;
  LAPPDOffset_minus_ps = 0;
  m_data->CStore.Get("LAPPDBGCorrection", LAPPDBGCorrection);
  m_data->CStore.Get("LAPPDTSCorrection", LAPPDTSCorrection);
  m_data->CStore.Get("LAPPDOffset_minus_ps", LAPPDOffset_minus_ps);
  LAPPDOffset = 0;
  m_data->CStore.Get("LAPPDOffset", LAPPDOffset);

  LAPPDBeamgate_ns = LAPPDBeamgate_ns + LAPPDBGCorrection + LAPPDOffset;
  LAPPDTimestamp_ns = LAPPDTimestamp_ns + LAPPDTSCorrection + LAPPDOffset;

  LAPPDBG_PPSBefore = 0;
  LAPPDBG_PPSAfter = 0;
  LAPPDBG_PPSDiff = 0;
  LAPPDBG_PPSMissing = 0;
  LAPPDTS_PPSBefore = 0;
  LAPPDTS_PPSAfter = 0;
  LAPPDTS_PPSDiff = 0;
  LAPPDTS_PPSMissing = 0;
  m_data->CStore.Get("BG_PPSBefore", LAPPDBG_PPSBefore);
  m_data->CStore.Get("BG_PPSAfter", LAPPDBG_PPSAfter);
  m_data->CStore.Get("BG_PPSDiff", LAPPDBG_PPSDiff);
  m_data->CStore.Get("BG_PPSMissing", LAPPDBG_PPSMissing);
  m_data->CStore.Get("TS_PPSBefore", LAPPDTS_PPSBefore);
  m_data->CStore.Get("TS_PPSAfter", LAPPDTS_PPSAfter);
  m_data->CStore.Get("TS_PPSDiff", LAPPDTS_PPSDiff);
  m_data->CStore.Get("TS_PPSMissing", LAPPDTS_PPSMissing);

  if (verbosityEBLAPPD > 1)
  {
    cout << "Processing new LAPPD data from store" << endl;
    cout << "Got info: LAPPDBeamgate_Raw: " << LAPPDBeamgate_Raw << ", LAPPDTimestamp_Raw: " << LAPPDTimestamp_Raw << ", LAPPDBGCorrection: " << LAPPDBGCorrection << ", LAPPDTSCorrection: " << LAPPDTSCorrection << ", LAPPDOffset: " << LAPPDOffset << ", LAPPDOffset_minus_ps: " << LAPPDOffset_minus_ps << ", LAPPDBG_PPSBefore: " << LAPPDBG_PPSBefore << ", LAPPDBG_PPSAfter: " << LAPPDBG_PPSAfter << ", LAPPDBG_PPSDiff: " << LAPPDBG_PPSDiff << ", LAPPDBG_PPSMissing: " << LAPPDBG_PPSMissing << ", LAPPDTS_PPSBefore: " << LAPPDTS_PPSBefore << ", LAPPDTS_PPSAfter: " << LAPPDTS_PPSAfter << ", LAPPDTS_PPSDiff: " << LAPPDTS_PPSDiff << ", LAPPDTS_PPSMissing: " << LAPPDTS_PPSMissing << endl;
    cout << "LAPPDBeamgate_ns:   " << LAPPDBeamgate_ns << endl;
    cout << "LAPPDTimestamp_ns: " << LAPPDTimestamp_ns << endl;
  }

  bool gotdata = m_data->CStore.Get("StoreLoadedLAPPDData", dat);

  if (gotdata)
  {
    Buffer_LAPPDTimestamp_ns.push_back(LAPPDTimestamp_ns);

    Buffer_LAPPDData.push_back(dat);
    Buffer_LAPPDBeamgate_ns.push_back(LAPPDBeamgate_ns);
    Buffer_LAPPDOffset.push_back(LAPPDOffset);
    Buffer_LAPPDBeamgate_Raw.push_back(LAPPDBeamgate_Raw);
    Buffer_LAPPDTimestamp_Raw.push_back(LAPPDTimestamp_Raw);
    Buffer_LAPPDBGCorrection.push_back(LAPPDBGCorrection);
    Buffer_LAPPDTSCorrection.push_back(LAPPDTSCorrection);
    Buffer_LAPPDOffset_minus_ps.push_back(LAPPDOffset_minus_ps);
    Buffer_RunCode.push_back(currentRunCode);

    Buffer_LAPPDBG_PPSBefore.push_back(LAPPDBG_PPSBefore);
    Buffer_LAPPDBG_PPSAfter.push_back(LAPPDBG_PPSAfter);
    Buffer_LAPPDBG_PPSDiff.push_back(LAPPDBG_PPSDiff);
    Buffer_LAPPDBG_PPSMissing.push_back(LAPPDBG_PPSMissing);
    Buffer_LAPPDTS_PPSBefore.push_back(LAPPDTS_PPSBefore);
    Buffer_LAPPDTS_PPSAfter.push_back(LAPPDTS_PPSAfter);
    Buffer_LAPPDTS_PPSDiff.push_back(LAPPDTS_PPSDiff);
    Buffer_LAPPDTS_PPSMissing.push_back(LAPPDTS_PPSMissing);

    MatchBuffer_LAPPDTimestamp_ns.push_back(LAPPDTimestamp_ns);

    if (LAPPDTS_PPSMissing != LAPPDBG_PPSMissing)
      Log("EBLAPPD: PPS missing in BG and TS are different, BG: " + std::to_string(LAPPDBG_PPSMissing) + ", TS: " + std::to_string(LAPPDTS_PPSMissing), v_warning, verbosityEBLAPPD);

    Log("EBLAPPD: Loaded LAPPD data to buffer, Buffer_LAPPDData size after this load is now " + std::to_string(Buffer_LAPPDData.size()), v_message, verbosityEBLAPPD);
  }

  return true;
}

bool EBLAPPD::Matching(int targetTrigger, int matchToTrack)
{
  Log("\033[1;34m******* EBLAPPD : Matching *******\033[0m", v_message, verbosityEBLAPPD);
  Log("EBLAPPD: Matching LAPPD data with target trigger " + std::to_string(targetTrigger) + " in track " + std::to_string(matchToTrack), v_message, verbosityEBLAPPD);

  std::map<int, std::vector<std::map<uint64_t, uint32_t>>> GroupedTriggersInTotal; // each map is a group of triggers, with the key is the target trigger word
  m_data->CStore.Get("GroupedTriggersInTotal", GroupedTriggersInTotal);
  // print how many trigger groups in each track

  vector<uint64_t> matchedLAPPDTimes;
  vector<int> indexToRemove;
  std::map<int, int> matchedNumberInTrack;

  // loop the LAPPDDataBuffer keys, and loop all the grouped triggers
  // in each group of trigger, find the target trigger word and it's time
  // fine the minimum time difference, if smaller than matchTolerance_ns, then save the time to PairedCTCTimeStamps and PairedLAPPDTimeStamps
  for (int i = 0; i < MatchBuffer_LAPPDTimestamp_ns.size(); i++)
  {
    uint64_t LAPPDtime = MatchBuffer_LAPPDTimestamp_ns.at(i);
    // if found LAPPDtime at PairedLAPPDTimeStamps, skip //shouldn't happen
    if (std::find(PairedLAPPDTimeStamps[matchToTrack].begin(), PairedLAPPDTimeStamps[matchToTrack].end(), LAPPDtime) != PairedLAPPDTimeStamps[matchToTrack].end())
    {
      Log("EBLAPPD: Buffer " + std::to_string(i) + " with time " + std::to_string(Buffer_LAPPDTimestamp_ns.at(i)) + ": Found a match already", v_message, verbosityEBLAPPD);
      continue;
    }

    // set minDT to 5 min
    uint64_t minDT = 5 * 60 * 1e9;
    uint64_t minDTTrigger = 0;
    uint64_t dt = 0;
    uint32_t matchedTrigWord = 0;
    int matchedTrack = 0;
    int matchedIndex = 0;

    for (const std::pair<int, std::vector<std::map<uint64_t, uint32_t>>>& pair : GroupedTriggersInTotal)
    {
      int TrackTriggerWord = pair.first;
      if (matchedNumberInTrack.find(TrackTriggerWord) == matchedNumberInTrack.end())
        matchedNumberInTrack.emplace(TrackTriggerWord, 0);
      if (TrackTriggerWord != matchToTrack && !matchToAllTriggers)
      {
        // Log("EBLAPPD: Skipping TrackTriggerWord " + std::to_string(TrackTriggerWord), v_debug, verbosityEBLAPPD);
        continue;
      }
      const vector<std::map<uint64_t, uint32_t>>& GroupedTriggers = pair.second;

      for (int j = 0; j < GroupedTriggers.size(); j++)
      {
        const map<uint64_t, uint32_t>& groupedTrigger = GroupedTriggers.at(j);
        // itearte over all the grouped triggers, if the value is target trigger, then calculate the time difference
        for (const std::pair<uint64_t, uint32_t>& p : groupedTrigger)
        {
          if (matchToAllTriggers || p.second == targetTrigger)
          {

            if (LAPPDtime > p.first)
            {
              dt = LAPPDtime - p.first;
            }
            else
            {
              dt = p.first - LAPPDtime;
            }
            if (dt < minDT)
            {
              minDT = dt;
              minDTTrigger = p.first;
              matchedTrigWord = p.second;
              matchedTrack = TrackTriggerWord;
              matchedIndex = j;
            }
          }
        }
      }
    }

    Log("EBLAPPD: at buffer " + std::to_string(i) + " with time " + std::to_string(Buffer_LAPPDTimestamp_ns.at(i)) + ", minDT: " + std::to_string(minDT), v_debug, verbosityEBLAPPD);
    if (minDT < matchTolerance_ns)
    {
      PairedCTCTimeStamps[matchedTrack].push_back(minDTTrigger);
      PairedLAPPDTimeStamps[matchedTrack].push_back(LAPPDtime);
      PairedLAPPD_TriggerIndex[matchedTrack].push_back(matchedIndex);

      matchedLAPPDTimes.push_back(LAPPDtime);
      indexToRemove.push_back(i);
      matchedLAPPDNumber++;
      matchedNumberInTrack[matchedTrack]++;
      Log("EBLAPPD: Buffer " + std::to_string(i) + " with time " + std::to_string(Buffer_LAPPDTimestamp_ns.at(i)) + ": Found a match for LAPPD data at " + std::to_string(LAPPDtime) + " with target trigger at " + std::to_string(minDTTrigger) + " with minDT " + std::to_string(minDT), v_message, verbosityEBLAPPD);
    }
  }
  Log("EBLAPPD: Finished matching LAPPD data with target triggers, " + std::to_string(matchedLAPPDTimes.size()) + " new matched found, total matchedLAPPDNumber = " + std::to_string(matchedLAPPDNumber) + " in buffer size = " + std::to_string(MatchBuffer_LAPPDTimestamp_ns.size()), v_message, verbosityEBLAPPD);

  for (int i = indexToRemove.size() - 1; i >= 0; i--)
  {
    MatchBuffer_LAPPDTimestamp_ns.erase(MatchBuffer_LAPPDTimestamp_ns.begin() + indexToRemove.at(i));
  }

  Log("EBLAPPD: Finished removing paired LAPPD data from match buffer, MatchBuffer_LAPPDTimestamp_ns size is now " + std::to_string(MatchBuffer_LAPPDTimestamp_ns.size()), v_message, verbosityEBLAPPD);
  // print all elements in matchedNumberInTrack with key and value
  for (std::pair<int, int> pair : matchedNumberInTrack)
  {
    Log("EBLAPPD: Match finished, matched number in Track " + std::to_string(pair.first) + " is = " + std::to_string(pair.second), v_message, verbosityEBLAPPD);
  }

  return true;
}
