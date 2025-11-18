#include "EBSaver.h"

EBSaver::EBSaver() : Tool() {}

bool EBSaver::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if (configfile != "")
    m_variables.Initialise(configfile); // loading config file
  // m_variables.Print();

  m_data = &data; // assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("verbosityEBSaver", verbosityEBSaver);
  savePath = "";
  m_variables.Get("savePath", savePath);
  saveName = "ProcessedData_";
  m_variables.Get("saveName", saveName);
  beamInfoFileName = "BeamInfo.root";
  m_variables.Get("beamInfoFileName", beamInfoFileName);

  saveTriggerGroups = true;
  savePMT = true;

  saveAllTriggers = false;
  m_variables.Get("saveAllTriggers", saveAllTriggers);
  saveMRD = true;
  m_variables.Get("saveMRD", saveMRD);
  saveLAPPD = true;
  m_variables.Get("saveLAPPD", saveLAPPD);
  saveOrphan = true;
  m_variables.Get("saveOrphan", saveOrphan);
  saveBeamInfo = true;
  m_variables.Get("saveBeamInfo", saveBeamInfo);

  saveRawBRFWaveform = true;
  m_variables.Get("saveRawBRFWaveform", saveRawBRFWaveform);
  saveRawRWMWaveform = true;
  m_variables.Get("saveRawRWMWaveform", saveRawRWMWaveform);

  ANNIEEvent = new BoostStore(false, 2);

  exeNumber = 0;
  savedTriggerGroupNumber = 0;
  savedPMTHitMapNumber = 0;
  savedMRDNumber = 0;
  savedLAPPDNumber = 0;

  savePerExe = 2000;

  if (savePMT)
    saveName += "PMT";
  if (saveMRD)
    saveName += "MRD";
  if (saveLAPPD)
    saveName += "LAPPD";

  m_data->CStore.Get("BeamTriggerMain", BeamTriggerMain);
  m_data->CStore.Get("LaserTriggerMain", LaserTriggerMain);
  m_data->CStore.Get("CosmicTriggerMain", CosmicTriggerMain);
  m_data->CStore.Get("LEDTriggerMain", LEDTriggerMain);
  m_data->CStore.Get("AmBeTriggerMain", AmBeTriggerMain);
  m_data->CStore.Get("PPSMain", PPSMain);

  InProgressHits = new std::map<uint64_t, std::map<unsigned long, std::vector<Hit>> *>;
  InProgressChkey = new std::map<uint64_t, std::vector<unsigned long>>;
  InProgressRecoADCHits = new std::map<uint64_t, std::map<unsigned long, std::vector<std::vector<ADCPulse>>>>;
  InProgressRecoADCHitsAux = new std::map<uint64_t, std::map<unsigned long, std::vector<std::vector<ADCPulse>>>>;
  InProgressHitsAux = new std::map<uint64_t, std::map<unsigned long, std::vector<Hit>> *>;
  FinishedRawAcqSize = new std::map<uint64_t, std::map<unsigned long, std::vector<int>>>;

  RWMRawWaveforms = new std::map<uint64_t, std::vector<uint16_t>>;
  BRFRawWaveforms = new std::map<uint64_t, std::vector<uint16_t>>;

  if (saveBeamInfo)
  {
    Log("EBSaver: saveBeamInfo is true, loading Beam Info", v_message, verbosityEBSaver);
    LoadBeamInfo();
  }

  return true;
}

bool EBSaver::Execute()
{

  bool saveProcessedFile = false;
  bool saveEverything = false;
  m_data->CStore.Get("SaveProcessedFile", saveProcessedFile);
  m_data->CStore.Get("SaveEverything", saveEverything);
  if (!saveProcessedFile && !saveEverything)
  {
    Log("EBSaver: Not saving data in this exe", v_message, verbosityEBSaver);
    return true;
  }
  cout << "\033[1;34m******* EBSaver : Start execute *******\033[0m" << endl;

  m_data->CStore.Get("RunCodeToSave", savingRunCode);
  GotAllDataFromOriginalBuffer();

  Log("EBSaver: Start saving data for run code " + std::to_string(savingRunCode), v_message, verbosityEBSaver);

  // clean the to remove buffer
  GroupedTriggerIndexToRemove.clear();
  PMTRunCodeToRemove.clear();
  MRDRunCodeToRemove.clear();
  LAPPDRunCodeToRemove.clear();

  PMTPairInfoToRemoveTime.clear();
  MRDPairInfoToRemoveTime.clear();
  LAPPDPairInfoToRemoveTime.clear();

  // get the grouped trigger, save the grouped triggers with the same runcode
  cout << "\033[1;34m******* EBSaver : Saving *******\033[0m" << endl;
  m_data->CStore.Get("GroupedTriggersInTotal", GroupedTriggersInTotal);
  m_data->CStore.Get("RunCodeInTotal", RunCodeInTotal);

  int savedEventNumber = 0;
  // loop each track in the RunCodeInTotal;
  for (auto const &track : RunCodeInTotal)
  {
    int triggerTrack = track.first;
    std::vector<int> RunCodeVector = track.second;
    // check if the RunCodeInTotal[tragetTrigWord] has the same size with GroupedTriggersInTotal[tragetTrigWord]
    Log("EBSaver: Looping trigger track " + std::to_string(triggerTrack) + " with RunCodeInTotal size " + std::to_string(RunCodeVector.size()) + " and GroupedTriggersInTotal size " + std::to_string(GroupedTriggersInTotal[triggerTrack].size()), v_message, verbosityEBSaver);

    if (RunCodeVector.size() != GroupedTriggersInTotal[triggerTrack].size())
    {
      Log("EBSaver: RunCodeInTotal and GroupedTriggersInTotal size not match, RunCodeVector size " + std::to_string(RunCodeVector.size()) + " and GroupedTriggersInTotal size " + std::to_string(GroupedTriggersInTotal[triggerTrack].size()), v_warning, verbosityEBSaver);
      continue;
    }
    // GroupedTriggersInTotal and RunCodeInTotal have the same size
    // same index is the cooresponding data
    for (int i = 0; i < RunCodeVector.size(); i++)
    {
      int runCode = RunCodeVector[i];
      // saving
      // if save Everything, in case there might be some mis aligned events not saved to that processed part file correctly and left in buffer, save them to corresponding part file
      if (runCode == savingRunCode)
      {
        Log("\033[1;34m ****EBSaver: Saving a new event with savedEventNumber = " + std::to_string(savedEventNumber) + " ****\033[0m", v_message, verbosityEBSaver);

        std::string saveFileName = savePath + saveName + "_R" + to_string(runCode / 100000) + "S" + to_string((runCode % 100000) / 10000 - 1) + "p" + to_string(runCode % 10000);
        Log("EBSaver: Saving to " + saveFileName + " with run code " + std::to_string(runCode) + " at index " + std::to_string(i), v_message, verbosityEBSaver);

        // if in last exe, the runcode for that group of trigger equals to the saving RunCode, save to ANNIEEvent Processed File
        SaveToANNIEEvent(saveFileName, runCode, triggerTrack, i);
        savedEventNumber++;
      }
    }
  }

  // remove the paired trigger and data timestamp at PairInfoToRemoveIndex from the buffer like PairedLAPPDTimeStamps
  for (auto const &track : GroupedTriggerIndexToRemove)
  {
    int triggerTrack = track.first;
    int groupIndex = track.second;
    GroupedTriggersInTotal[triggerTrack].erase(GroupedTriggersInTotal[triggerTrack].begin() + groupIndex);
    RunCodeInTotal[triggerTrack].erase(RunCodeInTotal[triggerTrack].begin() + groupIndex);
  }
  int PMTToRemove = 0;
  int MRDToRemove = 0;
  int LAPPDToRemove = 0;
  for (auto const &track : PMTPairInfoToRemoveTime)
  {
    int triggerTrack = track.first;
    std::vector<uint64_t> times = track.second;
    PMTToRemove += times.size();
    // print the size of times, and all elements in times
    Log("EBSaver: PMT PairInfoToRemoveTime at track " + std::to_string(triggerTrack) + " size " + std::to_string(times.size()), v_message, verbosityEBSaver);
    for (int i = 0; i < times.size(); i++)
      cout << i << ": " << times[i] << ", ";
  }
  cout << endl;
  for (auto const &track : MRDPairInfoToRemoveTime)
  {
    int triggerTrack = track.first;
    std::vector<uint64_t> times = track.second;
    MRDToRemove += times.size();
    Log("EBSaver: MRD PairInfoToRemoveTime at track " + std::to_string(triggerTrack) + " size " + std::to_string(times.size()), v_message, verbosityEBSaver);
    for (int i = 0; i < times.size(); i++)
      cout << i << ": " << times[i] << ", ";
  }
  cout << endl;
  for (auto const &track : LAPPDPairInfoToRemoveTime)
  {
    int triggerTrack = track.first;
    std::vector<uint64_t> times = track.second;
    LAPPDToRemove += times.size();
    Log("EBSaver: LAPPD PairInfoToRemoveTime at track " + std::to_string(triggerTrack) + " size " + std::to_string(times.size()), v_message, verbosityEBSaver);
    for (int i = 0; i < times.size(); i++)
      cout << i << ": " << times[i] << ", ";
  }
  cout << endl;

  Log("EBSaver: before remove from pair info buffer, going to remove PMT " + std::to_string(PMTToRemove) + ", MRD " + std::to_string(MRDToRemove) + ", LAPPD " + std::to_string(LAPPDToRemove), v_message, verbosityEBSaver);

  // print everything in PairedLAPPDTimeStamps for debug
  for (auto const &track : PairedLAPPDTimeStamps)
  {
    int triggerTrack = track.first;
    std::vector<uint64_t> times = track.second;
    Log("EBSaver: PairedLAPPDTimeStamps at track " + std::to_string(triggerTrack) + " size " + std::to_string(times.size()), v_message, verbosityEBSaver);
    for (int i = 0; i < times.size(); i++)
      cout << i << ": " << times[i] << ", ";
  }
  // print everything in Buffer_LAPPDTimestamp_ns for debug
  Log("EBSaver: Buffer_LAPPDTimestamp_ns size " + std::to_string(Buffer_LAPPDTimestamp_ns.size()), v_message, verbosityEBSaver);
  for (int i = 0; i < Buffer_LAPPDTimestamp_ns.size(); i++)
    cout << i << ": " << Buffer_LAPPDTimestamp_ns[i] << ", ";
  cout << endl;

  // delete LAPPD related timestamps
  int removedLAPPD = 0;
  for (auto const &track : LAPPDPairInfoToRemoveTime)
  {
    int triggerTrack = track.first;
    std::vector<uint64_t> indexes = track.second;
    for (int i = 0; i < indexes.size(); i++)
    {
      int foundIndex = -1;
      for (int j = 0; j < PairedLAPPDTimeStamps[triggerTrack].size(); j++)
      {
        if (PairedLAPPDTimeStamps[triggerTrack][j] == indexes[i])
        {
          foundIndex = j;
          break;
        }
      }
      if (foundIndex != -1)
      {
        removedLAPPD++;
        Log("EBSaver: Remove LAPPD data with LAPPDTime " + std::to_string(indexes[i]) + " at index " + std::to_string(foundIndex), v_message, verbosityEBSaver);
        PairedLAPPDTimeStamps[triggerTrack].erase(PairedLAPPDTimeStamps[triggerTrack].begin() + foundIndex);
        PairedLAPPDTriggerTimestamp[triggerTrack].erase(PairedLAPPDTriggerTimestamp[triggerTrack].begin() + foundIndex);
      }
    }
  }

  // delete PMT related timestamps

  for (auto const &track : PMTPairInfoToRemoveTime)
  {
    int triggerTrack = track.first;
    std::vector<uint64_t> indexes = track.second;
    for (int i = 0; i < indexes.size(); i++)
    {
      int foundIndex = -1;
      for (int j = 0; j < PairedPMTTimeStamps[triggerTrack].size(); j++)
      {
        if (PairedPMTTimeStamps[triggerTrack][j] == indexes[i])
        {
          foundIndex = j;
          break;
        }
      }
      if (foundIndex != -1)
      {
        PairedPMTTimeStamps[triggerTrack].erase(PairedPMTTimeStamps[triggerTrack].begin() + foundIndex);
        PairedPMTTriggerTimestamp[triggerTrack].erase(PairedPMTTriggerTimestamp[triggerTrack].begin() + foundIndex);
      }
    }
  }

  // delete MRD related timestamps
  for (auto const &track : MRDPairInfoToRemoveTime)
  {
    int triggerTrack = track.first;
    std::vector<uint64_t> indexes = track.second;
    for (int i = 0; i < indexes.size(); i++)
    {
      int foundIndex = -1;
      for (int j = 0; j < PairedMRDTimeStamps[triggerTrack].size(); j++)
      {
        if (PairedMRDTimeStamps[triggerTrack][j] == indexes[i])
        {
          foundIndex = j;
          break;
        }
      }
      if (foundIndex != -1)
      {
        PairedMRDTimeStamps[triggerTrack].erase(PairedMRDTimeStamps[triggerTrack].begin() + foundIndex);
        PairedMRDTriggerTimestamp[triggerTrack].erase(PairedMRDTriggerTimestamp[triggerTrack].begin() + foundIndex);
      }
    }
  }

  ANNIEEvent->Close();
  ANNIEEvent->Delete();
  delete ANNIEEvent;
  ANNIEEvent = new BoostStore(false, 2);

  if (saveEverything)
  {
    Log("EBSaver: Save everything", v_message, verbosityEBSaver);
  }

  // now save other data left in the data buffer to orphan
  // loop the runcode buffer of each data store, find which run code we need to save
  // then loop the run code vector, for each run code, loop all data buffers, save the left data with the save run code

  m_data->CStore.Set("GroupedTriggersInTotal", GroupedTriggersInTotal);
  m_data->CStore.Set("RunCodeInTotal", RunCodeInTotal);
  m_data->CStore.Set("PairedLAPPDTriggerTimestamp", PairedLAPPDTriggerTimestamp);
  m_data->CStore.Set("PairedLAPPDTimeStamps", PairedLAPPDTimeStamps);
  m_data->CStore.Set("PairedPMTTriggerTimestamp", PairedPMTTriggerTimestamp);
  m_data->CStore.Set("PairedPMTTimeStamps", PairedPMTTimeStamps);
  m_data->CStore.Set("PairedMRDTriggerTimestamp", PairedMRDTriggerTimestamp);
  m_data->CStore.Set("PairedMRDTimeStamps", PairedMRDTimeStamps);

  Log("EBSaver: Execute finished, saved PMT " + std::to_string(savedPMTHitMapNumber) + ", MRD " + std::to_string(savedMRDNumber) + ", LAPPD " + std::to_string(savedLAPPDNumber) + ", trigger group " + std::to_string(savedTriggerGroupNumber), v_message, verbosityEBSaver);

  Log("EBSaver: Set PMT pairing information buffer PairedPMTTimeStamps size " + std::to_string(PairedPMTTimeStamps.size()), v_message, verbosityEBSaver);
  // print size of each track
  for (auto const &track : PairedPMTTimeStamps)
    Log("EBSaver: track " + std::to_string(track.first) + " size " + std::to_string(track.second.size()), v_message, verbosityEBSaver);
  Log("EBSaver: Set MRD pairing information buffer PairedMRDTimeStamps size " + std::to_string(PairedMRDTimeStamps.size()), v_message, verbosityEBSaver);
  for (auto const &track : PairedMRDTimeStamps)
    Log("EBSaver: track " + std::to_string(track.first) + " size " + std::to_string(track.second.size()), v_message, verbosityEBSaver);
  Log("EBSaver: Set LAPPD pairing information buffer PairedLAPPDTimeStamps size " + std::to_string(PairedLAPPDTimeStamps.size()), v_message, verbosityEBSaver);
  for (auto const &track : PairedLAPPDTimeStamps)
    Log("EBSaver: track " + std::to_string(track.first) + " size " + std::to_string(track.second.size()), v_message, verbosityEBSaver);

  SetDataObjects();
  cout << "\033[1;34m******* EBSaver : Finished *******\033[0m" << endl;

  return true;
}

