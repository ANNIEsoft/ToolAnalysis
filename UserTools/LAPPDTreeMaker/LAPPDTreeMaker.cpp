#include "LAPPDTreeMaker.h"

LAPPDTreeMaker::LAPPDTreeMaker() : Tool() {}

bool LAPPDTreeMaker::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if (configfile != "")
    m_variables.Initialise(configfile); // loading config file
  // m_variables.Print();

  m_data = &data; // assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("treeMakerVerbosity", treeMakerVerbosity);
  m_variables.Get("treeMakerInputPulseLabel", treeMakerInputPulseLabel);
  m_variables.Get("treeMakerInputHitLabel", treeMakerInputHitLabel);
  treeMakerOutputFileName = "LAPPDTree.root";
  m_variables.Get("treeMakerOutputFileName", treeMakerOutputFileName);

  LoadPulse = false;
  m_variables.Get("LoadPulse", LoadPulse);
  LoadHit = false;
  m_variables.Get("LoadHit", LoadHit);
  LoadWaveform = false;
  m_variables.Get("LoadWaveform", LoadWaveform);
  LoadLAPPDDataTimeStamp = false;
  m_variables.Get("LoadLAPPDDataTimeStamp", LoadLAPPDDataTimeStamp);
  LoadPPSTimestamp = false;
  m_variables.Get("LoadPPSTimestamp", LoadPPSTimestamp);
  LoadRunInfoRaw = false;
  m_variables.Get("LoadRunInfoRaw", LoadRunInfoRaw);
  LoadRunInfoANNIEEvent = false;
  m_variables.Get("LoadRunInfoANNIEEvent", LoadRunInfoANNIEEvent);
  LoadTriggerInfo = false;
  m_variables.Get("LoadTriggerInfo", LoadTriggerInfo);
  LoadGroupedTriggerInfo = false;
  m_variables.Get("LoadGroupedTriggerInfo", LoadGroupedTriggerInfo);
  if (!LoadTriggerInfo)
    LoadGroupedTriggerInfo = false; // if not loading trigger, don't fill grouped trigger
  LoadGroupOption = "beam";
  m_variables.Get("LoadGroupOption", LoadGroupOption);
  MultiLAPPDMapTreeMaker = false;
  m_variables.Get("MultiLAPPDMapTreeMaker", MultiLAPPDMapTreeMaker);

  TString filename = treeMakerOutputFileName;
  file = new TFile(filename, "RECREATE");
  fPulse = new TTree("Pulse", "Pulse");
  fHit = new TTree("Hit", "Hit");
  fWaveform = new TTree("Waveform", "Waveform");
  fTimeStamp = new TTree("TimeStamp", "TimeStamp");
  fTrigger = new TTree("Trig", "Trig");
  fGroupedTrigger = new TTree("GTrig", "GTrig");

  fPulse->Branch("RunNumber", &RunNumber, "RunNumber/I");
  fPulse->Branch("SubRunNumber", &SubRunNumber, "SubRunNumber/I");
  fPulse->Branch("PartFileNumber", &PartFileNumber, "PartFileNumber/I");
  fPulse->Branch("EventNumber", &EventNumber, "EventNumber/I");
  fPulse->Branch("LAPPD_ID", &LAPPD_ID, "LAPPD_ID/I");
  fPulse->Branch("LAPPDDataTimeStamp0_UL", &LAPPDDataTimeStamp0_UL, "LAPPDDataTimeStamp0_UL/l");
  fPulse->Branch("LAPPDDataBeamgate0_UL", &LAPPDDataBeamgate0_UL, "LAPPDDataBeamgate0_UL/l");
  fPulse->Branch("ChannelID", &ChannelID, "ChannelID/I");
  fPulse->Branch("StripNumber", &StripNumber, "StripNumber/I");
  fPulse->Branch("PeakTime", &PeakTime, "PeakTime/D");
  fPulse->Branch("Charge", &Charge, "Charge/D");
  fPulse->Branch("PeakAmp", &PeakAmp, "PeakAmp/D");
  fPulse->Branch("PulseStart", &PulseStart, "PulseStart/D");
  fPulse->Branch("PulseEnd", &PulseEnd, "PulseEnd/D");
  fPulse->Branch("PulseSize", &PulseSize, "PulseSize/D");
  fPulse->Branch("PulseSide", &PulseSide, "PulseSide/I");
  fPulse->Branch("PulseThreshold", &PulseThreshold, "PulseThreshold/D");
  fPulse->Branch("PulseBaseline", &PulseBaseline, "PulseBaseline/D");
  if (MultiLAPPDMapTreeMaker)
  {
    fPulse->Branch("LTSRaw_0", &LTSRaw_0, "LTSRaw_0/l"); // Change fPulse to fHit/fWaveform for those trees
    fPulse->Branch("LBGRaw_0", &LBGRaw_0, "LBGRaw_0/l");
    fPulse->Branch("LOffset_ns_0", &LOffset_ns_0, "LOffset_ns_0/l");
    fPulse->Branch("LTSCorrection_0", &LTSCorrection_0, "LTSCorrection_0/I");
    fPulse->Branch("LBGCorrection_0", &LBGCorrection_0, "LBGCorrection_0/I");
    fPulse->Branch("LOSInMinusPS_0", &LOSInMinusPS_0, "LOSInMinusPS_0/I");
    
    fPulse->Branch("LTSRaw_1", &LTSRaw_1, "LTSRaw_1/l");
    fPulse->Branch("LBGRaw_1", &LBGRaw_1, "LBGRaw_1/l");
    fPulse->Branch("LOffset_ns_1", &LOffset_ns_1, "LOffset_ns_1/l");
    fPulse->Branch("LTSCorrection_1", &LTSCorrection_1, "LTSCorrection_1/I");
    fPulse->Branch("LBGCorrection_1", &LBGCorrection_1, "LBGCorrection_1/I");
    fPulse->Branch("LOSInMinusPS_1", &LOSInMinusPS_1, "LOSInMinusPS_1/I");

    fPulse->Branch("CTCPrimeTriggerTime", &CTCPrimeTriggerTime, "CTCPrimeTriggerTime/l");
  }

  fHit->Branch("RunNumber", &RunNumber, "RunNumber/I");
  fHit->Branch("SubRunNumber", &SubRunNumber, "SubRunNumber/I");
  fHit->Branch("PartFileNumber", &PartFileNumber, "PartFileNumber/I");
  fHit->Branch("EventNumber", &EventNumber, "EventNumber/I");
  fHit->Branch("LAPPD_ID", &LAPPD_ID, "LAPPD_ID/I");
  fHit->Branch("LAPPDDataTimeStamp0_UL", &LAPPDDataTimeStamp0_UL, "LAPPDDataTimeStamp0_UL/l");
  fHit->Branch("LAPPDDataBeamgate0_UL", &LAPPDDataBeamgate0_UL, "LAPPDDataBeamgate0_UL/l");
  fHit->Branch("StripNumber", &StripNumber, "StripNumber/I");
  fHit->Branch("HitTime", &HitTime, "HitTime/D");
  fHit->Branch("HitAmp", &HitAmp, "HitAmp/D");
  fHit->Branch("XPosTank", &XPosTank, "XPosTank/D");
  fHit->Branch("YPosTank", &YPosTank, "YPosTank/D");
  fHit->Branch("ZPosTank", &ZPosTank, "ZPosTank/D");
  fHit->Branch("ParallelPos", &ParallelPos, "ParallelPos/D");
  fHit->Branch("TransversePos", &TransversePos, "TransversePos/D");
  fHit->Branch("Pulse1StartTime", &Pulse1StartTime, "Pulse1StartTime/D");
  fHit->Branch("Pulse2StartTime", &Pulse2StartTime, "Pulse2StartTime/D");
  fHit->Branch("Pulse1LastTime", &Pulse1LastTime, "Pulse1LastTime/D");
  fHit->Branch("Pulse2LastTime", &Pulse2LastTime, "Pulse2LastTime/D");
  if (MultiLAPPDMapTreeMaker)
  {
    fHit->Branch("LTSRaw_0", &LTSRaw_0, "LTSRaw_0/l");
    fHit->Branch("LBGRaw_0", &LBGRaw_0, "LBGRaw_0/l");
    fHit->Branch("LOffset_ns_0", &LOffset_ns_0, "LOffset_ns_0/l");
    fHit->Branch("LTSCorrection_0", &LTSCorrection_0, "LTSCorrection_0/I");
    fHit->Branch("LBGCorrection_0", &LBGCorrection_0, "LBGCorrection_0/I");
    fHit->Branch("LOSInMinusPS_0", &LOSInMinusPS_0, "LOSInMinusPS_0/I");

    fHit->Branch("LTSRaw_1", &LTSRaw_1, "LTSRaw_1/l");
    fHit->Branch("LBGRaw_1", &LBGRaw_1, "LBGRaw_1/l");
    fHit->Branch("LOffset_ns_1", &LOffset_ns_1, "LOffset_ns_1/l");
    fHit->Branch("LTSCorrection_1", &LTSCorrection_1, "LTSCorrection_1/I");
    fHit->Branch("LBGCorrection_1", &LBGCorrection_1, "LBGCorrection_1/I");
    fHit->Branch("LOSInMinusPS_1", &LOSInMinusPS_1, "LOSInMinusPS_1/I");

    fHit->Branch("CTCPrimeTriggerTime", &CTCPrimeTriggerTime, "CTCPrimeTriggerTime/l");
  }

  fWaveform->Branch("RunNumber", &RunNumber, "RunNumber/I");
  fWaveform->Branch("SubRunNumber", &SubRunNumber, "SubRunNumber/I");
  fWaveform->Branch("PartFileNumber", &PartFileNumber, "PartFileNumber/I");
  fWaveform->Branch("EventNumber", &EventNumber, "EventNumber/I");
  fWaveform->Branch("LAPPD_ID", &LAPPD_ID, "LAPPD_ID/I");
  fWaveform->Branch("LAPPDDataTimeStamp0_UL", &LAPPDDataTimeStamp0_UL, "LAPPDDataTimeStamp0_UL/l");
  fWaveform->Branch("LAPPDDataBeamgate0_UL", &LAPPDDataBeamgate0_UL, "LAPPDDataBeamgate0_UL/l");
  fWaveform->Branch("StripNumber", &StripNumber, "StripNumber/I");
  fWaveform->Branch("PulseSide", &PulseSide, "PulseSide/I");
  fWaveform->Branch("WaveformMax", &waveformMaxValue, "WaveformMax/D");
  fWaveform->Branch("WaveformRMS", &waveformRMSValue, "WaveformRMS/D");
  fWaveform->Branch("WaveformMaxTimeBin", &waveformMaxTimeBinValue, "WaveformMaxTimeBin/I");
  fWaveform->Branch("waveformMaxFoundNear", &waveformMaxFoundNear, "waveformMaxFoundNear/O"); // O is boolean
  fWaveform->Branch("WaveformMaxNearing", &waveformMaxNearingValue, "WaveformMaxNearing/D");
  if (MultiLAPPDMapTreeMaker)
  {
    fWaveform->Branch("LTSRaw_0", &LTSRaw_0, "LTSRaw_0/l");
    fWaveform->Branch("LBGRaw_0", &LBGRaw_0, "LBGRaw_0/l");
    fWaveform->Branch("LOffset_ns_0", &LOffset_ns_0, "LOffset_ns_0/l");
    fWaveform->Branch("LTSCorrection_0", &LTSCorrection_0, "LTSCorrection_0/I");
    fWaveform->Branch("LBGCorrection_0", &LBGCorrection_0, "LBGCorrection_0/I");
    fWaveform->Branch("LOSInMinusPS_0", &LOSInMinusPS_0, "LOSInMinusPS_0/I");

    fWaveform->Branch("LTSRaw_1", &LTSRaw_1, "LTSRaw_1/l");
    fWaveform->Branch("LBGRaw_1", &LBGRaw_1, "LBGRaw_1/l");
    fWaveform->Branch("LOffset_ns_1", &LOffset_ns_1, "LOffset_ns_1/l");
    fWaveform->Branch("LTSCorrection_1", &LTSCorrection_1, "LTSCorrection_1/I");
    fWaveform->Branch("LBGCorrection_1", &LBGCorrection_1, "LBGCorrection_1/I");
    fWaveform->Branch("LOSInMinusPS_1", &LOSInMinusPS_1, "LOSInMinusPS_1/I");

    fWaveform->Branch("CTCPrimeTriggerTime", &CTCPrimeTriggerTime, "CTCPrimeTriggerTime/l");
  }

  fTimeStamp->Branch("RunNumber", &RunNumber, "RunNumber/I");
  fTimeStamp->Branch("SubRunNumber", &SubRunNumber, "SubRunNumber/I");
  fTimeStamp->Branch("PartFileNumber", &PartFileNumber, "PartFileNumber/I");
  fTimeStamp->Branch("EventNumber", &EventNumber, "EventNumber/I");
  fTimeStamp->Branch("LAPPD_ID", &LAPPD_ID, "LAPPD_ID/I");
  fTimeStamp->Branch("LAPPDDataTimeStamp0_UL", &LAPPDDataTimeStamp0_UL, "LAPPDDataTimeStamp0_UL/l");
  fTimeStamp->Branch("LAPPDDataBeamgate0_UL", &LAPPDDataBeamgate0_UL, "LAPPDDataBeamgate0_UL/l");
  fTimeStamp->Branch("LAPPDDataTimestamp0", &LAPPDDataTimestamp0_Part1, "LAPPDDataTimestamp0/l");
  fTimeStamp->Branch("LAPPDDataBeamgate0", &LAPPDDataBeamgate0_Part1, "LAPPDDataBeamgate0/l");
  fTimeStamp->Branch("LAPPDDataTimestamp0_Float", &LAPPDDataTimestamp0_Part2, "LAPPDDataTimestamp0_Float/D");
  fTimeStamp->Branch("LAPPDDataBeamgate0_Float", &LAPPDDataBeamgate0_Part2, "LAPPDDataBeamgate0_Float/D");
  fTimeStamp->Branch("LAPPDDataTimeStamp1_UL", &LAPPDDataTimeStamp1_UL, "LAPPDDataTimeStamp1_UL/l");
  fTimeStamp->Branch("LAPPDDataBeamgate1_UL", &LAPPDDataBeamgate1_UL, "LAPPDDataBeamgate1_UL/l");
  fTimeStamp->Branch("LAPPDDataTimestamp1", &LAPPDDataTimestamp1_Part1, "LAPPDDataTimestamp1/l");
  fTimeStamp->Branch("LAPPDDataBeamgate1", &LAPPDDataBeamgate1_Part1, "LAPPDDataBeamgate1/l");
  fTimeStamp->Branch("LAPPDDataTimestamp1_Float", &LAPPDDataTimestamp1_Part2, "LAPPDDataTimestamp1_Float/D");
  fTimeStamp->Branch("LAPPDDataBeamgate1_Float", &LAPPDDataBeamgate1_Part2, "LAPPDDataBeamgate1_Float/D");
  fTimeStamp->Branch("ppsDiff", &ppsDiff, "ppsDiff/L");
  fTimeStamp->Branch("ppsCount0", &ppsCount0, "ppsCount0/l");
  fTimeStamp->Branch("ppsCount1", &ppsCount1, "ppsCount1/l");
  fTimeStamp->Branch("ppsTime0", &ppsTime0, "ppsTime0/l");
  fTimeStamp->Branch("ppsTime1", &ppsTime1, "ppsTime1/l");

  // trigger trees have no related to LAPPD data or PPS, so don't need event number
  fTrigger->Branch("RunNumber", &RunNumber, "RunNumber/I");
  fTrigger->Branch("SubRunNumber", &SubRunNumber, "SubRunNumber/I");
  fTrigger->Branch("PartFileNumber", &PartFileNumber, "PartFileNumber/I");
  fTrigger->Branch("CTCTimeStamp", &CTCTimeStamp, "CTCTimeStamp/l");
  fTrigger->Branch("CTCTriggerWord", &CTCTriggerWord);
  fTrigger->Branch("trigNumInMap", &trigNumInThisMap, "trigNumInMap/I");
  fTrigger->Branch("trigIndexInMap", &trigIndexInThisMap, "trigNumInMap/I");

  fGroupedTrigger->Branch("RunNumber", &RunNumber, "RunNumber/I");
  fGroupedTrigger->Branch("SubRunNumber", &SubRunNumber, "SubRunNumber/I");
  fGroupedTrigger->Branch("PartFileNumber", &PartFileNumber, "PartFileNumber/I");
  fGroupedTrigger->Branch("gTrigWord", &groupedTriggerWords);
  fGroupedTrigger->Branch("gTrigTime", &groupedTriggerTimestamps);
  fGroupedTrigger->Branch("gTrigType", &groupedTriggerType, "gTrigType/I");
  fGroupedTrigger->Branch("gTrigNum", &TriggerGroupNumInThisEvent, "gTrigNum/I");

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry", _geom);
  EventNumber = 0;

  return true;
}

