#include "EBTriggerGrouper.h"

EBTriggerGrouper::EBTriggerGrouper() : Tool() {}

bool EBTriggerGrouper::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if (configfile != "")
    m_variables.Initialise(configfile); // loading config file
  // m_variables.Print();

  m_data = &data; // assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("verbosityEBTG", verbosityEBTG);
  savePath = "";
  m_variables.Get("savePath", savePath);
  m_variables.Get("GroupMode", GroupMode);
  m_variables.Get("GroupTolerance", GroupTolerance);
  m_variables.Get("GroupTrigWord", GroupTrigWord);

  TimeToTriggerWordMap = new std::map<uint64_t, std::vector<uint32_t>>;
  TimeToTriggerWordMapComplete = new std::map<uint64_t, std::vector<uint32_t>>;

  groupBeam = true;
  m_variables.Get("groupBeam", groupBeam);
  groupCosmic = false;
  m_variables.Get("groupCosmic", groupCosmic);
  groupLaser = true;
  m_variables.Get("groupLaser", groupLaser);
  groupLED = false;
  m_variables.Get("groupLED", groupLED);
  groupAmBe = false;
  m_variables.Get("groupAmBe", groupAmBe);
  groupPPS = false;
  m_variables.Get("groupPPS", groupPPS);
  groupNuMI = true;
  m_variables.Get("groupNuMI", groupNuMI);

  BeamTriggerMain = 14;
  BeamTolerance = 3000000; // use 3 ms here, the spill time difference is 66ms
  m_variables.Get("BeamTolerance", BeamTolerance);
  BeamTriggers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 14, 37, 38, 40, 41, 46};
  m_data->CStore.Set("BeamTriggersUsedForGroupping", BeamTriggers);
  m_data->CStore.Set("BeamTriggerMain", BeamTriggerMain);

  LaserTriggerMain = 47;
  m_variables.Get("LaserTriggerMain", LaserTriggerMain);
  LaserTolerance = 1000; // 1us
  m_variables.Get("LaserTolerance", LaserTolerance);
  LaserTriggers = {47, 46}; // 47 first, then 46, (~232ns in run 4692 data)
  m_data->CStore.Set("LaserTriggersUsedForGroupping", LaserTriggers);

  PPSTriggerMain = 32;
  PPSTolerance = 30000; // 30 us
  m_variables.Get("PPSTolerance", PPSTolerance);
  PPSTriggers = {32, 34}; // 32 first, then 34, (~20us in run 4802 data)
  m_data->CStore.Set("PPSTriggersUsedForGroupping", PPSTriggers);

  CosmicTriggerMain = 45; // TODO need to check
  m_variables.Get("CosmicTriggerMain", CosmicTriggerMain);
  CosmicTolerance = 100; // 330000ns, TODO
  m_variables.Get("CosmicTolerance", CosmicTolerance);
  CosmicTriggers = {44, 45, 36, 27, 28, 29, 30, 46}; // TODO: check the trigger words in cosmic run
  m_data->CStore.Set("CosmicTriggersUsedForGroupping", CosmicTriggers);

  LEDTriggerMain = 31; // need to check
  m_variables.Get("LEDTriggerMain", LEDTriggerMain);
  LEDTolerance = 1000; // 1ms 440ns in LED run 4792
  m_variables.Get("LEDTolerance", LEDTolerance);
  LEDTriggers = {33, 22, 31, 43, 46}; // in run 4792 [[22, 33], [4160001224, 4160001664]]
  m_data->CStore.Set("LEDTriggersUsedForGroupping", LEDTriggers);

  AmBeTriggerMain = 11; // TODO: need to check
  m_variables.Get("AmBeTriggerMain", AmBeTriggerMain);
  AmBeTolerance = 5000; // 2336 in Ambe run 4707
  m_variables.Get("AmBeTolerance", AmBeTolerance);
  AmBeTriggers = {11, 12, 15, 19, 46}; // in run 4707 [[19, 12, 15, 11], [21275647928, 21275647936, 21275647952, 21275650264]]
  m_data->CStore.Set("AmBeTriggersUsedForGroupping", AmBeTriggers);

  NuMITriggerMain = 42;
  m_variables.Get("NuMITriggerMain", NuMITriggerMain);
  NuMITolerance = 100; // 100ns
  m_variables.Get("NuMITolerance", NuMITolerance);
  NuMITriggers = {42, 46}; //
  m_data->CStore.Set("NuMITriggersUsedForGroupping", NuMITriggers);

  StoreTotalEntry = 0;

  maxNumAllowedInBuffer = 10000;
  m_variables.Get("maxNumAllowedInBuffer", maxNumAllowedInBuffer);
  Log("EBTG: maxNumAllowedInBuffer = " + std::to_string(maxNumAllowedInBuffer), v_message, verbosityEBTG);

  return true;
}