bool EBSaver::Finalise()
{
  Log("\033[1;34mEBSaver: Finalising\033[0m", v_message, verbosityEBSaver);
  ANNIEEvent->Close();
  ANNIEEvent->Delete();
  delete ANNIEEvent;

  Log("EBSaver: Finished built " + std::to_string(TotalBuiltEventsNumber) + " events", v_message, verbosityEBSaver);
  Log("EBSaver: Finished built " + std::to_string(savedTriggerGroupNumber) + " trigger groups", v_message, verbosityEBSaver);
  Log("EBSaver: Finished built " + std::to_string(savedPMTHitMapNumber) + " PMT hit maps", v_message, verbosityEBSaver);
  Log("EBSaver: Finished built " + std::to_string(savedMRDNumber) + " MRD events", v_message, verbosityEBSaver);
  Log("EBSaver: Finished built " + std::to_string(savedLAPPDNumber) + " LAPPD events", v_message, verbosityEBSaver);
  Log("EBSaver: matched but not built event number:    ***********", v_message, verbosityEBSaver);
  Log("\033[1;34mEBSaver: left PMT events in pairing info buffer " + std::to_string(PairedPMTTimeStamps.size()) + " \033[0m", v_message, verbosityEBSaver);
  // also print the size of each track with track word

  for (auto const &track : PairedPMTTimeStamps)
  {
    Log("EBSaver: track " + std::to_string(track.first) + ", total left trigger number " + std::to_string(track.second.size()), v_message, verbosityEBSaver);
    if (verbosityEBSaver > 9)
    {
      // count how many events not built for each trigger word
      std::map<int, int> unbuiltEvents;
      for (int i = 0; i < track.second.size(); i++)
      {
        if (PairedPMTTimeStamps[track.first][i] != 0)
          unbuiltEvents[PairedPMTTimeStamps[track.first][i]]++;
      }
      for (auto const &event : unbuiltEvents)
        Log("EBSaver: track " + std::to_string(track.first) + ", left trigger " + std::to_string(event.first) + " number " + std::to_string(event.second), v_message, verbosityEBSaver);
    }
  }
  Log("\033[1;34mEBSaver: left MRD events in pairing info buffer " + std::to_string(PairedMRDTimeStamps.size()) + " \033[0m", v_message, verbosityEBSaver);
  for (auto const &track : PairedMRDTimeStamps)
  {
    Log("EBSaver: track " + std::to_string(track.first) + ", left trigger number " + std::to_string(track.second.size()), v_message, verbosityEBSaver);
    if (verbosityEBSaver > 9)
    {
      // count how many events not built for each trigger word
      std::map<int, int> unbuiltEvents;
      for (int i = 0; i < track.second.size(); i++)
      {
        if (PairedMRDTimeStamps[track.first][i] != 0)
          unbuiltEvents[PairedMRDTimeStamps[track.first][i]]++;
      }
      for (auto const &event : unbuiltEvents)
        Log("EBSaver: track " + std::to_string(track.first) + ", left trigger " + std::to_string(event.first) + " number " + std::to_string(event.second), v_message, verbosityEBSaver);
    }
  }
  Log("\033[1;34mEBSaver: left LAPPD events in pairing info buffer " + std::to_string(PairedLAPPDTimeStamps.size()) + " \033[0m", v_message, verbosityEBSaver);
  for (auto const &track : PairedLAPPDTimeStamps)
  {
    Log("EBSaver: track " + std::to_string(track.first) + ", left trigger number " + std::to_string(track.second.size()), v_message, verbosityEBSaver);
    // count how many events not built for each trigger word
    if (verbosityEBSaver > 9)
    {
      std::map<int, int> unbuiltEvents;
      for (int i = 0; i < track.second.size(); i++)
      {
        if (PairedLAPPDTimeStamps[track.first][i] != 0)
          unbuiltEvents[PairedLAPPDTimeStamps[track.first][i]]++;
      }
      for (auto const &event : unbuiltEvents)
        Log("EBSaver: track " + std::to_string(track.first) + ", left trigger " + std::to_string(event.first) + " number " + std::to_string(event.second), v_message, verbosityEBSaver);
    }
  }
  Log("EBSaver: left event number:   ***********", v_message, verbosityEBSaver);
  Log("EBSaver: left PMT events in original data buffer " + std::to_string(InProgressHits->size()), v_message, verbosityEBSaver);
  Log("EBSaver: left MRD events in original data buffer " + std::to_string(MRDEvents.size()), v_message, verbosityEBSaver);
  Log("EBSaver: left LAPPD events in original data buffer " + std::to_string(Buffer_LAPPDTimestamp_ns.size()), v_message, verbosityEBSaver);
  // print the size of each track of

  // add debug print for the left MRD, print TriggerTimeWithoutMRD and PairedMRDTimeStamps to two txt file
  std::ofstream TriggerTimeWithoutMRDFile;
  TriggerTimeWithoutMRDFile.open("EBdebug_TriggerTimeWithoutMRD.txt");
  for (auto const &track : TriggerTimeWithoutMRD)
  {
    TriggerTimeWithoutMRDFile << track.first << " " << track.second << endl;
  }
  TriggerTimeWithoutMRDFile.close();
  std::ofstream PairedMRDTimeStampsFile;
  PairedMRDTimeStampsFile.open("EBdebug_PairedMRDTimeStamps.txt");
  for (auto const &track : PairedMRDTimeStamps)
  {
    for (int i = 0; i < track.second.size(); i++)
    {
      PairedMRDTimeStampsFile << track.first << " " << track.second[i] << endl;
    }
  }

  return true;
}

