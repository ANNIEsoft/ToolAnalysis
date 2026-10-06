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
  
  // Shared buffers
  m_data->CStore.Get("Buffer_LAPPDData", Buffer_LAPPDData); 
  m_data->CStore.Get("Buffer_LAPPDRunCode", Buffer_RunCode);

  // --- Board 0 Buffers Get ---
  m_data->CStore.Get("Buffer_LAPPDTimestamp_ns_0", Buffer_LAPPDTimestamp_ns_0);
  m_data->CStore.Get("Buffer_LAPPDBeamgate_ns_0", Buffer_LAPPDBeamgate_ns_0);
  m_data->CStore.Get("Buffer_LAPPDOffset_0", Buffer_LAPPDOffset_0);
  m_data->CStore.Get("Buffer_LAPPDBeamgate_Raw_0", Buffer_LAPPDBeamgate_Raw_0);
  m_data->CStore.Get("Buffer_LAPPDTimestamp_Raw_0", Buffer_LAPPDTimestamp_Raw_0);
  m_data->CStore.Get("Buffer_LAPPDBGCorrection_0", Buffer_LAPPDBGCorrection_0);
  m_data->CStore.Get("Buffer_LAPPDTSCorrection_0", Buffer_LAPPDTSCorrection_0);
  m_data->CStore.Get("Buffer_LAPPDOffset_minus_ps_0", Buffer_LAPPDOffset_minus_ps_0);
  
  m_data->CStore.Get("Buffer_LAPPDBG_PPSBefore_0", Buffer_LAPPDBG_PPSBefore_0);
  m_data->CStore.Get("Buffer_LAPPDBG_PPSAfter_0", Buffer_LAPPDBG_PPSAfter_0);
  m_data->CStore.Get("Buffer_LAPPDBG_PPSDiff_0", Buffer_LAPPDBG_PPSDiff_0);
  m_data->CStore.Get("Buffer_LAPPDBG_PPSMissing_0", Buffer_LAPPDBG_PPSMissing_0);
  m_data->CStore.Get("Buffer_LAPPDTS_PPSBefore_0", Buffer_LAPPDTS_PPSBefore_0);
  m_data->CStore.Get("Buffer_LAPPDTS_PPSAfter_0", Buffer_LAPPDTS_PPSAfter_0);
  m_data->CStore.Get("Buffer_LAPPDTS_PPSDiff_0", Buffer_LAPPDTS_PPSDiff_0);
  m_data->CStore.Get("Buffer_LAPPDTS_PPSMissing_0", Buffer_LAPPDTS_PPSMissing_0);

  // --- Board 1 Buffers Get ---
  m_data->CStore.Get("Buffer_LAPPDTimestamp_ns_1", Buffer_LAPPDTimestamp_ns_1);
  m_data->CStore.Get("Buffer_LAPPDBeamgate_ns_1", Buffer_LAPPDBeamgate_ns_1);
  m_data->CStore.Get("Buffer_LAPPDOffset_1", Buffer_LAPPDOffset_1);
  m_data->CStore.Get("Buffer_LAPPDBeamgate_Raw_1", Buffer_LAPPDBeamgate_Raw_1);
  m_data->CStore.Get("Buffer_LAPPDTimestamp_Raw_1", Buffer_LAPPDTimestamp_Raw_1);
  m_data->CStore.Get("Buffer_LAPPDBGCorrection_1", Buffer_LAPPDBGCorrection_1);
  m_data->CStore.Get("Buffer_LAPPDTSCorrection_1", Buffer_LAPPDTSCorrection_1);
  m_data->CStore.Get("Buffer_LAPPDOffset_minus_ps_1", Buffer_LAPPDOffset_minus_ps_1);
  
  m_data->CStore.Get("Buffer_LAPPDBG_PPSBefore_1", Buffer_LAPPDBG_PPSBefore_1);
  m_data->CStore.Get("Buffer_LAPPDBG_PPSAfter_1", Buffer_LAPPDBG_PPSAfter_1);
  m_data->CStore.Get("Buffer_LAPPDBG_PPSDiff_1", Buffer_LAPPDBG_PPSDiff_1);
  m_data->CStore.Get("Buffer_LAPPDBG_PPSMissing_1", Buffer_LAPPDBG_PPSMissing_1);
  m_data->CStore.Get("Buffer_LAPPDTS_PPSBefore_1", Buffer_LAPPDTS_PPSBefore_1);
  m_data->CStore.Get("Buffer_LAPPDTS_PPSAfter_1", Buffer_LAPPDTS_PPSAfter_1);
  m_data->CStore.Get("Buffer_LAPPDTS_PPSDiff_1", Buffer_LAPPDTS_PPSDiff_1);
  m_data->CStore.Get("Buffer_LAPPDTS_PPSMissing_1", Buffer_LAPPDTS_PPSMissing_1);

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
    Log("EBLAPPD: Matching reason is stopLoop = " + std::to_string(stopLoop) + ", runNum = " + std::to_string(runNum) 
        + ", exeNum = " + std::to_string(exeNum) + ", ForceLAPPDMatching = " + std::to_string(ForceLAPPDMatching), v_message, verbosityEBLAPPD);
    
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

  // Set Shared Buffers
  m_data->CStore.Set("Buffer_LAPPDData", Buffer_LAPPDData);
  m_data->CStore.Set("Buffer_LAPPDRunCode", Buffer_RunCode);

  // --- Board 0 Buffers Set ---
  m_data->CStore.Set("Buffer_LAPPDTimestamp_ns_0", Buffer_LAPPDTimestamp_ns_0);
  m_data->CStore.Set("Buffer_LAPPDBeamgate_ns_0", Buffer_LAPPDBeamgate_ns_0);
  m_data->CStore.Set("Buffer_LAPPDOffset_0", Buffer_LAPPDOffset_0);
  m_data->CStore.Set("Buffer_LAPPDBeamgate_Raw_0", Buffer_LAPPDBeamgate_Raw_0);
  m_data->CStore.Set("Buffer_LAPPDTimestamp_Raw_0", Buffer_LAPPDTimestamp_Raw_0);
  m_data->CStore.Set("Buffer_LAPPDBGCorrection_0", Buffer_LAPPDBGCorrection_0);
  m_data->CStore.Set("Buffer_LAPPDTSCorrection_0", Buffer_LAPPDTSCorrection_0);
  m_data->CStore.Set("Buffer_LAPPDOffset_minus_ps_0", Buffer_LAPPDOffset_minus_ps_0);
  
  m_data->CStore.Set("Buffer_LAPPDBG_PPSBefore_0", Buffer_LAPPDBG_PPSBefore_0);
  m_data->CStore.Set("Buffer_LAPPDBG_PPSAfter_0", Buffer_LAPPDBG_PPSAfter_0);
  m_data->CStore.Set("Buffer_LAPPDBG_PPSDiff_0", Buffer_LAPPDBG_PPSDiff_0);
  m_data->CStore.Set("Buffer_LAPPDBG_PPSMissing_0", Buffer_LAPPDBG_PPSMissing_0);
  m_data->CStore.Set("Buffer_LAPPDTS_PPSBefore_0", Buffer_LAPPDTS_PPSBefore_0);
  m_data->CStore.Set("Buffer_LAPPDTS_PPSAfter_0", Buffer_LAPPDTS_PPSAfter_0);
  m_data->CStore.Set("Buffer_LAPPDTS_PPSDiff_0", Buffer_LAPPDTS_PPSDiff_0);
  m_data->CStore.Set("Buffer_LAPPDTS_PPSMissing_0", Buffer_LAPPDTS_PPSMissing_0);

  // --- Board 1 Buffers Set ---
  m_data->CStore.Set("Buffer_LAPPDTimestamp_ns_1", Buffer_LAPPDTimestamp_ns_1);
  m_data->CStore.Set("Buffer_LAPPDBeamgate_ns_1", Buffer_LAPPDBeamgate_ns_1);
  m_data->CStore.Set("Buffer_LAPPDOffset_1", Buffer_LAPPDOffset_1);
  m_data->CStore.Set("Buffer_LAPPDBeamgate_Raw_1", Buffer_LAPPDBeamgate_Raw_1);
  m_data->CStore.Set("Buffer_LAPPDTimestamp_Raw_1", Buffer_LAPPDTimestamp_Raw_1);
  m_data->CStore.Set("Buffer_LAPPDBGCorrection_1", Buffer_LAPPDBGCorrection_1);
  m_data->CStore.Set("Buffer_LAPPDTSCorrection_1", Buffer_LAPPDTSCorrection_1);
  m_data->CStore.Set("Buffer_LAPPDOffset_minus_ps_1", Buffer_LAPPDOffset_minus_ps_1);
  
  m_data->CStore.Set("Buffer_LAPPDBG_PPSBefore_1", Buffer_LAPPDBG_PPSBefore_1);
  m_data->CStore.Set("Buffer_LAPPDBG_PPSAfter_1", Buffer_LAPPDBG_PPSAfter_1);
  m_data->CStore.Set("Buffer_LAPPDBG_PPSDiff_1", Buffer_LAPPDBG_PPSDiff_1);
  m_data->CStore.Set("Buffer_LAPPDBG_PPSMissing_1", Buffer_LAPPDBG_PPSMissing_1);
  m_data->CStore.Set("Buffer_LAPPDTS_PPSBefore_1", Buffer_LAPPDTS_PPSBefore_1);
  m_data->CStore.Set("Buffer_LAPPDTS_PPSAfter_1", Buffer_LAPPDTS_PPSAfter_1);
  m_data->CStore.Set("Buffer_LAPPDTS_PPSDiff_1", Buffer_LAPPDTS_PPSDiff_1);
  m_data->CStore.Set("Buffer_LAPPDTS_PPSMissing_1", Buffer_LAPPDTS_PPSMissing_1);

  exeNum++;
  return true;
}

