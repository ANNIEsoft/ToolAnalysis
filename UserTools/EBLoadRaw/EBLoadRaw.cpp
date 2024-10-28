#include "EBLoadRaw.h"

EBLoadRaw::EBLoadRaw() : Tool() {}

bool EBLoadRaw::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if (configfile != "")
    m_variables.Initialise(configfile); // loading config file
  // m_variables.Print();

  m_data = &data; // assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  m_variables.Get("verbosityEBLoadRaw", verbosityEBLoadRaw);

  ReadTriggerOverlap = false;
  m_variables.Get("ReadTriggerOverlap", ReadTriggerOverlap);
  m_variables.Get("InputFile", InputFile);
  OrganizedFileList = OrganizeRunParts(InputFile);

  LoadCTC = false;
  m_variables.Get("LoadCTC", LoadCTC);
  LoadPMT = false;
  m_variables.Get("LoadPMT", LoadPMT);
  LoadMRD = false;
  m_variables.Get("LoadMRD", LoadMRD);
  LoadLAPPD = false;
  m_variables.Get("LoadLAPPD", LoadLAPPD);

  FileCompleted = false;
  JumpBecauseLAPPD = false;
  ProcessingComplete = false;
  PMTPaused = false;
  MRDPaused = false;
  LAPPDPaused = false;
  CTCPaused = false;
  usingTriggerOverlap = false;
  LoadingFileNumber = 0;

  RunNumber = 0;
  SubRunNumber = 0;
  PartFileNumber = 0;

  CTCEntryNum = 0;
  PMTEntryNum = 0;
  MRDEntryNum = 0;
  LAPPDEntryNum = 0;

  PMTTotalEntries = 0;
  MRDTotalEntries = 0;
  LAPPDTotalEntries = 0;
  CTCTotalEntries = 0;
  PMTEntriesCompleted = false;
  MRDEntriesCompleted = false;
  LAPPDEntriesCompleted = false;
  CTCEntriesCompleted = false;
  LoadedPMTTotalEntries = 0;
  LoadedMRDTotalEntries = 0;
  LoadedLAPPDTotalEntries = 0;
  LoadedCTCTotalEntries = 0;

  PMTMatchingForced = false;
  MRDMatchingForced = false;
  LAPPDMatchingForced = false;

  RawData = new BoostStore(false, 0);
  PMTData = new BoostStore(false, 2);
  MRDData = new BoostStore(false, 2);
  LAPPDData = new BoostStore(false, 2);
  CTCData = new BoostStore(false, 2);
  CData = new std::vector<CardData>;
  TData = new TriggerData;
  MData = new MRDOut;
  LData = new PsecData;

  m_data->CStore.Set("FileProcessingComplete", false);
  RunCodeToSave = 0;

  return true;
}

