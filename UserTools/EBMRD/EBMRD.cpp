#include "EBMRD.h"

EBMRD::EBMRD() : Tool() {}

bool EBMRD::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if (configfile != "")
    m_variables.Initialise(configfile); // loading config file
  // m_variables.Print();

  m_data = &data; // assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("verbosityEBMRD", verbosityEBMRD);
  matchTargetTrigger = 8;
  m_variables.Get("matchTargetTrigger", matchTargetTrigger);
  matchTolerance_ns = 2000000; // default 2ms
  m_variables.Get("matchTolerance_ns", matchTolerance_ns);
  exePerMatch = 500;
  m_variables.Get("exePerMatch", exePerMatch);
  matchToAllTriggers = 0;
  m_variables.Get("matchToAllTriggers", matchToAllTriggers);

  matchedMRDNumber = 0;

  return true;
}

bool EBMRD::Execute()
{
  m_data->CStore.Get("RunCode", currentRunCode);

  m_data->CStore.Get("MRDEvents", MRDEvents);
  m_data->CStore.Get("MRDEventTriggerTypes", MRDEventTriggerTypes);
  m_data->CStore.Get("MRDBeamLoopback", MRDBeamLoopback);
  m_data->CStore.Get("MRDCosmicLoopback", MRDCosmicLoopback);
  m_data->CStore.Get("NewMRDDataAvailable", NewMRDDataAvailable);

  m_data->CStore.Get("PairedMRDTriggerTimestamp", PairedCTCTimeStamps);
  m_data->CStore.Get("PairedMRDTimeStamps", PairedMRDTimeStamps);

  Log("EBMRD: NewMRDDataAvailable = " + std::to_string(NewMRDDataAvailable) + ", Current loaded MRDEvents size is " + std::to_string(MRDEvents.size()), v_message, verbosityEBMRD);
  Log("EBMRD: Current buffer size is " + std::to_string(MRDEventsBuffer.size()), v_message, verbosityEBMRD);
  // loop the MRDEvents, save every event to MRDEventsBuffer if it's not already in the buffer
  int newLoadedEvents = 0;
  for (std::pair<uint64_t, std::vector<std::pair<unsigned long, int>>> p : MRDEvents)
  {
    uint64_t MTCtime = p.first;
    std::vector<std::pair<unsigned long, int>> WaveMap = p.second;
    // if find the MTCtime in the PairedMRDTimeStamps, then skip
    if (PairedMRDTimeStamps.size() > 0)
    {
      bool skip = false;
      for (std::pair<int, std::vector<uint64_t>> pair : PairedMRDTimeStamps)
      {
        for (uint64_t t : pair.second)
        {
          if (t == MTCtime)
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
    if (MRDEventsBuffer.find(MTCtime) == MRDEventsBuffer.end())
    {
      MRDEventsBuffer.emplace(MTCtime, WaveMap);
      newLoadedEvents++;
    }
  }
  Log("EBMRD: Finished loading MRDEvents to buffer, Buffer_MRDEvents size is now " + std::to_string(MRDEventsBuffer.size()) + " new loaded events = " + std::to_string(newLoadedEvents), v_message, verbosityEBMRD);

  bool stopLoop = false;
  m_data->vars.Get("StopLoop", stopLoop);
  int runNum = thisRunNum;
  m_data->vars.Get("RunNumber", thisRunNum);
  bool ForceMRDMatching = false;
  m_data->CStore.Get("ForceMRDMatching", ForceMRDMatching);

  if (exeNum % exePerMatch == 0 || runNum != thisRunNum || stopLoop || ForceMRDMatching)
  {
    Log("EBMRD: exeNum = " + std::to_string(exeNum) + ". Doing matching now", v_message, verbosityEBMRD);
    if (matchToAllTriggers)
    {
      Matching(0, 0);
    }
    else
    {
      bool BeamTriggerGroupped = false;
      m_data->CStore.Get("BeamTriggerGroupped", BeamTriggerGroupped);
      bool CosmicTriggerGroupped = false;
      m_data->CStore.Get("CosmicTriggerGroupped", CosmicTriggerGroupped);
      bool NuMITriggerGroupped = false;
      m_data->CStore.Get("NuMITriggerGroupped", NuMITriggerGroupped);
      
      
      if (BeamTriggerGroupped)
        Matching(matchTargetTrigger, 14);
      if (CosmicTriggerGroupped)
      {
        Matching(36, 36);
        Matching(45, 46);
      }
      if(NuMITriggerGroupped)
        Matching(42, 46);

      if(!BeamTriggerGroupped && !CosmicTriggerGroupped && !NuMITriggerGroupped)
        Log("EBMRD: BeamTriggerGroupped and CosmicTriggerGroupped are false, no beam trigger groupped in the grouper, stop matching", v_message, verbosityEBMRD);
    }
  }

  m_data->CStore.Set("PairedMRDTriggerTimestamp", PairedCTCTimeStamps);
  m_data->CStore.Set("PairedMRDTimeStamps", PairedMRDTimeStamps);
  m_data->CStore.Set("PairedMRD_TriggerIndex", PairedMRD_TriggerIndex);
  m_data->CStore.Set("MRDHitMapRunCode", MRDHitMapRunCode);

  exeNum++;
  return true;
}

bool EBMRD::Finalise()
{
  Log("\033[1;34mEBMRD: Finalising\033[0m", v_message, verbosityEBMRD);
  Log("EBMRD: Matched MRD number = " + std::to_string(matchedMRDNumber), v_message, verbosityEBMRD);
  Log("EBMRD: Unmatched MRD number = " + std::to_string(MRDEventsBuffer.size()), v_message, verbosityEBMRD);
  return true;
}

bool EBMRD::Matching(int targetTrigger, int matchToTrack)
{
  cout << "\033[1;34m******* EBMRD : Matching *******\033[0m" << endl;
  std::map<int, std::vector<std::map<uint64_t, uint32_t>>> GroupedTriggersInTotal; // each map is a group of triggers, with the key is the target trigger word
  m_data->CStore.Get("GroupedTriggersInTotal", GroupedTriggersInTotal);

  Log("EBMRD: Got GroupedTriggersInTotal[14] size: " + std::to_string(GroupedTriggersInTotal[14].size()), v_message, verbosityEBMRD);

  vector<uint64_t> matchedMRDTimes;
  std::map<int, int> matchedNumberInTrack;

  // loop the MRDEventsBuffer keys, and loop all the grouped triggers
  // in each group of trigger, find the target trigger word and it's time
  // fine the minimum time difference, if smaller than matchTolerance_ns, then save the time to PairedCTCTimeStamps and PairedMRDTimeStamps
  int loopNum = 0;
  for (std::pair<uint64_t, std::vector<std::pair<unsigned long, int>>> mrdpair : MRDEventsBuffer)
  {
    if (verbosityEBMRD > 11)
      cout << "******************EBMRD: new MRD event: " << loopNum << endl;
    uint64_t MTCtime = mrdpair.first;
    std::vector<std::pair<unsigned long, int>> WaveMap = mrdpair.second;
    // set minDT to 5 min
    uint64_t minDT = 5 * 60 * 1e9;
    uint64_t minDTTrigger = 0;
    uint64_t dt = 0;
    uint32_t matchedTrigWord = 0;
    int matchedTrack = 0;
    int matchedIndex = 0;
    for (std::pair<int, std::vector<std::map<uint64_t, uint32_t>>> pair : GroupedTriggersInTotal)
    {
      int TrackTriggerWord = pair.first;
      if (TrackTriggerWord != matchToTrack && !matchToAllTriggers)
      {
        Log("EBMRD: Skipping TrackTriggerWord " + std::to_string(TrackTriggerWord), v_debug, verbosityEBMRD);
        continue;
      }
      if (matchedNumberInTrack.find(TrackTriggerWord) == matchedNumberInTrack.end())
        matchedNumberInTrack.emplace(TrackTriggerWord, 0);

      vector<std::map<uint64_t, uint32_t>> groupedTriggers = pair.second;

      for (int i = 0; i < groupedTriggers.size(); i++)
      {
        map<uint64_t, uint32_t> groupedTrigger = groupedTriggers.at(i);
        // itearte over all the grouped triggers, if the value is target trigger, then calculate the time difference
        for (std::pair<uint64_t, uint32_t> p : groupedTrigger)
        {
          if (matchToAllTriggers || p.second == targetTrigger)
          {
            if (MTCtime > p.first)
            {
              dt = MTCtime - p.first;
            }
            else
            {
              dt = p.first - MTCtime;
            }
            if (dt < minDT)
            {
              minDT = dt;
              minDTTrigger = p.first;
              matchedTrigWord = p.second;
              matchedIndex = p.second;
              matchedTrack = TrackTriggerWord;

              // if(verbosityEBMRD > 11) cout<<"EBMRD: dt: "<<dt<<" minDT: "<<minDT<<" minDTTrigger: "<<minDTTrigger<<endl;
            }
          }
        }
      }
    }

    Log("EBMRD: looping MRD event " + std::to_string(loopNum) + " with time " + std::to_string(MTCtime) + ", minDT: " + std::to_string(minDT), v_debug, verbosityEBMRD);
    if (minDT < matchTolerance_ns)
    {
      PairedCTCTimeStamps[matchedTrack].push_back(minDTTrigger);
      PairedMRDTimeStamps[matchedTrack].push_back(MTCtime);
      PairedMRD_TriggerIndex[matchedTrack].push_back(matchedIndex);
      MRDHitMapRunCode.emplace(MTCtime, currentRunCode);
      matchedMRDTimes.push_back(MTCtime);
      matchedNumberInTrack[matchedTrack]++;
      matchedMRDNumber++;
      Log("EBMRD: Found a match for MRD data at " + std::to_string(MTCtime) + " with target trigger at " + std::to_string(minDTTrigger) + " with dt " + std::to_string(minDT), v_message, verbosityEBMRD);
    }
    loopNum++;
  }
  Log("EBMRD: total matched MRD event number = " + std::to_string(matchedMRDNumber), v_message, verbosityEBMRD);
  Log("EBMRD: Before erase, current number of unfinished MRD events after match in MRDEventsBuffer: " + std::to_string(MRDEventsBuffer.size()), v_message, verbosityEBMRD);
  for (uint64_t t : matchedMRDTimes)
  {
    MRDEventsBuffer.erase(t);
  }
  Log("EBMRD: After erase, current number of unfinished MRD events after match in MRDEventsBuffer: " + std::to_string(MRDEventsBuffer.size()), v_message, verbosityEBMRD);

  // print all elements in matchedNumberInTrack with key and value
  for (std::pair<int, int> pair : matchedNumberInTrack)
  {
    Log("EBMRD: Match finished, matched number in Track " + std::to_string(pair.first) + " is " + std::to_string(pair.second), v_message, verbosityEBMRD);
  }

  return true;
}