bool EBTriggerGrouper::Execute()
{
  bool NewCTCDataAvailable = false;
  m_data->CStore.Get("NewCTCDataAvailable", NewCTCDataAvailable);
  Log("EBTG: NewCTCDataAvailable = " + std::to_string(NewCTCDataAvailable), v_message, verbosityEBTG);
  bool FileProcessingComplete = false;
  m_data->CStore.Get("FileProcessingComplete", FileProcessingComplete);
  if (FileProcessingComplete)
  {
    Log("EBTG: FileProcessingComplete = " + std::to_string(FileProcessingComplete), v_message, verbosityEBTG);
    return true;
  }
  if (!NewCTCDataAvailable)
  {
    Log("EBTG: No new CTC data avaliable", v_message, verbosityEBTG);
    return true;
  }
  Log("EBTG: Get Run information", v_message, verbosityEBTG);
  m_data->CStore.Get("RunNumber", CurrentRunNum);
  m_data->CStore.Get("SubRunNumber", CurrentSubrunNum);
  int CurrentUsingPartNum = CurrentPartNum;
  m_data->CStore.Get("PartFileNumber", CurrentPartNum);
  m_data->CStore.Get("RunCode", currentRunCode);

  m_data->CStore.Get("usingTriggerOverlap", usingTriggerOverlap);
  if (usingTriggerOverlap)
  {
    CurrentPartNum -= 1;
    Log("EBTG: usingTriggerOverlap, CurrentPartNum was minused 1 to be " + std::to_string(CurrentPartNum), v_message, verbosityEBTG);
  }

  /*
    if (CurrentPartNum != CurrentUsingPartNum)
    {
      Log("EBTG: PartFileNumber changed from " + std::to_string(CurrentUsingPartNum) + " to " + std::to_string(CurrentPartNum) + " Clear buffer with size " + std::to_string(TrigTimeForGroup.size()), v_message, verbosityEBTG);
      TrigTimeForGroup.clear();
      TrigWordForGroup.clear();
    }*/

  Log("EBTG: Accessing new Trigger data in CStore", v_message, verbosityEBTG);
  m_data->CStore.Get("TimeToTriggerWordMap", TimeToTriggerWordMap);
  m_data->CStore.Get("TimeToTriggerWordMapComplete", TimeToTriggerWordMapComplete);

  // do grouping in the TimeToTriggerWordMap, remove the groupped triggers from both of them
  // 1. load the trigger from TimeToTriggerWordMap to local buffer TrigTimeForGroup and TrigWordForGroup, clean it
  // 2. group the triggers in buffer, save to GroupedTriggers, and remove grouped trigger from buffer
  // 2.1 group trigger based on trigger word selection or time Tolerance relative to the target trigger
  // 2.2 for left triggers, pair them with the grouped trigger using the same Tolerance fo cover triggers before the target trigger
  // 2.3 run for different trigger tracks. like pps

  // Finally we should have:
  //  1. GroupedTriggers: mutiple grouped trigger tracks
  //  2. buffer: left triggers in buffer, not grouped

  vector<uint64_t> TriggerToRemove;
  for (auto it = TimeToTriggerWordMap->begin(); it != TimeToTriggerWordMap->end(); ++it)
  {
    uint64_t timestamp = it->first;
    std::vector<uint32_t> triggers = it->second;
    bool stopLoading = false;

    // if found the target trigger in the trigger vector, stop loading to buffer.
    if (usingTriggerOverlap)
    {
      for (auto trig = triggers.begin(); trig != triggers.end(); ++trig)
      {
        if (*trig == GroupTrigWord)
          stopLoading = true;
        Log("EBTG: when using TriggerOverlap, met the new target trigger " + std::to_string(GroupTrigWord) + ", stop loading to buffer", v_message, verbosityEBTG);
      }
    }
    if (stopLoading)
      break;

    for (auto trig = triggers.begin(); trig != triggers.end(); ++trig)
    {
      TrigTimeForGroup.push_back(timestamp);
      TrigWordForGroup.push_back(*trig);
      TriggerToRemove.push_back(timestamp);
      StoreTotalEntry++;
      if (verbosityEBTG > 11)
        cout << "EBTG: TrigTimeForGroup: " << timestamp << " TrigWordForGroup: " << *trig << endl;
    }
  }
  Log("EBTG: finish loading trigger from TimeToTriggerWordMap, buffer TrigTimeForGroup size = " + std::to_string(TrigTimeForGroup.size()) + ", loaded TimeToTriggerWordMap size = " + std::to_string(TimeToTriggerWordMap->size()), v_message, verbosityEBTG);
  Log("EBTG: Start erase triggers from TimeToTriggerWordMap and TimeToTriggerWordMapComplete, TimeToTriggerWordMap and TimeToTriggerWordMapComplete size = " + std::to_string(TimeToTriggerWordMap->size()) + ", " + std::to_string(TimeToTriggerWordMapComplete->size()), v_message, verbosityEBTG);
  for (int i = 0; i < TriggerToRemove.size(); i++)
  {
    TimeToTriggerWordMap->erase(TriggerToRemove[i]);
    TimeToTriggerWordMapComplete->erase(TriggerToRemove[i]);
  }
  Log("EBTG: Finish erase triggers from TimeToTriggerWordMap and TimeToTriggerWordMapComplete, TimeToTriggerWordMap and TimeToTriggerWordMapComplete size = " + std::to_string(TimeToTriggerWordMap->size()) + ", " + std::to_string(TimeToTriggerWordMapComplete->size()), v_message, verbosityEBTG);

  /*{
    //TODO: this should be removed after later changes was made
    Log("EBTG: Start grouping based on trigger word " + std::to_string(GroupTrigWord), v_message, verbosityEBTG);

    GroupByTolerance(); // remove grouped trigger in the function
    FillByTolerance();
    //CleanBuffer();

    Log("EBTG: grouping and filling finishes, buffer size is " + std::to_string(TrigTimeForGroup.size()) + ", now save to CStore", v_message, verbosityEBTG);

    m_data->CStore.Set("GroupedTriggers", GroupedTriggers);
  }*/

  m_data->CStore.Set("TrigTimeForGroup", TrigTimeForGroup);
  m_data->CStore.Set("TrigWordForGroup", TrigWordForGroup);

  Log("EBTG: Start grouping for different trigger tracks", v_message, verbosityEBTG);
  if (groupBeam)
  {
    Log("EBTG: Grouping Beam Triggers", v_message, verbosityEBTG);
    int groupNum = GroupByTrigWord(BeamTriggerMain, BeamTriggers, BeamTolerance);
    int fillNum = FillByTrigWord(BeamTriggerMain, BeamTriggers, BeamTolerance);
    Log("EBTG: Found Beam Trigger group: " + std::to_string(groupNum) + ". Fill additional triggers to group: " + std::to_string(fillNum), v_message, verbosityEBTG);
    m_data->CStore.Set("BeamTriggerGroupped", true);
  }
  else
  {
    m_data->CStore.Set("BeamTriggerGroupped", false);
  }

  if (groupCosmic)
  {
    Log("EBTG: Grouping Cosmic Triggers", v_message, verbosityEBTG);
    int groupNum = GroupByTrigWord(CosmicTriggerMain, CosmicTriggers, CosmicTolerance);
    int fillNum = FillByTrigWord(CosmicTriggerMain, CosmicTriggers, CosmicTolerance);
    Log("EBTG: Found Cosmic Trigger group: " + std::to_string(groupNum) + ". Fill additional triggers to group: " + std::to_string(fillNum), v_message, verbosityEBTG);
    m_data->CStore.Set("CosmicTriggerGroupped", true);
  }
  else
  {
    m_data->CStore.Set("CosmicTriggerGroupped", false);
  }

  if (groupLaser)
  {
    Log("EBTG: Grouping Laser Triggers", v_message, verbosityEBTG);
    int groupNum = GroupByTrigWord(LaserTriggerMain, LaserTriggers, LaserTolerance);
    int fillNum = FillByTrigWord(LaserTriggerMain, LaserTriggers, LaserTolerance);
    Log("EBTG: Found Laser Trigger group: " + std::to_string(groupNum) + ". Fill additional triggers to group: " + std::to_string(fillNum), v_message, verbosityEBTG);
    m_data->CStore.Set("LaserTriggerGroupped", true);
  }
  else
  {
    m_data->CStore.Set("LaserTriggerGroupped", false);
  }

  if (groupLED)
  {
    Log("EBTG: Grouping LED Triggers", v_message, verbosityEBTG);
    int groupNum = GroupByTrigWord(LEDTriggerMain, LEDTriggers, LEDTolerance);
    int fillNum = FillByTrigWord(LEDTriggerMain, LEDTriggers, LEDTolerance);
    Log("EBTG: Found LED Trigger group: " + std::to_string(groupNum) + ". Fill additional triggers to group: " + std::to_string(fillNum), v_message, verbosityEBTG);
    m_data->CStore.Set("LEDTriggerGroupped", true);
  }
  else
  {
    m_data->CStore.Set("LEDTriggerGroupped", false);
  }

  if (groupAmBe)
  {
    Log("EBTG: Grouping AmBe Triggers", v_message, verbosityEBTG);
    int groupNum = GroupByTrigWord(AmBeTriggerMain, AmBeTriggers, AmBeTolerance);
    int fillNum = FillByTrigWord(AmBeTriggerMain, AmBeTriggers, AmBeTolerance);
    Log("EBTG: Found AmBe Trigger group: " + std::to_string(groupNum) + ". Fill additional triggers to group: " + std::to_string(fillNum), v_message, verbosityEBTG);
    m_data->CStore.Set("AmBeTriggerGroupped", true);
  }
  else
  {
    m_data->CStore.Set("AmBeTriggerGroupped", false);
  }

  if (groupPPS)
  {
    Log("EBTG: Grouping PPS Triggers", v_message, verbosityEBTG);
    int groupNum = GroupByTrigWord(PPSTriggerMain, PPSTriggers, PPSTolerance);
    int fillNum = FillByTrigWord(PPSTriggerMain, PPSTriggers, PPSTolerance);
    Log("EBTG: Found PPS Trigger group: " + std::to_string(groupNum) + ". Fill additional triggers to group: " + std::to_string(fillNum), v_message, verbosityEBTG);
    m_data->CStore.Set("PPSTriggerGroupped", true);
  }
  else
  {
    m_data->CStore.Set("PPSTriggerGroupped", false);
  }

  if (groupNuMI)
  {
    Log("EBTG: Grouping NuMI Triggers", v_message, verbosityEBTG);
    int groupNum = GroupByTrigWord(NuMITriggerMain, NuMITriggers, NuMITolerance);
    int fillNum = FillByTrigWord(NuMITriggerMain, NuMITriggers, NuMITolerance);
    Log("EBTG: Found NuMI Trigger group: " + std::to_string(groupNum) + ". Fill additional triggers to group: " + std::to_string(fillNum), v_message, verbosityEBTG);
    m_data->CStore.Set("NuMITriggerGroupped", true);
  }
  else
  {
    m_data->CStore.Set("NuMITriggerGroupped", false);
  }

  Log("EBTG: Grouping for different trigger tracks finishes, save to CStore", v_message, verbosityEBTG);
  m_data->CStore.Set("GroupedTriggersInTotal", GroupedTriggersInTotal);
  m_data->CStore.Set("RunCodeInTotal", RunCodeInTotal);

  int removedTriggerNumber = CleanTriggerBuffer();
  Log("EBTG: CleanTriggerBuffer finishes, removed " + std::to_string(removedTriggerNumber) + " triggers from buffer, buffer size = " + std::to_string(TrigTimeForGroup.size()), v_message, verbosityEBTG);

  return true;
}