bool EBLoadRaw::Execute()
{
  m_data->CStore.Set("NewRawDataEntryAccessed", false);
  m_data->CStore.Set("NewRawDataFileAccessed", false);
  m_data->CStore.Set("SaveProcessedFile", false);

  ProcessingComplete = false;
  if (FileCompleted)
  {
    if (CurrentFile != "NONE")
    {
      RunCodeToSave = RunCode(CurrentFile);
      m_data->CStore.Set("RunCodeToSave", RunCodeToSave);
      m_data->CStore.Set("SaveProcessedFile", true);
      Log("EBLoadRaw: File completed, saving file " + CurrentFile + " with RunCode: " + std::to_string(RunCodeToSave), v_message, verbosityEBLoadRaw);
    }
    ProcessingComplete = LoadNewFile();
  }

  if (ProcessingComplete)
  {
    RunCodeToSave = RunCode(CurrentFile);
    m_data->CStore.Set("RunCodeToSave", RunCodeToSave);
    m_data->CStore.Set("SaveProcessedFile", true);
    m_data->CStore.Set("SaveEverything", true);
    Log("EBLoadRaw: File completed, saving file " + CurrentFile + " with RunCode: " + std::to_string(RunCodeToSave), v_message, verbosityEBLoadRaw);

    m_data->CStore.Set("FileProcessingComplete", true);
    m_data->vars.Set("StopLoop", 1);
    Log("EBLoadRaw: All files have been processed, set pause flags, PMT: " + std::to_string(PMTPaused) + ", MRD: " + std::to_string(MRDPaused) + ", CTC: " + std::to_string(CTCPaused) + ", LAPPD: " + std::to_string(LAPPDPaused), v_message, verbosityEBLoadRaw);
    return true;
  }

  m_data->CStore.Get("PauseTankDecoding", PMTPaused);
  m_data->CStore.Get("PauseMRDDecoding", MRDPaused);
  m_data->CStore.Get("PauseCTCDecoding", CTCPaused);
  m_data->CStore.Get("PauseLAPPDDecoding", LAPPDPaused);

  if (OrganizedFileList.size() == 0)
  {
    Log("EBLoadRaw: No files to process.", v_warning, verbosityEBLoadRaw);
    m_data->vars.Set("StopLoop", 1);
    return true;
  }
  if (FileCompleted || CurrentFile == "NONE")
  {
    Log("EBLoadRaw: Loading new file. " + OrganizedFileList.at(LoadingFileNumber), v_message, verbosityEBLoadRaw);
    CurrentFile = OrganizedFileList.at(LoadingFileNumber);
    RawData->Initialise(CurrentFile.c_str());
    Log("EBLoadRaw: File loaded.", v_message, verbosityEBLoadRaw);
    m_data->CStore.Set("NewRawDataFileAccessed", true);
    if (verbosityEBLoadRaw > 4)
      RawData->Print(false);
    LoadRunInfo();
    LoadPMTData();
    LoadMRDData();
    LoadCTCData();
    LoadLAPPDData();

    PMTMatchingForced = false;
    MRDMatchingForced = false;
    LAPPDMatchingForced = false;
  }
  else
  {
    Log("EBLoadRaw: Loading next entry of current file " + CurrentFile, v_message, verbosityEBLoadRaw);
  }

  FileCompleted = false;
  if (JumpBecauseLAPPD)
  {
    FileCompleted = true;
    JumpBecauseLAPPD = false;
    Log("EBLoadRaw: Jumping to next file due to LAPPD data.", v_message, verbosityEBLoadRaw);
    return true;
  }

  // if more MRD events than VME PMT events, jump to next file
  // this is an old option, why?
  if (MRDTotalEntries > PMTTotalEntries)
  {
    // FileCompleted = true;
    Log("EBLoadRaw: Jumping to next file due to MRD entry is more than PMT entry.", v_message, verbosityEBLoadRaw);
    // return true;
  }

  if (LoadPMT && PMTEntryNum == PMTTotalEntries)
  {
    Log("EBLoadRaw: ALL PMT entries Loaded.", v_message, verbosityEBLoadRaw);
    PMTEntriesCompleted = true;
    PMTPaused = true;
  }
  if (LoadMRD && MRDEntryNum == MRDTotalEntries)
  {
    Log("EBLoadRaw: ALL MRD entries Loaded.", v_message, verbosityEBLoadRaw);
    MRDEntriesCompleted = true;
    MRDPaused = true;
  }
  if (LoadCTC && CTCEntryNum == CTCTotalEntries)
  {
    Log("EBLoadRaw: ALL CTC entries Loaded.", v_message, verbosityEBLoadRaw);
    CTCEntriesCompleted = true;
    CTCPaused = true;
  }
  if (LoadLAPPD && LAPPDEntryNum == LAPPDTotalEntries)
  {
    Log("EBLoadRaw: ALL LAPPD entries Loaded.", v_message, verbosityEBLoadRaw);
    LAPPDEntriesCompleted = true;
    LAPPDPaused = true;
  }

  if (LoadLAPPD && LAPPDTotalEntries < 0)
    LAPPDEntriesCompleted = true;

  m_data->CStore.Set("PauseTankDecoding", PMTPaused);
  m_data->CStore.Set("PauseMRDDecoding", MRDPaused);
  m_data->CStore.Set("PauseCTCDecoding", CTCPaused);
  m_data->CStore.Set("PauseLAPPDDecoding", LAPPDPaused);

  Log("EBLoadRaw: Set pause flags, PMT: " + std::to_string(PMTPaused) + ", MRD: " + std::to_string(MRDPaused) + ", CTC: " + std::to_string(CTCPaused) + ", LAPPD: " + std::to_string(LAPPDPaused), v_message, verbosityEBLoadRaw);

  if (LoadPMT && !PMTPaused && !PMTEntriesCompleted)
    LoadNextPMTData();
  if (LoadMRD && !MRDPaused && !MRDEntriesCompleted)
    LoadNextMRDData();
  if (LoadCTC && !CTCPaused && !CTCEntriesCompleted)
    LoadNextCTCData();
  if (LoadLAPPD && !LAPPDPaused && !LAPPDEntriesCompleted)
    LoadNextLAPPDData();

  // if all required data is loaded, set filecompleted flag to true
  if ((!LoadPMT || PMTEntriesCompleted) && (!LoadMRD || MRDEntriesCompleted) && (!LoadCTC || CTCEntriesCompleted) && (!LoadLAPPD || LAPPDEntriesCompleted))
  {
    FileCompleted = true;
    Log("EBLoadRaw: All data loaded.", v_message, verbosityEBLoadRaw);
  }

  if (verbosityEBLoadRaw > v_message)
  {
    std::cout << "**************************************************EBLoadRaw: Current progress after execute: " << std::endl;
    std::cout << "EBLoadRaw: Current file: " << CurrentFile << std::endl;
    if (LoadPMT)
      std::cout << "EBLoadRaw: PMT entries: " << PMTEntryNum << " / " << PMTTotalEntries << " = " << static_cast<double>(PMTEntryNum) / static_cast<double>(PMTTotalEntries) * 100 << "%" << std::endl;
    if (LoadMRD)
      std::cout << "EBLoadRaw: MRD entries: " << MRDEntryNum << " / " << MRDTotalEntries << " = " << static_cast<double>(MRDEntryNum) / static_cast<double>(MRDTotalEntries) * 100 << "%" << std::endl;
    if (LoadCTC)
      std::cout << "EBLoadRaw: CTC entries: " << CTCEntryNum << " / " << CTCTotalEntries << " = " << static_cast<double>(CTCEntryNum) / static_cast<double>(CTCTotalEntries) * 100 << "%" << std::endl;
    if (LoadLAPPD)
      std::cout << "EBLoadRaw: LAPPD entries: " << LAPPDEntryNum << " / " << LAPPDTotalEntries << " = " << static_cast<double>(LAPPDEntryNum) / static_cast<double>(LAPPDTotalEntries) * 100 << "%" << std::endl;
    std::cout << "**********************************************************************************************" << std::endl;
  }

  m_data->CStore.Set("MRDEntriesCompleted", MRDEntriesCompleted);
  m_data->CStore.Set("PMTEntriesCompleted", PMTEntriesCompleted);
  m_data->CStore.Set("CTCEntriesCompleted", CTCEntriesCompleted);
  m_data->CStore.Set("LAPPDEntriesCompleted", LAPPDEntriesCompleted);
  Log("EBLoadRaw: Set entries completed flags, PMT: " + std::to_string(PMTEntriesCompleted) + ", MRD: " + std::to_string(MRDEntriesCompleted) + ", CTC: " + std::to_string(CTCEntriesCompleted) + ", LAPPD: " + std::to_string(LAPPDEntriesCompleted), v_message, verbosityEBLoadRaw);

  m_data->CStore.Set("NewRawDataEntryAccessed", true);
  m_data->CStore.Set("FileCompleted", FileCompleted);

  Log("EBLoadRaw: Finished execution loop.", v_message, verbosityEBLoadRaw);
  return true;
}