bool LAPPDTreeMaker::Execute()
{
  if (treeMakerVerbosity > 0)
    cout << "LAPPDTreeMaker::Execute()" << endl;
  CleanVariables();

  m_data->CStore.Get("LoadingPPS", LoadingPPS);
  if (treeMakerVerbosity > 0)
    cout << "LoadingPPS: " << LoadingPPS << endl;

  m_data->CStore.Get("LAPPDana", LAPPDana);
  if (treeMakerVerbosity > 0)
    cout << "LAPPDana: " << LAPPDana << endl;

  if (LoadRunInfoRaw)
    LoadRunInfoFromRaw();

  if (LoadRunInfoANNIEEvent)
    LoadRunInfoFromANNIEEvent();

  bool loadTriggerPaused = false;
  m_data->CStore.Get("PauseCTCDecoding", loadTriggerPaused);
  if (treeMakerVerbosity > 0)
    cout << "loadTriggerPaused: " << loadTriggerPaused << endl;
  if (!loadTriggerPaused)
  {
    if (LoadTriggerInfo)
    {
      bool getTrig = m_data->CStore.Get("TimeToTriggerWordMap", TriggerWordMap);
      if (!getTrig)
        cout << "Error in getting trigger word map" << endl;
      if (getTrig)
      {
        if (treeMakerVerbosity > 0)
          cout << "LAPPDTreeMaker:: TimeToTriggerWordMap size: " << TriggerWordMap->size() << endl;
        FillTriggerTree();
      }
    }
  }

  if (LoadGroupedTriggerInfo)
    FillGroupedTriggerTree();

  if (MultiLAPPDMapTreeMaker && LAPPDana)
  {
    m_data->Stores["ANNIEEvent"]->Get("PrimaryTriggerTime", CTCPrimeTriggerTime);
    if (treeMakerVerbosity > 0)
      std::cout << "LAPPDTreeMaker::Execute() MultiLAPPDMapTreeMaker" << std::endl;
    
    LoadLAPPDMapInfo();
    
    if (treeMakerVerbosity > 0)
    {
      std::cout << "LAPPDMapTimeStampRaw: ";
      for (auto &item : LAPPDMapTimeStampRaw_0)
        std::cout << item << " ";

      for (auto &item : LAPPDMapTimeStampRaw_1)
        std::cout << item << " ";

      std::cout << "LAPPDTreeMaker::Execute() MultiLAPPDMapTreeMaker finished " << std::endl;
    }
  }

  if (LoadPPSTimestamp && LoadingPPS)
  {
    bool gotPPSTimestamp = m_data->CStore.Get("LAPPDPPSVector", pps_vector);
    bool gotPPSCounter = m_data->CStore.Get("LAPPDPPScount", pps_count_vector);
    if (!gotPPSTimestamp)
      cout << "Error in getting PPS timestamp" << endl;
    if (!gotPPSCounter)
      cout << "Error in getting PPS counter" << endl;
    if (gotPPSTimestamp && gotPPSCounter)
      FillPPSTimestamp();
  }

  if (LoadLAPPDDataTimeStamp && !LoadingPPS)
  {
    bool newDataEvent;
    m_data->CStore.Get("LAPPD_new_event", newDataEvent);
    if (newDataEvent)
      FillLAPPDDataTimeStamp();
    m_data->CStore.Set("LAPPD_new_event", true);
  }

  if (LoadPulse && LAPPDana)
  {
    if (treeMakerVerbosity > 0)
      cout << "LAPPDTreeMaker::Execute() LoadPulse" << endl;
    bool gotPulse = m_data->Stores["ANNIEEvent"]->Get(treeMakerInputPulseLabel, lappdPulses);
    if (!gotPulse)
      cout << "Error in getting LAPPD pulses" << endl;
    if (gotPulse)
      FillPulseTree();
  }

  if (LoadHit && LAPPDana)
  {
    if (treeMakerVerbosity > 0)
      cout << "LAPPDTreeMaker::Execute() LoadHit" << endl;
    bool gotHit = m_data->Stores["ANNIEEvent"]->Get(treeMakerInputHitLabel, lappdHits);
    if (!gotHit)
      cout << "Error in getting LAPPD hits" << endl;
    if (gotHit)
      FillHitTree();
  }

  if (LoadWaveform && LAPPDana)
  {
    if (treeMakerVerbosity > 0)
      cout << "LAPPDTreeMaker::Execute() LoadWaveform" << endl;
    bool gotWaveformMax = m_data->Stores["ANNIEEvent"]->Get("waveformMax", waveformMax);
    if (!gotWaveformMax)
      cout << "Error in getting waveform max" << endl;
    bool gotWaveformRMS = m_data->Stores["ANNIEEvent"]->Get("waveformRMS", waveformRMS);
    if (!gotWaveformRMS)
      cout << "Error in getting waveform RMS" << endl;
    bool gotWaveformMaxLast = m_data->Stores["ANNIEEvent"]->Get("waveformMaxLast", waveformMaxLast);
    if (!gotWaveformMaxLast)
      cout << "Error in getting waveform max last" << endl;
    bool gotWaveformMaxNearing = m_data->Stores["ANNIEEvent"]->Get("waveformMaxNearing", waveformMaxNearing);
    if (!gotWaveformMaxNearing)
      cout << "Error in getting waveform max nearing" << endl;
    bool gotWaveformMaxTimeBin = m_data->Stores["ANNIEEvent"]->Get("waveformMaxTimeBin", waveformMaxTimeBin);
    if (!gotWaveformMaxTimeBin)
      cout << "Error in getting waveform max time bin" << endl;

    if (gotWaveformMax && gotWaveformRMS && gotWaveformMaxLast && gotWaveformMaxNearing && gotWaveformMaxTimeBin)
      FillWaveformTree();
  }

  if (LAPPDana)
    EventNumber++;

  if (TriggerWordMap != nullptr)
    TriggerWordMap->clear();

  return true;
}