bool EBSaver::SaveToANNIEEvent(string saveFileName, int runCode, int triggerTrack, int trackIndex)
{
  SaveRunInfo(runCode);
  std::map<std::string, bool> DataStreams;
  DataStreams.emplace("Tank", 0);
  DataStreams.emplace("MRD", 0);
  DataStreams.emplace("CTC", 1);
  DataStreams.emplace("LAPPD", 0);

  SaveGroupedTriggers(triggerTrack, trackIndex);

  bool PMTSaved = false; // incase of multiple 14 found in one group
  bool MRDSaved = false;
  bool LAPPDSaved = false;
  bool beamInfoSaved = false;

  std::map<uint64_t, uint32_t> GroupedTrigger = GroupedTriggersInTotal[triggerTrack][trackIndex];
  // For each element in GroupedTrigger, find is it appear in PairedPMTTriggerTimestamp[triggerTrack],
  // if yes, got the index, access the same index at PairedPMTTimeStamps[triggerTrack], that uint64_t is the PMTTime
  for (auto &trigger : GroupedTrigger)
  {
    uint64_t triggerTime = trigger.first;
    uint32_t triggerType = trigger.second;
    if (triggerType == 14)
    {
      Log("EBSaver: Found undelayed beam trigger with time " + std::to_string(triggerTime) + " in GroupedTrigger", v_debug, verbosityEBSaver);
      if (!beamInfoSaved)
      {
        SaveBeamInfo(triggerTime);
        beamInfoSaved = true;
      }
    }

    if (PairedPMTTriggerTimestamp.find(triggerTrack) != PairedPMTTriggerTimestamp.end())
    {
      for (int i = 0; i < PairedPMTTriggerTimestamp.at(triggerTrack).size(); i++)
      {
        if (PairedPMTTriggerTimestamp.at(triggerTrack).at(i) == triggerTime)
        {
          uint64_t PMTTime = PairedPMTTimeStamps.at(triggerTrack).at(i);
          Log("EBSaver: Found trigger with time " + std::to_string(triggerTime) + " in PairedPMTTriggerTimestamp " + std::to_string(i) + " match with PMTTime " + std::to_string(PMTTime), v_debug, verbosityEBSaver);

          bool saved = SavePMTData(PMTTime);
          PMTSaved = saved;
          DataStreams["Tank"] = 1;
          if (saved)
            PMTPairInfoToRemoveTime[triggerTrack].push_back(PMTTime);
          Log("EBSaver: Saved " + std::to_string(savedPMTHitMapNumber) + " PMT data with PMTTime " + std::to_string(PMTTime), v_debug, verbosityEBSaver);
          // break;
        }
      }
    }

    // check if PairedMRDTriggerTimestamp has element with key = triggerTrack
    if (!MRDSaved)
    {
      Log("Finding trigger track = " + std::to_string(triggerTrack) + " in PairedMRDTriggerTimestamp", v_debug, verbosityEBSaver);
      // print PairedMRDTriggerTimestamp with track number and size
      for (auto const &track : PairedMRDTriggerTimestamp)
      {
        Log("EBSaver: in PairedMRDTriggerTimestamp track " + std::to_string(track.first) + ", left trigger number " + std::to_string(track.second.size()), v_debug, verbosityEBSaver);
      }
      for (auto const &track : PairedMRDTimeStamps)
      {
        Log("EBSaver: in PairedMRDTimeStamps track " + std::to_string(track.first) + ", left trigger number " + std::to_string(track.second.size()), v_debug, verbosityEBSaver);
      }
      if (PairedMRDTriggerTimestamp.find(triggerTrack) != PairedMRDTriggerTimestamp.end())
      {
        if (triggerTime > PairedMRDTriggerTimestamp.at(triggerTrack).at(0) && triggerTime < PairedMRDTriggerTimestamp.at(triggerTrack).at(PairedMRDTriggerTimestamp.at(triggerTrack).size() - 1))
        {
          uint64_t minDiff = 100 * 60 * 1e9;
          Log("Found trigger track = " + std::to_string(triggerTrack) + " in PairedMRDTriggerTimestamp with current searching trigger type = " + std::to_string(triggerType) + " and trigger time = " + std::to_string(triggerTime), v_debug, verbosityEBSaver);
          // print the size of PairedMRDTriggerTimestamp.at(triggerTrack), print the first and the last timestamp
          Log("EBSaver: PairedMRDTriggerTimestamp size " + std::to_string(PairedMRDTriggerTimestamp.at(triggerTrack).size()), v_debug, verbosityEBSaver);
          if (PairedMRDTriggerTimestamp.at(triggerTrack).size() > 0)
            Log("EBSaver: PairedMRDTriggerTimestamp first " + std::to_string(PairedMRDTriggerTimestamp.at(triggerTrack).at(0)) + ", last " + std::to_string(PairedMRDTriggerTimestamp.at(triggerTrack).at(PairedMRDTriggerTimestamp.at(triggerTrack).size() - 1)), v_debug, verbosityEBSaver);

          for (int i = 0; i < PairedMRDTriggerTimestamp.at(triggerTrack).size(); i++)
          {
            uint64_t diff = (PairedMRDTriggerTimestamp.at(triggerTrack).at(i) > triggerTime) ? PairedMRDTriggerTimestamp.at(triggerTrack).at(i) - triggerTime : triggerTime - PairedMRDTriggerTimestamp.at(triggerTrack).at(i);
            if (diff < minDiff)
              minDiff = diff;

            if (PairedMRDTriggerTimestamp.at(triggerTrack).at(i) == triggerTime || (diff < 1e6))
            {
              if (diff < 1e6)
              {
                Log("EBSaver: diff<1e6, is " + std::to_string(diff), v_debug, verbosityEBSaver);
                TriggerTimeWithoutMRD.emplace(PairedMRDTriggerTimestamp.at(triggerTrack).at(i), static_cast<int>(diff));
              }
              uint64_t MRDTime = PairedMRDTimeStamps.at(triggerTrack).at(i);
              bool saved = SaveMRDData(MRDTime);
              MRDSaved = saved;
              DataStreams["MRD"] = 1;
              if (MRDSaved)
                MRDPairInfoToRemoveTime[triggerTrack].push_back(MRDTime);
              Log("EBSaver: Saved " + std::to_string(savedMRDNumber) + " MRD data with MRDTime " + std::to_string(MRDTime), v_debug, verbosityEBSaver);
              Log("EBSaver: Found trigger with time " + std::to_string(triggerTime) + " in PairedMRDTriggerTimestamp " + std::to_string(i) + " match with MRDTime " + std::to_string(MRDTime), v_debug, verbosityEBSaver);
              // break;
              Log("EBSaver: While saving MRD data, use trigger time " + std::to_string(triggerTime) + " with minDiff " + std::to_string(minDiff), v_debug, verbosityEBSaver);
            }
          }
        }
        // Log("EBSaver: MRDData saved", 8, verbosityEBSaver);
      }
    }

    if (PairedLAPPDTriggerTimestamp.find(triggerTrack) != PairedLAPPDTriggerTimestamp.end())
    {
      for (int i = 0; i < PairedLAPPDTriggerTimestamp.at(triggerTrack).size(); i++)
      {
        if (PairedLAPPDTriggerTimestamp.at(triggerTrack).at(i) == triggerTime)
        {
          uint64_t LAPPDTime = PairedLAPPDTimeStamps.at(triggerTrack).at(i);
          bool saved = SaveLAPPDData(LAPPDTime);
          LAPPDSaved = saved;
          DataStreams["LAPPD"] = 1;
          if (LAPPDSaved)
            LAPPDPairInfoToRemoveTime[triggerTrack].push_back(LAPPDTime);
          Log("EBSaver: Saved " + std::to_string(savedLAPPDNumber) + " LAPPD data with LAPPDTime " + std::to_string(LAPPDTime), v_debug, verbosityEBSaver);
          Log("EBSaver: Found trigger with time " + std::to_string(triggerTime) + " in PairedLAPPDTriggerTimestamp " + std::to_string(i) + " match with LAPPDTime " + std::to_string(LAPPDTime), v_debug, verbosityEBSaver);
          // break;
        }
      }
      // Log("EBSaver: LAPPDData saved", 8, verbosityEBSaver);
    }
  }

  ANNIEEvent->Set("DataStreams", DataStreams);

  // if Datastream is not built, save a default empty data for that into the event
  if (DataStreams["Tank"] == 0)
    BuildEmptyPMTData();
  if (DataStreams["MRD"] == 0)
  {
    BuildEmptyMRDData();
    if (triggerTrack == 14)
    {
      // find the time of trigger 8 in current group
      uint64_t triggerTime = 0;
      for (auto &trigger : GroupedTrigger)
      {
        if (trigger.second == 8)
        {
          triggerTime = trigger.first;
          break;
        }
      }
      TriggerTimeWithoutMRD.emplace(triggerTime, triggerTrack);
    }
    else if (triggerTrack == 36)
    {
      // find the time of trigger 8 in current group
      uint64_t triggerTime = 0;
      for (auto &trigger : GroupedTrigger)
      {
        if (trigger.second == 36)
        {
          triggerTime = trigger.first;
          break;
        }
      }
      TriggerTimeWithoutMRD.emplace(triggerTime, triggerTrack);
    }
  }

  if (DataStreams["LAPPD"] == 0)
    BuildEmptyLAPPDData();

  Log("EBSaver: Print ANNIEEvent, Saving to " + saveFileName, v_debug, verbosityEBSaver);
  if (verbosityEBSaver > 8)
    ANNIEEvent->Print(false);
  ANNIEEvent->Save(saveFileName);
  TotalBuiltEventsNumber++;
  ANNIEEvent->Delete();

  return true;
}

bool EBSaver::SaveRunInfo(int runCode)
{
  int RunNumber = runCode / 100000;
  int SubRunNumber = (runCode % 100000) / 10000 - 1;
  int PartFileNumber = runCode % 10000;

  ANNIEEvent->Set("RunNumber", RunNumber);
  ANNIEEvent->Set("SubRunNumber", SubRunNumber);
  ANNIEEvent->Set("PartNumber", PartFileNumber);
  Log("EBSaver: Saving run info with RunNumber " + std::to_string(RunNumber) + " SubRunNumber " + std::to_string(SubRunNumber) + " PartFileNumber " + std::to_string(PartFileNumber), v_debug, verbosityEBSaver);
  return true;
}

bool EBSaver::SaveGroupedTriggers(int triggerTrack, int groupIndex)
{
  Log("EBSaver: Saving grouped triggers for trigger track " + std::to_string(triggerTrack) + " with group index " + std::to_string(groupIndex), v_debug, verbosityEBSaver);
  std::map<uint64_t, uint32_t> GroupedTrigger = GroupedTriggersInTotal[triggerTrack][groupIndex];
  GroupedTriggerIndexToRemove.emplace(triggerTrack, groupIndex);

  ANNIEEvent->Set("GroupedTrigger", GroupedTrigger);
  int matchTriggerType = triggerTrack;
  ANNIEEvent->Set("PrimaryTriggerWord", matchTriggerType);
  if (matchTriggerType != 14)
  {
    ANNIEEvent->Set("TriggerWord", matchTriggerType);
  }
  else
  {
    ANNIEEvent->Set("TriggerWord", 5);
  }

  // find the triggertrack trigger time in GroupedTrigger
  uint64_t primaryTrigTime = 0;
  for (auto &trigger : GroupedTrigger)
  {
    if (trigger.second == triggerTrack)
    {
      primaryTrigTime = trigger.first;
      break;
    }
  }

  ANNIEEvent->Set("PrimaryTriggerTime", primaryTrigTime);
  ANNIEEvent->Set("CTCTimestamp", primaryTrigTime);

  bool NCExtended = false;
  bool CCExtended = false;
  std::string TriggerType = "";
  int CTCWordExtended = 0;
  uint64_t ExtendedTriggerTime = 0;

  if (triggerTrack == BeamTriggerMain)
  {
    TriggerType = "Beam";
    // if found 40 in values of GroupedTrigger, set NCExtended to true
    // put the data at that index to ExtendedTriggerTime
    for (auto &trigger : GroupedTrigger)
    {
      if (trigger.second == 41)
      {
        NCExtended = true;
        ExtendedTriggerTime = trigger.first;
        CTCWordExtended = 1;
        break;
      }
    }
    for (auto &trigger : GroupedTrigger)
    {
      if (trigger.second == 40)
      {
        CCExtended = true;
        ExtendedTriggerTime = trigger.first;
        CTCWordExtended = 2;
        break;
      }
    }
  }
  else if (triggerTrack == LaserTriggerMain)
    TriggerType = "Laser";
  else if (triggerTrack == CosmicTriggerMain)
    TriggerType = "Cosmic";
  else if (triggerTrack == LEDTriggerMain)
    TriggerType = "LED";
  else if (triggerTrack == AmBeTriggerMain)
    TriggerType = "AmBe";
  else if (triggerTrack == PPSMain)
    TriggerType = "PPS";

  ANNIEEvent->Set("NCExtended", NCExtended);
  ANNIEEvent->Set("CCExtended", CCExtended);
  ANNIEEvent->Set("TriggerType", TriggerType);
  ANNIEEvent->Set("TriggerExtended", CTCWordExtended); // 0: normal, 1: NC extended, 2: CC extended
  // Notice, in the V1 eventbuilder, there is a requirement that the matched trigger time -  extended trigger time <5000, if not the extended will be 0
  // we don't have it here because the PMT was not matched to a single trigger, but a group.
  // you may need to select the events based on the time difference in the grouped trigger

  TimeClass TriggerTime(primaryTrigTime);
  TriggerClass TriggerData(TriggerType, matchTriggerType, CTCWordExtended, true, TriggerTime);
  ANNIEEvent->Set("TriggerData", TriggerData);

  savedTriggerGroupNumber++;
  Log("EBSaver: Saved trigger group with trigger type " + TriggerType + " and trigger time " + std::to_string(primaryTrigTime), v_debug, verbosityEBSaver);
  return true;
}