bool EBTriggerGrouper::Finalise()
{
  Log("\033[1;34mEBTG: Finalising\033[0m", v_message, verbosityEBTG);
  Log("EBTG: GroupedTriggersInTotal track number = " + std::to_string(GroupedTriggersInTotal.size()), v_message, verbosityEBTG);
  for (auto it = GroupedTriggersInTotal.begin(); it != GroupedTriggersInTotal.end(); ++it)
  {
    Log("EBTG: GroupedTriggersInTotal track " + std::to_string(it->first) + " size = " + std::to_string(it->second.size()), v_message, verbosityEBTG);
  }
  Log("EBTG: In configfile, set left trigger number = " + std::to_string(maxNumAllowedInBuffer), v_message, verbosityEBTG);
  Log("EBTG: Left Total loaded trigger number to buffer = " + std::to_string(StoreTotalEntry), v_message, verbosityEBTG);
  Log("EBTG: Ungrouped triggers in buffer size = " + std::to_string(TrigTimeForGroup.size()), v_message, verbosityEBTG);
  // calculate which trigger word left in TrigWordForGroup, and print them
  std::map<uint32_t, int> triggerWordCount;
  for (int i = 0; i < TrigWordForGroup.size(); i++)
  {
    triggerWordCount[TrigWordForGroup[i]]++;
  }
  for (auto it = triggerWordCount.begin(); it != triggerWordCount.end(); ++it)
  {
    Log("EBTG: Trigger word " + std::to_string(it->first) + " left in buffer " + std::to_string(it->second) + " times", v_message, verbosityEBTG);
  }
  Log("EBTG: Ungrouped triggers in TimeToTriggerWordMap size = " + std::to_string(TimeToTriggerWordMap->size()), v_message, verbosityEBTG);
  Log("EBTG: Ungrouped triggers in TimeToTriggerWordMapComplete size = " + std::to_string(TimeToTriggerWordMapComplete->size()), v_message, verbosityEBTG);

  // print first 10000 triggers to a txt for debug
  Log("EBTG: Save first 2000 triggers in each track of GroupedTriggersInTotal to TrigDebugGrouped.txt", v_message, verbosityEBTG);
  ofstream trigDebugGrouped;
  trigDebugGrouped.open("TrigDebugGrouped.txt");
  trigDebugGrouped << "trackTargetTrigger groupIndex triggerTime triggerWord" << endl;
  for (auto it = GroupedTriggersInTotal.begin(); it != GroupedTriggersInTotal.end(); ++it)
  {
    int i = 0;
    for (int j = 0; j < it->second.size(); j++)
    {
      for (auto trig = it->second[j].begin(); trig != it->second[j].end(); ++trig)
      {
        if (i > 2000)
          break;
        trigDebugGrouped << it->first << " " << j << " " << trig->first << " " << trig->second << endl;
        i++;
      }
      if (i > 2000)
        break;
    }
  }
  trigDebugGrouped.close();
  Log("EBTG: Save first 10000 triggers in TrigTimeForGroup to TrigDebugBufferLeft.txt", v_message, verbosityEBTG);
  ofstream trigDebugBufferLeft;
  trigDebugBufferLeft.open("TrigDebugBufferLeft.txt");
  for (int i = 0; i < TrigTimeForGroup.size(); i++)
  {
    if (i > 10000)
      break;
    trigDebugBufferLeft << i << " " << TrigTimeForGroup[i] << " " << TrigWordForGroup[i] << endl;
  }
  Log("EBTG: Save first 2000 triggers in TimeToTriggerWordMap to TrigDebug.txt", v_message, verbosityEBTG);
  ofstream trigDebug;
  trigDebug.open("TrigDebug.txt");
  int i = 0;
  for (auto it = TimeToTriggerWordMap->begin(); it != TimeToTriggerWordMap->end(); ++it)
  {
    if (i > 2000)
      break;
    trigDebug << it->first << " ";
    for (auto trig = it->second.begin(); trig != it->second.end(); ++trig)
    {
      trigDebug << *trig << " ";
    }
    trigDebug << endl;
    i++;
  }
  trigDebug.close();

  Log("EBTG: Printing SkippedDuplicateTriggers", v_message, verbosityEBTG);
  int totalSkipped = 0;
  for (auto it = SkippedDuplicateTriggers.begin(); it != SkippedDuplicateTriggers.end(); ++it)
  {
    Log("EBTG: Skipped " + std::to_string(it->second) + " groups of triggers for trigger track " + std::to_string(it->first), v_message, verbosityEBTG);
    totalSkipped += it->second;
  }
  Log("EBTG: Total skipped groups of triggers = " + std::to_string(totalSkipped), v_message, verbosityEBTG);

  return true;
}