bool LAPPDTreeMaker::Finalise()
{
  file->cd();
  fPulse->Write();
  fHit->Write();
  fWaveform->Write();
  fTimeStamp->Write();
  fTrigger->Write();
  fGroupedTrigger->Write();

  file->Close();
  delete file;

  return true;
}

void LAPPDTreeMaker::CleanVariables()
{
  LAPPDana = false;

  RunNumber = -9999;
  SubRunNumber = -9999;
  PartFileNumber = -9999;

  lappdPulses.clear();
  lappdHits.clear();
  lappdData.clear();
  waveformMax.clear();
  waveformRMS.clear();
  waveformMaxLast.clear();
  waveformMaxNearing.clear();
  waveformMaxTimeBin.clear();
  pps_vector.clear();
  pps_count_vector.clear();

  LAPPD_ID = -9999;
  ChannelID = -9999;
  PeakTime = -9999;
  Charge = -9999;
  PeakAmp = -9999;
  PulseStart = -9999;
  PulseEnd = -9999;
  PulseSize = -9999;
  PulseSide = -9999;
  PulseThreshold = -9999;
  PulseBaseline = -9999;

  HitTime = -9999;
  HitAmp = -9999;
  XPosTank = -9999;
  YPosTank = -9999;
  ZPosTank = -9999;
  ParallelPos = -9999;
  TransversePos = -9999;
  Pulse1StartTime = -9999;
  Pulse2StartTime = -9999;
  Pulse1LastTime = -9999;
  Pulse2LastTime = -9999;

  // ACDC 0
  LAPPDDataTimeStamp0_UL = 0;
  LAPPDDataBeamgate0_UL = 0;
  LAPPDDataTimestamp0_Part1 = 0;
  LAPPDDataBeamgate0_Part1 = 0;
  LAPPDDataTimestamp0_Part2 = -9999;
  LAPPDDataBeamgate0_Part2 = -9999;

  // ACDC 1
  LAPPDDataTimeStamp1_UL = 0;
  LAPPDDataBeamgate1_UL = 0;
  LAPPDDataTimestamp1_Part1 = 0;
  LAPPDDataBeamgate1_Part1 = 0;
  LAPPDDataTimestamp1_Part2 = -9999;
  LAPPDDataBeamgate1_Part2 = -9999;

  ppsDiff = -9999;
  ppsCount0 = 0;
  ppsCount1 = 0;
  ppsTime0 = 0;
  ppsTime1 = 0;

  CTCTimeStamp = 0;
  CTCTriggerWord.clear();
  trigNumInThisMap = -9999;
  trigIndexInThisMap = 0;

  groupedTriggerWordsVector.clear();
  groupedTriggerTimestampsVector.clear();
  groupedTriggerWords.clear();
  groupedTriggerTimestamps.clear();
  groupedTriggerByType.clear();
  groupedTriggerType = -9999;
  TriggerGroupNumInThisEvent = 0; // start from 0
  groupedTriggerType = -9999;

  LAPPD_IDs.clear();

  LAPPDMapTimeStampRaw_0.clear();
  LAPPDMapBeamgateRaw_0.clear();
  LAPPDMapOffsets_0.clear();
  LAPPDMapTSCorrections_0.clear();
  LAPPDMapBGCorrections_0.clear();
  LAPPDMapOSInMinusPS_0.clear();

  LAPPDMapTimeStampRaw_1.clear();
  LAPPDMapBeamgateRaw_1.clear();
  LAPPDMapOffsets_1.clear();
  LAPPDMapTSCorrections_1.clear();
  LAPPDMapBGCorrections_1.clear();
  LAPPDMapOSInMinusPS_1.clear();

  LAPPDDataMap.clear();
  
  LAPPDBeamgate_ns_0.clear();
  LAPPDTimeStamps_ns_0.clear();
  LAPPDTimeStampsRaw_0.clear();
  LAPPDBeamgatesRaw_0.clear();
  LAPPDOffsets_0.clear();
  LAPPDTSCorrection_0.clear();
  LAPPDBGCorrection_0.clear();
  LAPPDOSInMinusPS_0.clear();

  LAPPDBeamgate_ns_1.clear();
  LAPPDTimeStamps_ns_1.clear();
  LAPPDTimeStampsRaw_1.clear();
  LAPPDBeamgatesRaw_1.clear();
  LAPPDOffsets_1.clear();
  LAPPDTSCorrection_1.clear();
  LAPPDBGCorrection_1.clear();
  LAPPDOSInMinusPS_1.clear();
}