bool EBLAPPD::Finalise()
{
  Log("\033[1;34mEBLAPPD: Finalising\033[0m", v_message, verbosityEBLAPPD);
  Log("EBLAPPD: Matched LAPPD number = " + std::to_string(matchedLAPPDNumber), v_message, verbosityEBLAPPD);
  Log("EBLAPPD: Unmatched LAPPD number = " + std::to_string(MatchBuffer_LAPPDTimestamp_ns_0.size()), v_message, verbosityEBLAPPD);
  return true;
}

bool EBLAPPD::CleanData()
{
  // --- All Board 0 ---
  LAPPDBeamgate_ns_0 = 0;
  LAPPDTimestamp_ns_0 = 0;
  LAPPDOffset_0 = 0;
  LAPPDBeamgate_Raw_0 = 0;
  LAPPDTimestamp_Raw_0 = 0;
  LAPPDBGCorrection_0 = 0;
  LAPPDTSCorrection_0 = 0;
  LAPPDOffset_minus_ps_0 = 0;

  LAPPDBG_PPSBefore_0 = 0;
  LAPPDBG_PPSAfter_0 = 0;
  LAPPDBG_PPSDiff_0 = 0;
  LAPPDBG_PPSMissing_0 = 0;
  LAPPDTS_PPSBefore_0 = 0;
  LAPPDTS_PPSAfter_0 = 0;
  LAPPDTS_PPSDiff_0 = 0;
  LAPPDTS_PPSMissing_0 = 0;

  // --- All Board 1 ---
  LAPPDBeamgate_ns_1 = 0;
  LAPPDTimestamp_ns_1 = 0;
  LAPPDOffset_1 = 0;
  LAPPDBeamgate_Raw_1 = 0;
  LAPPDTimestamp_Raw_1 = 0;
  LAPPDBGCorrection_1 = 0;
  LAPPDTSCorrection_1 = 0;
  LAPPDOffset_minus_ps_1 = 0;

  LAPPDBG_PPSBefore_1 = 0;
  LAPPDBG_PPSAfter_1 = 0;
  LAPPDBG_PPSDiff_1 = 0;
  LAPPDBG_PPSMissing_1 = 0;
  LAPPDTS_PPSBefore_1 = 0;
  LAPPDTS_PPSAfter_1 = 0;
  LAPPDTS_PPSDiff_1 = 0;
  LAPPDTS_PPSMissing_1 = 0;

  return true;
}