bool EBTriggerGrouper::GroupByTolerance()
{
  Log("EBTG: GroupByTolerance()", v_warning, verbosityEBTG);
  Log("EBTG: Grouping by Tolerance: " + std::to_string(GroupTolerance), v_warning, verbosityEBTG);
  Log("EBTG: size of time buffer before grouping is " + std::to_string(TrigTimeForGroup.size()) + ", size of word buffer is " + std::to_string(TrigWordForGroup.size()), v_warning, verbosityEBTG);
  bool found = false;
  uint64_t prev_target_time = 0;
  std::map<uint64_t, uint32_t> ThisGroup;
  vector<int> toRemove;
  int totalGroupedTriggerNumber = 0;
  for (int i = 0; i < TrigTimeForGroup.size(); i++)
  {
    uint64_t dt = 0;
    if (TrigTimeForGroup[i] > prev_target_time)
      dt = TrigTimeForGroup[i] - prev_target_time;
    else if (TrigTimeForGroup[i] < prev_target_time)
      dt = prev_target_time - TrigTimeForGroup[i];
    else if (TrigTimeForGroup[i] == prev_target_time)
      dt = 0;

    if (!found && TrigWordForGroup[i] == GroupTrigWord)
    {
      found = true;
      ThisGroup.clear();
      prev_target_time = TrigTimeForGroup[i];
      ThisGroup.insert(std::pair<uint64_t, uint32_t>(TrigTimeForGroup[i], TrigWordForGroup[i]));
      toRemove.push_back(i);
    }
    else if (found && dt < GroupTolerance)
    {

      ThisGroup.insert(std::pair<uint64_t, uint32_t>(TrigTimeForGroup[i], TrigWordForGroup[i]));
      toRemove.push_back(i);
      if (TrigWordForGroup[i] == 10 || TrigWordForGroup[i] == 40 || TrigWordForGroup[i] == 41)
      {
        ThisGroup.insert(std::pair<uint64_t, uint32_t>(TrigTimeForGroup[i] + 1, TrigWordForGroup[i]));
      }
      ///!!! attention, the trigger word 10 always come with 8 with the same time, so it won't be inserted to the map
      // Therefore we add another 1ns to it, but it's not real
    }
    else if (found && dt > GroupTolerance)
    {
      if (verbosityEBTG > 9)
      {
        // print all triggers in this group
        cout << "EBTG: GroupByTolerance: ThisGroup size = " << ThisGroup.size() << endl;
        for (auto it = ThisGroup.begin(); it != ThisGroup.end(); ++it)
        {
          cout << it->first << " - " << it->second << ", ";
        }
        cout << endl;
      }
      GroupedTriggers.push_back(ThisGroup);
      totalGroupedTriggerNumber += ThisGroup.size();
      ThisGroup.clear();
      found = false;
    }
  }
  int totalToRemoveTriggerNumber = toRemove.size();
  // from end to beginning, remove the triggers in buffer based on toRemove index
  for (int i = toRemove.size() - 1; i >= 0; i--)
  {
    TrigTimeForGroup.erase(TrigTimeForGroup.begin() + toRemove[i]);
    TrigWordForGroup.erase(TrigWordForGroup.begin() + toRemove[i]);
  }
  Log("EBTG: GroupByTolerance Finished, accumulated saved " + std::to_string(GroupedTriggers.size()) + " group of triggers", v_warning, verbosityEBTG);
  Log("EBTG: buffer TriggersForGroup after grouping size = " + std::to_string(TrigTimeForGroup.size()), v_warning, verbosityEBTG);
  Log("EBTG: totalGroupedTriggerNumber in this grouping step is: " + std::to_string(totalGroupedTriggerNumber) + ", totalToRemoveTriggerNumber = " + std::to_string(totalToRemoveTriggerNumber), v_warning, verbosityEBTG);

  return true;
}