bool EBSaver::SavePMTData(uint64_t PMTTime)
{
  Log("EBSaver: Saving PMT data with PMTTime " + std::to_string(PMTTime), v_debug, verbosityEBSaver);
  // find PMTTime in InProgressHits, if not found, return false
  if (InProgressHits->find(PMTTime) == InProgressHits->end())
  {
    Log("EBSaver: PMT data with PMTTime " + std::to_string(PMTTime) + " not found in InProgressHits", v_debug, verbosityEBSaver);
    return false;
  }
  ANNIEEvent->Set("EventTimeTank", PMTTime);

  std::map<unsigned long, std::vector<Hit>> *PMTHits = InProgressHits->at(PMTTime);
  std::map<unsigned long, std::vector<std::vector<ADCPulse>>> PMTRecoADCHits = InProgressRecoADCHits->at(PMTTime);
  std::map<unsigned long, std::vector<Hit>> *PMTHitsAux = InProgressHitsAux->at(PMTTime);
  std::map<unsigned long, std::vector<std::vector<ADCPulse>>> PMTRecoADCHitsAux = InProgressRecoADCHitsAux->at(PMTTime);
  std::map<unsigned long, std::vector<int>> PMTRawAcqSize = FinishedRawAcqSize->at(PMTTime);

  Log("EBSaver: Got PMT data, saving", v_debug, verbosityEBSaver);

  // check does the ANNIEEvent already have the key "Hits", if yes, print the size of the vector of each key
  // also print the size of the vector of each key in new PMTHits
  // Then Merger the two maps
  std::map<unsigned long, std::vector<Hit>> *OldPMTHits = new std::map<unsigned long, std::vector<Hit>>;
  bool gotHits = ANNIEEvent->Get("Hits", OldPMTHits);
  if (gotHits)
  {
    Log("EBSaver: ANNIEEvent already has key Hits, size " + std::to_string(OldPMTHits->size()), v_debug, verbosityEBSaver);
    for (auto const &time : *OldPMTHits)
      Log("EBSaver: OldPMTHits old time " + std::to_string(time.first) + " size " + std::to_string(time.second.size()), v_debug, verbosityEBSaver);
    for (auto const &time : *PMTHits)
      Log("EBSaver: PMTHits new time " + std::to_string(time.first) + " size " + std::to_string(time.second.size()), v_debug, verbosityEBSaver);
    for (auto const &time : *OldPMTHits)
    {
      if (PMTHits->count(time.first) == 0)
      {
        PMTHits->emplace(time.first, time.second);
      }
      else
      {
        for (auto const &hit : time.second)
        {
          PMTHits->at(time.first).push_back(hit);
        }
      }
    }
    Log("EBSaver: Merged PMT data, PMTHits size " + std::to_string(PMTHits->size()), v_debug, verbosityEBSaver);
  }

  ANNIEEvent->Set("Hits", PMTHits, true);
  ANNIEEvent->Set("RecoADCData", PMTRecoADCHits);
  ANNIEEvent->Set("AuxHits", PMTHitsAux, true);
  ANNIEEvent->Set("RecoAuxADCData", PMTRecoADCHitsAux);
  ANNIEEvent->Set("RawAcqSize", PMTRawAcqSize);

  if (saveRawRWMWaveform)
  {
    // find PMTTime as key in RWMRawWaveforms, if found, save it, if not, save an empty vector
    if (RWMRawWaveforms->find(PMTTime) != RWMRawWaveforms->end() && RWMRawWaveforms->at(PMTTime).size() > 0)
    {
      std::vector<uint16_t> RWMRawWaveform = RWMRawWaveforms->at(PMTTime);
      ANNIEEvent->Set("RWMRawWaveform", RWMRawWaveform);
      Log("EBSaver: Saved RWM data with PMTTime " + std::to_string(PMTTime), v_debug, verbosityEBSaver);
      RWMRawWaveforms->erase(PMTTime);
    }
    else
    {
      std::vector<uint16_t> RWMRawWaveform;
      ANNIEEvent->Set("RWMRawWaveform", RWMRawWaveform);
      Log("EBSaver: Saved empty RWM data with PMTTime " + std::to_string(PMTTime), v_debug, verbosityEBSaver);
    }
  }
  if (saveRawBRFWaveform)
  {
    // find PMTTime as key in BRFRawWaveforms, if found, save it, if not, save an empty vector
    if (BRFRawWaveforms->find(PMTTime) != BRFRawWaveforms->end() && BRFRawWaveforms->at(PMTTime).size() > 0)
    {
      std::vector<uint16_t> BRFRawWaveform = BRFRawWaveforms->at(PMTTime);
      ANNIEEvent->Set("BRFRawWaveform", BRFRawWaveform);
      Log("EBSaver: Saved BRF data with PMTTime " + std::to_string(PMTTime), v_debug, verbosityEBSaver);
      BRFRawWaveforms->erase(PMTTime);
    }
    else
    {
      std::vector<uint16_t> BRFRawWaveform;
      ANNIEEvent->Set("BRFRawWaveform", BRFRawWaveform);
      Log("EBSaver: Saved empty BRF data with PMTTime " + std::to_string(PMTTime), v_debug, verbosityEBSaver);
    }
  }

  savedPMTHitMapNumber++;

  // erase the built data from original data buffer
  InProgressHits->erase(PMTTime);
  InProgressRecoADCHits->erase(PMTTime);
  InProgressHitsAux->erase(PMTTime);
  InProgressRecoADCHitsAux->erase(PMTTime);
  FinishedRawAcqSize->erase(PMTTime);

  Log("EBSaver: Saved PMT data with PMTTime " + std::to_string(PMTTime), v_debug, verbosityEBSaver);
  return true;
}

bool EBSaver::SaveMRDData(uint64_t MRDTime)
{
  Log("EBSaver: Saving MRD data with MRDTime " + std::to_string(MRDTime), v_debug, verbosityEBSaver);
  // find MRDTime in MRDEvents, if not found, return false
  if (MRDEvents.find(MRDTime) == MRDEvents.end())
  {
    Log("EBSaver: not found MRD data with MRDTime " + std::to_string(MRDTime) + " in MRDEvents", v_debug, verbosityEBSaver);
    return false;
  }

  // find the index of the MRDTime in PairedMRDTimeStamps
  std::vector<std::pair<unsigned long, int>> MRDHits = MRDEvents.at(MRDTime);
  std::string MRDTriggerType = MRDEventTriggerTypes.at(MRDTime);
  int beam_tdc = MRDBeamLoopback.at(MRDTime);
  int cosmic_tdc = MRDCosmicLoopback.at(MRDTime);

  std::map<unsigned long, std::vector<Hit>> *TDCData = new std::map<unsigned long, std::vector<Hit>>;

  for (unsigned int i_value = 0; i_value < MRDHits.size(); i_value++)
  {
    unsigned long channelkey = MRDHits.at(i_value).first;
    int hitTimeADC = MRDHits.at(i_value).second;
    double hitTime = 4000. - 4. * static_cast<double>(hitTimeADC);

    if (TDCData->count(channelkey) == 0)
    {
      std::vector<Hit> newhitvector;
      newhitvector.push_back(Hit(0, hitTime, 1.));
      TDCData->emplace(channelkey, newhitvector);
    }
    else
    {
      TDCData->at(channelkey).push_back(Hit(0, hitTime, 1.));
    }
  }

  std::map<std::string, int> mrd_loopback_tdc;
  mrd_loopback_tdc.emplace("BeamLoopbackTDC", beam_tdc);
  mrd_loopback_tdc.emplace("CosmicLoopbackTDC", cosmic_tdc);

  Log("EBSaver: TDCdata size: " + std::to_string(TDCData->size()), v_warning, verbosityEBSaver);

  ANNIEEvent->Set("TDCData", TDCData, true);
  TimeClass t(MRDTime);
  ANNIEEvent->Set("EventTimeMRD", t);
  ANNIEEvent->Set("MRDTriggerType", MRDTriggerType);
  ANNIEEvent->Set("MRDLoopbackTDC", mrd_loopback_tdc);

  savedMRDNumber++;

  // erase the built data from original data buffer
  MRDEvents.erase(MRDTime);
  MRDEventTriggerTypes.erase(MRDTime);
  MRDBeamLoopback.erase(MRDTime);
  MRDCosmicLoopback.erase(MRDTime);

  Log("EBSaver: Saved MRD data with MRDTime " + std::to_string(MRDTime), v_debug, verbosityEBSaver);
  return true;
}