bool LAPPDTreeMaker::LoadRunInfoFromRaw()
{
  if (treeMakerVerbosity > 0)
    cout << "LAPPDTreeMaker::LoadRunInfoFromRaw" << endl;
  m_data->CStore.Get("runNumber", RunNumber);
  m_data->CStore.Get("rawFileNumber", PartFileNumber);
  m_data->CStore.Get("subrunNumber", SubRunNumber);
  return true;
}

bool LAPPDTreeMaker::LoadRunInfoFromANNIEEvent()
{
  if (treeMakerVerbosity > 0)
    cout << "LAPPDTreeMaker::LoadRunInfoFromANNIEEvent" << endl;
  m_data->Stores["ANNIEEvent"]->Get("RunNumber", RunNumber);
  m_data->Stores["ANNIEEvent"]->Get("SubRunNumber", SubRunNumber);
  m_data->Stores["ANNIEEvent"]->Get("PartNumber", PartFileNumber);
  if (treeMakerVerbosity > 0)
    cout << "RunNumber: " << RunNumber << ", SubRunNumber: " << SubRunNumber << ", PartFileNumber: " << PartFileNumber << endl;
  return true;
}

bool LAPPDTreeMaker::FillPulseTree()
{
  // LAPPDPulse thisPulse(LAPPD_ID, channel, peakBin*(25./256.), Q, peakAmp, pulseStart, pulseStart+pulseSize);
  std::map<unsigned long, vector<vector<LAPPDPulse>>>::iterator it;
  int foundPulseNum = 0;
  if (lappdPulses.size() == 0)
  {
    if (treeMakerVerbosity > 0)
      cout << "No pulses found" << endl;
    return true;
  }
  for (it = lappdPulses.begin(); it != lappdPulses.end(); it++)
  {
    int stripno = it->first;
    vector<vector<LAPPDPulse>> stripPulses = it->second;
    vector<LAPPDPulse> pulse0 = stripPulses.at(0);
    vector<LAPPDPulse> pulse1 = stripPulses.at(1);
    for (int i = 0; i < pulse0.size(); i++)
    {
      PulseSide = 0;
      LAPPDPulse thisPulse = pulse0.at(i);
      LAPPD_ID = thisPulse.GetTubeId();
      ChannelID = thisPulse.GetChannelID();
      StripNumber = stripno;
      PeakTime = thisPulse.GetTime();
      Charge = thisPulse.GetCharge();
      PeakAmp = thisPulse.GetPeak();
      PulseStart = thisPulse.GetLowRange();
      PulseEnd = thisPulse.GetHiRange();
      PulseSize = PulseEnd - PulseStart;
      // cout << " tree maker, this pulse0 LAPPD ID is " << LAPPD_ID << endl;
      //  TODO save threshold and baseline
      if (MultiLAPPDMapTreeMaker)
      {
        // find the LAPPD_ID in LAPPD_IDs, get the index
        //  use that index to get the timestamp and beamgate
        //  if not found, set them to zero
        int index = std::distance(LAPPD_IDs.begin(), std::find(LAPPD_IDs.begin(), LAPPD_IDs.end(), LAPPD_ID));
        if (index < LAPPDMapTimeStampRaw_0.size())
        {
          LTSRaw_0 = LAPPDMapTimeStampRaw_0.at(index);
          LBGRaw_0 = LAPPDMapBeamgateRaw_0.at(index);
          LOffset_ns_0 = LAPPDMapOffsets_0.at(index);
          LTSCorrection_0 = LAPPDMapTSCorrections_0.at(index);
          LBGCorrection_0 = LAPPDMapBGCorrections_0.at(index);
          LOSInMinusPS_0 = LAPPDMapOSInMinusPS_0.at(index);
          
          LTSRaw_1 = LAPPDMapTimeStampRaw_1.at(index);
          LBGRaw_1 = LAPPDMapBeamgateRaw_1.at(index);
          LOffset_ns_1 = LAPPDMapOffsets_1.at(index);
          LTSCorrection_1 = LAPPDMapTSCorrections_1.at(index);
          LBGCorrection_1 = LAPPDMapBGCorrections_1.at(index);
          LOSInMinusPS_1 = LAPPDMapOSInMinusPS_1.at(index);
        }
        else
        {
          LTSRaw_0 = 0;
          LBGRaw_0 = 0;
          LOffset_ns_0 = 0;
          LTSCorrection_0 = 0;
          LBGCorrection_0 = 0;
          LOSInMinusPS_0 = 0;

          LTSRaw_1 = 0;
          LBGRaw_1 = 0;
          LOffset_ns_1 = 0;
          LTSCorrection_1 = 0;
          LBGCorrection_1 = 0;
          LOSInMinusPS_1 = 0;
        }
      }

      fPulse->Fill();
      foundPulseNum++;
    }
    for (int i = 0; i < pulse1.size(); i++)
    {
      PulseSide = 1;
      LAPPDPulse thisPulse = pulse1.at(i);
      LAPPD_ID = thisPulse.GetTubeId();
      ChannelID = thisPulse.GetChannelID();
      StripNumber = stripno;
      PeakTime = thisPulse.GetTime();
      Charge = thisPulse.GetCharge();
      PeakAmp = thisPulse.GetPeak();
      PulseStart = thisPulse.GetLowRange();
      PulseEnd = thisPulse.GetHiRange();
      PulseSize = PulseEnd - PulseStart;
      /*cout << " this pulse1 LAPPD ID is " << LAPPD_ID << endl;
      cout << "ThresReco pulse1 got LAPPD_IDs: ";
      for (int i = 0; i < LAPPD_IDs.size(); i++)
      {
        cout << LAPPD_IDs.at(i) << " ";
      }*/
      // TODO save threshold and baseline
      if (MultiLAPPDMapTreeMaker)
      {
        // find the index of LAPPD_ID in LAPPD_IDs, use that index to assign the timestamlUL and beam gate UL, if not found set them to zero
        int index = std::distance(LAPPD_IDs.begin(), std::find(LAPPD_IDs.begin(), LAPPD_IDs.end(), LAPPD_ID));
        if (index < LAPPDMapTimeStampRaw_0.size())
        {
          LTSRaw_0 = LAPPDMapTimeStampRaw_0.at(index);
          LBGRaw_0 = LAPPDMapBeamgateRaw_0.at(index);
          LOffset_ns_0 = LAPPDMapOffsets_0.at(index);
          LTSCorrection_0 = LAPPDMapTSCorrections_0.at(index);
          LBGCorrection_0 = LAPPDMapBGCorrections_0.at(index);
          LOSInMinusPS_0 = LAPPDMapOSInMinusPS_0.at(index);
          
          LTSRaw_1 = LAPPDMapTimeStampRaw_1.at(index);
          LBGRaw_1 = LAPPDMapBeamgateRaw_1.at(index);
          LOffset_ns_1 = LAPPDMapOffsets_1.at(index);
          LTSCorrection_1 = LAPPDMapTSCorrections_1.at(index);
          LBGCorrection_1 = LAPPDMapBGCorrections_1.at(index);
          LOSInMinusPS_1 = LAPPDMapOSInMinusPS_1.at(index);
        }
        else
        {
          LTSRaw_0 = 0;
          LBGRaw_0 = 0;
          LOffset_ns_0 = 0;
          LTSCorrection_0 = 0;
          LBGCorrection_0 = 0;
          LOSInMinusPS_0 = 0;

          LTSRaw_1 = 0;
          LBGRaw_1 = 0;
          LOffset_ns_1 = 0;
          LTSCorrection_1 = 0;
          LBGCorrection_1 = 0;
          LOSInMinusPS_1 = 0;
        }
      }
      fPulse->Fill();
      foundPulseNum++;
    }
  }
  if (treeMakerVerbosity > 0)
    cout << "Found and saved " << foundPulseNum << " pulses" << endl;
  return true;
}