bool EBLoadRaw::Finalise()
{
  RawData->Close();
  RawData->Delete();
  delete RawData;
  if (LoadPMT)
  {
    PMTData->Close();
    PMTData->Delete();
    delete PMTData;
  }
  if (LoadMRD)
  {
    MRDData->Close();
    MRDData->Delete();
    delete MRDData;
  }
  if (LoadLAPPD)
  {
    LAPPDData->Close();
    LAPPDData->Delete();
    delete LAPPDData;
  }
  if (LoadCTC)
  {
    CTCData->Close();
    CTCData->Delete();
    delete CTCData;
  }

  std::cout << "\033[1;34mEBLoadRaw: Finalising EBLoadRaw\033[0m" << std::endl;
  std::cout << "EBLoadRaw: Loaded " << OrganizedFileList.size() << " files "
            << " from " << OrganizedFileList.at(0) << " to " << OrganizedFileList.at(OrganizedFileList.size() - 1) << std::endl;
  std::cout << "EBLoadRaw: Loaded " << LoadedPMTTotalEntries << " PMT entries. " << std::endl;
  std::cout << "EBLoadRaw: Loaded " << LoadedMRDTotalEntries << " MRD entries. " << std::endl;
  std::cout << "EBLoadRaw: Loaded " << LoadedCTCTotalEntries << " CTC entries. " << std::endl;
  std::cout << "EBLoadRaw: Loaded " << LoadedLAPPDTotalEntries << " LAPPD entries. " << std::endl;

  return true;
}