bool EBSaver::SaveLAPPDData(uint64_t LAPPDTime)
{
  Log("EBSaver: Saving LAPPD data with LAPPDTime " + std::to_string(LAPPDTime), v_debug, verbosityEBSaver);
  // find LAPPDTime in Buffer_LAPPDTimestamp_ns, if not found, return false
  if (std::find(Buffer_LAPPDTimestamp_ns.begin(), Buffer_LAPPDTimestamp_ns.end(), LAPPDTime) == Buffer_LAPPDTimestamp_ns.end())
  {
    Log("EBSaver: LAPPD data with LAPPDTime " + std::to_string(LAPPDTime) + " not found in Buffer_LAPPDTimestamp_ns", v_debug, verbosityEBSaver);
    return false;
  }

  // find the index of the LAPPDTime in PairedLAPPDTimeStamps
  int index = -1;
  for (int i = 0; i < Buffer_LAPPDTimestamp_ns.size(); i++)
  {
    if (Buffer_LAPPDTimestamp_ns.at(i) == LAPPDTime)
    {
      index = i;
      break;
    }
  }

  // save the data at index i to ANNIE event, then remove that index from buffer
  if (index != -1)
  {
    std::map<uint64_t, PsecData> LAPPDDataMap;
    std::map<uint64_t, uint64_t> LAPPDBeamgate_ns;
    std::map<uint64_t, uint64_t> LAPPDTimeStamps_ns; // data and key are the same
    std::map<uint64_t, uint64_t> LAPPDTimeStampsRaw;
    std::map<uint64_t, uint64_t> LAPPDBeamgatesRaw;
    std::map<uint64_t, uint64_t> LAPPDOffsets;
    std::map<uint64_t, int> LAPPDTSCorrection;
    std::map<uint64_t, int> LAPPDBGCorrection;
    std::map<uint64_t, int> LAPPDOSInMinusPS;
    std::map<uint64_t, uint64_t> LAPPDBG_PPSBefore;
    std::map<uint64_t, uint64_t> LAPPDBG_PPSAfter;
    std::map<uint64_t, uint64_t> LAPPDBG_PPSDiff;
    std::map<uint64_t, int> LAPPDBG_PPSMissing;
    std::map<uint64_t, uint64_t> LAPPDTS_PPSBefore;
    std::map<uint64_t, uint64_t> LAPPDTS_PPSAfter;
    std::map<uint64_t, uint64_t> LAPPDTS_PPSDiff;
    std::map<uint64_t, int> LAPPDTS_PPSMissing;

    bool gotMap = ANNIEEvent->Get("LAPPDDataMap", LAPPDDataMap);
    bool gotBeamgates_ns = ANNIEEvent->Get("LAPPDBeamgate_ns", LAPPDBeamgate_ns);
    bool gotTimeStamps_ns = ANNIEEvent->Get("LAPPDTimeStamps_ns", LAPPDTimeStamps_ns);
    bool gotTimeStampsRaw = ANNIEEvent->Get("LAPPDTimeStampsRaw", LAPPDTimeStampsRaw);
    bool gotBeamgatesRaw = ANNIEEvent->Get("LAPPDBeamgatesRaw", LAPPDBeamgatesRaw);
    bool gotOffsets = ANNIEEvent->Get("LAPPDOffsets", LAPPDOffsets);
    bool gotTSCorrection = ANNIEEvent->Get("LAPPDTSCorrection", LAPPDTSCorrection);
    bool gotDBGCorrection = ANNIEEvent->Get("LAPPDBGCorrection", LAPPDBGCorrection);
    bool gotOSInMinusPS = ANNIEEvent->Get("LAPPDOSInMinusPS", LAPPDOSInMinusPS);
    bool gotBG_PPSBefore = ANNIEEvent->Get("LAPPDBG_PPSBefore", LAPPDBG_PPSBefore);
    bool gotBG_PPSAfter = ANNIEEvent->Get("LAPPDBG_PPSAfter", LAPPDBG_PPSAfter);
    bool gotBG_PPSDiff = ANNIEEvent->Get("LAPPDBG_PPSDiff", LAPPDBG_PPSDiff);
    bool gotBG_PPSMissing = ANNIEEvent->Get("LAPPDBG_PPSMissing", LAPPDBG_PPSMissing);
    bool gotTS_PPSBefore = ANNIEEvent->Get("LAPPDTS_PPSBefore", LAPPDTS_PPSBefore);
    bool gotTS_PPSAfter = ANNIEEvent->Get("LAPPDTS_PPSAfter", LAPPDTS_PPSAfter);
    bool gotTS_PPSDiff = ANNIEEvent->Get("LAPPDTS_PPSDiff", LAPPDTS_PPSDiff);
    bool gotTS_PPSMissing = ANNIEEvent->Get("LAPPDTS_PPSMissing", LAPPDTS_PPSMissing);

    LAPPDDataMap.emplace(LAPPDTime, Buffer_LAPPDData.at(index));
    LAPPDBeamgate_ns.emplace(LAPPDTime, Buffer_LAPPDBeamgate_ns.at(index));
    LAPPDTimeStamps_ns.emplace(LAPPDTime, Buffer_LAPPDTimestamp_ns.at(index));
    LAPPDTimeStampsRaw.emplace(LAPPDTime, Buffer_LAPPDTimestamp_Raw.at(index));
    LAPPDBeamgatesRaw.emplace(LAPPDTime, Buffer_LAPPDBeamgate_Raw.at(index));
    LAPPDOffsets.emplace(LAPPDTime, Buffer_LAPPDOffset.at(index));
    LAPPDTSCorrection.emplace(LAPPDTime, Buffer_LAPPDTSCorrection.at(index));
    LAPPDBGCorrection.emplace(LAPPDTime, Buffer_LAPPDBGCorrection.at(index));
    LAPPDOSInMinusPS.emplace(LAPPDTime, Buffer_LAPPDOffset_minus_ps.at(index));
    LAPPDBG_PPSBefore.emplace(LAPPDTime, Buffer_LAPPDBG_PPSBefore.at(index));
    LAPPDBG_PPSAfter.emplace(LAPPDTime, Buffer_LAPPDBG_PPSAfter.at(index));
    LAPPDBG_PPSDiff.emplace(LAPPDTime, Buffer_LAPPDBG_PPSDiff.at(index));
    LAPPDBG_PPSMissing.emplace(LAPPDTime, Buffer_LAPPDBG_PPSMissing.at(index));
    LAPPDTS_PPSBefore.emplace(LAPPDTime, Buffer_LAPPDTS_PPSBefore.at(index));
    LAPPDTS_PPSAfter.emplace(LAPPDTime, Buffer_LAPPDTS_PPSAfter.at(index));
    LAPPDTS_PPSDiff.emplace(LAPPDTime, Buffer_LAPPDTS_PPSDiff.at(index));
    LAPPDTS_PPSMissing.emplace(LAPPDTime, Buffer_LAPPDTS_PPSMissing.at(index));

    if (Buffer_LAPPDTS_PPSMissing.at(index) != Buffer_LAPPDBG_PPSMissing.at(index))
    {
      Log("EBSaver: LAPPDTS_PPSMissing is different from LAPPDBG_PPSMissing, LAPPDTS_PPSMissing " + std::to_string(Buffer_LAPPDTS_PPSMissing.at(index)) + " LAPPDBG_PPSMissing " + std::to_string(Buffer_LAPPDBG_PPSMissing.at(index)), v_message, verbosityEBSaver);
    }

    ANNIEEvent->Set("LAPPDDataMap", LAPPDDataMap);
    ANNIEEvent->Set("LAPPDBeamgate_ns", LAPPDBeamgate_ns);
    ANNIEEvent->Set("LAPPDTimeStamps_ns", LAPPDTimeStamps_ns);
    ANNIEEvent->Set("LAPPDTimeStampsRaw", LAPPDTimeStampsRaw);
    ANNIEEvent->Set("LAPPDBeamgatesRaw", LAPPDBeamgatesRaw);
    ANNIEEvent->Set("LAPPDOffsets", LAPPDOffsets);
    ANNIEEvent->Set("LAPPDTSCorrection", LAPPDTSCorrection);
    ANNIEEvent->Set("LAPPDBGCorrection", LAPPDBGCorrection);
    ANNIEEvent->Set("LAPPDOSInMinusPS", LAPPDOSInMinusPS);
    ANNIEEvent->Set("LAPPDBG_PPSBefore", LAPPDBG_PPSBefore);
    ANNIEEvent->Set("LAPPDBG_PPSAfter", LAPPDBG_PPSAfter);
    ANNIEEvent->Set("LAPPDBG_PPSDiff", LAPPDBG_PPSDiff);
    ANNIEEvent->Set("LAPPDBG_PPSMissing", LAPPDBG_PPSMissing);
    ANNIEEvent->Set("LAPPDTS_PPSBefore", LAPPDTS_PPSBefore);
    ANNIEEvent->Set("LAPPDTS_PPSAfter", LAPPDTS_PPSAfter);
    ANNIEEvent->Set("LAPPDTS_PPSDiff", LAPPDTS_PPSDiff);
    ANNIEEvent->Set("LAPPDTS_PPSMissing", LAPPDTS_PPSMissing);

    savedLAPPDNumber++;

    // erase the built data from original data buffer
    Buffer_LAPPDData.erase(Buffer_LAPPDData.begin() + index);
    Buffer_LAPPDBeamgate_ns.erase(Buffer_LAPPDBeamgate_ns.begin() + index);
    Buffer_LAPPDTimestamp_ns.erase(Buffer_LAPPDTimestamp_ns.begin() + index);
    Buffer_LAPPDTimestamp_Raw.erase(Buffer_LAPPDTimestamp_Raw.begin() + index);
    Buffer_LAPPDBeamgate_Raw.erase(Buffer_LAPPDBeamgate_Raw.begin() + index);
    Buffer_LAPPDOffset.erase(Buffer_LAPPDOffset.begin() + index);
    Buffer_LAPPDTSCorrection.erase(Buffer_LAPPDTSCorrection.begin() + index);
    Buffer_LAPPDBGCorrection.erase(Buffer_LAPPDBGCorrection.begin() + index);
    Buffer_LAPPDOffset_minus_ps.erase(Buffer_LAPPDOffset_minus_ps.begin() + index);
    Buffer_LAPPDBG_PPSBefore.erase(Buffer_LAPPDBG_PPSBefore.begin() + index);
    Buffer_LAPPDBG_PPSAfter.erase(Buffer_LAPPDBG_PPSAfter.begin() + index);
    Buffer_LAPPDBG_PPSDiff.erase(Buffer_LAPPDBG_PPSDiff.begin() + index);
    Buffer_LAPPDBG_PPSMissing.erase(Buffer_LAPPDBG_PPSMissing.begin() + index);
    Buffer_LAPPDTS_PPSBefore.erase(Buffer_LAPPDTS_PPSBefore.begin() + index);
    Buffer_LAPPDTS_PPSAfter.erase(Buffer_LAPPDTS_PPSAfter.begin() + index);
    Buffer_LAPPDTS_PPSDiff.erase(Buffer_LAPPDTS_PPSDiff.begin() + index);
    Buffer_LAPPDTS_PPSMissing.erase(Buffer_LAPPDTS_PPSMissing.begin() + index);

    Log("EBSaver: Saved LAPPD data with LAPPDTime " + std::to_string(LAPPDTime), v_debug, verbosityEBSaver);
    return true;
  }
  else
  {
    Log("EBSaver: Failed to find LAPPD data with LAPPDTime " + std::to_string(LAPPDTime), v_debug, verbosityEBSaver);

    return false;
  }
}

bool EBSaver::SaveOrphan(int runCode)
{
  // loop everything in the data buffer, save
  SaveOrphanPMT(runCode);
  SaveOrphanMRD(runCode);
  SaveOrphanLAPPD(runCode);
  SaveOrphanCTC(runCode);
  return true;
}

bool EBSaver::SaveOrphanPMT(int runCode)
{

  return true;
}

bool EBSaver::SaveOrphanMRD(int runCode)
{

  return true;
}

bool EBSaver::SaveOrphanCTC(int runCode)
{

  return true;
}

bool EBSaver::SaveOrphanLAPPD(int runCode)
{

  return true;
}