bool LAPPDTreeMaker::FillHitTree()
{
  // LAPPDHit hit(tubeID, averageTimeLow, averageAmp, positionInTank, positionOnLAPPD, pulse1LastTime, pulse2LastTime, pulse1StartTime, pulse2StartTime);
  int foundHitNum = 0;
  if (lappdHits.size() == 0)
  {
    if (treeMakerVerbosity > 0)
      cout << "No hits found" << endl;
    return true;
  }
  std::map<unsigned long, vector<LAPPDHit>>::iterator it;
  for (it = lappdHits.begin(); it != lappdHits.end(); it++)
  {
    int stripno = it->first;
    vector<LAPPDHit> stripHits = it->second;
    for (int i = 0; i < stripHits.size(); i++)
    {
      LAPPDHit thisHit = stripHits.at(i);
      LAPPD_ID = thisHit.GetTubeId();
      StripNumber = stripno;
      HitTime = thisHit.GetTime();
      HitAmp = thisHit.GetCharge();
      vector<double> position = thisHit.GetPosition();
      XPosTank = position.at(0);
      YPosTank = position.at(1);
      ZPosTank = position.at(2);
      vector<double> localPosition = thisHit.GetLocalPosition();
      ParallelPos = localPosition.at(0);
      TransversePos = localPosition.at(1);
      Pulse1StartTime = thisHit.GetPulse1StartTime();
      Pulse2StartTime = thisHit.GetPulse2StartTime();
      Pulse1LastTime = thisHit.GetPulse1LastTime();
      Pulse2LastTime = thisHit.GetPulse2LastTime();
      if (treeMakerVerbosity > 0)
      {
        cout << " this hit LAPPD ID is " << LAPPD_ID << endl;
        cout << "ThresReco hit got LAPPD_IDs: ";
        for (int i = 0; i < LAPPD_IDs.size(); i++)
        {
          cout << LAPPD_IDs.at(i) << " ";
        }
      }
      if (MultiLAPPDMapTreeMaker)
      {
        // find the LAPPD_ID in LAPPD_IDs, get the index
        //  use that index to get the timestamp and beamgate
        //  if not found, set them to zero
        int index = std::distance(LAPPD_IDs.begin(), std::find(LAPPD_IDs.begin(), LAPPD_IDs.end(), LAPPD_ID));
        if (index < LAPPDMapTimeStampRaw_0.size())
        {
          LTSRaw_0 = LAPPDMapTimeStampRaw_0.at(index);
          LBGRaw_0 = LAPPDMapBeamgateRaw_0.at(index);
          LOffset_ns_0 = LAPPDMapOffsets_0.at(index);
          LTSCorrection_0 = LAPPDMapTSCorrections_0.at(index);
          LBGCorrection_0 = LAPPDMapBGCorrections_0.at(index);
          LOSInMinusPS_0 = LAPPDMapOSInMinusPS_0.at(index);
          
          LTSRaw_1 = LAPPDMapTimeStampRaw_1.at(index);
          LBGRaw_1 = LAPPDMapBeamgateRaw_1.at(index);
          LOffset_ns_1 = LAPPDMapOffsets_1.at(index);
          LTSCorrection_1 = LAPPDMapTSCorrections_1.at(index);
          LBGCorrection_1 = LAPPDMapBGCorrections_1.at(index);
          LOSInMinusPS_1 = LAPPDMapOSInMinusPS_1.at(index);
        }
        else
        {
          LTSRaw_0 = 0;
          LBGRaw_0 = 0;
          LOffset_ns_0 = 0;
          LTSCorrection_0 = 0;
          LBGCorrection_0 = 0;
          LOSInMinusPS_0 = 0;

          LTSRaw_1 = 0;
          LBGRaw_1 = 0;
          LOffset_ns_1 = 0;
          LTSCorrection_1 = 0;
          LBGCorrection_1 = 0;
          LOSInMinusPS_1 = 0;
        }
      }
      fHit->Fill();
      foundHitNum++;
    }
  }
  if (treeMakerVerbosity > 0)
    cout << "Found and saved " << foundHitNum << " hits" << endl;
  return true;
}