std::vector<std::string> EBLoadRaw::OrganizeRunParts(std::string FileList)
{
  std::vector<std::string> OrganizedFiles;
  std::vector<std::string> UnorganizedFileList;
  std::vector<int> RunCodes;
  int ThisRunCode;
  // First, parse the lines and get all files.
  std::string line;
  ifstream myfile(FileList.c_str());
  if (myfile.is_open())
  {
    std::cout << "Lines in FileList being printed" << std::endl; // has our stuff;
    while (getline(myfile, line))
    {
      if (line.find("#") != std::string::npos)
        continue;
      std::string filename = line;
      int RunCodeNumber = RunCode(filename);

      if (RunCodeNumber != -9999)
      {
        UnorganizedFileList.push_back(filename);
        RunCodes.push_back(RunCodeNumber);
      }

    } // End parsing each line in file

    // Now, organize files based on the part number array
    std::vector<std::pair<int, std::string>> SortingVector;
    for (int i = 0; i < (int)UnorganizedFileList.size(); i++)
    {
      SortingVector.push_back(std::make_pair(RunCodes.at(i), UnorganizedFileList.at(i)));
    }
    std::sort(SortingVector.begin(), SortingVector.end());
    for (int j = 0; j < (int)SortingVector.size(); j++)
    {
      OrganizedFiles.push_back(SortingVector.at(j).second);
    }
  }
  // print the OrganizedFiles
  for (int i = 0; i < (int)OrganizedFiles.size(); i++)
  {
    std::cout << OrganizedFiles.at(i) << std::endl;
  }

  return OrganizedFiles;
}

int EBLoadRaw::RunCode(string fileName)
{
  // extract run number and file number from filename
  std::regex runNumber_regex("RAWDataR(\\d{4})");
  std::regex subrunNumber_regex("S(\\d{1,4})p");
  std::regex rawFileNumber_regex("p(\\d{1,4})$");
  std::smatch match;
  int runNumber = -9999;
  int subrunNumber = -9999;
  int rawFileNumber = -9999;
  bool allmatched = false;
  if (std::regex_search(fileName, match, runNumber_regex) && match.size() > 1)
  {
    runNumber = std::stoi(match.str(1));
    if (verbosityEBLoadRaw > 0)
      std::cout << "runNumber: " << runNumber << std::endl;
    m_data->CStore.Set("runNumber", runNumber);
    allmatched = true;
  }
  else
  {
    std::cout << "runNumber not found" << std::endl;
    m_data->CStore.Set("rawFileNumber", -9999);
  }

  if (std::regex_search(fileName, match, subrunNumber_regex) && match.size() > 1)
  {
    subrunNumber = std::stoi(match.str(1));
    if (verbosityEBLoadRaw > 0)
      std::cout << "subrunNumber: " << subrunNumber << std::endl;
    m_data->CStore.Set("subrunNumber", subrunNumber);
    allmatched = true;
  }
  else
  {
    std::cout << "subrunNumber not found" << std::endl;
    m_data->CStore.Set("subrunNumber", -9999);
  }

  if (std::regex_search(fileName, match, rawFileNumber_regex) && match.size() > 1)
  {
    rawFileNumber = std::stoi(match.str(1));
    if (verbosityEBLoadRaw > 0)
      std::cout << "rawFileNumber: " << rawFileNumber << std::endl;
    m_data->CStore.Set("rawFileNumber", rawFileNumber);
    allmatched = true;
  }
  else
  {
    std::cout << "rawFileNumber not found" << std::endl;
    m_data->CStore.Set("runNumber", -9999);
  }

  if (allmatched == true)
  {
    int runcode = runNumber * 100000 + ((subrunNumber + 1) * 10000) + rawFileNumber;
    cout << "EBLoadRaw: RunCode: " << runcode << endl;
    return runcode;
  }
  else
  {
    return -9999;
  }
}