bool EBSaver::GotAllDataFromOriginalBuffer()
{
  // got PMT data
  bool gotPMTHits = m_data->CStore.Get("InProgressHits", InProgressHits);
  bool gotPMTChkey = m_data->CStore.Get("InProgressChkey", InProgressChkey);
  bool gotIPRecoADCHits = m_data->CStore.Get("InProgressRecoADCHits", InProgressRecoADCHits);
  bool gotIPHitsAux = m_data->CStore.Get("InProgressHitsAux", InProgressHitsAux);
  bool gotIPRADCH = m_data->CStore.Get("InProgressRecoADCHitsAux", InProgressRecoADCHitsAux);
  bool gotFRAS = m_data->CStore.Get("FinishedRawAcqSize", FinishedRawAcqSize); // Filled in PhaseIIADCCalibrator
  bool gotRWM = m_data->CStore.Get("RWMRawWaveforms", RWMRawWaveforms);
  bool gotBRF = m_data->CStore.Get("BRFRawWaveforms", BRFRawWaveforms);

  if (!gotPMTHits || !gotPMTChkey || !gotIPRecoADCHits || !gotIPHitsAux || !gotIPRADCH || !gotFRAS)
  {
    Log("EBSaver: Failed to get some PMT data from buffer", v_message, verbosityEBSaver);
    // print which one was failed
    if (!gotPMTHits)
      Log("EBSaver: Failed to get PMT hits from buffer", v_message, verbosityEBSaver);
    if (!gotPMTChkey)
      Log("EBSaver: Failed to get PMT chkey from buffer", v_message, verbosityEBSaver);
    if (!gotIPRecoADCHits)
      Log("EBSaver: Failed to get PMT recoADCHits from buffer", v_message, verbosityEBSaver);
    if (!gotIPHitsAux)
      Log("EBSaver: Failed to get PMT hits aux from buffer", v_message, verbosityEBSaver);
    if (!gotIPRADCH)
      Log("EBSaver: Failed to get PMT recoADCHits aux from buffer", v_message, verbosityEBSaver);
    if (!gotFRAS)
      Log("EBSaver: Failed to get PMT raw acq size from buffer", v_message, verbosityEBSaver);
  }
  // got PMT match info
  bool gotPairedPMTTriggerTimestamp = m_data->CStore.Get("PairedPMTTriggerTimestamp", PairedPMTTriggerTimestamp);
  bool gotPairedPMTTimeStamps = m_data->CStore.Get("PairedPMTTimeStamps", PairedPMTTimeStamps);
  bool gotPairedPMT_TriggerIndex = m_data->CStore.Get("PairedPMT_TriggerIndex", PairedPMT_TriggerIndex);
  bool gotPMTHitmapRunCode = m_data->CStore.Get("PMTHitmapRunCode", PMTHitmapRunCode);
  if (!gotPairedPMTTriggerTimestamp || !gotPairedPMTTimeStamps || !gotPairedPMT_TriggerIndex || !gotPMTHitmapRunCode)
  {
    Log("EBSaver: Failed to get PMT match info from buffer", v_message, verbosityEBSaver);
    // print which one was failed
    if (!gotPairedPMTTriggerTimestamp)
      Log("EBSaver: Failed to get PairedPMTTriggerTimestamp", v_message, verbosityEBSaver);
    if (!gotPairedPMTTimeStamps)
      Log("EBSaver: Failed to get PairedPMTTimeStamps", v_message, verbosityEBSaver);
    if (!gotPairedPMT_TriggerIndex)
      Log("EBSaver: Failed to get PairedPMT_TriggerIndex", v_message, verbosityEBSaver);
    if (!gotPMTHitmapRunCode)
      Log("EBSaver: Failed to get PMTHitmapRunCode", v_message, verbosityEBSaver);
  }
  //
  // got MRD data
  bool gotMRDEvents = m_data->CStore.Get("MRDEvents", MRDEvents);
  bool gotMRDEventTriggerTypes = m_data->CStore.Get("MRDEventTriggerTypes", MRDEventTriggerTypes);
  bool gotMRDBeamLoopback = m_data->CStore.Get("MRDBeamLoopback", MRDBeamLoopback);
  bool gotMRDCosmicLoopback = m_data->CStore.Get("MRDCosmicLoopback", MRDCosmicLoopback);
  if (!gotMRDEvents || !gotMRDEventTriggerTypes || !gotMRDBeamLoopback || !gotMRDCosmicLoopback)
    Log("EBSaver: Failed to get some MRD data from buffer", v_message, verbosityEBSaver);
  // got MRD match info
  bool gotPairedMRDTriggerTimestamp = m_data->CStore.Get("PairedMRDTriggerTimestamp", PairedMRDTriggerTimestamp);
  bool gotPairedMRDTimeStamps = m_data->CStore.Get("PairedMRDTimeStamps", PairedMRDTimeStamps);
  bool gotPairedMRD_TriggerIndex = m_data->CStore.Get("PairedMRD_TriggerIndex", PairedMRD_TriggerIndex);
  bool gotMRDHitMapRunCode = m_data->CStore.Get("MRDHitMapRunCode", MRDHitMapRunCode);
  if (!gotPairedMRDTriggerTimestamp || !gotPairedMRDTimeStamps || !gotPairedMRD_TriggerIndex || !gotMRDHitMapRunCode)
    Log("EBSaver: Failed to get MRD match info from buffer", v_message, verbosityEBSaver);
  //
  // got LAPPD data
  bool gotBuffer_LAPPDTimestamp_ns = m_data->CStore.Get("Buffer_LAPPDTimestamp_ns", Buffer_LAPPDTimestamp_ns);
  bool gotBuffer_LAPPDData = m_data->CStore.Get("Buffer_LAPPDData", Buffer_LAPPDData);
  bool gotBuffer_LAPPDBeamgate_ns = m_data->CStore.Get("Buffer_LAPPDBeamgate_ns", Buffer_LAPPDBeamgate_ns);
  bool gotBuffer_LAPPDOffset = m_data->CStore.Get("Buffer_LAPPDOffset", Buffer_LAPPDOffset);
  bool gotBuffer_LAPPDBeamgate_Raw = m_data->CStore.Get("Buffer_LAPPDBeamgate_Raw", Buffer_LAPPDBeamgate_Raw);
  bool gotBuffer_LAPPDTimestamp_Raw = m_data->CStore.Get("Buffer_LAPPDTimestamp_Raw", Buffer_LAPPDTimestamp_Raw);
  bool gotBuffer_LAPPDBGCorrection = m_data->CStore.Get("Buffer_LAPPDBGCorrection", Buffer_LAPPDBGCorrection);
  bool gotBuffer_LAPPDTSCorrection = m_data->CStore.Get("Buffer_LAPPDTSCorrection", Buffer_LAPPDTSCorrection);
  bool gotBuffer_LAPPDOffset_minus_ps = m_data->CStore.Get("Buffer_LAPPDOffset_minus_ps", Buffer_LAPPDOffset_minus_ps);
  if (!gotBuffer_LAPPDTimestamp_ns || !gotBuffer_LAPPDData || !gotBuffer_LAPPDBeamgate_ns || !gotBuffer_LAPPDOffset || !gotBuffer_LAPPDBeamgate_Raw || !gotBuffer_LAPPDTimestamp_Raw || !gotBuffer_LAPPDBGCorrection || !gotBuffer_LAPPDTSCorrection || !gotBuffer_LAPPDOffset_minus_ps)
    Log("EBSaver: Failed to get some LAPPD data from buffer", v_message, verbosityEBSaver);
  bool gotBuffer_LAPPDBG_PPSBefore = m_data->CStore.Get("Buffer_LAPPDBG_PPSBefore", Buffer_LAPPDBG_PPSBefore);
  bool gotBuffer_LAPPDBG_PPSAfter = m_data->CStore.Get("Buffer_LAPPDBG_PPSAfter", Buffer_LAPPDBG_PPSAfter);
  bool gotBuffer_LAPPDBG_PPSDiff = m_data->CStore.Get("Buffer_LAPPDBG_PPSDiff", Buffer_LAPPDBG_PPSDiff);
  bool gotBuffer_LAPPDBG_PPSMissing = m_data->CStore.Get("Buffer_LAPPDBG_PPSMissing", Buffer_LAPPDBG_PPSMissing);
  bool gotBuffer_LAPPDTS_PPSBefore = m_data->CStore.Get("Buffer_LAPPDTS_PPSBefore", Buffer_LAPPDTS_PPSBefore);
  bool gotBuffer_LAPPDTS_PPSAfter = m_data->CStore.Get("Buffer_LAPPDTS_PPSAfter", Buffer_LAPPDTS_PPSAfter);
  bool gotBuffer_LAPPDTS_PPSDiff = m_data->CStore.Get("Buffer_LAPPDTS_PPSDiff", Buffer_LAPPDTS_PPSDiff);
  bool gotBuffer_LAPPDTS_PPSMissing = m_data->CStore.Get("Buffer_LAPPDTS_PPSMissing", Buffer_LAPPDTS_PPSMissing);
  if (!gotBuffer_LAPPDBG_PPSBefore || !gotBuffer_LAPPDBG_PPSAfter || !gotBuffer_LAPPDBG_PPSDiff || !gotBuffer_LAPPDBG_PPSMissing || !gotBuffer_LAPPDTS_PPSBefore || !gotBuffer_LAPPDTS_PPSAfter || !gotBuffer_LAPPDTS_PPSDiff || !gotBuffer_LAPPDTS_PPSMissing)
    Log("EBSaver: Failed to get LAPPD PPS data from buffer", v_message, verbosityEBSaver);

  // got LAPPD match info
  bool gotPairedLAPPDTriggerTimestamp = m_data->CStore.Get("PairedLAPPDTriggerTimestamp", PairedLAPPDTriggerTimestamp);
  bool gotPairedLAPPDTimeStamps = m_data->CStore.Get("PairedLAPPDTimeStamps", PairedLAPPDTimeStamps);
  bool gotPairedLAPPD_TriggerIndex = m_data->CStore.Get("PairedLAPPD_TriggerIndex", PairedLAPPD_TriggerIndex);
  bool gotLAPPDRunCode = m_data->CStore.Get("Buffer_LAPPDRunCode", Buffer_LAPPDRunCode);
  if (!gotPairedLAPPDTriggerTimestamp || !gotPairedLAPPDTimeStamps || !gotPairedLAPPD_TriggerIndex || !gotLAPPDRunCode)
  {
    Log("EBSaver: Failed to get LAPPD match info from buffer", v_message, verbosityEBSaver);
    // print which one was failed
    if (!gotPairedLAPPDTriggerTimestamp)
      Log("EBSaver: Failed to get PairedLAPPDTriggerTimestamp", v_message, verbosityEBSaver);
    if (!gotPairedLAPPDTimeStamps)
      Log("EBSaver: Failed to get PairedLAPPDTimeStamps", v_message, verbosityEBSaver);
    if (!gotPairedLAPPD_TriggerIndex)
      Log("EBSaver: Failed to get PairedLAPPD_TriggerIndex", v_message, verbosityEBSaver);
    if (!gotLAPPDRunCode)
      Log("EBSaver: Failed to get LAPPDRunCode", v_message, verbosityEBSaver);
  }

  Log("EBSaver: got PMT pairing information buffer PairedPMTTimeStamps size " + std::to_string(PairedPMTTimeStamps.size()), v_message, verbosityEBSaver);
  Log("EBSaver: got MRD pairing information buffer PairedMRDTimeStamps size " + std::to_string(PairedMRDTimeStamps.size()), v_message, verbosityEBSaver);
  for (auto const &track : PairedMRDTimeStamps)
  {
    int triggerTrack = track.first;
    std::vector<uint64_t> times = track.second;
    Log("EBSaver: PairedMRDTimeStamps at track " + std::to_string(triggerTrack) + " size " + std::to_string(times.size()), v_message, verbosityEBSaver);
    for (int i = 0; i < times.size(); i++)
      cout << i << ": " << times[i] << ", ";
  }
  cout << endl;

  Log("EBSaver: got LAPPD pairing information buffer PairedLAPPDTimeStamps size " + std::to_string(PairedLAPPDTimeStamps.size()), v_message, verbosityEBSaver);
  for (auto const &track : PairedLAPPDTimeStamps)
  {
    int triggerTrack = track.first;
    std::vector<uint64_t> times = track.second;
    Log("EBSaver: PairedLAPPDTimeStamps at track " + std::to_string(triggerTrack) + " size " + std::to_string(times.size()), v_message, verbosityEBSaver);
    for (int i = 0; i < times.size(); i++)
      cout << i << ": " << times[i] << ", ";
  }
  cout << endl;

  // print PairedLAPPD_TriggerIndex
  Log("EBSaver: got LAPPD pairing information buffer PairedLAPPD_TriggerIndex size " + std::to_string(PairedLAPPD_TriggerIndex.size()), v_message, verbosityEBSaver);
  for (auto const &track : PairedLAPPD_TriggerIndex)
  {
    int triggerTrack = track.first;
    std::vector<int> indexes = track.second;
    Log("EBSaver: PairedLAPPD_TriggerIndex at track " + std::to_string(triggerTrack) + " size " + std::to_string(indexes.size()), v_message, verbosityEBSaver);
    for (int i = 0; i < indexes.size(); i++)
      cout << i << ": " << indexes[i] << ", ";
  }
  cout << endl;

  // print Buffer_LAPPDBeamgate_ns
  Log("EBSaver: got LAPPD pairing information buffer Buffer_LAPPDBeamgate_ns size " + std::to_string(Buffer_LAPPDBeamgate_ns.size()), v_message, verbosityEBSaver);
  for (int i = 0; i < Buffer_LAPPDBeamgate_ns.size(); i++)
    cout << i << ": " << Buffer_LAPPDBeamgate_ns[i] << ", ";
  cout << endl;

  return true;
}

