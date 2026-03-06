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
  fPulse->Branch("LAPPDDataTimeStampUL", &LAPPDDataTimeStampUL, "LAPPDDataTimeStampUL/l");
  fPulse->Branch("LAPPDDataBeamgateUL", &LAPPDDataBeamgateUL, "LAPPDDataBeamgateUL/l");
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
    fPulse->Branch("LTSRaw", &LTSRaw, "LTSRaw/l");
    fPulse->Branch("LBGRaw", &LBGRaw, "LBGRaw/l");
    fPulse->Branch("LOffset_ns", &LOffset_ns, "LOffset_ns/l");
    fPulse->Branch("LTSCorrection", &LTSCorrection, "LTSCorrection/I");
    fPulse->Branch("LBGCorrection", &LBGCorrection, "LBGCorrection/I");
    fPulse->Branch("LOSInMinusPS", &LOSInMinusPS, "LOSInMinusPS/I");
    fPulse->Branch("CTCPrimeTriggerTime", &CTCPrimeTriggerTime, "CTCPrimeTriggerTime/l");
  }

  fHit->Branch("RunNumber", &RunNumber, "RunNumber/I");
  fHit->Branch("SubRunNumber", &SubRunNumber, "SubRunNumber/I");
  fHit->Branch("PartFileNumber", &PartFileNumber, "PartFileNumber/I");
  fHit->Branch("EventNumber", &EventNumber, "EventNumber/I");
  fHit->Branch("LAPPD_ID", &LAPPD_ID, "LAPPD_ID/I");
  fHit->Branch("LAPPDDataTimeStampUL", &LAPPDDataTimeStampUL, "LAPPDDataTimeStampUL/l");
  fHit->Branch("LAPPDDataBeamgateUL", &LAPPDDataBeamgateUL, "LAPPDDataBeamgateUL/l");
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
    fHit->Branch("LTSRaw", &LTSRaw, "LTSRaw/l");
    fHit->Branch("LBGRaw", &LBGRaw, "LBGRaw/l");
    fHit->Branch("LOffset_ns", &LOffset_ns, "LOffset_ns/l");
    fHit->Branch("LTSCorrection", &LTSCorrection, "LTSCorrection/I");
    fHit->Branch("LBGCorrection", &LBGCorrection, "LBGCorrection/I");
    fHit->Branch("LOSInMinusPS", &LOSInMinusPS, "LOSInMinusPS/I");
    fHit->Branch("CTCPrimeTriggerTime", &CTCPrimeTriggerTime, "CTCPrimeTriggerTime/l");
  }

  fWaveform->Branch("RunNumber", &RunNumber, "RunNumber/I");
  fWaveform->Branch("SubRunNumber", &SubRunNumber, "SubRunNumber/I");
  fWaveform->Branch("PartFileNumber", &PartFileNumber, "PartFileNumber/I");
  fWaveform->Branch("EventNumber", &EventNumber, "EventNumber/I");
  fWaveform->Branch("LAPPD_ID", &LAPPD_ID, "LAPPD_ID/I");
  fWaveform->Branch("LAPPDDataTimeStampUL", &LAPPDDataTimeStampUL, "LAPPDDataTimeStampUL/l");
  fWaveform->Branch("LAPPDDataBeamgateUL", &LAPPDDataBeamgateUL, "LAPPDDataBeamgateUL/l");
  fWaveform->Branch("StripNumber", &StripNumber, "StripNumber/I");
  fWaveform->Branch("PulseSide", &PulseSide, "PulseSide/I");
  fWaveform->Branch("WaveformMax", &waveformMaxValue, "WaveformMax/D");
  fWaveform->Branch("WaveformRMS", &waveformRMSValue, "WaveformRMS/D");
  fWaveform->Branch("WaveformMaxTimeBin", &waveformMaxTimeBinValue, "WaveformMaxTimeBin/I");
  fWaveform->Branch("waveformMaxFoundNear", &waveformMaxFoundNear, "waveformMaxFoundNear/O"); // O is boolean
  fWaveform->Branch("WaveformMaxNearing", &waveformMaxNearingValue, "WaveformMaxNearing/D");
  if (MultiLAPPDMapTreeMaker)
  {
    fWaveform->Branch("LTSRaw", &LTSRaw, "LTSRaw/l");
    fWaveform->Branch("LBGRaw", &LBGRaw, "LBGRaw/l");
    fWaveform->Branch("LOffset_ns", &LOffset_ns, "LOffset_ns/l");
    fWaveform->Branch("LTSCorrection", &LTSCorrection, "LTSCorrection/I");
    fWaveform->Branch("LBGCorrection", &LBGCorrection, "LBGCorrection/I");
    fWaveform->Branch("LOSInMinusPS", &LOSInMinusPS, "LOSInMinusPS/I");
    fWaveform->Branch("CTCPrimeTriggerTime", &CTCPrimeTriggerTime, "CTCPrimeTriggerTime/l");
  }

  fTimeStamp->Branch("RunNumber", &RunNumber, "RunNumber/I");
  fTimeStamp->Branch("SubRunNumber", &SubRunNumber, "SubRunNumber/I");
  fTimeStamp->Branch("PartFileNumber", &PartFileNumber, "PartFileNumber/I");
  fTimeStamp->Branch("EventNumber", &EventNumber, "EventNumber/I");
  fTimeStamp->Branch("LAPPD_ID", &LAPPD_ID, "LAPPD_ID/I");
  fTimeStamp->Branch("LAPPDDataTimeStampUL", &LAPPDDataTimeStampUL, "LAPPDDataTimeStampUL/l");
  fTimeStamp->Branch("LAPPDDataBeamgateUL", &LAPPDDataBeamgateUL, "LAPPDDataBeamgateUL/l");
  fTimeStamp->Branch("LAPPDDataTimestamp", &LAPPDDataTimestampPart1, "LAPPDDataTimestamp/l");
  fTimeStamp->Branch("LAPPDDataBeamgate", &LAPPDDataBeamgatePart1, "LAPPDDataBeamgate/l");
  fTimeStamp->Branch("LAPPDDataTimestampFloat", &LAPPDDataTimestampPart2, "LAPPDDataTimestampFloat/D");
  fTimeStamp->Branch("LAPPDDataBeamgateFloat", &LAPPDDataBeamgatePart2, "LAPPDDataBeamgateFloat/D");
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

  TriggerWordMap = new std::map<uint64_t, std::vector<uint32_t>>;
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
      cout << "LAPPDTreeMaker::Execute() MultiLAPPDMapTreeMaker" << endl;
    LoadLAPPDMapInfo();
    bool getMap = m_data->Stores["ANNIEEvent"]->Get("LAPPDDataMap", LAPPDDataMap);
    bool gotBeamgates_ns = m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgate_ns", LAPPDBeamgate_ns);
    bool gotTimeStamps_ns = m_data->Stores["ANNIEEvent"]->Get("LAPPDTimeStamps_ns", LAPPDTimeStamps_ns);
    bool gotTimeStampsRaw = m_data->Stores["ANNIEEvent"]->Get("LAPPDTimeStampsRaw", LAPPDTimeStampsRaw);
    bool gotBeamgatesRaw = m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgatesRaw", LAPPDBeamgatesRaw);
    bool gotOffsets = m_data->Stores["ANNIEEvent"]->Get("LAPPDOffsets", LAPPDOffsets);
    bool gotTSCorrection = m_data->Stores["ANNIEEvent"]->Get("LAPPDTSCorrection", LAPPDTSCorrection);
    bool gotDBGCorrection = m_data->Stores["ANNIEEvent"]->Get("LAPPDBGCorrection", LAPPDBGCorrection);
    bool gotOSInMinusPS = m_data->Stores["ANNIEEvent"]->Get("LAPPDOSInMinusPS", LAPPDOSInMinusPS);
    if (treeMakerVerbosity > 0)
      cout << "LAPPDTreeMaker::Execute() MultiLAPPDMapTreeMaker get map = " << getMap << endl;
    if (getMap)
    {
      if (treeMakerVerbosity > 0)
        cout << "map size: " << LAPPDDataMap.size() << endl;
      for (auto &item : LAPPDDataMap)
      {
        PsecData thisData = item.second;
        int thisLAPPD_ID = thisData.LAPPD_ID;

        uint64_t thisDataTime = item.first;
        uint64_t thisTSRaw = LAPPDTimeStampsRaw.at(thisDataTime);
        uint64_t thisBGRaw = LAPPDBeamgatesRaw.at(thisDataTime);
        uint64_t thisOffset = LAPPDOffsets.at(thisDataTime);
        int thisTSCorr = LAPPDTSCorrection.at(thisDataTime);
        int thisDBGCorr = LAPPDBGCorrection.at(thisDataTime);
        int thisOSInMinusPS = LAPPDOSInMinusPS.at(thisDataTime);

        if (treeMakerVerbosity > 0)
          cout << "outside tree maker, Got LAPPD ID: " << thisLAPPD_ID << ", time stamp: " << thisDataTime << ", TSraw " << thisTSRaw << ", BGraw " << thisBGRaw << ", offset " << thisOffset << ", TSCorr " << thisTSCorr << ", DBGCorr " << thisDBGCorr << ", OSInMinusPS " << thisOSInMinusPS << endl;

        LAPPD_IDs.push_back(thisLAPPD_ID);
        LAPPDMapTimeStampRaw.push_back(thisTSRaw);
        LAPPDMapBeamgateRaw.push_back(thisBGRaw);
        LAPPDMapOffsets.push_back(thisOffset);
        LAPPDMapTSCorrections.push_back(thisTSCorr);
        LAPPDMapBGCorrections.push_back(thisDBGCorr);
        LAPPDMapOSInMinusPS.push_back(thisOSInMinusPS);

        if (treeMakerVerbosity > 0)
          cout << "outside size of LAPPD_IDs: " << LAPPD_IDs.size() << endl;
      }
    }
    else
    {
      cout << "outside LAPPDTreeMaker::LoadLAPPDMapInfo, no LAPPDDataMap found" << endl;
    }

    if (treeMakerVerbosity > 0)
    {
      cout << "LAPPDMapTimeStampRaw: ";
      for (auto &item : LAPPDMapTimeStampRaw)
        cout << item << " ";
      cout << "LAPPDTreeMaker::Execute() MultiLAPPDMapTreeMaker finished " << endl;
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

  LAPPDDataTimeStampUL = 0;
  LAPPDDataBeamgateUL = 0;
  LAPPDDataTimestampPart1 = 0;
  LAPPDDataBeamgatePart1 = 0;
  LAPPDDataTimestampPart2 = -9999;
  LAPPDDataBeamgatePart2 = -9999;

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
  LAPPDMapTimeStampRaw.clear();
  LAPPDMapBeamgateRaw.clear();
  LAPPDMapOffsets.clear();
  LAPPDMapTSCorrections.clear();
  LAPPDMapBGCorrections.clear();
  LAPPDMapOSInMinusPS.clear();

  LAPPDDataMap.clear();
  LAPPDBeamgate_ns.clear();
  LAPPDTimeStamps_ns.clear();
  LAPPDTimeStampsRaw.clear();
  LAPPDBeamgatesRaw.clear();
  LAPPDOffsets.clear();
  LAPPDTSCorrection.clear();
  LAPPDBGCorrection.clear();
  LAPPDOSInMinusPS.clear();
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
        if (index < LAPPDMapTimeStampRaw.size())
        {
          LTSRaw = LAPPDMapTimeStampRaw.at(index);
          LBGRaw = LAPPDMapBeamgateRaw.at(index);
          LOffset_ns = LAPPDMapOffsets.at(index);
          LTSCorrection = LAPPDMapTSCorrections.at(index);
          LBGCorrection = LAPPDMapBGCorrections.at(index);
          LOSInMinusPS = LAPPDMapOSInMinusPS.at(index);
        }
        else
        {
          LTSRaw = 0;
          LBGRaw = 0;
          LOffset_ns = 0;
          LTSCorrection = 0;
          LBGCorrection = 0;
          LOSInMinusPS = 0;
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
        if (index < LAPPDMapTimeStampRaw.size())
        {
          LTSRaw = LAPPDMapTimeStampRaw.at(index);
          LBGRaw = LAPPDMapBeamgateRaw.at(index);
          LOffset_ns = LAPPDMapOffsets.at(index);
          LTSCorrection = LAPPDMapTSCorrections.at(index);
          LBGCorrection = LAPPDMapBGCorrections.at(index);
          LOSInMinusPS = LAPPDMapOSInMinusPS.at(index);
        }
        else
        {
          LTSRaw = 0;
          LBGRaw = 0;
          LOffset_ns = 0;
          LTSCorrection = 0;
          LBGCorrection = 0;
          LOSInMinusPS = 0;
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
        if (index < LAPPDMapTimeStampRaw.size())
        {
          LTSRaw = LAPPDMapTimeStampRaw.at(index);
          LBGRaw = LAPPDMapBeamgateRaw.at(index);
          LOffset_ns = LAPPDMapOffsets.at(index);
          LTSCorrection = LAPPDMapTSCorrections.at(index);
          LBGCorrection = LAPPDMapBGCorrections.at(index);
          LOSInMinusPS = LAPPDMapOSInMinusPS.at(index);
        }
        else
        {
          LTSRaw = 0;
          LBGRaw = 0;
          LOffset_ns = 0;
          LTSCorrection = 0;
          LBGCorrection = 0;
          LOSInMinusPS = 0;
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
        if (index < LAPPDMapTimeStampRaw.size())
        {
          LTSRaw = LAPPDMapTimeStampRaw.at(index);
          LBGRaw = LAPPDMapBeamgateRaw.at(index);
          LOffset_ns = LAPPDMapOffsets.at(index);
          LTSCorrection = LAPPDMapTSCorrections.at(index);
          LBGCorrection = LAPPDMapBGCorrections.at(index);
          LOSInMinusPS = LAPPDMapOSInMinusPS.at(index);
        }
        else
        {
          LTSRaw = 0;
          LBGRaw = 0;
          LOffset_ns = 0;
          LTSCorrection = 0;
          LBGCorrection = 0;
          LOSInMinusPS = 0;
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
  if (treeMakerVerbosity > 0)
    cout << "LAPPDTreeMaker::FillLAPPDDataTimeStamp. Before fill: LAPPDDataTimeStampUL: " << LAPPDDataTimeStampUL << ", LAPPDDataBeamgateUL: " << LAPPDDataBeamgateUL << ", LAPPDDataTimestampPart1: " << LAPPDDataTimestampPart1 << ", LAPPDDataBeamgatePart1: " << LAPPDDataBeamgatePart1 << ", LAPPDDataTimestampPart2: " << LAPPDDataTimestampPart2 << ", LAPPDDataBeamgatePart2: " << LAPPDDataBeamgatePart2 << endl;
  if (!MultiLAPPDMapTreeMaker)
  {
    m_data->CStore.Get("LAPPDBeamgate_Raw", LAPPDDataBeamgateUL);
    m_data->CStore.Get("LAPPDTimestamp_Raw", LAPPDDataTimeStampUL);
    m_data->CStore.Get("LAPPDBGIntCombined", LAPPDDataBeamgatePart1);
    m_data->CStore.Get("LAPPDBGFloat", LAPPDDataBeamgatePart2);
    m_data->CStore.Get("LAPPDTSIntCombined", LAPPDDataTimestampPart1);
    m_data->CStore.Get("LAPPDTSFloat", LAPPDDataTimestampPart2);
    m_data->CStore.Get("LAPPD_ID", LAPPD_ID);
    fTimeStamp->Fill();
  }
  else
  {
    for (int i = 0; i < LAPPD_IDs.size(); i++)
    {
      LAPPD_ID = LAPPD_IDs.at(i);
      LAPPDDataBeamgateUL = LAPPDMapBeamgateRaw.at(i);
      LAPPDDataTimeStampUL = LAPPDMapTimeStampRaw.at(i);
      fTimeStamp->Fill();
    }
  }
  if (treeMakerVerbosity > 0)
    cout << "LAPPDDataTimeStampUL: " << LAPPDDataTimeStampUL << ", LAPPDDataBeamgateUL: " << LAPPDDataBeamgateUL << ", LAPPDDataTimestampPart1: " << LAPPDDataTimestampPart1 << ", LAPPDDataBeamgatePart1: " << LAPPDDataBeamgatePart1 << ", LAPPDDataTimestampPart2: " << LAPPDDataTimestampPart2 << ", LAPPDDataBeamgatePart2: " << LAPPDDataBeamgatePart2 << endl;
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
  // cout << "LAPPDTreeMaker::LoadLAPPDMapInfo" << endl;
  bool getMap = m_data->Stores["ANNIEEvent"]->Get("LAPPDDataMap", LAPPDDataMap);
  bool gotBeamgates_ns = m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgate_ns", LAPPDBeamgate_ns);
  bool gotTimeStamps_ns = m_data->Stores["ANNIEEvent"]->Get("LAPPDTimeStamps_ns", LAPPDTimeStamps_ns);
  bool gotTimeStampsRaw = m_data->Stores["ANNIEEvent"]->Get("LAPPDTimeStampsRaw", LAPPDTimeStampsRaw);
  bool gotBeamgatesRaw = m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgatesRaw", LAPPDBeamgatesRaw);
  bool gotOffsets = m_data->Stores["ANNIEEvent"]->Get("LAPPDOffsets", LAPPDOffsets);
  bool gotTSCorrection = m_data->Stores["ANNIEEvent"]->Get("LAPPDTSCorrection", LAPPDTSCorrection);
  bool gotDBGCorrection = m_data->Stores["ANNIEEvent"]->Get("LAPPDBGCorrection", LAPPDBGCorrection);
  bool gotOSInMinusPS = m_data->Stores["ANNIEEvent"]->Get("LAPPDOSInMinusPS", LAPPDOSInMinusPS);
  // cout << "LAPPDTreeMaker::LoadLAPPDMapInfo() get map = " << getMap << endl;

  if (getMap)
  {
    if (treeMakerVerbosity > 0)
      cout << "map size: " << LAPPDDataMap.size() << endl;
    for (auto &item : LAPPDDataMap)
    {
      PsecData thisData = item.second;
      int thisLAPPD_ID = thisData.LAPPD_ID;

      uint64_t thisDataTime = item.first;
      uint64_t thisTSRaw = LAPPDTimeStampsRaw.at(thisDataTime);
      uint64_t thisBGRaw = LAPPDBeamgatesRaw.at(thisDataTime);
      uint64_t thisOffset = LAPPDOffsets.at(thisDataTime);
      int thisTSCorr = LAPPDTSCorrection.at(thisDataTime);
      int thisDBGCorr = LAPPDBGCorrection.at(thisDataTime);
      int thisOSInMinusPS = LAPPDOSInMinusPS.at(thisDataTime);

      if (treeMakerVerbosity > 0)
        cout << "tree maker, Got LAPPD ID: " << thisLAPPD_ID << ", time stamp: " << thisDataTime << ", TSraw " << thisTSRaw << ", BGraw " << thisBGRaw << ", offset " << thisOffset << ", TSCorr " << thisTSCorr << ", DBGCorr " << thisDBGCorr << ", OSInMinusPS " << thisOSInMinusPS << endl;

      LAPPD_IDs.push_back(thisLAPPD_ID);
      LAPPDMapTimeStampRaw.push_back(thisTSRaw);
      LAPPDMapBeamgateRaw.push_back(thisBGRaw);
      LAPPDMapOffsets.push_back(thisOffset);
      LAPPDMapTSCorrections.push_back(thisTSCorr);
      LAPPDMapBGCorrections.push_back(thisDBGCorr);
      LAPPDMapOSInMinusPS.push_back(thisOSInMinusPS);

      if (treeMakerVerbosity > 0)
        cout << "size of LAPPD_IDs: " << LAPPD_IDs.size() << endl;
    }
  }
  else
  {
    cout << "LAPPDTreeMaker::LoadLAPPDMapInfo, no LAPPDDataMap found" << endl;
  }
}