bool LAPPDTreeMaker::FillWaveformTree()
{
  if (waveformMax.size() == 0)
  {
    if (treeMakerVerbosity > 0)
      cout << "No waveforms found" << endl;
    return true;
  }
  std::map<unsigned long, vector<double>>::iterator it;
  for (it = waveformMax.begin(); it != waveformMax.end(); it++)
  {
    int key = it->first;
    LAPPD_ID = static_cast<int>(key / 60);
    int stripSide = static_cast<int>((key - LAPPD_ID * 60) / 30);
    StripNumber = key - LAPPD_ID * 60 - stripSide * 30;
    // cout << " this waveform LAPPD ID is " << LAPPD_ID << endl;

    for (int side = 0; side < 2; side++)
    {
      PulseSide = side;
      waveformMaxValue = waveformMax.at(key).at(side);
      waveformRMSValue = waveformRMS.at(key).at(side);
      waveformMaxFoundNear = waveformMaxLast.at(key).at(side);
      waveformMaxNearingValue = waveformMaxNearing.at(key).at(side);
      waveformMaxTimeBinValue = waveformMaxTimeBin.at(key).at(side);
      /*cout << " LAPPDTreeMaker fill waveform tree, LAPPD ID is " << LAPPD_ID << endl;
      cout << "ThresReco got LAPPD_IDs: ";
      for (int i = 0; i < LAPPD_IDs.size(); i++)
      {
        cout << LAPPD_IDs.at(i) << " ";
      }*/
      if (MultiLAPPDMapTreeMaker)
      {
        // find the LAPPD_ID in LAPPD_IDs, get the index
        //  use that index to get the timestamp and beamgate
        //  if not found, set them to zero
        int index = std::distance(LAPPD_IDs.begin(), std::find(LAPPD_IDs.begin(), LAPPD_IDs.end(), LAPPD_ID));
        
        if (index < LAPPDMapTimeStampRaw_0.size())
        {
          LTSRaw_0 = LAPPDMapTimeStampRaw_0.at(index);
          LBGRaw_0 = LAPPDMapBeamgateRaw_0.at(index);
          LOffset_ns_0 = LAPPDMapOffsets_0.at(index);
          LTSCorrection_0 = LAPPDMapTSCorrections_0.at(index);
          LBGCorrection_0 = LAPPDMapBGCorrections_0.at(index);
          LOSInMinusPS_0 = LAPPDMapOSInMinusPS_0.at(index);

          LTSRaw_1 = LAPPDMapTimeStampRaw_1.at(index);
          LBGRaw_1 = LAPPDMapBeamgateRaw_1.at(index);
          LOffset_ns_1 = LAPPDMapOffsets_1.at(index);
          LTSCorrection_1 = LAPPDMapTSCorrections_1.at(index);
          LBGCorrection_1 = LAPPDMapBGCorrections_1.at(index);
          LOSInMinusPS_1 = LAPPDMapOSInMinusPS_1.at(index);
        }
        else
        {
          LTSRaw_0 = 0;
          LBGRaw_0 = 0;
          LOffset_ns_0 = 0;
          LTSCorrection_0 = 0;
          LBGCorrection_0 = 0;
          LOSInMinusPS_0 = 0;

          LTSRaw_1 = 0;
          LBGRaw_1 = 0;
          LOffset_ns_1 = 0;
          LTSCorrection_1 = 0;
          LBGCorrection_1 = 0;
          LOSInMinusPS_1 = 0;
        }
      }
      fWaveform->Fill();
    }
  }
  if (treeMakerVerbosity > 0)
    cout << "Found and saved " << waveformMax.size() << " waveforms" << endl;
  return true;
}

bool LAPPDTreeMaker::FillPPSTimestamp()
{
  ppsTime0 = pps_vector.at(0);
  ppsTime1 = pps_vector.at(1);
  ppsCount0 = pps_count_vector.at(0);
  ppsCount1 = pps_count_vector.at(1);
  ppsDiff = ppsTime1 - ppsTime0;
  m_data->CStore.Get("LAPPD_ID", LAPPD_ID);
  if (treeMakerVerbosity > 0)
    cout << "LAPPD_ID: " << LAPPD_ID << ", ppsDiff: " << ppsDiff << ", ppsTime0: " << ppsTime0 << ", ppsTime1: " << ppsTime1 << ", ppsCount0: " << ppsCount0 << ", ppsCount1: " << ppsCount1 << endl;
  fTimeStamp->Fill();
  return true;
}

bool LAPPDTreeMaker::FillLAPPDDataTimeStamp()
{
  if (treeMakerVerbosity > 0) {
    std::cout << "LAPPDTreeMaker::FillLAPPDDataTimeStamp, before fill:"
          << "\n  ACDC 0:"
          << "\n    Timestamp UL   = " << LAPPDDataTimeStamp0_UL
          << "\n    Beamgate UL    = " << LAPPDDataBeamgate0_UL
          << "\n    Timestamp Part1 = " << LAPPDDataTimestamp0_Part1
          << "\n    Beamgate Part1  = " << LAPPDDataBeamgate0_Part1
          << "\n    Timestamp Part2 = " << LAPPDDataTimestamp0_Part2
          << "\n    Beamgate Part2  = " << LAPPDDataBeamgate0_Part2
          << "\n  ACDC 1:"
          << "\n    Timestamp UL   = " << LAPPDDataTimeStamp1_UL
          << "\n    Beamgate UL    = " << LAPPDDataBeamgate1_UL
          << "\n    Timestamp Part1 = " << LAPPDDataTimestamp1_Part1
          << "\n    Beamgate Part1  = " << LAPPDDataBeamgate1_Part1
          << "\n    Timestamp Part2 = " << LAPPDDataTimestamp1_Part2
          << "\n    Beamgate Part2  = " << LAPPDDataBeamgate1_Part2
          << "\n  LAPPD ID = " << LAPPD_ID
          << std::endl;
  }

  if (!MultiLAPPDMapTreeMaker)
  {

    // Retrieve ACDC 0 reconstructed objects
    m_data->CStore.Get("LAPPDBeamgate0_Raw", LAPPDDataBeamgate0_UL);
    m_data->CStore.Get("LAPPDTimestamp0_Raw", LAPPDDataTimeStamp0_UL);
    m_data->CStore.Get("LAPPDBG0_IntCombined", LAPPDDataBeamgate0_Part1);
    m_data->CStore.Get("LAPPDBG0_Float", LAPPDDataBeamgate0_Part2);
    m_data->CStore.Get("LAPPDTS0_IntCombined", LAPPDDataTimestamp0_Part1);
    m_data->CStore.Get("LAPPDTS0_Float", LAPPDDataTimestamp0_Part2);

    // Retrieve ACDC 1 reconstructed objects
    m_data->CStore.Get("LAPPDBeamgate1_Raw", LAPPDDataBeamgate1_UL);
    m_data->CStore.Get("LAPPDTimestamp1_Raw", LAPPDDataTimeStamp1_UL);
    m_data->CStore.Get("LAPPDBG1_IntCombined", LAPPDDataBeamgate1_Part1);
    m_data->CStore.Get("LAPPDBG1_Float", LAPPDDataBeamgate1_Part2);
    m_data->CStore.Get("LAPPDTS1_IntCombined", LAPPDDataTimestamp1_Part1);
    m_data->CStore.Get("LAPPDTS1_Float", LAPPDDataTimestamp1_Part2);

    m_data->CStore.Get("LAPPD_ID", LAPPD_ID);
    fTimeStamp->Fill();
  }
  else
  {
    for (int i = 0; i < LAPPD_IDs.size(); i++)
    {
      LAPPD_ID = LAPPD_IDs.at(i);
  
      LAPPDDataBeamgate0_UL = LAPPDMapBeamgateRaw_0.at(i);
      LAPPDDataTimeStamp0_UL = LAPPDMapTimeStampRaw_0.at(i);
      
      LAPPDDataBeamgate1_UL = LAPPDMapBeamgateRaw_1.at(i);
      LAPPDDataTimeStamp1_UL = LAPPDMapTimeStampRaw_1.at(i);

      fTimeStamp->Fill();
    }
  }
  if (treeMakerVerbosity > 0) {
    std::cout << "LAPPDTreeMaker::FillLAPPDDataTimeStamp, after fill:"
          << "\n  ACDC 0:"
          << "\n    Timestamp UL   = " << LAPPDDataTimeStamp0_UL
          << "\n    Beamgate UL    = " << LAPPDDataBeamgate0_UL
          << "\n    Timestamp Part1 = " << LAPPDDataTimestamp0_Part1
          << "\n    Beamgate Part1  = " << LAPPDDataBeamgate0_Part1
          << "\n    Timestamp Part2 = " << LAPPDDataTimestamp0_Part2
          << "\n    Beamgate Part2  = " << LAPPDDataBeamgate0_Part2
          << "\n  ACDC 1:"
          << "\n    Timestamp UL   = " << LAPPDDataTimeStamp1_UL
          << "\n    Beamgate UL    = " << LAPPDDataBeamgate1_UL
          << "\n    Timestamp Part1 = " << LAPPDDataTimestamp1_Part1
          << "\n    Beamgate Part1  = " << LAPPDDataBeamgate1_Part1
          << "\n    Timestamp Part2 = " << LAPPDDataTimestamp1_Part2
          << "\n    Beamgate Part2  = " << LAPPDDataBeamgate1_Part2
          << "\n  LAPPD ID = " << LAPPD_ID
          << std::endl;
  }
  
  return true;
}