void EBSaver::SetDataObjects()
{
  // after erase those data, set them back to CStore
  // set PMT data
  m_data->CStore.Set("InProgressHits", InProgressHits);
  m_data->CStore.Set("InProgressChkey", InProgressChkey);
  m_data->CStore.Set("InProgressRecoADCHits", InProgressRecoADCHits);
  m_data->CStore.Set("InProgressHitsAux", InProgressHitsAux);
  m_data->CStore.Set("InProgressRecoADCHitsAux", InProgressRecoADCHitsAux);
  m_data->CStore.Set("FinishedRawAcqSize", FinishedRawAcqSize);
  m_data->CStore.Set("RWMRawWaveforms", RWMRawWaveforms);
  m_data->CStore.Set("BRFRawWaveforms", BRFRawWaveforms);
  // set PMT match info
  m_data->CStore.Set("PairedPMTTriggerTimestamp", PairedPMTTriggerTimestamp);
  m_data->CStore.Set("PairedPMTTimeStamps", PairedPMTTimeStamps);
  m_data->CStore.Set("PairedPMT_TriggerIndex", PairedPMT_TriggerIndex);
  m_data->CStore.Set("PMTHitmapRunCode", PMTHitmapRunCode);
  // set MRD data
  m_data->CStore.Set("MRDEvents", MRDEvents);
  m_data->CStore.Set("MRDEventTriggerTypes", MRDEventTriggerTypes);
  m_data->CStore.Set("MRDBeamLoopback", MRDBeamLoopback);
  m_data->CStore.Set("MRDCosmicLoopback", MRDCosmicLoopback);
  // set MRD match info
  m_data->CStore.Set("PairedMRDTriggerTimestamp", PairedMRDTriggerTimestamp);
  m_data->CStore.Set("PairedMRDTimeStamps", PairedMRDTimeStamps);
  m_data->CStore.Set("PairedMRD_TriggerIndex", PairedMRD_TriggerIndex);
  m_data->CStore.Set("MRDHitMapRunCode", MRDHitMapRunCode);
  // set LAPPD data
  m_data->CStore.Set("Buffer_LAPPDTimestamp_ns", Buffer_LAPPDTimestamp_ns);
  m_data->CStore.Set("Buffer_LAPPDData", Buffer_LAPPDData);
  m_data->CStore.Set("Buffer_LAPPDBeamgate_ns", Buffer_LAPPDBeamgate_ns);
  m_data->CStore.Set("Buffer_LAPPDOffset", Buffer_LAPPDOffset);
  m_data->CStore.Set("Buffer_LAPPDBeamgate_Raw", Buffer_LAPPDBeamgate_Raw);
  m_data->CStore.Set("Buffer_LAPPDTimestamp_Raw", Buffer_LAPPDTimestamp_Raw);
  m_data->CStore.Set("Buffer_LAPPDBGCorrection", Buffer_LAPPDBGCorrection);
  m_data->CStore.Set("Buffer_LAPPDTSCorrection", Buffer_LAPPDTSCorrection);
  m_data->CStore.Set("Buffer_LAPPDOffset_minus_ps", Buffer_LAPPDOffset_minus_ps);
  m_data->CStore.Set("Buffer_LAPPDBG_PPSBefore", Buffer_LAPPDBG_PPSBefore);
  m_data->CStore.Set("Buffer_LAPPDBG_PPSAfter", Buffer_LAPPDBG_PPSAfter);
  m_data->CStore.Set("Buffer_LAPPDBG_PPSDiff", Buffer_LAPPDBG_PPSDiff);
  m_data->CStore.Set("Buffer_LAPPDBG_PPSMissing", Buffer_LAPPDBG_PPSMissing);
  m_data->CStore.Set("Buffer_LAPPDTS_PPSBefore", Buffer_LAPPDTS_PPSBefore);
  m_data->CStore.Set("Buffer_LAPPDTS_PPSAfter", Buffer_LAPPDTS_PPSAfter);
  m_data->CStore.Set("Buffer_LAPPDTS_PPSDiff", Buffer_LAPPDTS_PPSDiff);
  m_data->CStore.Set("Buffer_LAPPDTS_PPSMissing", Buffer_LAPPDTS_PPSMissing);
  // set LAPPD match info
  m_data->CStore.Set("PairedLAPPDTriggerTimestamp", PairedLAPPDTriggerTimestamp);
  m_data->CStore.Set("PairedLAPPDTimeStamps", PairedLAPPDTimeStamps);
  m_data->CStore.Set("PairedLAPPD_TriggerIndex", PairedLAPPD_TriggerIndex);
  m_data->CStore.Set("Buffer_LAPPDRunCode", Buffer_LAPPDRunCode);
}

void EBSaver::BuildEmptyPMTData()
{
  std::map<unsigned long, std::vector<Hit>> *PMTHits = new std::map<unsigned long, std::vector<Hit>>;
  std::map<unsigned long, std::vector<std::vector<ADCPulse>>> PMTRecoADCHits;
  std::map<unsigned long, std::vector<Hit>> *PMTHitsAux = new std::map<unsigned long, std::vector<Hit>>;
  std::map<unsigned long, std::vector<std::vector<ADCPulse>>> PMTRecoADCHitsAux;
  std::map<unsigned long, std::vector<int>> PMTRawAcqSize;

  ANNIEEvent->Set("Hits", PMTHits, true);
  ANNIEEvent->Set("RecoADCData", PMTRecoADCHits);
  ANNIEEvent->Set("AuxHits", PMTHitsAux, true);
  ANNIEEvent->Set("RecoAuxADCData", PMTRecoADCHitsAux);
  ANNIEEvent->Set("RawAcqSize", PMTRawAcqSize);
}

void EBSaver::BuildEmptyMRDData()
{
  std::map<unsigned long, std::vector<Hit>> *TDCData = new std::map<unsigned long, std::vector<Hit>>;
  std::string MRDTriggerType = "";
  std::map<std::string, int> mrd_loopback_tdc;

  ANNIEEvent->Set("TDCData", TDCData, true);
  TimeClass t(0);
  ANNIEEvent->Set("EventTimeMRD", t);
  ANNIEEvent->Set("MRDTriggerType", MRDTriggerType);
  ANNIEEvent->Set("MRDLoopbackTDC", mrd_loopback_tdc);
}

void EBSaver::BuildEmptyLAPPDData()
{
  std::map<uint64_t, PsecData> LAPPDDataMap;
  std::map<uint64_t, uint64_t> LAPPDBeamgate_ns;
  std::map<uint64_t, uint64_t> LAPPDTimeStamps_ns; // data and key are the same
  std::map<uint64_t, uint64_t> LAPPDTimeStampsRaw;
  std::map<uint64_t, uint64_t> LAPPDBeamgatesRaw;
  std::map<uint64_t, uint64_t> LAPPDOffsets;
  std::map<uint64_t, int> LAPPDTSCorrection;
  std::map<uint64_t, int> LAPPDBGCorrection;
  std::map<uint64_t, int> LAPPDOSInMinusPS;
  std::map<uint64_t, uint64_t> LAPPDBG_PPSBefore;
  std::map<uint64_t, uint64_t> LAPPDBG_PPSAfter;
  std::map<uint64_t, uint64_t> LAPPDBG_PPSDiff;
  std::map<uint64_t, int> LAPPDBG_PPSMissing;
  std::map<uint64_t, uint64_t> LAPPDTS_PPSBefore;
  std::map<uint64_t, uint64_t> LAPPDTS_PPSAfter;
  std::map<uint64_t, uint64_t> LAPPDTS_PPSDiff;
  std::map<uint64_t, int> LAPPDTS_PPSMissing;

  ANNIEEvent->Set("LAPPDDataMap", LAPPDDataMap);
  ANNIEEvent->Set("LAPPDBeamgate_ns", LAPPDBeamgate_ns);
  ANNIEEvent->Set("LAPPDTimeStamps_ns", LAPPDTimeStamps_ns);
  ANNIEEvent->Set("LAPPDTimeStampsRaw", LAPPDTimeStampsRaw);
  ANNIEEvent->Set("LAPPDBeamgatesRaw", LAPPDBeamgatesRaw);
  ANNIEEvent->Set("LAPPDOffsets", LAPPDOffsets);
  ANNIEEvent->Set("LAPPDTSCorrection", LAPPDTSCorrection);
  ANNIEEvent->Set("LAPPDBGCorrection", LAPPDBGCorrection);
  ANNIEEvent->Set("LAPPDOSInMinusPS", LAPPDOSInMinusPS);
  ANNIEEvent->Set("LAPPDBG_PPSBefore", LAPPDBG_PPSBefore);
  ANNIEEvent->Set("LAPPDBG_PPSAfter", LAPPDBG_PPSAfter);
  ANNIEEvent->Set("LAPPDBG_PPSDiff", LAPPDBG_PPSDiff);
  ANNIEEvent->Set("LAPPDBG_PPSMissing", LAPPDBG_PPSMissing);
  ANNIEEvent->Set("LAPPDTS_PPSBefore", LAPPDTS_PPSBefore);
  ANNIEEvent->Set("LAPPDTS_PPSAfter", LAPPDTS_PPSAfter);
  ANNIEEvent->Set("LAPPDTS_PPSDiff", LAPPDTS_PPSDiff);
  ANNIEEvent->Set("LAPPDTS_PPSMissing", LAPPDTS_PPSMissing);
}