bool EBLoadRaw::LoadRunInfo()
{
  int runCode = RunCode(CurrentFile);
  Log("EBLoadRaw: Loading run information, RunCode: " + std::to_string(runCode), v_message, verbosityEBLoadRaw);

  RunNumber = runCode / 100000;
  SubRunNumber = (runCode % 100000) / 10000 - 1;
  PartFileNumber = runCode % 10000;

  Store Postgress;

  std::cout << "EBLoadRaw: loading run information, RunNumber: " << RunNumber << ", SubRunNumber: " << SubRunNumber << ", PartFileNumber: " << PartFileNumber << std::endl;

  Postgress.Set("RunNumber", RunNumber);
  Postgress.Set("SubRunNumber", SubRunNumber);
  Postgress.Set("PartFileNumber", PartFileNumber);
  Postgress.Set("RunType", -1);
  Postgress.Set("StartTime", -1);

  m_data->CStore.Set("RunInfoPostgress", Postgress);

  m_data->CStore.Set("RunNumber", RunNumber);
  m_data->CStore.Set("SubRunNumber", SubRunNumber);
  m_data->CStore.Set("PartFileNumber", PartFileNumber);
  m_data->CStore.Set("RunCode", runCode);

  return true;
}

// load new file and it's related boost store
bool EBLoadRaw::LoadNewFile()
{
  bool EndOfProcessing = false;
  LoadingFileNumber++;

  RawData->Close();
  RawData->Delete();
  delete RawData;
  RawData = new BoostStore(false, 0);
  PMTData->Close();
  PMTData->Delete();
  delete PMTData;
  PMTData = new BoostStore(false, 2);
  MRDData->Close();
  MRDData->Delete();
  delete MRDData;
  MRDData = new BoostStore(false, 2);
  LAPPDData->Close();
  LAPPDData->Delete();
  delete LAPPDData;
  LAPPDData = new BoostStore(false, 2);
  CTCData->Close();
  CTCData->Delete();
  delete CTCData;
  CTCData = new BoostStore(false, 2);

  PMTEntryNum = 0;
  MRDEntryNum = 0;
  LAPPDEntryNum = 0;
  CTCEntryNum = 0;

  PMTEntriesCompleted = false;
  MRDEntriesCompleted = false;
  LAPPDEntriesCompleted = false;
  CTCEntriesCompleted = false;

  m_data->CStore.Set("PauseTankDecoding", false);
  m_data->CStore.Set("PauseMRDDecoding", false);
  m_data->CStore.Set("PauseCTCDecoding", false);
  m_data->CStore.Set("PauseLAPPDDecoding", false);

  if (LoadingFileNumber == OrganizedFileList.size())
  {
    EndOfProcessing = true;
    m_data->CStore.Set("PauseTankDecoding", true);
    m_data->CStore.Set("PauseMRDDecoding", true);
    m_data->CStore.Set("PauseCTCDecoding", true);
    m_data->CStore.Set("PauseLAPPDDecoding", true);
  }

  return EndOfProcessing;
}

bool EBLoadRaw::LoadPMTData()
{
  Log("EBLoadRaw: Loading PMTData.", v_message, verbosityEBLoadRaw);
  RawData->Get("PMTData", *PMTData);
  PMTData->Header->Get("TotalEntries", PMTTotalEntries);
  LoadedPMTTotalEntries += PMTTotalEntries;
  Log("EBLoadRaw: PMTData loaded, TotalEntries: " + std::to_string(PMTTotalEntries), v_message, verbosityEBLoadRaw);

  if (verbosityEBLoadRaw > 3)
    PMTData->Print(false);
  if (verbosityEBLoadRaw > 3)
    PMTData->Header->Print(false);
  return true;
}

bool EBLoadRaw::LoadMRDData()
{
  Log("EBLoadRaw: Loading MRDData.", v_message, verbosityEBLoadRaw);
  RawData->Get("CCData", *MRDData);
  MRDData->Header->Get("TotalEntries", MRDTotalEntries);
  LoadedMRDTotalEntries += MRDTotalEntries;
  Log("EBLoadRaw: MRDData loaded, TotalEntries: " + std::to_string(MRDTotalEntries), v_message, verbosityEBLoadRaw);
  if (verbosityEBLoadRaw > 3)
    MRDData->Print(false);
  return true;
}