bool LAPPDTreeMaker::FillTriggerTree()
{
  if (treeMakerVerbosity > 0)
    cout << "LAPPDTreeMaker::FillTriggerTree" << endl;

  trigNumInThisMap = TriggerWordMap->size();
  for (const auto &item : *TriggerWordMap)
  {
    CTCTimeStamp = item.first;
    CTCTriggerWord = item.second;
    fTrigger->Fill();
    trigIndexInThisMap++;
  }
  if (treeMakerVerbosity > 0)
    cout << "FillTriggerTree: " << trigNumInThisMap << " triggers filled" << endl;

  return true;
}

bool LAPPDTreeMaker::FillGroupedTriggerTree()
{
  if (treeMakerVerbosity > 0)
    cout << "LAPPDTreeMaker::FillGroupedTriggerTree" << endl;

  // fill new triggers from the map to ungrouped trigger buffer
  for (const auto &item : *TriggerWordMap)
  {
    CTCTimeStamp = item.first;
    CTCTriggerWord = item.second;

    for (const auto &trigw : CTCTriggerWord)
    {
      unGroupedTriggerTimestamps.push_back(CTCTimeStamp);
      unGroupedTriggerWords.push_back(trigw);
    }
  }

  if (LoadGroupOption == "beam")
    GroupTriggerByBeam();
  else if (LoadGroupOption == "laser")
    GroupTriggerByLaser();

  GroupPPSTrigger();

  for (int i = 0; i < groupedTriggerWordsVector.size(); i++)
  {
    groupedTriggerWords = groupedTriggerWordsVector.at(i);
    groupedTriggerTimestamps = groupedTriggerTimestampsVector.at(i);
    groupedTriggerType = groupedTriggerByType.at(i);
    fGroupedTrigger->Fill();
    TriggerGroupNumInThisEvent += 1;
  }

  CleanTriggers();

  return true;
}

bool LAPPDTreeMaker::GroupTriggerByBeam()
{
  if (treeMakerVerbosity > 0)
    cout << "LAPPDTreeMaker::GroupTriggerByBeam" << endl;

  vector<int> removeIndex;
  bool beamNow = false;
  bool trigger2ed = false;
  bool trigger14ed = false;
  bool trigger1ed = false;
  int beamConfirm = 0;
  vector<uint32_t> thisTGroup;
  vector<unsigned long> thisTTimestamp;
  bool ppsNow = false;

  for (int i = 0; i < unGroupedTriggerWords.size(); i++)
  {
    uint32_t tWord = unGroupedTriggerWords.at(i);
    unsigned long tTime = unGroupedTriggerTimestamps.at(i);
    bool pushedToBuffer = false;
    if (!beamNow)
    {
      // not in a beam group
      if (tWord == 3)
      {
        beamNow = true;
        beamConfirm = 0;
        thisTGroup.push_back(tWord);
        thisTTimestamp.push_back(tTime);
        pushedToBuffer = true;
        trigger2ed = false;
        trigger14ed = false;
        trigger1ed = false;
      }
    }
    else
    { // while in a group
      if (beamConfirm <= 2 && tWord == 3)
      {
        // in grouping mode, meet a new start, but haven't got enough trigger 14 and 2
        // clear buffer, restart the grouping from this t 3
        thisTGroup.clear();
        thisTTimestamp.clear();
        beamNow = true;
        thisTGroup.push_back(tWord);
        thisTTimestamp.push_back(tTime);
        pushedToBuffer = true;
        trigger2ed = false;
        trigger14ed = false;
        trigger1ed = false;
      }

      if (beamConfirm > 2 && tWord == 10)
      {
        // if beam confirmed and meet a trigger 10, save the grouped triggers
        // 10 always come with 8, which is delayed trigger to MRD
        thisTGroup.push_back(tWord);
        thisTTimestamp.push_back(tTime);
        pushedToBuffer = true;
        // if found trigger 14 and 2, save the grouped triggers
        if ((std::find(thisTGroup.begin(), thisTGroup.end(), 14) != thisTGroup.end()) && (std::find(thisTGroup.begin(), thisTGroup.end(), 2) != thisTGroup.end()))
        {
          groupedTriggerWordsVector.push_back(thisTGroup);
          groupedTriggerTimestampsVector.push_back(thisTTimestamp);
          groupedTriggerByType.push_back(14);
        }
        thisTGroup.clear();
        thisTTimestamp.clear();
        beamNow = false;
        trigger14ed = false;
        trigger2ed = false;
        trigger1ed = false;
        beamConfirm = 0;
      }

      if (!pushedToBuffer && tWord != 32 && tWord != 34 && tWord != 36)
      {
        if (tWord == 2)
        {
          trigger2ed = true;
          beamConfirm++;
        }

        if (tWord != 14)
        {
          thisTGroup.push_back(tWord);
          thisTTimestamp.push_back(tTime);
        }
        else if (!trigger14ed)
        {
          thisTGroup.push_back(tWord);
          thisTTimestamp.push_back(tTime);
          trigger14ed = true;
          beamConfirm++;
        }
        pushedToBuffer = true;
      }
    }

    if (pushedToBuffer)
    {
      removeIndex.push_back(i);
    }
  }

  vector<uint32_t> unRemovedTriggerWords;
  vector<unsigned long> unRemovedTriggerTimestamps;
  for (int i = 0; i < unGroupedTriggerWords.size(); i++)
  {
    if (std::find(removeIndex.begin(), removeIndex.end(), i) == removeIndex.end())
    {
      unRemovedTriggerWords.push_back(unGroupedTriggerWords.at(i));
      unRemovedTriggerTimestamps.push_back(unGroupedTriggerTimestamps.at(i));
    }
  }
  unGroupedTriggerTimestamps = unRemovedTriggerTimestamps;
  unGroupedTriggerWords = unRemovedTriggerWords;

  return true;
}