bool EBTriggerGrouper::FillByTolerance()
{
  Log("EBTG: FillByTolerance()", v_warning, verbosityEBTG);
  Log("EBTG: Fill by Tolerance: " + std::to_string(GroupTolerance), v_warning, verbosityEBTG);
  Log("EBTG: size of time buffer before refilling is " + std::to_string(TrigTimeForGroup.size()) + ", size of word buffer is " + std::to_string(TrigWordForGroup.size()), v_warning, verbosityEBTG);

  int addedTriggerNumber = 0;

  for (int i = TrigTimeForGroup.size() - 1; i >= 0; i--)
  {
    for (int j = 0; j < GroupedTriggers.size(); j++)
    {

      for (auto it = GroupedTriggers[j].begin(); it != GroupedTriggers[j].end(); ++it)
      {
        if (it->second == GroupTrigWord)
        {
          uint64_t dt = (TrigTimeForGroup[i] > it->first) ? (TrigTimeForGroup[i] - it->first) : (it->first - TrigTimeForGroup[i]);
          if (dt < GroupTolerance)
          {
            GroupedTriggers[j].insert(std::make_pair(TrigTimeForGroup[i], TrigWordForGroup[i]));
            TrigTimeForGroup.erase(TrigTimeForGroup.begin() + i);
            TrigWordForGroup.erase(TrigWordForGroup.begin() + i);
            addedTriggerNumber++;
            break;
          }
        }
      }
    }
  }

  Log("EBTG: FillByTolerance Finished, addedTriggerNumber = " + std::to_string(addedTriggerNumber), v_warning, verbosityEBTG);
  Log("EBTG: buffer TriggersForGroup after refilling size = " + std::to_string(TrigTimeForGroup.size()), v_warning, verbosityEBTG);

  return true;
}