bool EBLoadRaw::LoadCTCData()
{
  Log("EBLoadRaw: Loading CTCData.", v_message, verbosityEBLoadRaw);
  RawData->Get("TrigData", *CTCData);
  if (verbosityEBLoadRaw > 3)
    CTCData->Print(false);
  CTCData->Header->Get("TotalEntries", CTCTotalEntries);
  LoadedCTCTotalEntries += CTCTotalEntries;
  if (verbosityEBLoadRaw > 3)
    CTCData->Header->Print(false);
  if (ReadTriggerOverlap)
  {
    std::stringstream ss_trigoverlap;
    ss_trigoverlap << "TrigOverlap_R" << RunNumber << "S" << SubRunNumber << "p" << PartFileNumber;
    std::cout << "EBLoadRaw: Loading Trigger Overlap data: " << ss_trigoverlap.str() << std::endl;
    BoostStore TrigOverlapStore;
    bool store_exist = TrigOverlapStore.Initialise(ss_trigoverlap.str().c_str());
    std::cout << "EBLoadRaw: Trigger Overlap store exist: " << store_exist << std::endl;
    if (store_exist)
    {
      CTCTotalEntries++;
      std::cout << "EBLoadRaw: total trigger entry with overlap is: " << CTCTotalEntries << std::endl;
    }
  }
  return true;
}

bool EBLoadRaw::LoadLAPPDData()
{
  Log("EBLoadRaw: Loading LAPPDData.", v_message, verbosityEBLoadRaw);
  try
  {
    RawData->Get("LAPPDData", *LAPPDData);
    LAPPDData->Header->Get("TotalEntries", LAPPDTotalEntries);
    Log("EBLoadRaw: LAPPDData loaded, TotalEntries: " + std::to_string(LAPPDTotalEntries), v_message, verbosityEBLoadRaw);
    if (verbosityEBLoadRaw > 3)
    {
      LAPPDData->Print(false);
      LAPPDData->Header->Print(false);
    }
    if (LAPPDTotalEntries < 0)
    {

      cout << "EBLoadRaw: LAPPDData entry < 0, found " << LAPPDTotalEntries << ", set to 0" << endl;
      LAPPDTotalEntries = 0;
      LAPPDEntriesCompleted = true;
    }
    else if (LAPPDTotalEntries > 100000)
    {
      cout << "EBLoadRaw: LAPPDData entry very large, found " << LAPPDTotalEntries << ", return and set jump because LAPPD = true" << endl;
      JumpBecauseLAPPD = true;
      return true;
    }
    LoadedLAPPDTotalEntries += LAPPDTotalEntries;
  }
  catch (std::exception &e)
  {
    std::cout << "EBLoadRaw: LAPPDData not found in file." << std::endl;
    LAPPDTotalEntries = 0;
    LAPPDEntriesCompleted = true;
  }
  Log("EBLoadRaw: LAPPDData has " + std::to_string(LAPPDTotalEntries) + " entries.", v_message, verbosityEBLoadRaw);
  return true;
}

// load next entry of the current file
bool EBLoadRaw::LoadNextPMTData()
{
  Log("EBLoadRaw: Loading next PMTData entry " + std::to_string(PMTEntryNum) + " of " + std::to_string(PMTTotalEntries), v_warning, verbosityEBLoadRaw);
  PMTData->GetEntry(PMTEntryNum);
  Log("EBLoadRaw: Getting the PMT card data entry", v_warning, verbosityEBLoadRaw);
  PMTData->Get("CardData", *CData);
  Log("EBLoadRaw: Setting into CStore", v_warning, verbosityEBLoadRaw);
  m_data->CStore.Set("CardData", CData);
  Log("EBLoadRaw: Setting PMT entry num to CStore", v_warning, verbosityEBLoadRaw);
  m_data->CStore.Set("TankEntryNum", PMTEntryNum);
  PMTEntryNum++;

  if (PMTEntryNum == PMTTotalEntries && !PMTMatchingForced)
  {
    // force the PMT matching when all PMT entries are completed or PMTEntryNum is greater than PMTTotalEntries
    Log("EBLoadRaw: PMTEntriesCompleted, force PMT matching", v_message, verbosityEBLoadRaw);
    bool ForcePMTMatching = true;
    m_data->CStore.Set("ForcePMTMatching", ForcePMTMatching);
    PMTMatchingForced = true;
  }
  else
  {
    bool ForcePMTMatching = false;
    m_data->CStore.Set("ForcePMTMatching", ForcePMTMatching);
  }

  return true;
}