bool LAPPDTreeMaker::GroupPPSTrigger()
{
  if (treeMakerVerbosity > 0)
    cout << "LAPPDTreeMaker::GroupPPSTrigger" << endl;

  vector<int> removeIndex;
  vector<uint32_t> thisTGroup;
  vector<unsigned long> thisTTimestamp;
  bool ppsNow = false;
  for (int i = 0; i < unGroupedTriggerWords.size(); i++)
  {
    uint32_t tWord = unGroupedTriggerWords.at(i);
    unsigned long tTime = unGroupedTriggerTimestamps.at(i);
    bool pushedToBuffer = false;
    if (!ppsNow)
    {
      if (tWord == 32)
      {
        ppsNow = true;
        thisTGroup.push_back(tWord);
        thisTTimestamp.push_back(tTime);
        pushedToBuffer = true;
      }
    }
    else
    {
      if (tWord == 34)
      {
        thisTGroup.push_back(tWord);
        thisTTimestamp.push_back(tTime);
        pushedToBuffer = true;
        groupedTriggerWordsVector.push_back(thisTGroup);
        groupedTriggerTimestampsVector.push_back(thisTTimestamp);
        groupedTriggerByType.push_back(32);
        thisTGroup.clear();
        thisTTimestamp.clear();
        ppsNow = false;
      }
    }

    if (pushedToBuffer)
    {
      removeIndex.push_back(i);
    }
  }

  vector<uint32_t> unRemovedTriggerWords;
  vector<unsigned long> unRemovedTriggerTimestamps;
  for (int i = 0; i < unGroupedTriggerWords.size(); i++)
  {
    if (std::find(removeIndex.begin(), removeIndex.end(), i) == removeIndex.end())
    {
      unRemovedTriggerWords.push_back(unGroupedTriggerWords.at(i));
      unRemovedTriggerTimestamps.push_back(unGroupedTriggerTimestamps.at(i));
    }
  }
  unGroupedTriggerTimestamps = unRemovedTriggerTimestamps;
  unGroupedTriggerWords = unRemovedTriggerWords;
  return true;
}

bool LAPPDTreeMaker::GroupTriggerByLaser()
{
  // waiting for implementation
  return true;
}

void LAPPDTreeMaker::CleanTriggers()
{
  // only keep the last 1000 triggers
  // create new vector to store the last 1000 triggers
  if (unGroupedTriggerWords.size() > 1000)
  {
    std::vector<uint32_t> lastTrigWords(unGroupedTriggerWords.end() - 1000, unGroupedTriggerWords.end());
    unGroupedTriggerWords = lastTrigWords;
    std::vector<unsigned long> lastTrigTimestamps(unGroupedTriggerTimestamps.end() - 1000, unGroupedTriggerTimestamps.end());
    unGroupedTriggerTimestamps = lastTrigTimestamps;
  }
}

void LAPPDTreeMaker::LoadLAPPDMapInfo()
{
  bool getMap = m_data->Stores["ANNIEEvent"]->Get("LAPPDDataMap", LAPPDDataMap);
  
  // ACDC 0 
  bool gotBeamgates_ns_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgate_ns_0", LAPPDBeamgate_ns_0);
  bool gotTimeStamps_ns_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDTimeStamps_ns_0", LAPPDTimeStamps_ns_0);
  bool gotTimeStampsRaw_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDTimeStampsRaw_0", LAPPDTimeStampsRaw_0);
  bool gotBeamgatesRaw_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgatesRaw_0", LAPPDBeamgatesRaw_0);
  bool gotOffsets_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDOffsets_0", LAPPDOffsets_0);
  bool gotTSCorrection_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDTSCorrection_0", LAPPDTSCorrection_0);
  bool gotDBGCorrection_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDBGCorrection_0", LAPPDBGCorrection_0);
  bool gotOSInMinusPS_0 = m_data->Stores["ANNIEEvent"]->Get("LAPPDOSInMinusPS_0", LAPPDOSInMinusPS_0);

  // ACDC 1
  bool gotBeamgates_ns_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgate_ns_1", LAPPDBeamgate_ns_1);
  bool gotTimeStamps_ns_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDTimeStamps_ns_1", LAPPDTimeStamps_ns_1);
  bool gotTimeStampsRaw_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDTimeStampsRaw_1", LAPPDTimeStampsRaw_1);
  bool gotBeamgatesRaw_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgatesRaw_1", LAPPDBeamgatesRaw_1);
  bool gotOffsets_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDOffsets_1", LAPPDOffsets_1);
  bool gotTSCorrection_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDTSCorrection_1", LAPPDTSCorrection_1);
  bool gotDBGCorrection_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDBGCorrection_1", LAPPDBGCorrection_1);
  bool gotOSInMinusPS_1 = m_data->Stores["ANNIEEvent"]->Get("LAPPDOSInMinusPS_1", LAPPDOSInMinusPS_1);

  if (getMap)
  {
    if (treeMakerVerbosity > 0)
      cout << "map size: " << LAPPDDataMap.size() << endl;
    
    for (auto &item : LAPPDDataMap)
    {
      PsecData thisData = item.second;
      int thisLAPPD_ID = thisData.LAPPD_ID;

      uint64_t thisDataTime = item.first;
      
      uint64_t thisTSRaw_0 = LAPPDTimeStampsRaw_0.at(thisDataTime);
      uint64_t thisBGRaw_0 = LAPPDBeamgatesRaw_0.at(thisDataTime);
      uint64_t thisOffset_0 = LAPPDOffsets_0.at(thisDataTime);
      int thisTSCorr_0 = LAPPDTSCorrection_0.at(thisDataTime);
      int thisDBGCorr_0 = LAPPDBGCorrection_0.at(thisDataTime);
      int thisOSInMinusPS_0 = LAPPDOSInMinusPS_0.at(thisDataTime);

      uint64_t thisTSRaw_1 = LAPPDTimeStampsRaw_1.at(thisDataTime);
      uint64_t thisBGRaw_1 = LAPPDBeamgatesRaw_1.at(thisDataTime);
      uint64_t thisOffset_1 = LAPPDOffsets_1.at(thisDataTime);
      int thisTSCorr_1 = LAPPDTSCorrection_1.at(thisDataTime);
      int thisDBGCorr_1 = LAPPDBGCorrection_1.at(thisDataTime);
      int thisOSInMinusPS_1 = LAPPDOSInMinusPS_1.at(thisDataTime);

      if (treeMakerVerbosity > 0) {
        cout << "tree maker, Got LAPPD ID: " << thisLAPPD_ID << ", time stamp: " << thisDataTime 
             << "\n  ACDC 0 - TSraw " << thisTSRaw_0 << ", BGraw " << thisBGRaw_0 << ", offset " << thisOffset_0 
             << ", TSCorr " << thisTSCorr_0 << ", DBGCorr " << thisDBGCorr_0 << ", OSInMinusPS " << thisOSInMinusPS_0 
             << "\n  ACDC 1 - TSraw " << thisTSRaw_1 << ", BGraw " << thisBGRaw_1 << ", offset " << thisOffset_1 
             << ", TSCorr " << thisTSCorr_1 << ", DBGCorr " << thisDBGCorr_1 << ", OSInMinusPS " << thisOSInMinusPS_1 << endl;
      }

      LAPPD_IDs.push_back(thisLAPPD_ID);
      
      LAPPDMapTimeStampRaw_0.push_back(thisTSRaw_0);
      LAPPDMapBeamgateRaw_0.push_back(thisBGRaw_0);
      LAPPDMapOffsets_0.push_back(thisOffset_0);
      LAPPDMapTSCorrections_0.push_back(thisTSCorr_0);
      LAPPDMapBGCorrections_0.push_back(thisDBGCorr_0);
      LAPPDMapOSInMinusPS_0.push_back(thisOSInMinusPS_0);

      LAPPDMapTimeStampRaw_1.push_back(thisTSRaw_1);
      LAPPDMapBeamgateRaw_1.push_back(thisBGRaw_1);
      LAPPDMapOffsets_1.push_back(thisOffset_1);
      LAPPDMapTSCorrections_1.push_back(thisTSCorr_1);
      LAPPDMapBGCorrections_1.push_back(thisDBGCorr_1);
      LAPPDMapOSInMinusPS_1.push_back(thisOSInMinusPS_1);

      if (treeMakerVerbosity > 0)
        cout << "size of LAPPD_IDs: " << LAPPD_IDs.size() << endl;
    }
  }
  else
  {
    cout << "LAPPDTreeMaker::LoadLAPPDMapInfo, no LAPPDDataMap found" << endl;
  }
}