bool EBTriggerGrouper::CleanBuffer()
{
  Log("EBTG: CleanBuffer()", v_warning, verbosityEBTG);
  Log("EBTG: size of time buffer before cleaning is " + std::to_string(TrigTimeForGroup.size()) + ", size of word buffer is " + std::to_string(TrigWordForGroup.size()), v_warning, verbosityEBTG);
  // check is there any target trigger word in the buffer
  // if yes, print a warning message
  // only leave the last 100 triggers in the buffer
  int numberOfTargetTriggers = 0;
  for (int i = 0; i < TrigWordForGroup.size(); i++)
  {
    if (TrigWordForGroup[i] == GroupTrigWord)
    {
      numberOfTargetTriggers++;
    }
  }
  if (numberOfTargetTriggers > 0)
  {
    Log("EBTG: CleanBuffer: Warning: there are " + std::to_string(numberOfTargetTriggers) + " target trigger words in the buffer", v_warning, verbosityEBTG);
  }
  else
  {
    Log("EBTG: CleanBuffer: no target trigger words left in the buffer.", v_warning, verbosityEBTG);
  }
  if (TrigTimeForGroup.size() > 100)
  {
    TrigTimeForGroup.erase(TrigTimeForGroup.begin(), TrigTimeForGroup.end() - 100);
    TrigWordForGroup.erase(TrigWordForGroup.begin(), TrigWordForGroup.end() - 100);
  }
  Log("EBTG: CleanBuffer Finished, buffer size = " + std::to_string(TrigTimeForGroup.size()), v_warning, verbosityEBTG);
  return true;
}