bool EBLoadRaw::LoadNextMRDData()
{
  Log("EBLoadRaw: Loading next MRDData entry " + std::to_string(MRDEntryNum) + " of " + std::to_string(MRDTotalEntries), v_warning, verbosityEBLoadRaw);
  MRDData->GetEntry(MRDEntryNum);
  MRDData->Get("Data", *MData);
  m_data->CStore.Set("MRDData", MData, true);
  m_data->CStore.Set("MRDEntryNum", MRDEntryNum);
  MRDEntryNum++;

  if (MRDEntryNum == MRDTotalEntries && !MRDMatchingForced)
  {
    Log("EBLoadRaw: MRDEntriesCompleted, force MRD matching", v_message, verbosityEBLoadRaw);
    bool ForceMRDMatching = true;
    m_data->CStore.Set("ForceMRDMatching", ForceMRDMatching);
    MRDMatchingForced = true;
  }
  else
  {
    bool ForceMRDMatching = false;
    m_data->CStore.Set("ForceMRDMatching", ForceMRDMatching);
  }

  return true;
}

bool EBLoadRaw::LoadNextLAPPDData()
{
  Log("EBLoadRaw: Loading next LAPPDData entry " + std::to_string(LAPPDEntryNum) + " of " + std::to_string(LAPPDTotalEntries), v_warning, verbosityEBLoadRaw);
  LAPPDData->GetEntry(LAPPDEntryNum);
  LAPPDData->Get("LAPPDData", *LData);
  m_data->CStore.Set("LAPPDData", LData);
  m_data->CStore.Set("LAPPDEntryNum", LAPPDEntryNum);
  m_data->CStore.Set("LAPPDanaData", true);
  LAPPDEntryNum++;

  if (LAPPDEntryNum == LAPPDTotalEntries && !LAPPDMatchingForced)
  {
    Log("EBLoadRaw: LAPPDEntriesCompleted, force LAPPD matching", v_message, verbosityEBLoadRaw);
    bool ForceLAPPDMatching = true;
    m_data->CStore.Set("ForceLAPPDMatching", ForceLAPPDMatching);
    LAPPDMatchingForced = true;
  }
  else
  {
    bool ForceLAPPDMatching = false;
    m_data->CStore.Set("ForceLAPPDMatching", ForceLAPPDMatching);
  }

  return true;
}

bool EBLoadRaw::LoadNextCTCData()
{
  Log("EBLoadRaw: Loading next CTCData entry " + std::to_string(CTCEntryNum) + " of " + std::to_string(CTCTotalEntries), v_warning, verbosityEBLoadRaw);
  if (!ReadTriggerOverlap)
  {
    CTCData->GetEntry(CTCEntryNum);
    CTCData->Get("TrigData", *TData);
    Log("EBLoadRaw: Loaded CTCData entry " + std::to_string(CTCEntryNum), v_warning, verbosityEBLoadRaw);
  }
  else
  {
    if (CTCEntryNum != CTCTotalEntries - 1)
    {
      CTCData->GetEntry(CTCEntryNum);
      CTCData->Get("TrigData", *TData);
      m_data->CStore.Set("usingTriggerOverlap", false);
      Log("EBLoadRaw: Loaded CTCData entry " + std::to_string(CTCEntryNum), v_warning, verbosityEBLoadRaw);
    }
    else
    {
      BoostStore TrigOverlapStore;
      std::stringstream ss_trigoverlap;
      ss_trigoverlap << "TrigOverlap_R" << RunNumber << "S" << SubRunNumber << "p" << PartFileNumber;
      bool got_trig_o = TrigOverlapStore.Initialise(ss_trigoverlap.str().c_str());
      if (got_trig_o)
      {
        TrigOverlapStore.Get("TrigData", *TData);
        m_data->CStore.Set("usingTriggerOverlap", true);
      }
      else
        std::cout << "EBLoadRaw: Trigger Overlap data not found while loading" << std::endl;
    }
    m_data->CStore.Set("TrigData", TData);

    Log("EBLoadRaw: Loaded CTCData entry " + std::to_string(CTCEntryNum), v_warning, verbosityEBLoadRaw);
    CTCEntryNum++;
  }
  return true;
}