bool EBLAPPD::LoadLAPPDData()
{
  // Get the LAPPD beamgate
  // --- All Board 0 ---
  m_data->CStore.Get("LAPPDBeamgate0_Raw", LAPPDBeamgate_Raw_0);
  m_data->CStore.Get("LAPPDTimestamp0_Raw", LAPPDTimestamp_Raw_0);
  m_data->CStore.Get("LAPPDBGCorrection_0", LAPPDBGCorrection_0);
  m_data->CStore.Get("LAPPDTSCorrection_0", LAPPDTSCorrection_0);
  m_data->CStore.Get("LAPPDOffset_minus_ps_0", LAPPDOffset_minus_ps_0);
  m_data->CStore.Get("LAPPDOffset_0", LAPPDOffset_0);

  // Convert to ns
  LAPPDBeamgate_ns_0 = (LAPPDBeamgate_Raw_0 + LAPPDBGCorrection_0) * 3.125 + LAPPDOffset_0;
  LAPPDTimestamp_ns_0 = (LAPPDTimestamp_Raw_0 + LAPPDTSCorrection_0) * 3.125 + LAPPDOffset_0;

  m_data->CStore.Get("BG_PPSBefore_0", LAPPDBG_PPSBefore_0);
  m_data->CStore.Get("BG_PPSAfter_0", LAPPDBG_PPSAfter_0);
  m_data->CStore.Get("BG_PPSDiff_0", LAPPDBG_PPSDiff_0);
  m_data->CStore.Get("BG_PPSMissing_0", LAPPDBG_PPSMissing_0);
  m_data->CStore.Get("TS_PPSBefore_0", LAPPDTS_PPSBefore_0);
  m_data->CStore.Get("TS_PPSAfter_0", LAPPDTS_PPSAfter_0);
  m_data->CStore.Get("TS_PPSDiff_0", LAPPDTS_PPSDiff_0);
  m_data->CStore.Get("TS_PPSMissing_0", LAPPDTS_PPSMissing_0);

  // --- All Board 1 ---
  m_data->CStore.Get("LAPPDBeamgate1_Raw", LAPPDBeamgate_Raw_1);
  m_data->CStore.Get("LAPPDTimestamp1_Raw", LAPPDTimestamp_Raw_1);
  m_data->CStore.Get("LAPPDBGCorrection_1", LAPPDBGCorrection_1);
  m_data->CStore.Get("LAPPDTSCorrection_1", LAPPDTSCorrection_1);
  m_data->CStore.Get("LAPPDOffset_minus_ps_1", LAPPDOffset_minus_ps_1);
  m_data->CStore.Get("LAPPDOffset_1", LAPPDOffset_1);

  // Convert to ns
  LAPPDBeamgate_ns_1 = (LAPPDBeamgate_Raw_1 + LAPPDBGCorrection_1) * 3.125 + LAPPDOffset_1;
  LAPPDTimestamp_ns_1 = (LAPPDTimestamp_Raw_1 + LAPPDTSCorrection_1) * 3.125 + LAPPDOffset_1;

  m_data->CStore.Get("BG_PPSBefore_1", LAPPDBG_PPSBefore_1);
  m_data->CStore.Get("BG_PPSAfter_1", LAPPDBG_PPSAfter_1);
  m_data->CStore.Get("BG_PPSDiff_1", LAPPDBG_PPSDiff_1);
  m_data->CStore.Get("BG_PPSMissing_1", LAPPDBG_PPSMissing_1);
  m_data->CStore.Get("TS_PPSBefore_1", LAPPDTS_PPSBefore_1);
  m_data->CStore.Get("TS_PPSAfter_1", LAPPDTS_PPSAfter_1);
  m_data->CStore.Get("TS_PPSDiff_1", LAPPDTS_PPSDiff_1);
  m_data->CStore.Get("TS_PPSMissing_1", LAPPDTS_PPSMissing_1);

  if (verbosityEBLAPPD > 1)
  {
    std::cout << "Processing new LAPPD data from store" << std::endl;
    
    // --- Logging Board 0 ---
    std::cout << "Got info Board 0: LAPPDBeamgate_Raw: " << LAPPDBeamgate_Raw_0 << ", LAPPDTimestamp_Raw: " << LAPPDTimestamp_Raw_0 
         << ", LAPPDBGCorrection: " << LAPPDBGCorrection_0 << ", LAPPDTSCorrection: " << LAPPDTSCorrection_0 
         << ", LAPPDOffset: " << LAPPDOffset_0 << ", LAPPDOffset_minus_ps: " << LAPPDOffset_minus_ps_0 
         << ", LAPPDBG_PPSBefore: " << LAPPDBG_PPSBefore_0 << ", LAPPDBG_PPSAfter: " << LAPPDBG_PPSAfter_0 
         << ", LAPPDBG_PPSDiff: " << LAPPDBG_PPSDiff_0 << ", LAPPDBG_PPSMissing: " << LAPPDBG_PPSMissing_0 
         << ", LAPPDTS_PPSBefore: " << LAPPDTS_PPSBefore_0 << ", LAPPDTS_PPSAfter: " << LAPPDTS_PPSAfter_0 
         << ", LAPPDTS_PPSDiff: " << LAPPDTS_PPSDiff_0 << ", LAPPDTS_PPSMissing: " << LAPPDTS_PPSMissing_0 << std::endl;
    std::cout << "LAPPDBeamgate_ns_0:   " << LAPPDBeamgate_ns_0 << std::endl;
    std::cout << "LAPPDTimestamp_ns_0: " << LAPPDTimestamp_ns_0 << std::endl;

    // --- Logging Board 1 ---
    std::cout << "Got info Board 1: LAPPDBeamgate_Raw: " << LAPPDBeamgate_Raw_1 << ", LAPPDTimestamp_Raw: " << LAPPDTimestamp_Raw_1 
         << ", LAPPDBGCorrection: " << LAPPDBGCorrection_1 << ", LAPPDTSCorrection: " << LAPPDTSCorrection_1 
         << ", LAPPDOffset: " << LAPPDOffset_1 << ", LAPPDOffset_minus_ps: " << LAPPDOffset_minus_ps_1 
         << ", LAPPDBG_PPSBefore: " << LAPPDBG_PPSBefore_1 << ", LAPPDBG_PPSAfter: " << LAPPDBG_PPSAfter_1 
         << ", LAPPDBG_PPSDiff: " << LAPPDBG_PPSDiff_1 << ", LAPPDBG_PPSMissing: " << LAPPDBG_PPSMissing_1 
         << ", LAPPDTS_PPSBefore: " << LAPPDTS_PPSBefore_1 << ", LAPPDTS_PPSAfter: " << LAPPDTS_PPSAfter_1 
         << ", LAPPDTS_PPSDiff: " << LAPPDTS_PPSDiff_1 << ", LAPPDTS_PPSMissing: " << LAPPDTS_PPSMissing_1 << std::endl;
    std::cout << "LAPPDBeamgate_ns_1:   " << LAPPDBeamgate_ns_1 << std::endl;
    std::cout << "LAPPDTimestamp_ns_1: " << LAPPDTimestamp_ns_1 << std::endl;
  }

  bool gotdata = m_data->CStore.Get("StoreLoadedLAPPDData", dat);
  if (gotdata)
  {
    // Shared buffers
    Buffer_LAPPDData.push_back(dat);
    Buffer_RunCode.push_back(currentRunCode);

    // --- Push all Board 0 buffers ---
    Buffer_LAPPDTimestamp_ns_0.push_back(LAPPDTimestamp_ns_0);
    Buffer_LAPPDBeamgate_ns_0.push_back(LAPPDBeamgate_ns_0);
    Buffer_LAPPDOffset_0.push_back(LAPPDOffset_0);
    Buffer_LAPPDBeamgate_Raw_0.push_back(LAPPDBeamgate_Raw_0);
    Buffer_LAPPDTimestamp_Raw_0.push_back(LAPPDTimestamp_Raw_0);
    Buffer_LAPPDBGCorrection_0.push_back(LAPPDBGCorrection_0);
    Buffer_LAPPDTSCorrection_0.push_back(LAPPDTSCorrection_0);
    Buffer_LAPPDOffset_minus_ps_0.push_back(LAPPDOffset_minus_ps_0);

    Buffer_LAPPDBG_PPSBefore_0.push_back(LAPPDBG_PPSBefore_0);
    Buffer_LAPPDBG_PPSAfter_0.push_back(LAPPDBG_PPSAfter_0);
    Buffer_LAPPDBG_PPSDiff_0.push_back(LAPPDBG_PPSDiff_0);
    Buffer_LAPPDBG_PPSMissing_0.push_back(LAPPDBG_PPSMissing_0);
    Buffer_LAPPDTS_PPSBefore_0.push_back(LAPPDTS_PPSBefore_0);
    Buffer_LAPPDTS_PPSAfter_0.push_back(LAPPDTS_PPSAfter_0);
    Buffer_LAPPDTS_PPSDiff_0.push_back(LAPPDTS_PPSDiff_0);
    Buffer_LAPPDTS_PPSMissing_0.push_back(LAPPDTS_PPSMissing_0);

    MatchBuffer_LAPPDTimestamp_ns_0.push_back(LAPPDTimestamp_ns_0);

    // --- Push all Board 1 buffers ---
    Buffer_LAPPDTimestamp_ns_1.push_back(LAPPDTimestamp_ns_1);
    Buffer_LAPPDBeamgate_ns_1.push_back(LAPPDBeamgate_ns_1);
    Buffer_LAPPDOffset_1.push_back(LAPPDOffset_1);
    Buffer_LAPPDBeamgate_Raw_1.push_back(LAPPDBeamgate_Raw_1);
    Buffer_LAPPDTimestamp_Raw_1.push_back(LAPPDTimestamp_Raw_1);
    Buffer_LAPPDBGCorrection_1.push_back(LAPPDBGCorrection_1);
    Buffer_LAPPDTSCorrection_1.push_back(LAPPDTSCorrection_1);
    Buffer_LAPPDOffset_minus_ps_1.push_back(LAPPDOffset_minus_ps_1);

    Buffer_LAPPDBG_PPSBefore_1.push_back(LAPPDBG_PPSBefore_1);
    Buffer_LAPPDBG_PPSAfter_1.push_back(LAPPDBG_PPSAfter_1);
    Buffer_LAPPDBG_PPSDiff_1.push_back(LAPPDBG_PPSDiff_1);
    Buffer_LAPPDBG_PPSMissing_1.push_back(LAPPDBG_PPSMissing_1);
    Buffer_LAPPDTS_PPSBefore_1.push_back(LAPPDTS_PPSBefore_1);
    Buffer_LAPPDTS_PPSAfter_1.push_back(LAPPDTS_PPSAfter_1);
    Buffer_LAPPDTS_PPSDiff_1.push_back(LAPPDTS_PPSDiff_1);
    Buffer_LAPPDTS_PPSMissing_1.push_back(LAPPDTS_PPSMissing_1);

    MatchBuffer_LAPPDTimestamp_ns_1.push_back(LAPPDTimestamp_ns_1);

    if (LAPPDTS_PPSMissing_0 != LAPPDBG_PPSMissing_0)
      Log("EBLAPPD: PPS missing in BG and TS are different for Board 0, BG: " + std::to_string(LAPPDBG_PPSMissing_0) + ", TS: " + std::to_string(LAPPDTS_PPSMissing_0), 
          v_warning, verbosityEBLAPPD);
      
    if (LAPPDTS_PPSMissing_1 != LAPPDBG_PPSMissing_1)
      Log("EBLAPPD: PPS missing in BG and TS are different for Board 1, BG: " + std::to_string(LAPPDBG_PPSMissing_1) + ", TS: " + std::to_string(LAPPDTS_PPSMissing_1), 
          v_warning, verbosityEBLAPPD);

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

  vector<uint64_t> matchedLAPPDTimes;
  vector<int> indexToRemove;
  std::map<int, int> matchedNumberInTrack;

  // Loop over the LAPPDDataBuffer keys, and all the grouped triggers
  // in each group of trigger, find the target trigger word and its time
  // find the minimum time difference, if smaller than matchTolerance_ns, then save the time to PairedCTCTimeStamps and PairedLAPPDTimeStamps
  
  // We loop based on the size of Board 0's buffer (Board 1 is identical in size)
  for (int i = 0; i < MatchBuffer_LAPPDTimestamp_ns_0.size(); i++)
  {
    
    uint64_t LAPPDtime_0 = MatchBuffer_LAPPDTimestamp_ns_0.at(i);
    uint64_t LAPPDtime_1 = MatchBuffer_LAPPDTimestamp_ns_1.at(i);
    
    // If found LAPPDtime at PairedLAPPDTimeStamps, skip 
    // Shouldn't happen
    bool already_matched = 
        (std::find(PairedLAPPDTimeStamps[matchToTrack].begin(), PairedLAPPDTimeStamps[matchToTrack].end(), LAPPDtime_0) != PairedLAPPDTimeStamps[matchToTrack].end()) || 
        (std::find(PairedLAPPDTimeStamps[matchToTrack].begin(), PairedLAPPDTimeStamps[matchToTrack].end(), LAPPDtime_1) != PairedLAPPDTimeStamps[matchToTrack].end());

    if (already_matched)
    {
        Log("EBLAPPD: Buffer " + std::to_string(i) + " with time " + std::to_string(Buffer_LAPPDTimestamp_ns_0.at(i)) + ": Found a match already", 
            v_message, verbosityEBLAPPD);
        continue;
    }

    // set minDT to 5 minutes
    uint64_t minDT = 5ULL * 60ULL * 1000000000ULL;
    uint64_t minDTTrigger = 0;
    uint32_t matchedTrigWord = 0;
    int matchedTrack = 0;
    int matchedIndex = 0;

    // We will keep track of which board actually provided the best match
    uint64_t matchedLAPPDTime = 0; 
    std::string winningBoard = "";

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
        
            // Iterate over all the grouped triggers, if the value is target trigger, then calculate the time difference
            for (const std::pair<uint64_t, uint32_t>& p : groupedTrigger)
            {
                if (matchToAllTriggers || p.second == targetTrigger)
                {
                    // Compute dt for both boards
                    uint64_t dt_0 = (LAPPDtime_0 > p.first) ? (LAPPDtime_0 - p.first) : (p.first - LAPPDtime_0);
                    uint64_t dt_1 = (LAPPDtime_1 > p.first) ? (LAPPDtime_1 - p.first) : (p.first - LAPPDtime_1);

                    // Find the best dt between the two boards
                    uint64_t current_min_dt = std::min(dt_0, dt_1);
                    uint64_t current_best_time = (dt_0 < dt_1) ? LAPPDtime_0 : LAPPDtime_1;
                    std::string current_winning_board = (dt_0 < dt_1) ? "Board 0" : "Board 1";

                    if (current_min_dt < minDT)
                    {
                        minDT = current_min_dt;
                        minDTTrigger = p.first;
                        matchedTrigWord = p.second;
                        matchedTrack = TrackTriggerWord;
                        matchedIndex = j;
                        matchedLAPPDTime = current_best_time;
                        winningBoard = current_winning_board;
                    }
                }
            }
        }
    }

    Log("EBLAPPD: at buffer " + std::to_string(i) + " with times [B0: " + std::to_string(LAPPDtime_0) + ", B1: " + std::to_string(LAPPDtime_1) + "], minDT: " 
        + std::to_string(minDT), v_debug, verbosityEBLAPPD);

    if (minDT < matchTolerance_ns)
    {
      PairedCTCTimeStamps[matchedTrack].push_back(minDTTrigger);
      PairedLAPPDTimeStamps[matchedTrack].push_back(matchedLAPPDTime);
      PairedLAPPD_TriggerIndex[matchedTrack].push_back(matchedIndex);

      matchedLAPPDTimes.push_back(matchedLAPPDTime);
      indexToRemove.push_back(i);
      matchedLAPPDNumber++;
      matchedNumberInTrack[matchedTrack]++;

      Log("EBLAPPD: Buffer " + std::to_string(i) + " (" + winningBoard + ") found a match! LAPPD time: " + std::to_string(matchedLAPPDTime) + 
          ", Target trigger: " + std::to_string(minDTTrigger) + ", minDT: " + std::to_string(minDT), v_message, verbosityEBLAPPD);
    }
  }
 
  Log("EBLAPPD: Finished matching LAPPD data with target triggers, " + std::to_string(matchedLAPPDTimes.size()) 
      + " new matches found, total matchedLAPPDNumber = " + std::to_string(matchedLAPPDNumber) 
      + " in buffer size = " + std::to_string(MatchBuffer_LAPPDTimestamp_ns_0.size()), v_message, verbosityEBLAPPD);

  for (int i = indexToRemove.size() - 1; i >= 0; i--)
  {
      MatchBuffer_LAPPDTimestamp_ns_0.erase(MatchBuffer_LAPPDTimestamp_ns_0.begin() + indexToRemove.at(i));
      MatchBuffer_LAPPDTimestamp_ns_1.erase(MatchBuffer_LAPPDTimestamp_ns_1.begin() + indexToRemove.at(i));    
  }

  Log("EBLAPPD: Finished removing paired LAPPD data from match buffers, size is now " + std::to_string(MatchBuffer_LAPPDTimestamp_ns_0.size()), v_message, verbosityEBLAPPD);
  
  // print all elements in matchedNumberInTrack with key and value
  for (std::pair<int, int> pair : matchedNumberInTrack)
  {
      Log("EBLAPPD: Match finished, matched number in Track " + std::to_string(pair.first) + " is = " + std::to_string(pair.second), v_message, verbosityEBLAPPD);
  }

  return true;
}