int EBTriggerGrouper::GroupByTrigWord(uint32_t mainTrigWord, vector<uint32_t> TrigWords, int tolerance)
{
  Log("EBTG: GroupByTrigWord()", v_warning, verbosityEBTG);
  Log("EBTG: Grouping by TrigWord: " + std::to_string(mainTrigWord), v_warning, verbosityEBTG);
  Log("EBTG: size of time buffer before grouping is " + std::to_string(TrigTimeForGroup.size()) + ", size of word buffer is " + std::to_string(TrigWordForGroup.size()), v_warning, verbosityEBTG);
  bool found = false;
  uint64_t prev_target_time = 0;
  std::map<uint64_t, uint32_t> ThisGroup;
  vector<int> toRemove;
  int totalGroupedTriggerNumber = 0;
  uint64_t trackTriggerTime = 0;
  uint32_t trackTriggerWord = 0;

  for (int i = 0; i < TrigTimeForGroup.size(); i++)
  {
    uint64_t dt = 0;
    if (TrigTimeForGroup[i] > prev_target_time)
      dt = TrigTimeForGroup[i] - prev_target_time;
    else if (TrigTimeForGroup[i] < prev_target_time)
      dt = prev_target_time - TrigTimeForGroup[i];
    else if (TrigTimeForGroup[i] == prev_target_time)
      dt = 0;

    if (!found && TrigWordForGroup[i] == mainTrigWord)
    {
      found = true;
      ThisGroup.clear();
      prev_target_time = TrigTimeForGroup[i];
      ThisGroup.insert(std::pair<uint64_t, uint32_t>(TrigTimeForGroup[i], TrigWordForGroup[i]));
      trackTriggerTime = TrigTimeForGroup[i];
      trackTriggerWord = TrigWordForGroup[i];
      toRemove.push_back(i);
    }
    else if (found && dt < tolerance)
    {
      // if found TrigWordForGroup in TrigWords, insert to ThisGroup
      if (std::find(TrigWords.begin(), TrigWords.end(), TrigWordForGroup[i]) != TrigWords.end())
      {
        ThisGroup.insert(std::pair<uint64_t, uint32_t>(TrigTimeForGroup[i], TrigWordForGroup[i]));
        toRemove.push_back(i);
        if (mainTrigWord == 14 && (TrigWordForGroup[i] == 10 || TrigWordForGroup[i] == 40 || TrigWordForGroup[i] == 41))
        {
          ThisGroup.insert(std::pair<uint64_t, uint32_t>(TrigTimeForGroup[i] + 1, TrigWordForGroup[i]));
        }
        ///!!! attention, the trigger word 10 always come with 8 with the same time, so it won't be inserted to the map
        // Therefore we add another 1ns to it, but it's not real
      }
    }
    else if (found && dt > tolerance)
    {
      if (verbosityEBTG > 9)
      {
        // print all triggers in this group
        cout << "EBTG: GroupByTrigWord: ThisGroup size = " << ThisGroup.size() << endl;
        for (auto it = ThisGroup.begin(); it != ThisGroup.end(); ++it)
        {
          cout << it->first << " - " << it->second << ", ";
        }
        cout << endl;
      }
      // check is the current track trigger already exist in GroupedTriggersInTotal, if so, don't add it
      // find trackTriggerTime in each element in GroupedTriggersInTotal[mainTrigWord]
      bool trackTriggerExist = false;
      for (int j = 0; j < GroupedTriggersInTotal[mainTrigWord].size(); j++)
      {
        for (auto it = GroupedTriggersInTotal[mainTrigWord][j].begin(); it != GroupedTriggersInTotal[mainTrigWord][j].end(); ++it)
        {
          if (it->first == trackTriggerTime && it->second == trackTriggerWord)
          {
            trackTriggerExist = true;
            break;
          }
        }
      }
      if (!trackTriggerExist)
      {
        GroupedTriggersInTotal[mainTrigWord].push_back(ThisGroup);
      }
      else
      {
        // in it's track of SkippedDuplicateTriggers, plus one. if found it, ++, if not, emplace 1
        if (SkippedDuplicateTriggers.find(mainTrigWord) != SkippedDuplicateTriggers.end())
        {
          SkippedDuplicateTriggers[mainTrigWord]++;
        }
        else
        {
          SkippedDuplicateTriggers.emplace(mainTrigWord, 1);
        }

        Log("EBTG: Found a duplicated main trigger with word " + std::to_string(mainTrigWord) + " and time " + std::to_string(trackTriggerTime), v_message, verbosityEBTG);
      }

      if (verbosityEBTG > 9)
        cout << "EBTG: saving grouped trigger with currentRunCode = " << currentRunCode << endl;
      if (!trackTriggerExist)
        RunCodeInTotal[mainTrigWord].push_back(currentRunCode);
      else
        Log("EBTG: Found a duplicated main trigger with word " + std::to_string(mainTrigWord) + " and time " + std::to_string(trackTriggerTime) + ", so skip adding run code vector", v_message, verbosityEBTG);
      totalGroupedTriggerNumber += ThisGroup.size();
      ThisGroup.clear();
      found = false;
    }
  }
  int totalToRemoveTriggerNumber = toRemove.size();
  // from end to beginning, remove the triggers in buffer based on toRemove index
  for (int i = toRemove.size() - 1; i >= 0; i--)
  {
    TrigTimeForGroup.erase(TrigTimeForGroup.begin() + toRemove[i]);
    TrigWordForGroup.erase(TrigWordForGroup.begin() + toRemove[i]);
  }
  Log("EBTG: GroupByTrigWord Finished, accumulated saved " + std::to_string(GroupedTriggers.size()) + " group of triggers", v_warning, verbosityEBTG);
  Log("EBTG: buffer TriggersForGroup after grouping size = " + std::to_string(TrigTimeForGroup.size()), v_warning, verbosityEBTG);
  Log("EBTG: totalGroupedTriggerNumber in this grouping step is: " + std::to_string(totalGroupedTriggerNumber) + ", totalToRemoveTriggerNumber = " + std::to_string(totalToRemoveTriggerNumber), v_warning, verbosityEBTG);

  return totalToRemoveTriggerNumber;
}