void EBSaver::LoadBeamInfo()
{
  TFile *file = new TFile(beamInfoFileName.c_str(), "READ");
  TTree *tree;
  file->GetObject("BeamTree", tree);

  if (!tree)
  {
    cout << "EBSaver: Failed to load beam info from file with name: " << beamInfoFileName << endl;
    return;
  }

  // copy from IFBeamDBInterfaceV2.cpp
  //  Here's some documentation for some of the parameters stored in the beam
  // database. It's taken from the MicroBooNE operations wiki:
  // http://tinyurl.com/z3c4mxs
  //
  // The status page shows the present reading of beamline instrumentation. All
  // of this data is being stored to IF beam Database. The "IF Beam DB
  // dashboard":http://dbweb4.fnal.gov:8080/ifbeam/app/BNBDash/index provides
  // another view of beam data. Some of it is redundant to the status page, but
  // it verifies that the data is being stored in the database. At present the
  // page shows following devices:
  //   * TOR860, TOR875 - two toroids in BNB measuring beam intensity. TOR860 is
  //     at the beginning of the beamline, and TOR875 is at the end.
  //   * THCURR - horn current
  //   * HWTOUT - horn water temperature coming out of the horn.
  //   * BTJT2 - target temperature
  //   * HP875, VP875 - beam horizontal and vertical positions at the 875
  //     location, about 4.5 m upstream of the target center.
  //   * HPTG1, VPTG1 - beam horizontal and vertical positions immediately
  //     (about 2.5 m) upstream of the target center.
  //   * HPTG2, VPTG2 - beam horizontal and vertical positions more immediately
  //     (about 1.5 m) upstream of the target center.
  //   * Because there are no optics between H/VP875 and H/VPTG2, the movements
  //     on these monitors should scale with the difference in distances.
  //   * BTJT2 - target air cooling temperature. Four RTD measure the return
  //     temperature of the cooling air. This is the one closest to the target.
  //   * BTH2T2 - target air cooling temperature. This is the temperature of the
  //     air going into the horn.

  // For bunch rotation, the information can be found here:
  // https://beamdocs.fnal.gov/AD/DocDB/0069/006904/008/20181214BunchRotation.pdf
  // The B_BRRMP has unit of volt, others are not clear.

  // additionally, the unit of E_TOR860 and E_TOR875 is E12

  uint64_t timestamp;
  double E_TOR860, E_TOR875, THCURR, BTJT2, HP875, VP875, HPTG1, VPTG1, HPTG2, VPTG2, BTH2T2;
  double B_BRRMPL, B_BRRMPQ, B_BRRMPS, B_BRRMP;

  tree->SetBranchAddress("Timestamp", &timestamp);
  tree->SetBranchAddress("E_TOR860", &E_TOR860);
  tree->SetBranchAddress("E_TOR875", &E_TOR875);
  tree->SetBranchAddress("E_THCURR", &THCURR);
  tree->SetBranchAddress("E_BTJT2", &BTJT2);
  tree->SetBranchAddress("E_HP875", &HP875);
  tree->SetBranchAddress("E_VP875", &VP875);
  tree->SetBranchAddress("E_HPTG1", &HPTG1);
  tree->SetBranchAddress("E_VPTG1", &VPTG1);
  tree->SetBranchAddress("E_HPTG2", &HPTG2);
  tree->SetBranchAddress("E_VPTG2", &VPTG2);
  tree->SetBranchAddress("E_BTH2T2", &BTH2T2);
  tree->SetBranchAddress("B_BRRMPL", &B_BRRMPL);
  tree->SetBranchAddress("B_BRRMPQ", &B_BRRMPQ);
  tree->SetBranchAddress("B_BRRMPS", &B_BRRMPS);
  tree->SetBranchAddress("B_BRRMP", &B_BRRMP);

  Long64_t nentries = tree->GetEntries();
  Log("EBSaver: Loading beam infor, total entries in beam info file: " + std::to_string(nentries), v_message, verbosityEBSaver);
  for (Long64_t i = 0; i < nentries; ++i)
  {
    tree->GetEntry(i);
    if (i % (static_cast<int>(nentries / 10)) == 0)
      Log("EBSaver: Loading beam info, processed " + std::to_string(i) + " entries", v_message, verbosityEBSaver);
    BeamInfoTimestamps.push_back(timestamp);
    E_TOR860_map.emplace(timestamp, E_TOR860);
    E_TOR875_map.emplace(timestamp, E_TOR875);
    THCURR_map.emplace(timestamp, THCURR);
    BTJT2_map.emplace(timestamp, BTJT2);
    HP875_map.emplace(timestamp, HP875);
    VP875_map.emplace(timestamp, VP875);
    HPTG1_map.emplace(timestamp, HPTG1);
    VPTG1_map.emplace(timestamp, VPTG1);
    HPTG2_map.emplace(timestamp, HPTG2);
    VPTG2_map.emplace(timestamp, VPTG2);
    BTH2T2_map.emplace(timestamp, BTH2T2);
    B_BRRMPL_map.emplace(timestamp, B_BRRMPL);
    B_BRRMPQ_map.emplace(timestamp, B_BRRMPQ);
    B_BRRMPS_map.emplace(timestamp, B_BRRMPS);
    B_BRRMP_map.emplace(timestamp, B_BRRMP);
  }

  Log("EBSaver: Loaded number of E_TOR860 entries: " + std::to_string(E_TOR860_map.size()), v_message, verbosityEBSaver);

  Log("EBSaver: Finished loading beam info from " + beamInfoFileName, v_message, verbosityEBSaver);

  return;
}

bool EBSaver::SaveBeamInfo(uint64_t TriggerTime)
{
  double E_TOR860, E_TOR875, THCURR, BTJT2, HP875, VP875, HPTG1, VPTG1, HPTG2, VPTG2, BTH2T2;
  double B_BRRMPL, B_BRRMPQ, B_BRRMPS, B_BRRMP;
  // find the closest timestamp in vector<uint64_t> BeamInfoTimestamps

  uint64_t closestTimestamp = 0;
  double minDiff = 60 * 60 * 1e9; // 60 minutes
  int minIndex = -1;

  for (int i = 0; i < BeamInfoTimestamps.size(); i++)
  {
    uint64_t timestamp = BeamInfoTimestamps.at(i) * 1e6;
    double diff = (timestamp > TriggerTime) ? timestamp - TriggerTime : TriggerTime - timestamp;
    if (i == 0)
      Log("EBSaver: First beam info timestamp is " + std::to_string(timestamp) + " with diff = " + std::to_string(diff), v_warning, verbosityEBSaver);
    else if (i == BeamInfoTimestamps.size() - 1)
      Log("EBSaver: Last beam info timestamp is " + std::to_string(timestamp) + " with diff = " + std::to_string(diff), v_warning, verbosityEBSaver);

    if (diff < minDiff)
    {
      minDiff = diff;
      minIndex = i;
    }
  }

  if (minIndex == -1)
  {
    Log("EBSaver: Failed to find the closest beam info timestamp to trigger time", v_message, verbosityEBSaver);

    uint64_t beamInfoTime_long = 0;
    int64_t timeDiff = -9999;
    double defaultVal = -9999.;
    int beam_good = 0;
    int bunch_rotation_on = 0;
    ANNIEEvent->Set("BeamInfoTime", beamInfoTime_long);
    ANNIEEvent->Set("BeamInfoTimeToTriggerDiff", timeDiff);
    ANNIEEvent->Set("beam_E_TOR860", defaultVal);
    ANNIEEvent->Set("beam_E_TOR875", defaultVal);
    ANNIEEvent->Set("beam_THCURR", defaultVal);
    ANNIEEvent->Set("beam_BTJT2", defaultVal);
    ANNIEEvent->Set("beam_HP875", defaultVal);
    ANNIEEvent->Set("beam_VP875", defaultVal);
    ANNIEEvent->Set("beam_HPTG1", defaultVal);
    ANNIEEvent->Set("beam_VPTG1", defaultVal);
    ANNIEEvent->Set("beam_HPTG2", defaultVal);
    ANNIEEvent->Set("beam_VPTG2", defaultVal);
    ANNIEEvent->Set("beam_BTH2T2", defaultVal);
    ANNIEEvent->Set("beam_B_BRRMPL", defaultVal);
    ANNIEEvent->Set("beam_B_BRRMPQ", defaultVal);
    ANNIEEvent->Set("beam_B_BRRMPS", defaultVal);
    ANNIEEvent->Set("beam_B_BRRMP", defaultVal);
    ANNIEEvent->Set("beam_good", beam_good);
    ANNIEEvent->Set("bunch_rotation_on", bunch_rotation_on);

    Log("EBSaver: Saved beam info with time " + std::to_string(0) + ", pot E_TOR860 = " + std::to_string(-9999) + ", beam_good = " + std::to_string(-9999), v_message, verbosityEBSaver);
  }
  else
  {
    Log("Found the closest beam info timestamp to trigger time at index " + std::to_string(minIndex) + " with time " + std::to_string(BeamInfoTimestamps.at(minIndex)) + " and dt = " + std::to_string(minDiff), v_message, verbosityEBSaver);

    // get the precise difference
    uint64_t beamInfoTime = BeamInfoTimestamps.at(minIndex);
    ULong64_t beamInfoTime_long = beamInfoTime * 1e6;
    int64_t timeDiff = beamInfoTime_long - TriggerTime;

    // check if the map has the key
    E_TOR860 = E_TOR860_map.at(beamInfoTime);
    E_TOR875 = E_TOR875_map.at(beamInfoTime);
    THCURR = THCURR_map.at(beamInfoTime);
    BTJT2 = BTJT2_map.at(beamInfoTime);
    HP875 = HP875_map.at(beamInfoTime);
    VP875 = VP875_map.at(beamInfoTime);
    HPTG1 = HPTG1_map.at(beamInfoTime);
    VPTG1 = VPTG1_map.at(beamInfoTime);
    HPTG2 = HPTG2_map.at(beamInfoTime);
    VPTG2 = VPTG2_map.at(beamInfoTime);
    BTH2T2 = BTH2T2_map.at(beamInfoTime);
    B_BRRMPL = B_BRRMPL_map.at(beamInfoTime);
    B_BRRMPQ = B_BRRMPQ_map.at(beamInfoTime);
    B_BRRMPS = B_BRRMPS_map.at(beamInfoTime);
    B_BRRMP = B_BRRMP_map.at(beamInfoTime);

    ANNIEEvent->Set("BeamInfoTime", beamInfoTime);
    ANNIEEvent->Set("BeamInfoTimeToTriggerDiff", timeDiff);

    ANNIEEvent->Set("beam_E_TOR860", E_TOR860);
    ANNIEEvent->Set("beam_E_TOR875", E_TOR875);
    ANNIEEvent->Set("beam_THCURR", THCURR);
    ANNIEEvent->Set("beam_BTJT2", BTJT2);
    ANNIEEvent->Set("beam_HP875", HP875);
    ANNIEEvent->Set("beam_VP875", VP875);
    ANNIEEvent->Set("beam_HPTG1", HPTG1);
    ANNIEEvent->Set("beam_VPTG1", VPTG1);
    ANNIEEvent->Set("beam_HPTG2", HPTG2);
    ANNIEEvent->Set("beam_VPTG2", VPTG2);
    ANNIEEvent->Set("beam_BTH2T2", BTH2T2);
    ANNIEEvent->Set("beam_B_BRRMPL", B_BRRMPL);
    ANNIEEvent->Set("beam_B_BRRMPQ", B_BRRMPQ);
    ANNIEEvent->Set("beam_B_BRRMPS", B_BRRMPS);
    ANNIEEvent->Set("beam_B_BRRMP", B_BRRMP);

    int beam_good = 0;
    
    // the upper limit is increased from 176 to 178 to include earlier runs before 2022 summer shutdown 
    // current plot can be fround at docdb 6475, thanks to Steven Doran
    // https://annie-docdb.fnal.gov/cgi-bin/sso/ShowDocument?docid=6475
    // so, the beam_ok for beam cluster trees before v1.2 requires current >172 and <176. 
    // the files processed after are using this new upper limit
    if (THCURR > 172 && THCURR < 178)
    {
      if (E_TOR860 > 0.5 && E_TOR860 < 8 && E_TOR875 > 0.5 && E_TOR875 < 8 && (E_TOR875 - E_TOR860) / E_TOR860 < 0.05)
        beam_good = 1;
    }

    ANNIEEvent->Set("beam_good", beam_good);

    int bunch_rotation_on = 0;
    if (B_BRRMP > 0)
    {
      bunch_rotation_on = 1;
    }
    ANNIEEvent->Set("bunch_rotation_on", bunch_rotation_on);

    Log("EBSaver: Saved beam info with time " + std::to_string(beamInfoTime) + ", pot E_TOR860 = " + std::to_string(E_TOR860) + ", beam_good = " + std::to_string(beam_good), v_message, verbosityEBSaver);
  }

  return true;
}