int EBTriggerGrouper::FillByTrigWord(uint32_t mainTrigWord, vector<uint32_t> TrigWords, int tolerance)
{
  // Fill the trigger maps at GroupedTriggersInTotal[mainTrigWord]
  // for all triggers in the buffer, if the trigger word is in the vector, and in the range of main TrigWord - the tolerance
  // push the trigger into the map

  Log("EBTG: FillByTrigWord()", v_warning, verbosityEBTG);
  Log("EBTG: Fill by TrigWord: " + std::to_string(mainTrigWord), v_warning, verbosityEBTG);
  Log("EBTG: size of time buffer before refilling is " + std::to_string(TrigTimeForGroup.size()) + ", size of word buffer is " + std::to_string(TrigWordForGroup.size()), v_warning, verbosityEBTG);

  int addedTriggerNumber = 0;
  vector<int> indexToRemove;

  for (int i = TrigTimeForGroup.size() - 1; i >= 0; i--)
  {
    if (verbosityEBTG > 11)
      cout << "EBTG: FillByTrigWord: mainTrigWord = " << mainTrigWord << ", TrigWordForGroup[i] = " << TrigWordForGroup[i] << ", TrigTimeForGroup[i] = " << TrigTimeForGroup[i] << ", tolerance = " << tolerance << endl;
    // check is current trigger word in TrigWords
    if (std::find(TrigWords.begin(), TrigWords.end(), TrigWordForGroup[i]) == TrigWords.end())
      continue;
    bool triggerToAdd = false;
    int insertTrack = 0;
    int insertGroupIndex = 0;
    uint64_t insertedDT = tolerance;
    for (int j = 0; j < GroupedTriggersInTotal[mainTrigWord].size(); j++)
    {
      for (auto it = GroupedTriggersInTotal[mainTrigWord][j].begin(); it != GroupedTriggersInTotal[mainTrigWord][j].end(); ++it)
      {
        if (it->second == mainTrigWord)
        {
          uint64_t dt = (TrigTimeForGroup[i] > it->first) ? (TrigTimeForGroup[i] - it->first) : (it->first - TrigTimeForGroup[i]);
          if (dt < tolerance && dt < insertedDT)
          {
            insertedDT = dt;
            insertTrack = mainTrigWord;
            insertGroupIndex = j;
            triggerToAdd = true;
            Log("EBTG: FillByTrigWord: Found trigger to add, insertTrack = " + std::to_string(insertTrack) + ", insertGroupIndex = " + std::to_string(insertGroupIndex) + ", insert DT = " + std::to_string(insertedDT), 9, verbosityEBTG);
            break;
          }
        }
      }
    }
    if (triggerToAdd)
    {
      GroupedTriggersInTotal[mainTrigWord][insertGroupIndex].insert(std::make_pair(TrigTimeForGroup[i], TrigWordForGroup[i]));
      indexToRemove.push_back(i);
      if (verbosityEBTG > 11)
        cout << "EBTG: FillByTrigWord: add this trigger: " << TrigTimeForGroup[i] << " - " << TrigWordForGroup[i] << endl;
      addedTriggerNumber++;
    }
  }
  Log("EBTG: FillByTrigWord Finished, addedTriggerNumber = " + std::to_string(addedTriggerNumber), v_warning, verbosityEBTG);
  Log("EBTG: indexToRemove size = " + std::to_string(indexToRemove.size()) + ", buffer size before remove is " + std::to_string(TrigTimeForGroup.size()), v_warning, verbosityEBTG);

  // sort indexToRemove from large to small, then remove the triggers in buffer based on toRemove index
  std::sort(indexToRemove.begin(), indexToRemove.end(), std::greater<int>());

  if (verbosityEBTG > 11)
  {
    // print all index to remove
    cout << "EBTG: FillByTrigWord: indexToRemove: ";
    for (int i = 0; i < indexToRemove.size(); i++)
    {
      cout << indexToRemove[i] << ", ";
    }
  }

  for (int i = 0; i < indexToRemove.size(); i++)
  {
    TrigTimeForGroup.erase(TrigTimeForGroup.begin() + indexToRemove[i]);
    TrigWordForGroup.erase(TrigWordForGroup.begin() + indexToRemove[i]);
  }
  Log("EBTG: buffer TriggersForGroup after refilling and removing size = " + std::to_string(TrigTimeForGroup.size()), v_warning, verbosityEBTG);

  return addedTriggerNumber;
}

int EBTriggerGrouper::CleanTriggerBuffer()
{
  // remove very early trigger in TrigTimeForGroup and TrigWordForGroup
  // only leave the latest maxNumAllowedInBuffer elements
  int removedNumber = 0;
  if (TrigTimeForGroup.size() > maxNumAllowedInBuffer)
  {
    removedNumber = TrigTimeForGroup.size() - maxNumAllowedInBuffer;
    removedTriggerInBuffer += removedNumber;
    Log("EBTG: CleanTriggerBuffer, will remove the earliest " + std::to_string(TrigTimeForGroup.size() - maxNumAllowedInBuffer) + " triggers in buffer", v_message, verbosityEBTG);
    TrigTimeForGroup.erase(TrigTimeForGroup.begin(), TrigTimeForGroup.end() - maxNumAllowedInBuffer);
    TrigWordForGroup.erase(TrigWordForGroup.begin(), TrigWordForGroup.end() - maxNumAllowedInBuffer);
  }
  return removedNumber;
}