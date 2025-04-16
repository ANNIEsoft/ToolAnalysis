#include "ANNIEEventTreeMaker.h"

ANNIEEventTreeMaker::ANNIEEventTreeMaker() : Tool() {}

bool ANNIEEventTreeMaker::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if (configfile != "")
    m_variables.Initialise(configfile); // loading config file
  // m_variables.Print();

  m_data = &data; // assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("ANNIEEventTreeMakerVerbosity", ANNIEEventTreeMakerVerbosity);
  m_variables.Get("isData", isData);
  m_variables.Get("HasGenie", hasGenie);

  fillAllTriggers = true;
  m_variables.Get("fillAllTriggers", fillAllTriggers);
  fill_singleTrigger = true;
  m_variables.Get("fill_singleTrigger", fill_singleTrigger);
  fill_singleTriggerWord = 14;
  m_variables.Get("fill_singleTriggerWord", fill_singleTriggerWord);
  fill_TriggerWord = {14, 47};

  m_variables.Get("fillCleanEventsOnly", fillCleanEventsOnly);
  m_variables.Get("fillLAPPDEventsOnly", fillLAPPDEventsOnly);

  m_variables.Get("TankHitInfo_fill", TankHitInfo_fill);
  m_variables.Get("TankCluster_fill", TankCluster_fill);
  m_variables.Get("cluster_TankHitInfo_fill", cluster_TankHitInfo_fill);
  m_variables.Get("MRDHitInfo_fill", MRDHitInfo_fill);
  m_variables.Get("RWMBRF_fill", RWMBRF_fill);

  m_variables.Get("MCTruth_fill", MCTruth_fill);
  m_variables.Get("MRDReco_fill", MRDReco_fill);
  m_variables.Get("TankReco_fill", TankReco_fill);
  m_variables.Get("RecoDebug_fill", RecoDebug_fill);
  m_variables.Get("muonTruthRecoDiff_fill", muonTruthRecoDiff_fill);
  m_variables.Get("LAPPDData_fill", LAPPDData_fill);
  m_variables.Get("SiPMPulseInfo_fill", SiPMPulseInfo_fill);
  m_variables.Get("LAPPDReco_fill", LAPPDReco_fill);
  m_variables.Get("LAPPD_PPS_fill", LAPPD_PPS_fill);
  m_variables.Get("LAPPD_Waveform_fill", LAPPD_Waveform_fill);
  m_variables.Get("LAPPD_MC_fill", LAPPD_MC_fill);

  std::string output_filename = "ANNIEEventTree.root";
  m_variables.Get("OutputFile", output_filename);
  fOutput_tfile = new TFile(output_filename.c_str(), "recreate");
  fANNIETree = new TTree("Event", "ANNIE Phase II Event Tree");

  m_data->CStore.Get("AuxChannelNumToTypeMap", AuxChannelNumToTypeMap);
  m_data->CStore.Get("ChannelNumToTankPMTSPEChargeMap", ChannelKeyToSPEMap);
  m_data->CStore.Get("pmt_tubeid_to_channelkey_data", pmtid_to_channelkey);
  m_data->CStore.Get("channelkey_to_pmtid", channelkey_to_pmtid);
  m_data->CStore.Get("channelkey_to_mrdpmtid", channelkey_to_mrdpmtid);
  m_data->CStore.Get("mrdpmtid_to_channelkey_data", mrdpmtid_to_channelkey_data);
  m_data->CStore.Get("channelkey_to_faccpmtid", channelkey_to_faccpmtid);

  auto get_geometry = m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry", geom);
  if (!get_geometry)
  {
    Log("ANNIEEventTreeMaker Tool: Error retrieving Geometry from ANNIEEvent!", v_error, ANNIEEventTreeMakerVerbosity);
    return false;
  }

  // general information in an event
  fANNIETree->Branch("runNumber", &fRunNumber, "runNumber/I");
  fANNIETree->Branch("subrunNumber", &fSubrunNumber, "subrunNumber/I");
  fANNIETree->Branch("partFileNumber", &fPartFileNumber, "partFileNumber/I");
  fANNIETree->Branch("runType", &fRunType, "runType/I");
  fANNIETree->Branch("eventNumber", &fEventNumber, "eventNumber/I"); //
  fANNIETree->Branch("PrimaryTriggerWord", &fPrimaryTriggerWord, "PrimaryTriggerWord/I");
  fANNIETree->Branch("GroupedTriggerTime", &fGroupedTriggerTime);
  fANNIETree->Branch("GroupedTriggerWord", &fGroupedTriggerWord);
  fANNIETree->Branch("trigword", &fTriggerword, "trigword/I"); // keep for trigger 5 or 14

  // flags from event selector
  fANNIETree->Branch("Extended", &fExtended, "Extended/I");
  fANNIETree->Branch("HasTank", &fHasTank, "HasTank/I");
  fANNIETree->Branch("HasMRD", &fHasMRD, "HasMRD/I");
  fANNIETree->Branch("HasLAPPD", &fHasLAPPD, "HasLAPPD/I");
  fANNIETree->Branch("TankMRDCoinc", &fTankMRDCoinc, "TankMRDCoinc/I");
  fANNIETree->Branch("NoVeto", &fNoVeto, "NoVeto/I");
  fANNIETree->Branch("MRDTriggerType", &fMRDTriggerTypeInt, "MRDTriggerType/I");
  //fANNIETree->Branch("MRDTriggerTypeString", &fMRDTriggerType);

  // event information of data
  fANNIETree->Branch("eventTimeTank", &fEventTimeTank, "eventTimeTank/l");
  fANNIETree->Branch("eventTimeMRD", &fEventTimeMRD, "eventTimeMRD/l");

  // beam information
  if (BeamInfo_fill)
  {
    fANNIETree->Branch("beam_pot_875", &fPot, "beam_pot_875/D");
    fANNIETree->Branch("beam_ok", &fBeamok, "beam_ok/I");
    fANNIETree->Branch("BunchRotationOn", &fBunchRotationOn, "BunchRotationOn/I");
    fANNIETree->Branch("beam_E_TOR860", &beam_E_TOR860, "beam_E_TOR860/D");
    fANNIETree->Branch("beam_E_TOR875", &beam_E_TOR875, "beam_E_TOR875/D");
    fANNIETree->Branch("beam_THCURR", &beam_THCURR, "beam_THCURR/D");
    fANNIETree->Branch("beam_BTJT2", &beam_BTJT2, "beam_BTJT2/D");
    fANNIETree->Branch("beam_HP875", &beam_HP875, "beam_HP875/D");
    fANNIETree->Branch("beam_VP875", &beam_VP875, "beam_VP875/D");
    fANNIETree->Branch("beam_HPTG1", &beam_HPTG1, "beam_HPTG1/D");
    fANNIETree->Branch("beam_VPTG1", &beam_VPTG1, "beam_VPTG1/D");
    fANNIETree->Branch("beam_HPTG2", &beam_HPTG2, "beam_HPTG2/D");
    fANNIETree->Branch("beam_VPTG2", &beam_VPTG2, "beam_VPTG2/D");
    fANNIETree->Branch("beam_BTH2T2", &beam_BTH2T2, "beam_BTH2T2/D");
    fANNIETree->Branch("beam_B_BRRMPL", &beam_B_BRRMPL, "beam_B_BRRMPL/D");
    fANNIETree->Branch("beam_B_BRRMPQ", &beam_B_BRRMPQ, "beam_B_BRRMPQ/D");
    fANNIETree->Branch("beam_B_BRRMPS", &beam_B_BRRMPS, "beam_B_BRRMPS/D");
    fANNIETree->Branch("beam_B_BRRMP", &beam_B_BRRMP, "beam_B_BRRMP/D");
    fANNIETree->Branch("BeamInfoTime", &fBeamInfoTime, "BeamInfoTime/l");
    fANNIETree->Branch("BeamInfoTimeToTriggerDiff", &fBeamInfoTimeToTriggerDiff, "BeamInfoTimeToTriggerDiff/L");
  }

  if (RWMBRF_fill)
  {
    fANNIETree->Branch("RWMRisingStart", &fRWMRisingStart, "fRWMRisingStart/D");
    fANNIETree->Branch("RWMRisingEnd", &fRWMRisingEnd, "fRWMRisingEnd/D");
    fANNIETree->Branch("RWMHalfRising", &fRWMHalfRising, "fRWMHalfRising/D");
    fANNIETree->Branch("RWMFHWM", &fRWMFHWM, "fRWMFHWM/D");
    fANNIETree->Branch("RWMFirstPeak", &fRWMFirstPeak, "fRWMFirstPeak/D");

    fANNIETree->Branch("BRFFirstPeak", &fBRFFirstPeak, "fBRFFirstPeak/D");
    fANNIETree->Branch("BRFAveragePeak", &fBRFAveragePeak, "fBRFAveragePeak/D");
    fANNIETree->Branch("BRFFirstPeakFit", &fBRFFirstPeakFit, "fBRFFirstPeakFit/D");
  }

  //********************************************************//

  // Tank cluster information

  fANNIETree->Branch("numberOfClusters", &fNumberOfClusters, "fNumberOfClusters/I");
  fANNIETree->Branch("clusterTime", &fClusterTimeV);
  fANNIETree->Branch("clusterCharge", &fClusterChargeV);
  fANNIETree->Branch("clusterPE", &fClusterPEV);
  fANNIETree->Branch("clusterMaxPE", &fClusterMaxPEV);
  fANNIETree->Branch("clusterChargePointX", &fClusterChargePointXV);
  fANNIETree->Branch("clusterChargePointY", &fClusterChargePointYV);
  fANNIETree->Branch("clusterChargePointZ", &fClusterChargePointZV);
  fANNIETree->Branch("clusterChargeBalance", &fClusterChargeBalanceV);
  fANNIETree->Branch("clusterHits", &fClusterHits);
  fANNIETree->Branch("Cluster_HitX", &fCluster_HitX);
  fANNIETree->Branch("Cluster_HitY", &fCluster_HitY);
  fANNIETree->Branch("Cluster_HitZ", &fCluster_HitZ);
  fANNIETree->Branch("Cluster_HitT", &fCluster_HitT);
  fANNIETree->Branch("Cluster_HitQ", &fCluster_HitQ);
  fANNIETree->Branch("Cluster_HitPE", &fCluster_HitPE);
  fANNIETree->Branch("Cluster_HitType", &fCluster_HitType);
  fANNIETree->Branch("Cluster_HitDetID", &fCluster_HitDetID);
  fANNIETree->Branch("Cluster_HitChankey", &fCluster_HitChankey);
  fANNIETree->Branch("Cluster_HitChankeyMC", &fCluster_HitChankeyMC);

  // MRD cluster information
  fANNIETree->Branch("eventTimeMRD", &fEventTimeMRD_Tree);
  fANNIETree->Branch("MRDClusterNumber", &fMRDClusterNumber);
  fANNIETree->Branch("MRDClusterTime", &fMRDClusterTime);
  fANNIETree->Branch("MRDClusterTimeSigma", &fMRDClusterTimeSigma);
  fANNIETree->Branch("MRDClusterHitNumber", &fMRDClusterHitNumber);

  if (TankHitInfo_fill)
  {
    // Tank Hit information
    fANNIETree->Branch("nhits", &fNHits, "nhits/I");
    fANNIETree->Branch("filter", &fIsFiltered);
    fANNIETree->Branch("hitX", &fHitX);
    fANNIETree->Branch("hitY", &fHitY);
    fANNIETree->Branch("hitZ", &fHitZ);
    fANNIETree->Branch("hitT", &fHitT);
    fANNIETree->Branch("hitQ", &fHitQ);
    fANNIETree->Branch("hitPE", &fHitPE);
    fANNIETree->Branch("hitType", &fHitType);
    fANNIETree->Branch("hitDetID", &fHitDetID);
    fANNIETree->Branch("hitChankey", &fHitChankey);
    fANNIETree->Branch("hitChankeyMC", &fHitChankeyMC);
  }

  if (SiPMPulseInfo_fill)
  {
    fANNIETree->Branch("SiPMhitQ", &fSiPMHitQ);
    fANNIETree->Branch("SiPMhitT", &fSiPMHitT);
    fANNIETree->Branch("SiPMhitAmplitude", &fSiPMHitAmplitude);
    fANNIETree->Branch("SiPMNum", &fSiPMNum);
    fANNIETree->Branch("SiPM1NPulses", &fSiPM1NPulses, "SiPM1NPulses/I");
    fANNIETree->Branch("SiPM2NPulses", &fSiPM2NPulses, "SiPM2NPulses/I");
  }

  if (LAPPDData_fill)
  {
    fANNIETree->Branch("LAPPD_ID", &fLAPPD_ID);
    fANNIETree->Branch("fLAPPD_Count", &fLAPPD_Count);
    fANNIETree->Branch("LAPPD_Beamgate_ns", &fLAPPD_Beamgate_ns);
    fANNIETree->Branch("LAPPD_Timestamp_ns", &fLAPPD_Timestamp_ns);
    fANNIETree->Branch("LAPPD_Beamgate_Raw", &fLAPPD_Beamgate_Raw);
    fANNIETree->Branch("LAPPD_Timestamp_Raw", &fLAPPD_Timestamp_Raw);
    fANNIETree->Branch("LAPPD_Offset", &fLAPPD_Offset);
    fANNIETree->Branch("LAPPD_TSCorrection", &fLAPPD_TSCorrection);
    fANNIETree->Branch("LAPPD_BGCorrection", &fLAPPD_BGCorrection);
    fANNIETree->Branch("LAPPD_OSInMinusPS", &fLAPPD_OSInMinusPS);
    if (LAPPD_PPS_fill)
    {
      fANNIETree->Branch("LAPPD_BGPPSBefore", &fLAPPD_BGPPSBefore);
      fANNIETree->Branch("LAPPD_BGPPSAfter", &fLAPPD_BGPPSAfter);
      fANNIETree->Branch("LAPPD_BGPPSDiff", &fLAPPD_BGPPSDiff);
      fANNIETree->Branch("LAPPD_BGPPSMissing", &fLAPPD_BGPPSMissing);
      fANNIETree->Branch("LAPPD_TSPPSBefore", &fLAPPD_TSPPSBefore);
      fANNIETree->Branch("LAPPD_TSPPSAfter", &fLAPPD_TSPPSAfter);
      fANNIETree->Branch("LAPPD_TSPPSDiff", &fLAPPD_TSPPSDiff);
      fANNIETree->Branch("LAPPD_TSPPSMissing", &fLAPPD_TSPPSMissing);
    }
    fANNIETree->Branch("LAPPD_BG_switchBit0", &fLAPPD_BG_switchBit0);
    fANNIETree->Branch("LAPPD_BG_switchBit1", &fLAPPD_BG_switchBit1);
  }

  // LAPPD reconstruction information
  if (LAPPDReco_fill)
  {
    // fANNIETree->Branch("LAPPDPulseTimeStampUL", &fLAPPDPulseTimeStampUL, "LAPPDPulseTimeStampUL/l");
    // fANNIETree->Branch("LAPPDPulseBeamgateUL", &fLAPPDPulseBeamgateUL, "LAPPDPulseBeamgateUL/l");
    //  actually we don't need these two for pulse and hit, because it is impossible to have multiple events from one LAPPD in one ANNIEEvent due to the dead time and the trigger scheme
    //  LAPPD ID is enough to find the corresponding unix timestamp
    //  but leave them there in case

    fANNIETree->Branch("LAPPD_PulseIDs", &fLAPPD_IDs);
    fANNIETree->Branch("LAPPD_ChannelID", &fChannelID);
    fANNIETree->Branch("LAPPD_PeakTime", &fPulsePeakTime);
    fANNIETree->Branch("LAPPD_PulseHalfHeightTime", &fPulseHalfHeightTime);
    fANNIETree->Branch("LAPPD_PeakAmp", &fPulsePeakAmp);
    fANNIETree->Branch("LAPPD_Charge", &fPulseCharge);
    fANNIETree->Branch("LAPPD_PulseStart", &fPulseStart);
    fANNIETree->Branch("LAPPD_PulseEnd", &fPulseEnd);
    fANNIETree->Branch("LAPPD_PulseWidth", &fPulseWidth);
    fANNIETree->Branch("LAPPD_PulseSide", &fPulseSide);
    fANNIETree->Branch("LAPPD_PulseStripNum", &fPulseStripNum);

    // fANNIETree->Branch("LAPPDHitTimeStampUL", &fLAPPDHitTimeStampUL, "LAPPDHitTimeStampUL/l");
    // fANNIETree->Branch("LAPPDHitBeamgateUL", &fLAPPDHitBeamgateUL, "LAPPDHitBeamgateUL/l");

    fANNIETree->Branch("LAPPDID_Hit", &fLAPPDHit_IDs);
    fANNIETree->Branch("LAPPDHitChannel", &fLAPPDHitChannel);
    fANNIETree->Branch("LAPPDHitStrip", &fLAPPDHitStrip);
    fANNIETree->Branch("LAPPDHitTime", &fLAPPDHitTime);
    fANNIETree->Branch("LAPPDHitAmp", &fLAPPDHitAmp);
    fANNIETree->Branch("LAPPDHitParallelPos", &fLAPPDHitParallelPos);
    fANNIETree->Branch("LAPPDHitTransversePos", &fLAPPDHitTransversePos);
    fANNIETree->Branch("LAPPDHitP1StartTime", &fLAPPDHitP1StartTime);
    fANNIETree->Branch("LAPPDHitP2StartTime", &fLAPPDHitP2StartTime);
    fANNIETree->Branch("LAPPDHitP1EndTime", &fLAPPDHitP1EndTime);
    fANNIETree->Branch("LAPPDHitP2EndTime", &fLAPPDHitP2EndTime);

    fANNIETree->Branch("LAPPDHitP1PeakTime", &fLAPPDHitP1PeakTime);
    fANNIETree->Branch("LAPPDHitP2PeakTime", &fLAPPDHitP2PeakTime);
    fANNIETree->Branch("LAPPDHitP1PeakAmp", &fLAPPDHitP1PeakAmp);
    fANNIETree->Branch("LAPPDHitP2PeakAmp", &fLAPPDHitP2PeakAmp);
    fANNIETree->Branch("LAPPDHitP1HalfHeightTime", &fLAPPDHitP1HalfHeightTime);
    fANNIETree->Branch("LAPPDHitP2HalfHeightTime", &fLAPPDHitP2HalfHeightTime);
    fANNIETree->Branch("LAPPDHitP1HalfEndTime", &fLAPPDHitP1HalfEndTime);
    fANNIETree->Branch("LAPPDHitP2HalfEndTime", &fLAPPDHitP2HalfEndTime);
    fANNIETree->Branch("LAPPDHitP1Charge", &fLAPPDHitP1Charge);
    fANNIETree->Branch("LAPPDHitP2Charge", &fLAPPDHitP2Charge);

    /*
    fANNIETree->Branch("LAPPDWaveformChankey", &LAPPDWaveformChankey, "LAPPDWaveformChankey/I");
    fANNIETree->Branch("WaveformMax", &waveformMaxValue, "WaveformMax/D");
    fANNIETree->Branch("WaveformRMS", &waveformRMSValue, "WaveformRMS/D");
    fANNIETree->Branch("WaveformMaxTimeBin", &waveformMaxTimeBinValue, "WaveformMaxTimeBin/I");
    fANNIETree->Branch("waveformMaxFoundNear", &waveformMaxFoundNear, "waveformMaxFoundNear/O"); // O is boolean
    fANNIETree->Branch("WaveformMaxNearing", &waveformMaxNearingValue, "WaveformMaxNearing/D");
    */
  }

  LAPPDWaveformInputLabel = "BLsubtractedLAPPDData";
  m_variables.Get("LAPPDWaveformInputLabel", LAPPDWaveformInputLabel);
  if (LAPPD_Waveform_fill)
  {
    // Log("ANNIEEventTreeMaker Tool: LAPPDWaveformInputLabel = " + LAPPDWaveformInputLabel, 0, ANNIEEventTreeMakerVerbosity);
    fANNIETree->Branch("LAPPDWaveform", &fLAPPDWaveforms);
  }

  if (LAPPD_MC_fill)
  {
    fANNIETree->Branch("LAPPDMCHitTubeIDs", &fLAPPDMCHitTubeIDs);
    fANNIETree->Branch("LAPPDMCHitChankeys", &fLAPPDMCHitChankeys);
    fANNIETree->Branch("LAPPDMCHitTime", &fLAPPDMCHitTime);
    fANNIETree->Branch("LAPPDMCHitCharge", &fLAPPDMCHitCharge);
    fANNIETree->Branch("LAPPDMCHitX", &fLAPPDMCHitX);
    fANNIETree->Branch("LAPPDMCHitY", &fLAPPDMCHitY);
    fANNIETree->Branch("LAPPDMCHitZ", &fLAPPDMCHitZ);
    fANNIETree->Branch("LAPPDMCHitParallelPos", &fLAPPDMCHitParallelPos);
    fANNIETree->Branch("LAPPDMCHitTransversePos", &fLAPPDMCHitTransversePos);
  }

  if (MRDHitInfo_fill)
  {
    fANNIETree->Branch("MRDHitClusterIndex", &fMRDHitClusterIndex);
    fANNIETree->Branch("MRDhitT", &fMRDHitT);
    fANNIETree->Branch("MRDHitCharge", &fMRDHitCharge);
    fANNIETree->Branch("MRDHitDigitPMT", &fMRDHitDigitPMT);

    fANNIETree->Branch("MRDhitDetID", &fMRDHitDetID);
    fANNIETree->Branch("MRDhitChankey", &fMRDHitChankey);
    fANNIETree->Branch("MRDhitChankeyMC", &fMRDHitChankeyMC);
    fANNIETree->Branch("FMVhitT", &fFMVHitT);
    fANNIETree->Branch("FMVhitDetID", &fFMVHitDetID);
    fANNIETree->Branch("FMVhitChankey", &fFMVHitChankey);
    fANNIETree->Branch("FMVhitChankeyMC", &fFMVHitChankeyMC);
    fANNIETree->Branch("vetoHit", &fVetoHit, "vetoHit/I");
  }

  if (MRDReco_fill)
  {
    fANNIETree->Branch("MRDClusterIndex", &fMRDClusterIndex);
    fANNIETree->Branch("NumClusterTracks", &fNumClusterTracks);
    fANNIETree->Branch("MRDTrackAngle", &fMRDTrackAngle);
    fANNIETree->Branch("MRDTrackAngleError", &fMRDTrackAngleError);
    fANNIETree->Branch("MRDPenetrationDepth", &fMRDPenetrationDepth);
    fANNIETree->Branch("MRDTrackLength", &fMRDTrackLength);
    fANNIETree->Branch("MRDEntryPointRadius", &fMRDEntryPointRadius);
    fANNIETree->Branch("MRDEnergyLoss", &fMRDEnergyLoss);
    fANNIETree->Branch("MRDEnergyLossError", &fMRDEnergyLossError);
    fANNIETree->Branch("MRDTrackStartX", &fMRDTrackStartX);
    fANNIETree->Branch("MRDTrackStartY", &fMRDTrackStartY);
    fANNIETree->Branch("MRDTrackStartZ", &fMRDTrackStartZ);
    fANNIETree->Branch("MRDTrackStopX", &fMRDTrackStopX);
    fANNIETree->Branch("MRDTrackStopY", &fMRDTrackStopY);
    fANNIETree->Branch("MRDTrackStopZ", &fMRDTrackStopZ);
    fANNIETree->Branch("MRDSide", &fMRDSide);
    fANNIETree->Branch("MRDStop", &fMRDStop);
    fANNIETree->Branch("MRDThrough", &fMRDThrough);
  }

  fANNIETree->Branch("eventStatusApplied", &fEventStatusApplied, "eventStatusApplied/I");
  fANNIETree->Branch("eventStatusFlagged", &fEventStatusFlagged, "eventStatusFlagged/I");

  // MC truth information for muons
  // Output to tree when MCTruth_fill = 1 in config
  if (MCTruth_fill)
  {
    fTrueNeutCapVtxX = new std::vector<double>;
    fTrueNeutCapVtxY = new std::vector<double>;
    fTrueNeutCapVtxZ = new std::vector<double>;
    fTrueNeutCapNucleus = new std::vector<double>;
    fTrueNeutCapTime = new std::vector<double>;
    fTrueNeutCapGammas = new std::vector<double>;
    fTrueNeutCapE = new std::vector<double>;
    fTrueNeutCapGammaE = new std::vector<double>;
    fTruePrimaryPdgs = new std::vector<int>;
    fANNIETree->Branch("triggerNumber", &fiMCTriggerNum, "triggerNumber/I");
    fANNIETree->Branch("mcEntryNumber", &fMCEventNum, "mcEntryNumber/I");
    fANNIETree->Branch("trueVtxX", &fTrueVtxX, "trueVtxX/D");
    fANNIETree->Branch("trueVtxY", &fTrueVtxY, "trueVtxY/D");
    fANNIETree->Branch("trueVtxZ", &fTrueVtxZ, "trueVtxZ/D");
    fANNIETree->Branch("trueVtxTime", &fTrueVtxTime, "trueVtxTime/D");
    fANNIETree->Branch("trueDirX", &fTrueDirX, "trueDirX/D");
    fANNIETree->Branch("trueDirY", &fTrueDirY, "trueDirY/D");
    fANNIETree->Branch("trueDirZ", &fTrueDirZ, "trueDirZ/D");
    fANNIETree->Branch("trueAngle", &fTrueAngle, "trueAngle/D");
    fANNIETree->Branch("truePhi", &fTruePhi, "truePhi/D");
    fANNIETree->Branch("trueMuonEnergy", &fTrueMuonEnergy, "trueMuonEnergy/D");
    fANNIETree->Branch("truePrimaryPdg", &fTruePrimaryPdg, "truePrimaryPdg/I");
    fANNIETree->Branch("trueTrackLengthInWater", &fTrueTrackLengthInWater, "trueTrackLengthInWater/D");
    fANNIETree->Branch("trueTrackLengthInMRD", &fTrueTrackLengthInMRD, "trueTrackLengthInMRD/D");
    fANNIETree->Branch("trueMultiRing", &fTrueMultiRing, "trueMultiRing/I");
    fANNIETree->Branch("Pi0Count", &fPi0Count, "Pi0Count/I");
    fANNIETree->Branch("PiPlusCount", &fPiPlusCount, "PiPlusCount/I");
    fANNIETree->Branch("PiMinusCount", &fPiMinusCount, "PiMinusCount/I");
    fANNIETree->Branch("K0Count", &fK0Count, "K0Count/I");
    fANNIETree->Branch("KPlusCount", &fKPlusCount, "KPlusCount/I");
    fANNIETree->Branch("KMinusCount", &fKMinusCount, "KMinusCount/I");
    fANNIETree->Branch("truePrimaryPdgs", &fTruePrimaryPdgs);
    fANNIETree->Branch("trueNeutCapVtxX", &fTrueNeutCapVtxX);
    fANNIETree->Branch("trueNeutCapVtxY", &fTrueNeutCapVtxY);
    fANNIETree->Branch("trueNeutCapVtxZ", &fTrueNeutCapVtxZ);
    fANNIETree->Branch("trueNeutCapNucleus", &fTrueNeutCapNucleus);
    fANNIETree->Branch("trueNeutCapTime", &fTrueNeutCapTime);
    fANNIETree->Branch("trueNeutCapGammas", &fTrueNeutCapGammas);
    fANNIETree->Branch("trueNeutCapE", &fTrueNeutCapE);
    fANNIETree->Branch("trueNeutCapGammaE", &fTrueNeutCapGammaE);
    fANNIETree->Branch("trueNeutrinoEnergy", &fTrueNeutrinoEnergy, "trueNeutrinoEnergy/D");
    fANNIETree->Branch("trueNeutrinoMomentum_X", &fTrueNeutrinoMomentum_X, "trueNeutrinoMomentum_X/D");
    fANNIETree->Branch("trueNeutrinoMomentum_Y", &fTrueNeutrinoMomentum_Y, "trueNeutrinoMomentum_Y/D");
    fANNIETree->Branch("trueNeutrinoMomentum_Z", &fTrueNeutrinoMomentum_Z, "trueNeutrinoMomentum_Z/D");
    fANNIETree->Branch("trueNuIntxVtx_X", &fTrueNuIntxVtx_X, "trueNuIntxVtx_X/D");
    fANNIETree->Branch("trueNuIntxVtx_Y", &fTrueNuIntxVtx_Y, "trueNuIntxVtx_Y/D");
    fANNIETree->Branch("trueNuIntxVtx_Z", &fTrueNuIntxVtx_Z, "trueNuIntxVtx_Z/D");
    fANNIETree->Branch("trueNuIntxVtx_T", &fTrueNuIntxVtx_T, "trueNuIntxVtx_T/D");
    fANNIETree->Branch("trueFSLVtx_X", &fTrueFSLVtx_X, "trueFSLVtx_X/D");
    fANNIETree->Branch("trueFSLVtx_Y", &fTrueFSLVtx_Y, "trueFSLVtx_Y/D");
    fANNIETree->Branch("trueFSLVtx_Z", &fTrueFSLVtx_Z, "trueFSLVtx_Z/D");
    fANNIETree->Branch("trueFSLMomentum_X", &fTrueFSLMomentum_X, "trueFSLMomentum_X/D");
    fANNIETree->Branch("trueFSLMomentum_Y", &fTrueFSLMomentum_Y, "trueFSLMomentum_Y/D");
    fANNIETree->Branch("trueFSLMomentum_Z", &fTrueFSLMomentum_Z, "trueFSLMomentum_Z/D");
    fANNIETree->Branch("trueFSLTime", &fTrueFSLTime, "trueFSLTime/D");
    fANNIETree->Branch("trueFSLMass", &fTrueFSLMass, "trueFSLMass/D");
    fANNIETree->Branch("trueFSLPdg", &fTrueFSLPdg, "trueFSLPdg/I");
    fANNIETree->Branch("trueFSLEnergy", &fTrueFSLEnergy, "trueFSLEnergy/D");
    fANNIETree->Branch("trueQ2", &fTrueQ2, "trueQ2/D");
    fANNIETree->Branch("trueCC", &fTrueCC, "trueCC/I");
    fANNIETree->Branch("trueNC", &fTrueNC, "trueNC/I");
    fANNIETree->Branch("trueQEL", &fTrueQEL, "trueQEL/I");
    fANNIETree->Branch("trueRES", &fTrueRES, "trueRES/I");
    fANNIETree->Branch("trueDIS", &fTrueDIS, "trueDIS/I");
    fANNIETree->Branch("trueCOH", &fTrueCOH, "trueCOH/I");
    fANNIETree->Branch("trueMEC", &fTrueMEC, "trueMEC/I");
    fANNIETree->Branch("trueNeutrons", &fTrueNeutrons, "trueNeutrons/I");
    fANNIETree->Branch("trueProtons", &fTrueProtons, "trueProtons/I");
    fANNIETree->Branch("truePi0", &fTruePi0, "truePi0/I");
    fANNIETree->Branch("truePiPlus", &fTruePiPlus, "truePiPlus/I");
    fANNIETree->Branch("truePiPlusCher", &fTruePiPlusCher, "truePiPlusCher/I");
    fANNIETree->Branch("truePiMinus", &fTruePiMinus, "truePiMinus/I");
    fANNIETree->Branch("truePiMinusCher", &fTruePiMinusCher, "truePiMinusCher/I");
    fANNIETree->Branch("trueKPlus", &fTrueKPlus, "trueKPlus/I");
    fANNIETree->Branch("trueKPlusCher", &fTrueKPlusCher, "trueKPlusCher/I");
    fANNIETree->Branch("trueKMinus", &fTrueKMinus, "trueKMinus/I");
    fANNIETree->Branch("trueKMinusCher", &fTrueKMinusCher, "trueKMinusCher/I");
  }

  // Reconstructed variables after full Muon Reco Analysis
  if (TankReco_fill)
  {
    fANNIETree->Branch("recoVtxX", &fRecoVtxX, "recoVtxX/D");
    fANNIETree->Branch("recoVtxY", &fRecoVtxY, "recoVtxY/D");
    fANNIETree->Branch("recoVtxZ", &fRecoVtxZ, "recoVtxZ/D");
    fANNIETree->Branch("recoVtxTime", &fRecoVtxTime, "recoVtxTime/D");
    fANNIETree->Branch("recoDirX", &fRecoDirX, "recoDirX/D");
    fANNIETree->Branch("recoDirY", &fRecoDirY, "recoDirY/D");
    fANNIETree->Branch("recoDirZ", &fRecoDirZ, "recoDirZ/D");
    fANNIETree->Branch("recoAngle", &fRecoAngle, "recoAngle/D");
    fANNIETree->Branch("recoPhi", &fRecoPhi, "recoPhi/D");
    fANNIETree->Branch("recoVtxFOM", &fRecoVtxFOM, "recoVtxFOM/D");
    fANNIETree->Branch("recoStatus", &fRecoStatus, "recoStatus/I");
  }

  // Reconstructed variables from each step in Muon Reco Analysis
  // Currently output when RecoDebug_fill = 1 in config
  if (RecoDebug_fill)
  {
    fANNIETree->Branch("seedVtxX", &fSeedVtxX);
    fANNIETree->Branch("seedVtxY", &fSeedVtxY);
    fANNIETree->Branch("seedVtxZ", &fSeedVtxZ);
    fANNIETree->Branch("seedVtxFOM", &fSeedVtxFOM);
    fANNIETree->Branch("seedVtxTime", &fSeedVtxTime, "seedVtxTime/D");

    fANNIETree->Branch("pointPosX", &fPointPosX, "pointPosX/D");
    fANNIETree->Branch("pointPosY", &fPointPosY, "pointPosY/D");
    fANNIETree->Branch("pointPosZ", &fPointPosZ, "pointPosZ/D");
    fANNIETree->Branch("pointPosTime", &fPointPosTime, "pointPosTime/D");
    fANNIETree->Branch("pointPosFOM", &fPointPosFOM, "pointPosFOM/D");
    fANNIETree->Branch("pointPosStatus", &fPointPosStatus, "pointPosStatus/I");

    fANNIETree->Branch("pointDirX", &fPointDirX, "pointDirX/D");
    fANNIETree->Branch("pointDirY", &fPointDirY, "pointDirY/D");
    fANNIETree->Branch("pointDirZ", &fPointDirZ, "pointDirZ/D");
    fANNIETree->Branch("pointDirTime", &fPointDirTime, "pointDirTime/D");
    fANNIETree->Branch("pointDirStatus", &fPointDirStatus, "pointDirStatus/I");
    fANNIETree->Branch("pointDirFOM", &fPointDirFOM, "pointDirFOM/D");

    fANNIETree->Branch("pointVtxPosX", &fPointVtxPosX, "pointVtxPosX/D");
    fANNIETree->Branch("pointVtxPosY", &fPointVtxPosY, "pointVtxPosY/D");
    fANNIETree->Branch("pointVtxPosZ", &fPointVtxPosZ, "pointVtxPosZ/D");
    fANNIETree->Branch("pointVtxTime", &fPointVtxTime, "pointVtxTime/D");
    fANNIETree->Branch("pointVtxDirX", &fPointVtxDirX, "pointVtxDirX/D");
    fANNIETree->Branch("pointVtxDirY", &fPointVtxDirY, "pointVtxDirY/D");
    fANNIETree->Branch("pointVtxDirZ", &fPointVtxDirZ, "pointVtxDirZ/D");
    fANNIETree->Branch("pointVtxFOM", &fPointVtxFOM, "pointVtxFOM/D");
    fANNIETree->Branch("pointVtxStatus", &fPointVtxStatus, "pointVtxStatus/I");
  }

  // Difference in MC Truth and Muon Reconstruction Analysis
  // Output to tree when muonTruthRecoDiff_fill = 1 in config
  if (muonTruthRecoDiff_fill)
  {
    fANNIETree->Branch("deltaVtxX", &fDeltaVtxX, "deltaVtxX/D");
    fANNIETree->Branch("deltaVtxY", &fDeltaVtxY, "deltaVtxY/D");
    fANNIETree->Branch("deltaVtxZ", &fDeltaVtxZ, "deltaVtxZ/D");
    fANNIETree->Branch("deltaVtxR", &fDeltaVtxR, "deltaVtxR/D");
    fANNIETree->Branch("deltaVtxT", &fDeltaVtxT, "deltaVtxT/D");
    fANNIETree->Branch("deltaParallel", &fDeltaParallel, "deltaParallel/D");
    fANNIETree->Branch("deltaPerpendicular", &fDeltaPerpendicular, "deltaPerpendicular/D");
    fANNIETree->Branch("deltaAzimuth", &fDeltaAzimuth, "deltaAzimuth/D");
    fANNIETree->Branch("deltaZenith", &fDeltaZenith, "deltaZenith/D");
    fANNIETree->Branch("deltaAngle", &fDeltaAngle, "deltaAngle/D");
  }

  return true;
}

bool ANNIEEventTreeMaker::Execute()
{
  //****************************** Reset Variables *************************************//
  ResetVariables();

  //****************************** fillCleanEventsOnly *************************************//
  //  If only clean events are built, return true for dirty events
  auto get_flagsapp = m_data->Stores.at("RecoEvent")->Get("EventFlagApplied", fEventStatusApplied);
  auto get_flags = m_data->Stores.at("RecoEvent")->Get("EventFlagged", fEventStatusFlagged);

  if (fillCleanEventsOnly)
  {
    // auto get_cutstatus = m_data->Stores.at("RecoEvent")->Get("EventCutStatus",fEventCutStatus);
    if (!get_flagsapp || !get_flags)
    {
      Log("PhaseITreeMaker tool: No Event status applied or flagged bitmask!!", v_error, ANNIEEventTreeMakerVerbosity);
      return false;
    }
    // check if event passes the cut
    if ((fEventStatusFlagged) != 0)
    {
      //  if (!fEventCutStatus){
      Log("ANNIEEventTreeMaker Tool: Event was flagged with one of the active cuts.", v_debug, ANNIEEventTreeMakerVerbosity);
      return true;
    }
  }
  // done

  //****************************** Fill Event Info *************************************//
  bool keepLoading = LoadEventInfo();
  if (!keepLoading)
    return false;
  LoadBeamInfo();
  // done

  //****************************** Fill RWM BRF Info *************************************//
  if (RWMBRF_fill)
  {
    LoadRWMBRFInfo();
  }

  //****************************** Fill Hit Info *************************************//
  if (TankHitInfo_fill)
  {
    // this will fill all hits in this event
    LoadAllTankHits();
  }
  if (SiPMPulseInfo_fill)
  {
    LoadSiPMHits();
  }
  // done
  //****************************** Fill Cluster Info *************************************//
  if (TankCluster_fill)
  {
    LoadClusterInfo();
  }

  //****************************** Fill LAPPD Info *************************************//
  if (LAPPDData_fill)
  {
    LoadLAPPDInfo();
  }
  if (LAPPD_Waveform_fill)
  {
    FillLAPPDWaveform();
  }

  if (LAPPD_MC_fill)
  {
    FillLAPPDMCHitInfo();
  }

  //****************************** Fill MRD Info *************************************//
  if (MRDHitInfo_fill)
  {
    LoadMRDCluster();
  }

  //****************************** Fill Reco Info *************************************//
  bool got_reco = false;
  if (TankReco_fill)
  {
    got_reco = FillTankRecoInfo();
  }

  if (RecoDebug_fill)
    FillRecoDebugInfo();

  //****************************** Fill MCTruth Info *************************************//
  bool gotmctruth = false;
  if (MCTruth_fill)
  {
    gotmctruth = FillMCTruthInfo(); // todo
  }
  if (muonTruthRecoDiff_fill)
    this->FillTruthRecoDiffInfo(gotmctruth, got_reco);

  //****************************** Fill Reco Summary *************************************//
  if (got_reco && gotmctruth && (ANNIEEventTreeMakerVerbosity > 4))
  {
    RecoSummary();
  }

  //****************************** Fill Tree *************************************//
  fANNIETree->Fill();

  processedEvents++;
  return true;
}

bool ANNIEEventTreeMaker::Finalise()
{
  Log("ANNIEEventTreeMaker Tool: Got " + std::to_string(processedEvents) + " events", 0, ANNIEEventTreeMakerVerbosity);
  fOutput_tfile->cd();
  fANNIETree->Write();
  fOutput_tfile->Close();
  Log("ANNIEEventTreeMaker Tool: Tree written to file", 0, ANNIEEventTreeMakerVerbosity);

  return true;
}

void ANNIEEventTreeMaker::ResetVariables()
{
  // Reset all variables

  // Event Info
  fRunNumber = 0;
  fSubrunNumber = 0;
  fPartFileNumber = 0;
  fEventNumber = 0;
  fPrimaryTriggerWord = 0;
  fPrimaryTriggerTime = 0;
  fTriggerword = 0;
  fExtended = 0;
  fTankMRDCoinc = 0;
  fNoVeto = 0;
  fHasTank = 0;
  fHasMRD = 0;
  fHasLAPPD = 0;
  fGroupedTriggerTime.clear();
  fGroupedTriggerWord.clear();
  fDataStreams.clear();
  fEventTimeTank = 0;
  fEventTimeMRD = 0;
  fMRDTriggerType = "";
  fMRDTriggerTypeInt = -9999;

  // beam info
  fPot = -9999;
  fBeamok = 0;
  fBunchRotationOn = 0;
  beam_E_TOR860 = -9999;
  beam_E_TOR875 = -9999;
  beam_THCURR = -9999;
  beam_BTJT2 = -9999;
  beam_HP875 = -9999;
  beam_VP875 = -9999;
  beam_HPTG1 = -9999;
  beam_VPTG1 = -9999;
  beam_HPTG2 = -9999;
  beam_VPTG2 = -9999;
  beam_BTH2T2 = -9999;
  beam_B_BRRMPL = -9999;
  beam_B_BRRMPQ = -9999;
  beam_B_BRRMPS = -9999;
  beam_B_BRRMP = -9999;
  fBeamInfoTime = 0;
  fBeamInfoTimeToTriggerDiff = -9999;

  // RWM BRF info
  fRWMRisingStart = -9999;
  fRWMRisingEnd = -9999;
  fRWMHalfRising = -9999;
  fRWMFHWM = -9999;
  fRWMFirstPeak = -9999;
  fBRFFirstPeak = -9999;
  fBRFAveragePeak = -9999;
  fBRFFirstPeakFit = -9999;

  // TankHitInfo
  fNHits = 0;
  fIsFiltered.clear();
  fHitX.clear();
  fHitY.clear();
  fHitZ.clear();
  fHitT.clear();
  fHitQ.clear();
  fHitPE.clear();
  fHitType.clear();
  fHitDetID.clear();
  fHitChankey.clear();
  fHitChankeyMC.clear();

  // SiPMPulse Info
  fSiPM1NPulses = 0;
  fSiPM2NPulses = 0;
  fSiPMHitQ.clear();
  fSiPMHitT.clear();
  fSiPMHitAmplitude.clear();
  fSiPMNum.clear();

  // LAPPDData_fill
  fLAPPD_Count = 0;
  fLAPPD_ID.clear();
  fLAPPD_Beamgate_ns.clear();
  fLAPPD_Timestamp_ns.clear();
  fLAPPD_Beamgate_Raw.clear();
  fLAPPD_Timestamp_Raw.clear();
  fLAPPD_Offset.clear();
  fLAPPD_TSCorrection.clear();
  fLAPPD_BGCorrection.clear();
  fLAPPD_OSInMinusPS.clear();
  fLAPPD_BG_switchBit0.clear();
  fLAPPD_BG_switchBit1.clear();
  // LAPPD_PPS_fill
  fLAPPD_BGPPSBefore.clear();
  fLAPPD_BGPPSAfter.clear();
  fLAPPD_BGPPSDiff.clear();
  fLAPPD_BGPPSMissing.clear();
  fLAPPD_TSPPSBefore.clear();
  fLAPPD_TSPPSAfter.clear();
  fLAPPD_TSPPSDiff.clear();
  fLAPPD_TSPPSMissing.clear();

  // LAPPD Reco Fill
  fLAPPDPulseTimeStampUL.clear();
  fLAPPDPulseBeamgateUL.clear();
  fLAPPD_IDs.clear();
  fChannelID.clear();
  fPulsePeakTime.clear();
  fPulseHalfHeightTime.clear();
  fPulseCharge.clear();
  fPulsePeakAmp.clear();
  fPulseStart.clear();
  fPulseEnd.clear();
  fPulseWidth.clear();
  fPulseSide.clear();
  fPulseStripNum.clear();
  fChannelBaseline.clear();

  fLAPPDHitTimeStampUL.clear();
  fLAPPDHitBeamgateUL.clear();
  fLAPPDHit_IDs.clear();
  fLAPPDHitChannel.clear();
  fLAPPDHitStrip.clear();
  fLAPPDHitTime.clear();
  fLAPPDHitAmp.clear();
  fLAPPDHitParallelPos.clear();
  fLAPPDHitTransversePos.clear();
  fLAPPDHitP1StartTime.clear();
  fLAPPDHitP2StartTime.clear();
  fLAPPDHitP1EndTime.clear();
  fLAPPDHitP2EndTime.clear();
  fLAPPDHitP1PeakTime.clear();
  fLAPPDHitP2PeakTime.clear();
  fLAPPDHitP1PeakAmp.clear();
  fLAPPDHitP2PeakAmp.clear();
  fLAPPDHitP1HalfHeightTime.clear();
  fLAPPDHitP2HalfHeightTime.clear();
  fLAPPDHitP1HalfEndTime.clear();
  fLAPPDHitP2HalfEndTime.clear();
  fLAPPDHitP1Charge.clear();
  fLAPPDHitP2Charge.clear();

  LAPPDWaveformChankey.clear();
  waveformMaxValue.clear();
  waveformRMSValue.clear();
  waveformMaxFoundNear.clear();
  waveformMaxNearingValue.clear();
  waveformMaxTimeBinValue.clear();

  // LAPPD waveform fill
  fLAPPDWaveforms.clear();

  // LAPPD MC fill
  fLAPPDMCHitTubeIDs.clear();
  fLAPPDMCHitChankeys.clear();
  fLAPPDMCHitTime.clear();
  fLAPPDMCHitCharge.clear();
  fLAPPDMCHitX.clear();
  fLAPPDMCHitY.clear();
  fLAPPDMCHitZ.clear();
  fLAPPDMCHitParallelPos.clear();
  fLAPPDMCHitTransversePos.clear();

  // tank cluster information
  fNumberOfClusters = 0;

  fClusterHits.clear();
  fClusterChargeV.clear();
  fClusterTimeV.clear();
  fClusterPEV.clear();

  fCluster_HitX.clear();
  fCluster_HitY.clear();
  fCluster_HitZ.clear();
  fCluster_HitT.clear();
  fCluster_HitQ.clear();
  fCluster_HitPE.clear();
  fCluster_HitType.clear();
  fCluster_HitDetID.clear();
  fCluster_HitChankey.clear();
  fCluster_HitChankeyMC.clear();

  fClusterMaxPEV.clear();
  fClusterChargePointXV.clear();
  fClusterChargePointYV.clear();
  fClusterChargePointZV.clear();
  fClusterChargeBalanceV.clear();

  // MRD cluster information
  fEventTimeMRD_Tree = 0;
  fMRDClusterNumber = 0;
  fMRDClusterHitNumber.clear();
  fMRDClusterTime.clear();
  fMRDClusterTimeSigma.clear();

  fVetoHit = 0;
  fMRDHitClusterIndex.clear();
  fMRDHitT.clear();
  fMRDHitCharge.clear();
  fMRDHitDigitPMT.clear();
  fMRDHitDetID.clear();
  fMRDHitChankey.clear();
  fMRDHitChankeyMC.clear();
  fFMVHitT.clear();
  fFMVHitDetID.clear();
  fFMVHitChankey.clear();
  fFMVHitChankeyMC.clear();

  fNumMRDClusterTracks = 0;
  fMRDTrackAngle.clear();
  fMRDTrackAngleError.clear();
  fMRDPenetrationDepth.clear();
  fMRDTrackLength.clear();
  fMRDEntryPointRadius.clear();
  fMRDEnergyLoss.clear();
  fMRDEnergyLossError.clear();
  fMRDTrackStartX.clear();
  fMRDTrackStartY.clear();
  fMRDTrackStartZ.clear();
  fMRDTrackStopX.clear();
  fMRDTrackStopY.clear();
  fMRDTrackStopZ.clear();
  fMRDSide.clear();
  fMRDStop.clear();
  fMRDThrough.clear();
  fMRDClusterIndex.clear();
  fNumClusterTracks.clear();

  // fillCleanEventsOnly
  fEventStatusApplied = 0;
  fEventStatusFlagged = 0;

  // MCTruth_fill
  if (MCTruth_fill)
  {
    fMCEventNum = 0;
    fMCTriggerNum = 0;
    fiMCTriggerNum = -9999;

    fTrueVtxX = -9999;
    fTrueVtxY = -9999;
    fTrueVtxZ = -9999;
    fTrueVtxTime = -9999;
    fTrueDirX = -9999;
    fTrueDirY = -9999;
    fTrueDirZ = -9999;
    fTrueAngle = -9999;
    fTruePhi = -9999;
    fTrueMuonEnergy = -9999;
    fTruePrimaryPdg = -9999;
    fTrueTrackLengthInWater = -9999;
    fTrueTrackLengthInMRD = -9999;
    fTruePrimaryPdgs->clear();
    fTrueNeutCapVtxX->clear();
    fTrueNeutCapVtxY->clear();
    fTrueNeutCapVtxZ->clear();
    fTrueNeutCapNucleus->clear();
    fTrueNeutCapTime->clear();
    fTrueNeutCapGammas->clear();
    fTrueNeutCapE->clear();
    fTrueNeutCapGammaE->clear();
    fTrueMultiRing = -9999;
  }

  // Genie information for event
  fTrueNeutrinoEnergy = -9999;
  fTrueNeutrinoMomentum_X = -9999;
  fTrueNeutrinoMomentum_Y = -9999;
  fTrueNeutrinoMomentum_Z = -9999;
  fTrueNuIntxVtx_X = -9999;
  fTrueNuIntxVtx_Y = -9999;
  fTrueNuIntxVtx_Z = -9999;
  fTrueNuIntxVtx_T = -9999;
  fTrueFSLVtx_X = -9999;
  fTrueFSLVtx_Y = -9999;
  fTrueFSLVtx_Z = -9999;
  fTrueFSLMomentum_X = -9999;
  fTrueFSLMomentum_Y = -9999;
  fTrueFSLMomentum_Z = -9999;
  fTrueFSLTime = -9999;
  fTrueFSLMass = -9999;
  fTrueFSLPdg = -9999;
  fTrueFSLEnergy = -9999;
  fTrueQ2 = -9999;
  fTrueCC = -9999;
  fTrueNC = -9999;
  fTrueQEL = -9999;
  fTrueRES = -9999;
  fTrueDIS = -9999;
  fTrueCOH = -9999;
  fTrueMEC = -9999;
  fTrueNeutrons = -9999;
  fTrueProtons = -9999;
  fTruePi0 = -9999;
  fTruePiPlus = -9999;
  fTruePiPlusCher = -9999;
  fTruePiMinus = -9999;
  fTruePiMinusCher = -9999;
  fTrueKPlus = -9999;
  fTrueKPlusCher = -9999;
  fTrueKMinus = -9999;
  fTrueKMinusCher = -9999;

  // TankReco_fill
  fRecoVtxX = -9999;
  fRecoVtxY = -9999;
  fRecoVtxZ = -9999;
  fRecoVtxTime = -9999;
  fRecoVtxFOM = -9999;
  fRecoDirX = -9999;
  fRecoDirY = -9999;
  fRecoDirZ = -9999;
  fRecoAngle = -9999;
  fRecoPhi = -9999;
  fRecoStatus = -9999;

  // RecoDebug_fill
  fSeedVtxX.clear();
  fSeedVtxY.clear();
  fSeedVtxZ.clear();
  fSeedVtxFOM.clear();
  fSeedVtxTime = -9999;

  // Reco vertex
  // Point Position Vertex
  fPointVtxPosX = -9999;
  fPointVtxPosY = -9999;
  fPointVtxPosZ = -9999;
  fPointVtxTime = -9999;
  fPointPosFOM = -9999;
  fPointPosStatus = -9999;
  fPointVtxDirX = -9999;
  fPointVtxDirY = -9999;
  fPointVtxDirZ = -9999;
  fPointDirTime = -9999;
  fPointDirFOM = -9999;
  fPointDirStatus = -9999;

  // Point Vertex Finder
  fPointVtxPosX = -9999;
  fPointVtxPosY = -9999;
  fPointVtxPosZ = -9999;
  fPointVtxTime = -9999;
  fPointVtxDirX = -9999;
  fPointVtxDirY = -9999;
  fPointVtxDirZ = -9999;
  fPointVtxFOM = -9999;
  fPointVtxStatus = -9999;

  // Difference between MC and Truth
  fDeltaVtxX = -9999;
  fDeltaVtxY = -9999;
  fDeltaVtxZ = -9999;
  fDeltaVtxR = -9999;
  fDeltaVtxT = -9999;
  fDeltaParallel = -9999;
  fDeltaPerpendicular = -9999;
  fDeltaAzimuth = -9999;
  fDeltaZenith = -9999;
  fDeltaAngle = -9999;

  // Pion and kaon counts for event
  fPi0Count = 0;
  fPiPlusCount = 0;
  fPiMinusCount = 0;
  fK0Count = 0;
  fKPlusCount = 0;
  fKMinusCount = 0;

  // Event Info
  fDataStreams.clear();
  GroupedTrigger.clear();

  // LAPPDData_fill
  LAPPDDataMap.clear();
  LAPPDBeamgate_ns.clear();
  LAPPDTimeStamps_ns.clear();
  LAPPDTimeStampsRaw.clear();
  LAPPDBeamgatesRaw.clear();
  LAPPDOffsets.clear();
  LAPPDTSCorrection.clear();
  LAPPDBGCorrection.clear();
  LAPPDOSInMinusPS.clear();
  SwitchBitBG.clear();
  // LAPPD_PPS_fill
  LAPPDBG_PPSBefore.clear();
  LAPPDBG_PPSAfter.clear();
  LAPPDBG_PPSDiff.clear();
  LAPPDBG_PPSMissing.clear();
  LAPPDTS_PPSBefore.clear();
  LAPPDTS_PPSAfter.clear();
  LAPPDTS_PPSDiff.clear();
  LAPPDTS_PPSMissing.clear();

  // LAPPD Reco Fill
  lappdPulses.clear();
  lappdHits.clear();

  waveformMax.clear();
  waveformRMS.clear();
  waveformMaxLast.clear();
  waveformMaxNearing.clear();
  waveformMaxTimeBin.clear();
}

bool ANNIEEventTreeMaker::LoadEventInfo()
{
  Log("ANNIEEventTreeMaker Tool: LoadEventInfo", v_warning, ANNIEEventTreeMakerVerbosity);
  m_data->Stores["ANNIEEvent"]->Get("RunNumber", fRunNumber);
  m_data->Stores["ANNIEEvent"]->Get("SubRunNumber", fSubrunNumber);
  m_data->Stores["ANNIEEvent"]->Get("PartNumber", fPartFileNumber);
  bool gotEventNumber = m_data->Stores["ANNIEEvent"]->Get("EventNumber", fEventNumber);
  if (!gotEventNumber)
  {
    uint32_t enm = 9998;
    m_data->CStore.Get("EventNumberTree", enm);
    cout << "ANNIEEventTreeMaker Tool: Not get the event number from ANNIEEvent, get from CStore: " << enm << endl;
    fEventNumber = static_cast<int>(enm);
  }

  m_data->Stores["ANNIEEvent"]->Get("PrimaryTriggerWord", fPrimaryTriggerWord);
  if (fPrimaryTriggerWord == 14)
    trigword = 5;
  else
    trigword = fPrimaryTriggerWord;
  m_data->Stores["ANNIEEvent"]->Get("PrimaryTriggerTime", fPrimaryTriggerTime);
  m_data->Stores["ANNIEEvent"]->Get("GroupedTrigger", GroupedTrigger);
  m_data->Stores["ANNIEEvent"]->Get("TriggerWord", fTriggerword);
  m_data->Stores["ANNIEEvent"]->Get("TriggerExtended", fExtended);
  m_data->Stores["ANNIEEvent"]->Get("DataStreams", fDataStreams);

  bool pmtmrdcoinc, noveto;
  m_data->Stores["RecoEvent"]->Get("PMTMRDCoinc", pmtmrdcoinc);
  m_data->Stores["RecoEvent"]->Get("NoVeto", noveto);
  if (pmtmrdcoinc)
    fTankMRDCoinc = 1;
  else
    fTankMRDCoinc = 0;

  if (noveto)
    fNoVeto = 1;
  else
    fNoVeto = 0;

  if (fDataStreams["Tank"] == 1)
    fHasTank = 1;
  else
    fHasTank = 0;

  if (fDataStreams["MRD"] == 1)
  {
    fHasMRD = 1;
    m_data->Stores["ANNIEEvent"]->Get("MRDTriggerType", fMRDTriggerType);
    Log("ANNIEEventTreeMaker Tool: MRD Trigger Type: " + fMRDTriggerType, v_debug, ANNIEEventTreeMakerVerbosity);
    bool cosmic = fMRDTriggerType == "Cosmic";
    Log("ANNIEEventTreeMaker Tool: Cosmic Trigger: " + std::to_string(cosmic), v_debug, ANNIEEventTreeMakerVerbosity);
    if (fMRDTriggerType == "Beam")
      fMRDTriggerTypeInt = 1;
    else if (fMRDTriggerType == "Cosmic")
      fMRDTriggerTypeInt = 2;
    else
      fMRDTriggerTypeInt = 0;
  }
  else
    fHasMRD = 0;

  if (fDataStreams["LAPPD"] == 1)
    fHasLAPPD = 1;
  else
    fHasLAPPD = 0;

  for (std::map<uint64_t, uint32_t>::iterator it = GroupedTrigger.begin(); it != GroupedTrigger.end(); ++it)
  {
    uint64_t key = it->first;
    uint32_t value = it->second;

    fGroupedTriggerTime.push_back(key);
    fGroupedTriggerWord.push_back(value);
  }

  bool gotETT = m_data->Stores["ANNIEEvent"]->Get("EventTimeTank", fEventTimeTank);
  if (!gotETT)
    fEventTimeTank = 0;

  TimeClass tm;
  bool gotETMRD = m_data->Stores["ANNIEEvent"]->Get("EventTimeMRD", tm);
  fEventTimeMRD = (ULong64_t)tm.GetNs();

  if (!gotETMRD)
    fEventTimeMRD = 0;

  if (fillAllTriggers)
    return true;
  else if (fill_singleTrigger)
  {
    if (fPrimaryTriggerWord == fill_singleTriggerWord)
      return true;
    else
      return false;
  }
  else if (fill_TriggerWord.size() > 0)
  {
    for (auto trig : fill_TriggerWord)
    {
      if (fPrimaryTriggerWord == trig)
        return true;
    }
    return false;
  }
  else
    return true;
}

void ANNIEEventTreeMaker::LoadBeamInfo()
{
  Log("ANNIEEventTreeMaker Tool: LoadBeamInfo", v_debug, ANNIEEventTreeMakerVerbosity);

  m_data->Stores["ANNIEEvent"]->Get("beam_E_TOR860", beam_E_TOR860);
  m_data->Stores["ANNIEEvent"]->Get("beam_E_TOR875", beam_E_TOR875);
  m_data->Stores["ANNIEEvent"]->Get("beam_THCURR", beam_THCURR);
  m_data->Stores["ANNIEEvent"]->Get("beam_BTJT2", beam_BTJT2);
  m_data->Stores["ANNIEEvent"]->Get("beam_HP875", beam_HP875);
  m_data->Stores["ANNIEEvent"]->Get("beam_VP875", beam_VP875);
  m_data->Stores["ANNIEEvent"]->Get("beam_HPTG1", beam_HPTG1);
  m_data->Stores["ANNIEEvent"]->Get("beam_VPTG1", beam_VPTG1);
  m_data->Stores["ANNIEEvent"]->Get("beam_HPTG2", beam_HPTG2);
  m_data->Stores["ANNIEEvent"]->Get("beam_VPTG2", beam_VPTG2);
  m_data->Stores["ANNIEEvent"]->Get("beam_BTH2T2", beam_BTH2T2);
  m_data->Stores["ANNIEEvent"]->Get("beam_B_BRRMPL", beam_B_BRRMPL);
  m_data->Stores["ANNIEEvent"]->Get("beam_B_BRRMPQ", beam_B_BRRMPQ);
  m_data->Stores["ANNIEEvent"]->Get("beam_B_BRRMPS", beam_B_BRRMPS);
  m_data->Stores["ANNIEEvent"]->Get("beam_B_BRRMP", beam_B_BRRMP);

  m_data->Stores["ANNIEEvent"]->Get("beam_E_TOR875", fPot);
  m_data->Stores["ANNIEEvent"]->Get("beam_good", fBeamok);
  m_data->Stores["ANNIEEvent"]->Get("bunch_rotation_on", fBunchRotationOn);

  m_data->Stores["ANNIEEvent"]->Get("BeamInfoTime", fBeamInfoTime);
  m_data->Stores["ANNIEEvent"]->Get("BeamInfoTimeToTriggerDiff", fBeamInfoTimeToTriggerDiff);
}

void ANNIEEventTreeMaker::LoadRWMBRFInfo()
{
  Log("ANNIEEventTreeMaker Tool: LoadRWMBRFInfo", v_debug, ANNIEEventTreeMakerVerbosity);
  m_data->Stores["ANNIEEvent"]->Get("RWMRisingStart", fRWMRisingStart);
  m_data->Stores["ANNIEEvent"]->Get("RWMRisingEnd", fRWMRisingEnd);
  m_data->Stores["ANNIEEvent"]->Get("RWMHalfRising", fRWMHalfRising);
  m_data->Stores["ANNIEEvent"]->Get("RWMFHWM", fRWMFHWM);
  m_data->Stores["ANNIEEvent"]->Get("RWMFirstPeak", fRWMFirstPeak);

  m_data->Stores["ANNIEEvent"]->Get("BRFFirstPeak", fBRFFirstPeak);
  m_data->Stores["ANNIEEvent"]->Get("BRFAveragePeak", fBRFAveragePeak);
  m_data->Stores["ANNIEEvent"]->Get("BRFFirstPeakFit", fBRFFirstPeakFit);
}

void ANNIEEventTreeMaker::LoadAllTankHits()
{
  Log("ANNIEEventTreeMaker Tool: LoadAllTankHits", v_debug, ANNIEEventTreeMakerVerbosity);
  std::map<unsigned long, std::vector<Hit>> *Hits = nullptr;
  std::map<unsigned long, std::vector<MCHit>> *MCHits = nullptr;
  bool got_hits = false;
  if (isData)
    got_hits = m_data->Stores["ANNIEEvent"]->Get("Hits", Hits);
  else
    got_hits = m_data->Stores["ANNIEEvent"]->Get("MCHits", MCHits);
  if (!got_hits)
  {
    std::cout << "No Hits store in ANNIEEvent. Continuing to build tree " << std::endl;
    return;
  }

  Position detector_center = geom->GetTankCentre();
  double tank_center_x = detector_center.X();
  double tank_center_y = detector_center.Y();
  double tank_center_z = detector_center.Z();
  fNHits = 0;

  std::map<unsigned long, std::vector<Hit>>::iterator it_tank_data;
  std::map<unsigned long, std::vector<MCHit>>::iterator it_tank_mc;
  if (isData)
    it_tank_data = (*Hits).begin();
  else
    it_tank_mc = (*MCHits).begin();
  bool loop_tank = true;
  int hits_size = (isData) ? Hits->size() : MCHits->size();
  if (hits_size == 0)
    loop_tank = false;

  while (loop_tank)
  {
    // start to fill tank info to vectors
    unsigned long channel_key;
    if (isData)
      channel_key = it_tank_data->first;
    else
      channel_key = it_tank_mc->first;
    Detector *this_detector = geom->ChannelToDetector(channel_key);
    Position det_position = this_detector->GetDetectorPosition();
    unsigned long detkey = this_detector->GetDetectorID();
    unsigned long channel_key_data = channel_key;
    if (!isData)
    {
      int wcsimid = channelkey_to_pmtid.at(channel_key);
      channel_key_data = pmtid_to_channelkey[wcsimid];
    }
    std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(channel_key);
    std::map<int, double>::iterator it_mc = ChannelKeyToSPEMap.find(channel_key_data);
    bool SPE_available = true;
    if (isData)
      SPE_available = (it != ChannelKeyToSPEMap.end());
    else
      SPE_available = (it_mc != ChannelKeyToSPEMap.end());
    if (SPE_available)
    { // Charge to SPE conversion is available
      if (isData)
      {
        std::vector<Hit> ThisPMTHits = it_tank_data->second;
        fNHits += ThisPMTHits.size();
        for (Hit &ahit : ThisPMTHits)
        {
          double hit_charge = ahit.GetCharge();
          double hit_PE = hit_charge / ChannelKeyToSPEMap.at(channel_key);
          fHitX.push_back((det_position.X() - tank_center_x));
          fHitY.push_back((det_position.Y() - tank_center_y));
          fHitZ.push_back((det_position.Z() - tank_center_z));
          fHitT.push_back(ahit.GetTime());
          fHitQ.push_back(hit_charge);
          fHitPE.push_back(hit_PE);
          fHitDetID.push_back(detkey);
          fHitChankey.push_back(channel_key);
          fHitChankeyMC.push_back(channel_key);
          fHitType.push_back(RecoDigit::PMT8inch); // 0 For PMTs
        }
      }
      else
      {
        std::vector<MCHit> ThisPMTHits = it_tank_mc->second;
        fNHits += ThisPMTHits.size();
        for (MCHit &ahit : ThisPMTHits)
        {
          double hit_PE = ahit.GetCharge();
          double hit_charge = hit_PE * ChannelKeyToSPEMap.at(channel_key_data);
          fHitX.push_back((det_position.X() - tank_center_x));
          fHitY.push_back((det_position.Y() - tank_center_y));
          fHitZ.push_back((det_position.Z() - tank_center_z));
          fHitT.push_back(ahit.GetTime());
          fHitQ.push_back(hit_charge);
          fHitPE.push_back(hit_PE);
          fHitDetID.push_back(detkey);
          fHitChankey.push_back(channel_key_data);
          fHitChankeyMC.push_back(channel_key);
          fHitType.push_back(RecoDigit::PMT8inch); // 0 For PMTs
        }
      }
    }

    if (isData)
    {
      it_tank_data++;
      if (it_tank_data == (*Hits).end())
        loop_tank = false;
    }
    else
    {
      it_tank_mc++;
      if (it_tank_mc == (*MCHits).end())
        loop_tank = false;
    }
  }
  return;
}

void ANNIEEventTreeMaker::LoadSiPMHits()
{
  Log("ANNIEEventTreeMaker Tool: LoadSiPMHits", v_debug, ANNIEEventTreeMakerVerbosity);
  std::map<unsigned long, std::vector<std::vector<ADCPulse>>> aux_pulse_map;
  m_data->Stores.at("ANNIEEvent")->Get("RecoADCAuxHits", aux_pulse_map);
  fSiPM1NPulses = 0;
  fSiPM2NPulses = 0;
  for (const auto &temp_pair : aux_pulse_map)
  {
    const auto &channel_key = temp_pair.first;
    // For now, only calibrate the SiPM waveforms
    int sipm_number = -1;
    if (AuxChannelNumToTypeMap->at(channel_key) == "SiPM1")
    {
      sipm_number = 1;
    }
    else if (AuxChannelNumToTypeMap->at(channel_key) == "SiPM2")
    {
      sipm_number = 2;
    }
    else
      continue;

    std::vector<std::vector<ADCPulse>> sipm_minibuffers = temp_pair.second;
    size_t num_minibuffers = sipm_minibuffers.size(); // Should be size 1 in FrankDAQ mode
    for (size_t mb = 0; mb < num_minibuffers; ++mb)
    {
      std::vector<ADCPulse> thisbuffer_pulses = sipm_minibuffers.at(mb);
      if (sipm_number == 1)
        fSiPM1NPulses += thisbuffer_pulses.size();
      if (sipm_number == 2)
        fSiPM2NPulses += thisbuffer_pulses.size();
      for (size_t i = 0; i < thisbuffer_pulses.size(); i++)
      {
        ADCPulse apulse = thisbuffer_pulses.at(i);
        fSiPMHitAmplitude.push_back(apulse.amplitude());
        fSiPMHitT.push_back(apulse.peak_time());
        fSiPMHitQ.push_back(apulse.charge());
        fSiPMNum.push_back(sipm_number);
      }
    }
  }
}

void ANNIEEventTreeMaker::LoadLAPPDInfo()
{
  m_data->Stores["ANNIEEvent"]->Get("LAPPDDataMap", LAPPDDataMap);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgate_ns", LAPPDBeamgate_ns);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDTimeStamps_ns", LAPPDTimeStamps_ns);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDTimeStampsRaw", LAPPDTimeStampsRaw);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDBeamgatesRaw", LAPPDBeamgatesRaw);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDOffsets", LAPPDOffsets);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDTSCorrection", LAPPDTSCorrection);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDBGCorrection", LAPPDBGCorrection);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDOSInMinusPS", LAPPDOSInMinusPS);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSBefore", LAPPDBG_PPSBefore);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSAfter", LAPPDBG_PPSAfter);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSDiff", LAPPDBG_PPSDiff);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDBG_PPSMissing", LAPPDBG_PPSMissing);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSBefore", LAPPDTS_PPSBefore);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSAfter", LAPPDTS_PPSAfter);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSDiff", LAPPDTS_PPSDiff);
  m_data->Stores["ANNIEEvent"]->Get("LAPPDTS_PPSMissing", LAPPDTS_PPSMissing);
  m_data->Stores["ANNIEEvent"]->Get("SwitchBitBG", SwitchBitBG);

  if (LAPPDDataMap.size() != 0)
  {
    FillLAPPDInfo();
    // print the content of fDataStreams, and the size of data map
    // cout<<"Found LAPPDData, LAPPDDataMap Size: "<<LAPPDDataMap.size()<<endl;
    if (LAPPDReco_fill)
    {
      FillLAPPDPulse();
      FillLAPPDHit();
    }
  }
}

void ANNIEEventTreeMaker::FillLAPPDInfo()
{
  fLAPPD_Count = LAPPDDataMap.size();
  for (std::map<uint64_t, PsecData>::iterator it = LAPPDDataMap.begin(); it != LAPPDDataMap.end(); ++it)
  {
    uint64_t key = it->first;
    PsecData psecData = it->second;
    fLAPPD_ID.push_back(psecData.LAPPD_ID);
    fLAPPD_Beamgate_ns.push_back(LAPPDBeamgate_ns[key]);
    fLAPPD_Timestamp_ns.push_back(LAPPDTimeStamps_ns[key]);
    fLAPPD_Beamgate_Raw.push_back(LAPPDBeamgatesRaw[key]);
    fLAPPD_Timestamp_Raw.push_back(LAPPDTimeStampsRaw[key]);
    fLAPPD_Offset.push_back(LAPPDOffsets[key]);
    fLAPPD_TSCorrection.push_back(LAPPDTSCorrection[key]);
    fLAPPD_BGCorrection.push_back(LAPPDBGCorrection[key]);
    fLAPPD_OSInMinusPS.push_back(LAPPDOSInMinusPS[key]);
    fLAPPD_BGPPSBefore.push_back(LAPPDBG_PPSBefore[key]);
    fLAPPD_BGPPSAfter.push_back(LAPPDBG_PPSAfter[key]);
    fLAPPD_BGPPSDiff.push_back(LAPPDBG_PPSDiff[key]);
    fLAPPD_BGPPSMissing.push_back(LAPPDBG_PPSMissing[key]);
    fLAPPD_TSPPSBefore.push_back(LAPPDTS_PPSBefore[key]);
    fLAPPD_TSPPSAfter.push_back(LAPPDTS_PPSAfter[key]);
    fLAPPD_TSPPSDiff.push_back(LAPPDTS_PPSDiff[key]);
    fLAPPD_TSPPSMissing.push_back(LAPPDTS_PPSMissing[key]);
    // check if SwitchBitBG has the key
    if (SwitchBitBG.find(psecData.LAPPD_ID) != SwitchBitBG.end())
    {
      if (SwitchBitBG[psecData.LAPPD_ID].size() != 2)
      {
        cout << "ANNIEEventTreeMaker: SwitchBitBG size is not 2, LAPPD_ID: " << psecData.LAPPD_ID << ", size: " << SwitchBitBG[psecData.LAPPD_ID].size() << endl;
        continue;
      }
      fLAPPD_BG_switchBit0.push_back(SwitchBitBG[psecData.LAPPD_ID][0]);
      fLAPPD_BG_switchBit1.push_back(SwitchBitBG[psecData.LAPPD_ID][1]);
      continue;
    }
  }
}

void ANNIEEventTreeMaker::FillLAPPDPulse()
{
  bool gotPulse = m_data->Stores["ANNIEEvent"]->Get("LAPPDPulses", lappdPulses);
  if (gotPulse)
  {

    std::map<unsigned long, vector<vector<LAPPDPulse>>>::iterator it;
    for (it = lappdPulses.begin(); it != lappdPulses.end(); it++)
    {
      int stripno = it->first;
      vector<vector<LAPPDPulse>> stripPulses = it->second;

      vector<LAPPDPulse> pulse0 = stripPulses.at(0);
      vector<LAPPDPulse> pulse1 = stripPulses.at(1);
      for (int i = 0; i < pulse0.size(); i++)
      {
        fPulseSide.push_back(0);
        LAPPDPulse thisPulse = pulse0.at(i);
        fLAPPD_IDs.push_back(thisPulse.GetTubeId());
        fChannelID.push_back(thisPulse.GetChannelID());
        fPulseStripNum.push_back(stripno);
        fPulsePeakTime.push_back(thisPulse.GetTime());
        fPulseHalfHeightTime.push_back(thisPulse.GetHalfHeightTime());
        fPulseCharge.push_back(thisPulse.GetCharge());
        fPulsePeakAmp.push_back(thisPulse.GetPeak());
        fPulseStart.push_back(thisPulse.GetLowRange());
        fPulseEnd.push_back(thisPulse.GetHiRange());
        fPulseWidth.push_back(thisPulse.GetHiRange() - thisPulse.GetLowRange());
      }
      for (int i = 0; i < pulse1.size(); i++)
      {
        fPulseSide.push_back(1);
        LAPPDPulse thisPulse = pulse1.at(i);
        fLAPPD_IDs.push_back(thisPulse.GetTubeId());
        fChannelID.push_back(thisPulse.GetChannelID());
        fPulseStripNum.push_back(stripno);
        fPulsePeakTime.push_back(thisPulse.GetTime());
        fPulseHalfHeightTime.push_back(thisPulse.GetHalfHeightTime());
        fPulseCharge.push_back(thisPulse.GetCharge());
        fPulsePeakAmp.push_back(thisPulse.GetPeak());
        fPulseStart.push_back(thisPulse.GetLowRange());
        fPulseEnd.push_back(thisPulse.GetHiRange());
        fPulseWidth.push_back(thisPulse.GetHiRange() - thisPulse.GetLowRange());
      }
    }
  }
}

void ANNIEEventTreeMaker::FillLAPPDHit()
{
  bool gotHit = m_data->Stores["ANNIEEvent"]->Get("LAPPDHits", lappdHits);

  if (gotHit)
  {
    std::map<unsigned long, vector<LAPPDHit>>::iterator it;
    for (it = lappdHits.begin(); it != lappdHits.end(); it++)
    {
      int stripno = it->first;
      vector<LAPPDHit> stripHits = it->second;
      for (int i = 0; i < stripHits.size(); i++)
      {
        LAPPDHit thisHit = stripHits.at(i);
        LAPPDPulse p1 = thisHit.GetPulse1();
        LAPPDPulse p2 = thisHit.GetPulse2();
        fLAPPDHit_IDs.push_back(thisHit.GetTubeId());
        fLAPPDHitStrip.push_back(stripno);
        fLAPPDHitTime.push_back(thisHit.GetTime());
        fLAPPDHitAmp.push_back(thisHit.GetCharge());
        vector<double> position = thisHit.GetPosition();
        /*
        XPosTank = position.at(0);
        YPosTank = position.at(1);
        ZPosTank = position.at(2);*/
        vector<double> localPosition = thisHit.GetLocalPosition();
        fLAPPDHitParallelPos.push_back(localPosition.at(0));
        fLAPPDHitTransversePos.push_back(localPosition.at(1));
        // fLAPPDHitP1StartTime.push_back(thisHit.GetPulse1StartTime());
        // fLAPPDHitP2StartTime.push_back(thisHit.GetPulse2StartTime());
        // fLAPPDHitP1EndTime.push_back(thisHit.GetPulse1LastTime());
        // fLAPPDHitP2EndTime.push_back(thisHit.GetPulse2LastTime());
        // cout<<"Pulse 1 start time: "<<p1.GetLowRange()<<", Pulse 2 start time: "<<p2.GetLowRange()<<endl;
        fLAPPDHitP1StartTime.push_back(p1.GetLowRange());
        fLAPPDHitP2StartTime.push_back(p2.GetLowRange());
        fLAPPDHitP1EndTime.push_back(p1.GetHiRange());
        fLAPPDHitP2EndTime.push_back(p2.GetHiRange());

        fLAPPDHitP1PeakTime.push_back(p1.GetTime());
        fLAPPDHitP2PeakTime.push_back(p2.GetTime());
        fLAPPDHitP1PeakAmp.push_back(p1.GetPeak());
        fLAPPDHitP2PeakAmp.push_back(p2.GetPeak());

        fLAPPDHitP1HalfHeightTime.push_back(p1.GetHalfHeightTime());
        fLAPPDHitP2HalfHeightTime.push_back(p2.GetHalfHeightTime());

        fLAPPDHitP1HalfEndTime.push_back(p1.GetHalfEndTime());
        fLAPPDHitP2HalfEndTime.push_back(p2.GetHalfEndTime());

        fLAPPDHitP1Charge.push_back(p1.GetCharge());
        fLAPPDHitP2Charge.push_back(p2.GetCharge());
      }
    }
  }
}

bool ANNIEEventTreeMaker::LoadClusterInfo()
{
  Log("ANNIEEventTreeMaker Tool: LoadClusterInfo", v_debug, ANNIEEventTreeMakerVerbosity);

  std::map<double, std::vector<Hit>> *m_all_clusters = nullptr;
  std::map<double, std::vector<MCHit>> *m_all_clusters_MC = nullptr;
  std::map<double, std::vector<unsigned long>> *m_all_clusters_detkeys = nullptr;

  bool get_clusters = false;
  if (isData)
  {
    get_clusters = m_data->CStore.Get("ClusterMap", m_all_clusters);
    if (!get_clusters)
    {
      std::cout << "ANNIEEventTreeMaker tool: No clusters found!" << std::endl;
      return false;
    }
  }
  else
  {
    get_clusters = m_data->CStore.Get("ClusterMapMC", m_all_clusters_MC);
    if (!get_clusters)
    {
      std::cout << "ANNIEEventTreeMaker tool: No clusters found (MC)!" << std::endl;
      return false;
    }
  }
  get_clusters = m_data->CStore.Get("ClusterMapDetkey", m_all_clusters_detkeys);
  if (!get_clusters)
  {
    std::cout << "ANNIEEventTreeMaker tool: No cluster detkeys found!" << std::endl;
    return false;
  }
  Log("ANNIEEventTreeMaker Tool: Accessing pairs in all_clusters map", v_debug, ANNIEEventTreeMakerVerbosity);

  int cluster_num = 0;
  int cluster_size = 0;
  if (isData)
    cluster_size = (int)m_all_clusters->size();
  else
    cluster_size = (int)m_all_clusters_MC->size();

  std::map<double, std::vector<Hit>>::iterator it_cluster_pair;
  std::map<double, std::vector<MCHit>>::iterator it_cluster_pair_mc;
  bool loop_map = true;
  if (isData)
    it_cluster_pair = (*m_all_clusters).begin();
  else
    it_cluster_pair_mc = (*m_all_clusters_MC).begin();
  if (cluster_size == 0)
    loop_map = false;

  fNumberOfClusters = cluster_size;
  while (loop_map)
  {
    if (isData)
    {
      std::vector<Hit> cluster_hits = it_cluster_pair->second;
      double thisClusterTime = it_cluster_pair->first;
      if (cluster_TankHitInfo_fill)
      {
        Log("ANNIEEventTreeMaker Tool: Loading tank cluster hits into cluster tree", v_debug, ANNIEEventTreeMakerVerbosity);
        fClusterTimeV.push_back(thisClusterTime);
        this->LoadTankClusterHits(cluster_hits);
      }

      bool good_class = this->LoadTankClusterClassifiers(it_cluster_pair->first);
      if (!good_class)
      {
        if (ANNIEEventTreeMakerVerbosity > 3)
          Log("ANNIEEventTreeMaker Tool: No cluster classifiers.  Continuing tree", v_debug, ANNIEEventTreeMakerVerbosity);
      }
    }
    else
    {
      std::vector<MCHit> cluster_hits = it_cluster_pair_mc->second;
      double thisClusterTime = it_cluster_pair_mc->first;
      std::vector<unsigned long> cluster_detkeys = m_all_clusters_detkeys->at(it_cluster_pair_mc->first);
      if (cluster_TankHitInfo_fill)
      {
        Log("ANNIEEventTreeMaker Tool: Loading tank cluster hits into cluster tree", v_debug, ANNIEEventTreeMakerVerbosity);
        fClusterTimeV.push_back(thisClusterTime);
        this->LoadTankClusterHitsMC(cluster_hits, cluster_detkeys);
      }
      bool good_class = this->LoadTankClusterClassifiers(it_cluster_pair_mc->first);
      if (!good_class)
      {
        if (ANNIEEventTreeMakerVerbosity > 3)
          Log("ANNIEEventTreeMaker Tool: No cluster classifiers.  Continuing tree", v_debug, ANNIEEventTreeMakerVerbosity);
      }
    }

    if (isData)
    {
      it_cluster_pair++;
      if (it_cluster_pair == (*m_all_clusters).end())
        loop_map = false;
    }
    else
    {
      it_cluster_pair_mc++;
      if (it_cluster_pair_mc == (*m_all_clusters_MC).end())
        loop_map = false;
    }
  }

  return true;
}

void ANNIEEventTreeMaker::LoadTankClusterHits(std::vector<Hit> cluster_hits)
{
  Position detector_center = geom->GetTankCentre();
  double tank_center_x = detector_center.X();
  double tank_center_y = detector_center.Y();
  double tank_center_z = detector_center.Z();

  double ClusterCharge = 0;
  double ClusterPE = 0;
  int ClusterHitNum = 0;
  vector<double> HitXV;
  vector<double> HitYV;
  vector<double> HitZV;
  vector<double> HitTV;
  vector<double> HitQV;
  vector<double> HitPEV;
  vector<int> HitTypeV;
  vector<int> HitDetIDV;
  vector<int> HitChankeyV;
  vector<int> HitCKMC;

  for (int i = 0; i < (int)cluster_hits.size(); i++)
  {
    int channel_key = cluster_hits.at(i).GetTubeId();
    std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(channel_key);
    if (it != ChannelKeyToSPEMap.end())
    { // Charge to SPE conversion is available
      Detector *this_detector = geom->ChannelToDetector(channel_key);
      unsigned long detkey = this_detector->GetDetectorID();
      Position det_position = this_detector->GetDetectorPosition();
      double hit_charge = cluster_hits.at(i).GetCharge();
      double hit_PE = hit_charge / ChannelKeyToSPEMap.at(channel_key);
      HitXV.push_back((det_position.X() - tank_center_x));
      HitYV.push_back((det_position.Y() - tank_center_y));
      HitZV.push_back((det_position.Z() - tank_center_z));
      HitQV.push_back(hit_charge);
      HitPEV.push_back(hit_PE);
      HitTV.push_back(cluster_hits.at(i).GetTime());
      HitDetIDV.push_back(detkey);

      HitChankeyV.push_back(channel_key);
      HitCKMC.push_back(channel_key);
      HitTypeV.push_back(RecoDigit::PMT8inch);
      ClusterCharge += hit_charge;
      ClusterPE += hit_PE;
      ClusterHitNum += 1;
    }
    else
    {
      if (ANNIEEventTreeMakerVerbosity > 4)
      {
        std::cout << "FOUND A HIT FOR CHANNELKEY " << channel_key << "BUT NO CONVERSION " << "TO PE AVAILABLE.  SKIPPING PE." << std::endl;
      }
    }
  }
  fClusterHits.push_back(ClusterHitNum);
  fClusterChargeV.push_back(ClusterCharge);
  fClusterPEV.push_back(ClusterPE);

  fCluster_HitX.push_back(HitXV);
  fCluster_HitY.push_back(HitYV);
  fCluster_HitZ.push_back(HitZV);
  fCluster_HitT.push_back(HitTV);
  fCluster_HitQ.push_back(HitQV);
  fCluster_HitPE.push_back(HitPEV);
  fCluster_HitType.push_back(HitTypeV);
  fCluster_HitDetID.push_back(HitDetIDV);
  fCluster_HitChankey.push_back(HitChankeyV);
  fCluster_HitChankeyMC.push_back(HitCKMC);

  return;
}

void ANNIEEventTreeMaker::LoadTankClusterHitsMC(std::vector<MCHit> cluster_hits, std::vector<unsigned long> cluster_detkeys)
{
  Position detector_center = geom->GetTankCentre();
  double tank_center_x = detector_center.X();
  double tank_center_y = detector_center.Y();
  double tank_center_z = detector_center.Z();

  double ClusterCharge = 0;
  double ClusterPE = 0;
  int ClusterHitNum = 0;
  vector<double> HitXV;
  vector<double> HitYV;
  vector<double> HitZV;
  vector<double> HitTV;
  vector<double> HitQV;
  vector<double> HitPEV;
  vector<int> HitTypeV;
  vector<int> HitDetIDV;
  vector<int> HitChankeyV;
  vector<int> HitCKMC;

  for (int i = 0; i < (int)cluster_hits.size(); i++)
  {
    unsigned long detkey = cluster_detkeys.at(i);
    int channel_key = (int)detkey;
    int tubeid = cluster_hits.at(i).GetTubeId();
    unsigned long utubeid = (unsigned long)tubeid;
    int wcsimid = channelkey_to_pmtid.at(utubeid);
    unsigned long detkey_data = pmtid_to_channelkey[wcsimid];
    int channel_key_data = (int)detkey_data;
    std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(channel_key_data);
    if (it != ChannelKeyToSPEMap.end())
    { // Charge to SPE conversion is available
      Detector *this_detector = geom->ChannelToDetector(tubeid);
      Position det_position = this_detector->GetDetectorPosition();
      unsigned long detkey = this_detector->GetDetectorID();
      double hit_PE = cluster_hits.at(i).GetCharge();
      double hit_charge = hit_PE * ChannelKeyToSPEMap.at(channel_key_data);
      HitXV.push_back((det_position.X() - tank_center_x));
      HitYV.push_back((det_position.Y() - tank_center_y));
      HitZV.push_back((det_position.Z() - tank_center_z));
      HitQV.push_back(hit_charge);
      HitPEV.push_back(hit_PE);
      HitTV.push_back(cluster_hits.at(i).GetTime());
      HitDetIDV.push_back(detkey);
      HitChankeyV.push_back(channel_key_data);
      HitCKMC.push_back(channel_key);
      HitTypeV.push_back(RecoDigit::PMT8inch);
      ClusterCharge += hit_charge;
      ClusterPE += hit_PE;
      ClusterHitNum += 1;
    }
    else
    {
      if (ANNIEEventTreeMakerVerbosity > 4)
      {
        std::cout << "FOUND A HIT FOR CHANNELKEY " << channel_key_data << "(MC detkey: " << channel_key << ", chankey = " << utubeid << ", wcsimid = " << wcsimid << ") BUT NO CONVERSION " << "TO PE AVAILABLE.  SKIPPING PE." << std::endl;
      }
    }
  }
  fClusterHits.push_back(ClusterHitNum);
  fClusterChargeV.push_back(ClusterCharge);
  fClusterPEV.push_back(ClusterPE);

  fCluster_HitX.push_back(HitXV);
  fCluster_HitY.push_back(HitYV);
  fCluster_HitZ.push_back(HitZV);
  fCluster_HitT.push_back(HitTV);
  fCluster_HitQ.push_back(HitQV);
  fCluster_HitPE.push_back(HitPEV);
  fCluster_HitType.push_back(HitTypeV);
  fCluster_HitDetID.push_back(HitDetIDV);
  fCluster_HitChankey.push_back(HitChankeyV);
  fCluster_HitChankeyMC.push_back(HitCKMC);

  return;
}

bool ANNIEEventTreeMaker::LoadTankClusterClassifiers(double cluster_time)
{
  // Save classifiers to ANNIEEvent
  Log("PhaseITreeMaker tool: Getting cluster classifiers", v_debug, ANNIEEventTreeMakerVerbosity);
  std::map<double, double> ClusterMaxPEs;
  std::map<double, Position> ClusterChargePoints;
  std::map<double, double> ClusterChargeBalances;

  bool got_ccp = m_data->Stores.at("ANNIEEvent")->Get("ClusterChargePoints", ClusterChargePoints);
  bool got_ccb = m_data->Stores.at("ANNIEEvent")->Get("ClusterChargeBalances", ClusterChargeBalances);
  bool got_cmpe = m_data->Stores.at("ANNIEEvent")->Get("ClusterMaxPEs", ClusterMaxPEs);
  bool good_classifiers = got_ccp && got_ccb && got_cmpe;
  if (!good_classifiers)
  {
    Log("PhaseITreeMaker tool: One of the charge cluster classifiers is not available", v_debug, ANNIEEventTreeMakerVerbosity);
  }
  else
  {
    Log("PhaseITreeMaker tool: Setting fCluster variables to classifier parameters", v_debug, ANNIEEventTreeMakerVerbosity);
    fClusterMaxPEV.push_back(ClusterMaxPEs.at(cluster_time));
    Position ClusterChargePoint = ClusterChargePoints.at(cluster_time);
    fClusterChargePointXV.push_back(ClusterChargePoint.X());
    fClusterChargePointYV.push_back(ClusterChargePoint.Y());
    fClusterChargePointZV.push_back(ClusterChargePoint.Z());
    fClusterChargeBalanceV.push_back(ClusterChargeBalances.at(cluster_time));
  }
  return good_classifiers;
}

void ANNIEEventTreeMaker::LoadMRDCluster()
{
  std::vector<double> mrddigittimesthisevent;
  std::vector<double> mrddigitchargesthisevent;
  ;
  std::vector<int> mrddigitpmtsthisevent;
  std::vector<unsigned long> mrddigitchankeysthisevent;
  std::vector<std::vector<int>> MrdTimeClusters;

  bool get_clusters = m_data->CStore.Get("MrdTimeClusters", MrdTimeClusters);
  if (!get_clusters)
  {
    std::cout << "ANNIEEventTreeMaker tool: No MRD clusters found! Did you run the TimeClustering tool?" << std::endl;
    return;
  }

  int num_mrd_clusters;
  m_data->CStore.Get("NumMrdTimeClusters", num_mrd_clusters);

  if (num_mrd_clusters > 0)
  {
    m_data->CStore.Get("MrdDigitTimes", mrddigittimesthisevent);
    m_data->CStore.Get("MrdDigitPmts", mrddigitpmtsthisevent);
    m_data->CStore.Get("MrdDigitChankeys", mrddigitchankeysthisevent);
    m_data->CStore.Get("MrdDigitCharges", mrddigitchargesthisevent);
    m_data->CStore.Get("MrdDigitPmts", mrddigitpmtsthisevent);
  }

  std::map<unsigned long, vector<Hit>> *TDCData = nullptr;
  std::map<unsigned long, vector<MCHit>> *TDCData_MC = nullptr;

  int TrigHasVetoHit = 0;
  bool has_tdc = false;
  if (isData)
    has_tdc = m_data->Stores["ANNIEEvent"]->Get("TDCData", TDCData); // a std::map<ChannelKey,vector<TDCHit>>
  else
    has_tdc = m_data->Stores["ANNIEEvent"]->Get("TDCData", TDCData_MC);
  if (!has_tdc)
  {
    std::cout << "No TDCData store in ANNIEEvent." << std::endl;
  }

  int cluster_num = 0;
  for (int i = 0; i < (int)MrdTimeClusters.size(); i++)
  {
    int tdcdata_size = (isData) ? TDCData->size() : TDCData_MC->size();
    int fMRDClusterHits = 0;
    if (has_tdc && tdcdata_size > 0)
    {
      Log("ANNIEEventTreeMaker tool: Looping over FACC/MRD hits... looking for Veto activity", v_debug, ANNIEEventTreeMakerVerbosity);
      if (isData)
      {
        for (auto &&anmrdpmt : (*TDCData))
        {
          unsigned long chankey = anmrdpmt.first;
          std::vector<Hit> mrdhits = anmrdpmt.second;
          Detector *thedetector = geom->ChannelToDetector(chankey);
          unsigned long detkey = thedetector->GetDetectorID();
          if (thedetector->GetDetectorElement() == "Veto")
          {
            TrigHasVetoHit = 1; // this is a veto hit, not an MRD hit.
            for (int j = 0; j < (int)mrdhits.size(); j++)
            {
              fFMVHitT.push_back(mrdhits.at(j).GetTime());
              fFMVHitDetID.push_back(detkey);
              fFMVHitChankey.push_back(chankey);
              fFMVHitChankeyMC.push_back(chankey);
            }
          }
        }
      }
      else
      {
        for (auto &&anmrdpmt : (*TDCData_MC))
        {
          unsigned long chankey = anmrdpmt.first;
          Detector *thedetector = geom->ChannelToDetector(chankey);
          unsigned long detkey = thedetector->GetDetectorID();
          if (thedetector->GetDetectorElement() == "Veto")
          {
            TrigHasVetoHit = 1; // this is a veto hit, not an MRD hit.
            int wcsimid = channelkey_to_faccpmtid.at(chankey) - 1;
            unsigned long chankey_data = wcsimid;
            std::vector<MCHit> mrdhits = anmrdpmt.second;
            for (int j = 0; j < (int)mrdhits.size(); j++)
            {
              fFMVHitT.push_back(mrdhits.at(j).GetTime());
              fFMVHitDetID.push_back(detkey);
              fFMVHitChankey.push_back(chankey_data);
              fFMVHitChankeyMC.push_back(chankey);
            }
          }
        }
      }
    }

    fVetoHit = TrigHasVetoHit;
    std::vector<int> ThisClusterIndices = MrdTimeClusters.at(i);
    for (int j = 0; j < (int)ThisClusterIndices.size(); j++)
    {
      Detector *thedetector = geom->ChannelToDetector(mrddigitchankeysthisevent.at(ThisClusterIndices.at(j)));
      unsigned long detkey = thedetector->GetDetectorID();

      fMRDHitT.push_back(mrddigittimesthisevent.at(ThisClusterIndices.at(j)));
      fMRDHitClusterIndex.push_back(i);
      fMRDHitCharge.push_back(mrddigitchargesthisevent.at(ThisClusterIndices.at(j)));
      fMRDHitDigitPMT.push_back(mrddigitpmtsthisevent.at(ThisClusterIndices.at(j)));
      fMRDHitDetID.push_back(detkey);
      if (isData)
        fMRDHitChankey.push_back(mrddigitchankeysthisevent.at(ThisClusterIndices.at(j)));
      else
      {
        int wcsimid = channelkey_to_mrdpmtid.at(mrddigitchankeysthisevent.at(ThisClusterIndices.at(j))) - 1;
        unsigned long chankey_data = mrdpmtid_to_channelkey_data[wcsimid];
        fMRDHitChankey.push_back(chankey_data);
      }
      fMRDHitChankeyMC.push_back(mrddigitchankeysthisevent.at(ThisClusterIndices.at(j)));
      fMRDClusterHits += 1;
    }

    double MRDThisClusterTime = 0;
    double MRDThisClusterTimeSigma = 0;
    ComputeMeanAndVariance(fMRDHitT, MRDThisClusterTime, MRDThisClusterTimeSigma);
    // FIXME: calculate fMRDClusterTime

    // Standard run level information
    Log("ANNIEEventTreeMaker Tool: MRD cluster, Getting run level information from ANNIEEvent", v_debug, ANNIEEventTreeMakerVerbosity);

    if (MRDReco_fill)
    {
      int ThisMRDClusterTrackNum = this->LoadMRDTrackReco(i);
      fNumClusterTracks.push_back(ThisMRDClusterTrackNum);
      // Get the track info
    }
    cluster_num++;

    fMRDClusterHitNumber.push_back(fMRDClusterHits);
    fMRDClusterTime.push_back(MRDThisClusterTime);
    fMRDClusterTimeSigma.push_back(MRDThisClusterTimeSigma);
  }
  fMRDClusterNumber = cluster_num;
  Log("ANNIEEventTreeMaker Tool: MRD cluster, Finished loading MRD cluster info", v_debug, ANNIEEventTreeMakerVerbosity);
}

int ANNIEEventTreeMaker::LoadMRDTrackReco(int SubEventID)
{
  std::vector<BoostStore> *theMrdTracks; // the reconstructed tracks
  int numtracksinev;

  // Check for valid track criteria
  m_data->Stores["MRDTracks"]->Get("MRDTracks", theMrdTracks);
  m_data->Stores["MRDTracks"]->Get("NumMrdTracks", numtracksinev);
  // Loop over reconstructed tracks

  Position StartVertex;
  Position StopVertex;
  double TrackLength = -9999;
  double TrackAngle = -9999;
  double TrackAngleError = -9999;
  double PenetrationDepth = -9999;
  Position MrdEntryPoint;
  double EnergyLoss = -9999; // in MeV
  double EnergyLossError = -9999;
  double EntryPointRadius = -9999;
  bool IsMrdPenetrating;
  bool IsMrdStopped;
  bool IsMrdSideExit;

  int NumClusterTracks = 0;
  for (int tracki = 0; tracki < numtracksinev; tracki++)
  {
    BoostStore *thisTrackAsBoostStore = &(theMrdTracks->at(tracki));
    int TrackEventID = -1;
    // get track properties that are needed for the through-going muon selection
    thisTrackAsBoostStore->Get("MrdSubEventID", TrackEventID);
    if (TrackEventID != SubEventID)
      continue;

    // If we're here, this track is associated with this cluster
    thisTrackAsBoostStore->Get("StartVertex", StartVertex);
    thisTrackAsBoostStore->Get("StopVertex", StopVertex);
    thisTrackAsBoostStore->Get("TrackAngle", TrackAngle);
    thisTrackAsBoostStore->Get("TrackAngleError", TrackAngleError);
    thisTrackAsBoostStore->Get("PenetrationDepth", PenetrationDepth);
    thisTrackAsBoostStore->Get("MrdEntryPoint", MrdEntryPoint);
    thisTrackAsBoostStore->Get("EnergyLoss", EnergyLoss);
    thisTrackAsBoostStore->Get("EnergyLossError", EnergyLossError);
    thisTrackAsBoostStore->Get("IsMrdPenetrating", IsMrdPenetrating); // bool
    thisTrackAsBoostStore->Get("IsMrdStopped", IsMrdStopped);         // bool
    thisTrackAsBoostStore->Get("IsMrdSideExit", IsMrdSideExit);
    TrackLength = sqrt(pow((StopVertex.X() - StartVertex.X()), 2) + pow(StopVertex.Y() - StartVertex.Y(), 2) + pow(StopVertex.Z() - StartVertex.Z(), 2)) * 100.0;
    EntryPointRadius = sqrt(pow(MrdEntryPoint.X(), 2) + pow(MrdEntryPoint.Y(), 2)) * 100.0; // convert to cm
    PenetrationDepth = PenetrationDepth * 100.0;

    // Push back some properties
    fMRDTrackAngle.push_back(TrackAngle);
    fMRDTrackAngleError.push_back(TrackAngleError);
    fMRDTrackLength.push_back(TrackLength);
    fMRDPenetrationDepth.push_back(PenetrationDepth);
    fMRDEntryPointRadius.push_back(EntryPointRadius);
    fMRDEnergyLoss.push_back(EnergyLoss);
    fMRDEnergyLossError.push_back(EnergyLossError);
    fMRDTrackStartX.push_back(StartVertex.X());
    fMRDTrackStartY.push_back(StartVertex.Y());
    fMRDTrackStartZ.push_back(StartVertex.Z());
    fMRDTrackStopX.push_back(StopVertex.X());
    fMRDTrackStopY.push_back(StopVertex.Y());
    fMRDTrackStopZ.push_back(StopVertex.Z());
    fMRDStop.push_back(IsMrdStopped);
    fMRDSide.push_back(IsMrdSideExit);
    fMRDThrough.push_back(IsMrdPenetrating);
    NumClusterTracks += 1;
    fMRDClusterIndex.push_back(SubEventID);
  }
  return NumClusterTracks;
}

// MC and tank reco information below

bool ANNIEEventTreeMaker::FillTankRecoInfo()
{
  bool got_reco_info = true;
  auto *reco_event = m_data->Stores["RecoEvent"];
  if (!reco_event)
  {
    Log("Error: The PhaseITreeMaker tool could not find the RecoEvent Store", v_error, ANNIEEventTreeMakerVerbosity);
    got_reco_info = false;
  }
  // Read reconstructed Vertex
  RecoVertex *recovtx = 0;
  auto get_extendedvtx = m_data->Stores.at("RecoEvent")->Get("ExtendedVertex", recovtx);
  if (!get_extendedvtx)
  {
    Log("Warning: The PhaseITreeMaker tool could not find ExtendedVertex. Continuing to build tree", v_message, ANNIEEventTreeMakerVerbosity);
    got_reco_info = false;
  }
  else
  {
    fRecoVtxX = recovtx->GetPosition().X();
    fRecoVtxY = recovtx->GetPosition().Y();
    fRecoVtxZ = recovtx->GetPosition().Z();
    fRecoVtxTime = recovtx->GetTime();
    fRecoVtxFOM = recovtx->GetFOM();
    fRecoDirX = recovtx->GetDirection().X();
    fRecoDirY = recovtx->GetDirection().Y();
    fRecoDirZ = recovtx->GetDirection().Z();
    fRecoAngle = TMath::ACos(fRecoDirZ);
    if (fRecoDirX > 0.0)
    {
      fRecoPhi = atan(fRecoDirY / fRecoDirX);
    }
    if (fRecoDirX < 0.0)
    {
      fRecoPhi = atan(fRecoDirY / fRecoDirX);
      if (fRecoDirY > 0.0)
        fRecoPhi += TMath::Pi();
      if (fRecoDirY <= 0.0)
        fRecoPhi -= TMath::Pi();
    }
    if (fRecoDirX == 0.0)
    {
      if (fRecoDirY > 0.0)
        fRecoPhi = 0.5 * TMath::Pi();
      else if (fRecoDirY < 0.0)
        fRecoPhi = -0.5 * TMath::Pi();
      else
        fRecoPhi = 0;
    }
    fRecoStatus = recovtx->GetStatus();
  }
  return got_reco_info;
}

void ANNIEEventTreeMaker::FillRecoDebugInfo()
{
  // Read Seed candidates
  std::vector<RecoVertex> *seedvtxlist = 0;
  auto get_seedvtxlist = m_data->Stores.at("RecoEvent")->Get("vSeedVtxList", seedvtxlist); ///> Get List of seeds from "RecoEvent"
  if (get_seedvtxlist)
  {
    for (auto &seed : *seedvtxlist)
    {
      fSeedVtxX.push_back(seed.GetPosition().X());
      fSeedVtxY.push_back(seed.GetPosition().Y());
      fSeedVtxZ.push_back(seed.GetPosition().Z());
      fSeedVtxTime = seed.GetTime();
    }
  }
  else
  {
    Log("ANNIEEventTreeMaker  Tool: No Seed List found.  Continuing to build tree ", v_message, ANNIEEventTreeMakerVerbosity);
  }
  std::vector<double> *seedfomlist = 0;
  auto get_seedfomlist = m_data->Stores.at("RecoEvent")->Get("vSeedFOMList", seedfomlist); ///> Get List of seed FOMs from "RecoEvent"
  if (get_seedfomlist)
  {
    for (auto &seedFOM : *seedfomlist)
    {
      fSeedVtxFOM.push_back(seedFOM);
    }
  }
  else
  {
    Log("ANNIEEventTreeMaker  Tool: No Seed FOM List found.  Continuing to build tree ", v_message, ANNIEEventTreeMakerVerbosity);
  }

  // Read PointPosition-fitted Vertex
  RecoVertex *pointposvtx = 0;
  auto get_pointposdata = m_data->Stores.at("RecoEvent")->Get("PointPosition", pointposvtx);
  if (get_pointposdata)
  {
    fPointPosX = pointposvtx->GetPosition().X();
    fPointPosY = pointposvtx->GetPosition().Y();
    fPointPosZ = pointposvtx->GetPosition().Z();
    fPointPosTime = pointposvtx->GetTime();
    fPointPosFOM = pointposvtx->GetFOM();
    fPointPosStatus = pointposvtx->GetStatus();
  }
  else
  {
    Log("ANNIEEventTreeMaker Tool: No PointPosition Tool data found.  Continuing to build remaining tree", v_message, ANNIEEventTreeMakerVerbosity);
  }

  // Read PointDirection-fitted Vertex
  RecoVertex *pointdirvtx = 0;
  auto get_pointdirdata = m_data->Stores.at("RecoEvent")->Get("PointDirection", pointdirvtx);
  if (get_pointdirdata)
  {
    fPointDirX = pointdirvtx->GetDirection().X();
    fPointDirY = pointdirvtx->GetDirection().Y();
    fPointDirZ = pointdirvtx->GetDirection().Z();
    fPointDirTime = pointdirvtx->GetTime();
    fPointDirFOM = pointdirvtx->GetFOM();
    fPointDirStatus = pointdirvtx->GetStatus();
  }
  else
  {
    Log("ANNIEEventTreeMaker Tool: No PointDirection Tool data found.  Continuing to build remaining tree", v_message, ANNIEEventTreeMakerVerbosity);
  }

  // Read PointVertex Tool's fitted Vertex
  RecoVertex *pointvtx = 0;
  auto get_pointvtxdata = m_data->Stores.at("RecoEvent")->Get("PointVertex", pointvtx);
  if (get_pointvtxdata)
  {
    fPointVtxPosX = pointvtx->GetPosition().X();
    fPointVtxPosY = pointvtx->GetPosition().Y();
    fPointVtxPosZ = pointvtx->GetPosition().Z();
    fPointVtxDirX = pointvtx->GetDirection().X();
    fPointVtxDirY = pointvtx->GetDirection().Y();
    fPointVtxDirZ = pointvtx->GetDirection().Z();
    fPointVtxTime = pointvtx->GetTime();
    fPointVtxFOM = pointvtx->GetFOM();
    fPointVtxStatus = pointvtx->GetStatus();
  }
  else
  {
    Log("ANNIEEventTreeMaker Tool: No PointVertex Tool data found.  Continuing to build remaining tree", v_message, ANNIEEventTreeMakerVerbosity);
  }
}

bool ANNIEEventTreeMaker::FillMCTruthInfo()
{
  bool successful_load = true;
  // MC entry number
  m_data->Stores.at("ANNIEEvent")->Get("MCEventNum", fMCEventNum);
  // MC trigger number
  m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum", fMCTriggerNum);
  std::string logmessage = "  Retriving information for MCEntry " + to_string(fMCEventNum) +
                           ", MCTrigger " + to_string(fMCTriggerNum) + ", EventNumber " + to_string(fEventNumber);
  Log(logmessage, v_message, ANNIEEventTreeMakerVerbosity);

  fiMCTriggerNum = (int)fMCTriggerNum;

  std::map<std::string, std::vector<double>> MCNeutCap;
  bool get_neutcap = m_data->Stores.at("ANNIEEvent")->Get("MCNeutCap", MCNeutCap);
  if (!get_neutcap)
  {
    Log("ANNIEEventTreeMaker: Did not find MCNeutCap in ANNIEEvent Store!", v_warning, ANNIEEventTreeMakerVerbosity);
  }
  std::map<std::string, std::vector<std::vector<double>>> MCNeutCapGammas;
  bool get_neutcap_gammas = m_data->Stores.at("ANNIEEvent")->Get("MCNeutCapGammas", MCNeutCapGammas);
  if (!get_neutcap_gammas)
  {
    Log("ANNIEEventTreeMaker: Did not find MCNeutCapGammas in ANNIEEvent Store!", v_warning, ANNIEEventTreeMakerVerbosity);
  }

  for (std::map<std::string, std::vector<std::vector<double>>>::iterator it = MCNeutCapGammas.begin(); it != MCNeutCapGammas.end(); it++)
  {
    std::vector<std::vector<double>> mcneutgammas = it->second;
    for (int i_cap = 0; i_cap < (int)mcneutgammas.size(); i_cap++)
    {
      std::vector<double> capgammas = mcneutgammas.at(i_cap);
      if (ANNIEEventTreeMakerVerbosity > 1)
      {
        for (int i_gamma = 0; i_gamma < (int)capgammas.size(); i_gamma++)
        {
          std::cout << "gamma # " << i_gamma << ", energy: " << capgammas.at(i_gamma) << std::endl;
        }
      }
    }
  }

  RecoVertex *truevtx = 0;
  auto get_muonMC = m_data->Stores.at("RecoEvent")->Get("TrueVertex", truevtx);
  auto get_muonMCEnergy = m_data->Stores.at("RecoEvent")->Get("TrueMuonEnergy", fTrueMuonEnergy);
  auto get_pdg = m_data->Stores.at("RecoEvent")->Get("PdgPrimary", fTruePrimaryPdg);
  if (get_muonMC && get_muonMCEnergy)
  {
    fTrueVtxX = truevtx->GetPosition().X();
    fTrueVtxY = truevtx->GetPosition().Y();
    fTrueVtxZ = truevtx->GetPosition().Z();
    fTrueVtxTime = truevtx->GetTime();
    fTrueDirX = truevtx->GetDirection().X();
    fTrueDirY = truevtx->GetDirection().Y();
    fTrueDirZ = truevtx->GetDirection().Z();
    double TrueAngRad = TMath::ACos(fTrueDirZ);
    fTrueAngle = TrueAngRad / (TMath::Pi() / 180.0); // radians->degrees
    if (fTrueDirX > 0.0)
    {
      fTruePhi = atan(fTrueDirY / fTrueDirX);
    }
    if (fTrueDirX < 0.0)
    {
      fTruePhi = atan(fTrueDirY / fTrueDirX);
      if (fTrueDirY > 0.0)
        fTruePhi += TMath::Pi();
      if (fTrueDirY <= 0.0)
        fTruePhi -= TMath::Pi();
    }
    if (fTrueDirX == 0.0)
    {
      if (fTrueDirY > 0.0)
        fTruePhi = 0.5 * TMath::Pi();
      else if (fTrueDirY < 0.0)
        fTruePhi = -0.5 * TMath::Pi();
      else
        fTruePhi = 0;
    }
  }
  else
  {
    Log("ANNIEEventTreeMaker Tool: Missing MC Energy/Vertex info; is this MC?  Continuing to build remaining tree", v_message, ANNIEEventTreeMakerVerbosity);
    successful_load = false;
  }
  double waterT, MRDT;
  auto get_tankTrackLength = m_data->Stores.at("RecoEvent")->Get("TrueTrackLengthInWater", waterT);
  auto get_MRDTrackLength = m_data->Stores.at("RecoEvent")->Get("TrueTrackLengthInMRD", MRDT);
  if (get_tankTrackLength && get_MRDTrackLength)
  {
    fTrueTrackLengthInWater = waterT;
    fTrueTrackLengthInMRD = MRDT;
  }
  else
  {
    Log("ANNIEEventTreeMaker Tool: True track lengths missing. Continuing to build tree", v_message, ANNIEEventTreeMakerVerbosity);
    successful_load = false;
  }

  bool IsMultiRing = false;
  bool get_multi = m_data->Stores.at("RecoEvent")->Get("MCMultiRingEvent", IsMultiRing);
  if (get_multi)
  {
    fTrueMultiRing = (IsMultiRing) ? 1 : 0;
  }
  else
  {
    Log("ANNIEEventTreeMaker Tool: True Multi Ring information missing. Continuing to build tree", v_message, ANNIEEventTreeMakerVerbosity);
    successful_load = false;
  }

  std::vector<int> primary_pdgs;
  bool has_primaries = m_data->Stores.at("RecoEvent")->Get("PrimaryPdgs", primary_pdgs);
  if (has_primaries)
  {
    for (int i_part = 0; i_part < (int)primary_pdgs.size(); i_part++)
    {
      fTruePrimaryPdgs->push_back(primary_pdgs.at(i_part));
    }
  }
  else
  {
    Log("ANNIEEventTreeMaker Tool: Primary Pdgs information missing. Continuing to build tree", v_message, ANNIEEventTreeMakerVerbosity);
    successful_load = false;
  }

  int pi0count, pipcount, pimcount, K0count, Kpcount, Kmcount;
  auto get_pi0 = m_data->Stores.at("RecoEvent")->Get("MCPi0Count", pi0count);
  auto get_pim = m_data->Stores.at("RecoEvent")->Get("MCPiMinusCount", pimcount);
  auto get_pip = m_data->Stores.at("RecoEvent")->Get("MCPiPlusCount", pipcount);
  auto get_k0 = m_data->Stores.at("RecoEvent")->Get("MCK0Count", K0count);
  auto get_km = m_data->Stores.at("RecoEvent")->Get("MCKMinusCount", Kmcount);
  auto get_kp = m_data->Stores.at("RecoEvent")->Get("MCKPlusCount", Kpcount);
  if (get_pi0 && get_pim && get_pip && get_k0 && get_km && get_kp)
  {
    // set values in tree to thouse grabbed from the RecoEvent Store
    fPi0Count = pi0count;
    fPiPlusCount = pipcount;
    fPiMinusCount = pimcount;
    fK0Count = K0count;
    fKPlusCount = Kpcount;
    fKMinusCount = Kmcount;
  }
  else
  {
    Log("ANNIEEventTreeMaker Tool: Missing MC Pion/Kaon count information. Continuing to build remaining tree", v_message, ANNIEEventTreeMakerVerbosity);
    successful_load = false;
  }

  if (MCNeutCap.count("CaptVtxX") > 0)
  {
    std::vector<double> n_vtxx = MCNeutCap["CaptVtxX"];
    std::vector<double> n_vtxy = MCNeutCap["CaptVtxY"];
    std::vector<double> n_vtxz = MCNeutCap["CaptVtxZ"];
    std::vector<double> n_parent = MCNeutCap["CaptParent"];
    std::vector<double> n_ngamma = MCNeutCap["CaptNGamma"];
    std::vector<double> n_totale = MCNeutCap["CaptTotalE"];
    std::vector<double> n_time = MCNeutCap["CaptTime"];
    std::vector<double> n_nuc = MCNeutCap["CaptNucleus"];

    for (int i_cap = 0; i_cap < (int)n_vtxx.size(); i_cap++)
    {
      fTrueNeutCapVtxX->push_back(n_vtxx.at(i_cap));
      fTrueNeutCapVtxY->push_back(n_vtxy.at(i_cap));
      fTrueNeutCapVtxZ->push_back(n_vtxz.at(i_cap));
      fTrueNeutCapNucleus->push_back(n_nuc.at(i_cap));
      fTrueNeutCapTime->push_back(n_time.at(i_cap));
      fTrueNeutCapGammas->push_back(n_ngamma.at(i_cap));
      fTrueNeutCapE->push_back(n_totale.at(i_cap));
    }
  }

  if (ANNIEEventTreeMakerVerbosity > 1)
    std::cout << "MCNeutCapGammas count CaptGammas: " << MCNeutCapGammas.count("CaptGammas") << std::endl;
  if (MCNeutCapGammas.count("CaptGammas") > 0)
  {
    std::vector<std::vector<double>> cap_energies = MCNeutCapGammas["CaptGammas"];
    if (ANNIEEventTreeMakerVerbosity > 1)
      std::cout << "cap_energies size: " << cap_energies.size() << std::endl;
    for (int i_cap = 0; i_cap < (int)cap_energies.size(); i_cap++)
    {
      for (int i_gamma = 0; i_gamma < cap_energies.at(i_cap).size(); i_gamma++)
      {
        if (ANNIEEventTreeMakerVerbosity > 1)
          std::cout << "gamma energy: " << cap_energies.at(i_cap).at(i_gamma) << std::endl;
        fTrueNeutCapGammaE->push_back(cap_energies.at(i_cap).at(i_gamma));
      }
    }
  }

  // Load genie information
  if (hasGenie)
  {
    double TrueNeutrinoEnergy, TrueQ2, TrueNuIntxVtx_X, TrueNuIntxVtx_Y, TrueNuIntxVtx_Z, TrueNuIntxVtx_T;
    double TrueFSLeptonMass, TrueFSLeptonEnergy, TrueFSLeptonTime;
    bool TrueCC, TrueNC, TrueQEL, TrueDIS, TrueCOH, TrueMEC, TrueRES;
    int fsNeutrons, fsProtons, fsPi0, fsPiPlus, fsPiPlusCher, fsPiMinus, fsPiMinusCher;
    int fsKPlus, fsKPlusCher, fsKMinus, fsKMinusCher, TrueNuPDG, TrueFSLeptonPdg;
    Position TrueFSLeptonVtx;
    Direction TrueFSLeptonMomentum;
    Direction TrueNeutrinoMomentum;
    bool get_neutrino_energy = m_data->Stores["GenieInfo"]->Get("NeutrinoEnergy", TrueNeutrinoEnergy);
    bool get_neutrino_mom = m_data->Stores["GenieInfo"]->Get("NeutrinoMomentum", TrueNeutrinoMomentum);
    bool get_neutrino_vtxx = m_data->Stores["GenieInfo"]->Get("NuIntxVtx_X", TrueNuIntxVtx_X);
    bool get_neutrino_vtxy = m_data->Stores["GenieInfo"]->Get("NuIntxVtx_Y", TrueNuIntxVtx_Y);
    bool get_neutrino_vtxz = m_data->Stores["GenieInfo"]->Get("NuIntxVtx_Z", TrueNuIntxVtx_Z);
    bool get_neutrino_vtxt = m_data->Stores["GenieInfo"]->Get("NuIntxVtx_T", TrueNuIntxVtx_T);
    bool get_q2 = m_data->Stores["GenieInfo"]->Get("EventQ2", TrueQ2);
    bool get_cc = m_data->Stores["GenieInfo"]->Get("IsWeakCC", TrueCC);
    bool get_nc = m_data->Stores["GenieInfo"]->Get("IsWeakNC", TrueNC);
    bool get_qel = m_data->Stores["GenieInfo"]->Get("IsQuasiElastic", TrueQEL);
    bool get_res = m_data->Stores["GenieInfo"]->Get("IsResonant", TrueRES);
    bool get_dis = m_data->Stores["GenieInfo"]->Get("IsDeepInelastic", TrueDIS);
    bool get_coh = m_data->Stores["GenieInfo"]->Get("IsCoherent", TrueCOH);
    bool get_mec = m_data->Stores["GenieInfo"]->Get("IsMEC", TrueMEC);
    bool get_n = m_data->Stores["GenieInfo"]->Get("NumFSNeutrons", fsNeutrons);
    bool get_p = m_data->Stores["GenieInfo"]->Get("NumFSProtons", fsProtons);
    bool get_pi0 = m_data->Stores["GenieInfo"]->Get("NumFSPi0", fsPi0);
    bool get_piplus = m_data->Stores["GenieInfo"]->Get("NumFSPiPlus", fsPiPlus);
    bool get_pipluscher = m_data->Stores["GenieInfo"]->Get("NumFSPiPlusCher", fsPiPlusCher);
    bool get_piminus = m_data->Stores["GenieInfo"]->Get("NumFSPiMinus", fsPiMinus);
    bool get_piminuscher = m_data->Stores["GenieInfo"]->Get("NumFSPiMinusCher", fsPiMinusCher);
    bool get_kplus = m_data->Stores["GenieInfo"]->Get("NumFSKPlus", fsKPlus);
    bool get_kpluscher = m_data->Stores["GenieInfo"]->Get("NumFSKPlusCher", fsKPlusCher);
    bool get_kminus = m_data->Stores["GenieInfo"]->Get("NumFSKMinus", fsKMinus);
    bool get_kminuscher = m_data->Stores["GenieInfo"]->Get("NumFSKMinusCher", fsKMinusCher);
    bool get_fsl_vtx = m_data->Stores["GenieInfo"]->Get("FSLeptonVertex", TrueFSLeptonVtx);
    bool get_fsl_momentum = m_data->Stores["GenieInfo"]->Get("FSLeptonMomentum", TrueFSLeptonMomentum);
    bool get_fsl_time = m_data->Stores["GenieInfo"]->Get("FSLeptonTime", TrueFSLeptonTime);
    bool get_fsl_mass = m_data->Stores["GenieInfo"]->Get("FSLeptonMass", TrueFSLeptonMass);
    bool get_fsl_pdg = m_data->Stores["GenieInfo"]->Get("FSLeptonPdg", TrueFSLeptonPdg);
    bool get_fsl_energy = m_data->Stores["GenieInfo"]->Get("FSLeptonEnergy", TrueFSLeptonEnergy);
    if (ANNIEEventTreeMakerVerbosity > 1)
    {
      std::cout << "get_neutrino_energy: " << get_neutrino_energy << "get_neutrino_vtxx: " << get_neutrino_vtxx << "get_neutrino_vtxy: " << get_neutrino_vtxy << "get_neutrino_vtxz: " << get_neutrino_vtxz << "get_neutrino_time: " << get_neutrino_vtxt << std::endl;
      std::cout << "get_q2: " << get_q2 << ", get_cc: " << get_cc << ", get_qel: " << get_qel << ", get_res: " << get_res << ", get_dis: " << get_dis << ", get_coh: " << get_coh << ", get_mec: " << get_mec << std::endl;
      std::cout << "get_n: " << get_n << ", get_p: " << get_p << ", get_pi0: " << get_pi0 << ", get_piplus: " << get_piplus << ", get_pipluscher: " << get_pipluscher << ", get_piminus: " << get_piminus << ", get_piminuscher: " << get_piminuscher << ", get_kplus: " << get_kplus << ", get_kpluscher: " << get_kpluscher << ", get_kminus: " << get_kminus << ", get_kminuscher: " << get_kminuscher << std::endl;
      std::cout << "get_fsl_vtx: " << get_fsl_vtx << ", get_fsl_momentum: " << get_fsl_momentum << ", get_fsl_time: " << get_fsl_time << ", get_fsl_mass: " << get_fsl_mass << ", get_fsl_pdg: " << get_fsl_pdg << ", get_fsl_energy: " << get_fsl_energy << std::endl;
    }
    if (get_neutrino_energy && get_neutrino_mom && get_neutrino_vtxx && get_neutrino_vtxy && get_neutrino_vtxz && get_neutrino_vtxt && get_q2 && get_cc && get_nc && get_qel && get_res && get_dis && get_coh && get_mec && get_n && get_p && get_pi0 && get_piplus && get_pipluscher && get_piminus && get_piminuscher && get_kplus && get_kpluscher && get_kminus && get_kminuscher && get_fsl_vtx && get_fsl_momentum && get_fsl_time && get_fsl_mass && get_fsl_pdg && get_fsl_energy)
    {
      fTrueNeutrinoEnergy = TrueNeutrinoEnergy;
      fTrueNeutrinoMomentum_X = TrueNeutrinoMomentum.X();
      fTrueNeutrinoMomentum_Y = TrueNeutrinoMomentum.Y();
      fTrueNeutrinoMomentum_Z = TrueNeutrinoMomentum.Z();
      fTrueNuIntxVtx_X = TrueNuIntxVtx_X;
      fTrueNuIntxVtx_Y = TrueNuIntxVtx_Y;
      fTrueNuIntxVtx_Z = TrueNuIntxVtx_Z;
      fTrueNuIntxVtx_T = TrueNuIntxVtx_T;
      fTrueFSLVtx_X = TrueFSLeptonVtx.X();
      fTrueFSLVtx_Y = TrueFSLeptonVtx.Y();
      fTrueFSLVtx_Z = TrueFSLeptonVtx.Z();
      fTrueFSLMomentum_X = TrueFSLeptonMomentum.X();
      fTrueFSLMomentum_Y = TrueFSLeptonMomentum.Y();
      fTrueFSLMomentum_Z = TrueFSLeptonMomentum.Z();
      fTrueFSLTime = TrueFSLeptonTime;
      fTrueFSLMass = TrueFSLeptonMass;
      fTrueFSLPdg = TrueFSLeptonPdg;
      fTrueFSLEnergy = TrueFSLeptonEnergy;
      fTrueQ2 = TrueQ2;
      fTrueCC = (TrueCC) ? 1 : 0;
      fTrueNC = (TrueNC) ? 1 : 0;
      fTrueQEL = (TrueQEL) ? 1 : 0;
      fTrueRES = (TrueRES) ? 1 : 0;
      fTrueDIS = (TrueDIS) ? 1 : 0;
      fTrueCOH = (TrueCOH) ? 1 : 0;
      fTrueMEC = (TrueMEC) ? 1 : 0;
      fTrueNeutrons = fsNeutrons;
      fTrueProtons = fsProtons;
      fTruePi0 = fsPi0;
      fTruePiPlus = fsPiPlus;
      fTruePiPlusCher = fsPiPlusCher;
      fTruePiMinus = fsPiMinus;
      fTruePiMinusCher = fsPiMinusCher;
      fTrueKPlus = fsKPlus;
      fTrueKPlusCher = fsKPlusCher;
      fTrueKMinus = fsKMinus;
      fTrueKMinusCher = fsKMinusCher;
    }
    else
    {
      Log("ANNIEEventTreeMaker tool: Did not find GENIE information. Continuing building remaining tree", v_message, ANNIEEventTreeMakerVerbosity);
      successful_load = false;
    }
  } // end if hasGenie

  return successful_load;
}

void ANNIEEventTreeMaker::FillTruthRecoDiffInfo(bool successful_mcload, bool successful_recoload)
{
  if (!successful_mcload || !successful_recoload)
  {
    Log("ANNIEEventTreeMaker Tool: Error loading True Muon Vertex or Extended Vertex information.  Continuing to build remaining tree", v_message, ANNIEEventTreeMakerVerbosity);
  }
  else
  {
    // Make sure MCTruth Information is loaded from store
    // Let's fill in stuff from the RecoSummary
    fDeltaVtxX = fRecoVtxX - fTrueVtxX;
    fDeltaVtxY = fRecoVtxY - fTrueVtxY;
    fDeltaVtxZ = fRecoVtxZ - fTrueVtxZ;
    fDeltaVtxT = fRecoVtxTime - fTrueVtxTime;
    fDeltaVtxR = sqrt(pow(fDeltaVtxX, 2) + pow(fDeltaVtxY, 2) + pow(fDeltaVtxZ, 2));
    fDeltaParallel = fDeltaVtxX * fRecoDirX + fDeltaVtxY * fRecoDirY + fDeltaVtxZ * fRecoDirZ;
    fDeltaPerpendicular = sqrt(pow(fDeltaVtxR, 2) - pow(fDeltaParallel, 2));
    fDeltaAzimuth = (fRecoAngle - fTrueAngle) / (TMath::Pi() / 180.0);
    fDeltaZenith = (fRecoPhi - fTruePhi) / (TMath::Pi() / 180.0);
    double cosphi = fTrueDirX * fRecoDirX + fTrueDirY * fRecoDirY + fTrueDirZ * fRecoDirZ;
    double phi = TMath::ACos(cosphi);              // radians
    double TheAngle = phi / (TMath::Pi() / 180.0); // radians->degrees
    fDeltaAngle = TheAngle;
  }
}

void ANNIEEventTreeMaker::RecoSummary()
{

  // get reconstruction output
  double dx = fRecoVtxX - fTrueVtxX;
  double dy = fRecoVtxY - fTrueVtxY;
  double dz = fRecoVtxZ - fTrueVtxZ;
  double dt = fRecoVtxTime - fTrueVtxTime;
  double deltaR = sqrt(dx * dx + dy * dy + dz * dz);
  double cosphi = 0., phi = 0., DeltaAngle = 0.;
  cosphi = fTrueDirX * fRecoDirX + fTrueDirY * fRecoDirY + fTrueDirZ * fRecoDirZ;
  phi = TMath::ACos(cosphi);                // radians
  DeltaAngle = phi / (TMath::Pi() / 180.0); // radians->degrees
  std::cout << "============================================================================" << std::endl;
  std::cout << " Event number " << fEventNumber << std::endl;
  std::cout << "  trueVtx=(" << fTrueVtxX << ", " << fTrueVtxY << ", " << fTrueVtxZ << ", " << fTrueVtxTime << std::endl
            << " TrueMuonEnergy= " << fTrueMuonEnergy << " Primary Pdg = " << fTruePrimaryPdg << std::endl
            << "           " << fTrueDirX << ", " << fTrueDirY << ", " << fTrueDirZ << ") " << std::endl;
  std::cout << "  recoVtx=(" << fRecoVtxX << ", " << fRecoVtxY << ", " << fRecoVtxZ << ", " << fRecoVtxTime << std::endl
            << "           " << fRecoDirX << ", " << fRecoDirY << ", " << fRecoDirZ << ") " << std::endl;
  std::cout << "  DeltaR = " << deltaR << "[cm]" << "\t" << "  DeltaAngle = " << DeltaAngle << " [degree]" << std::endl;
  std::cout << "  FOM = " << fRecoVtxFOM << std::endl;
  std::cout << "  RecoStatus = " << fRecoStatus << std::endl;
  std::cout << std::endl;
}

void ANNIEEventTreeMaker::FillLAPPDWaveform()
{
  // Log("ANNIEEventTreeMaker: Filling LAPPD Waveform", 0, ANNIEEventTreeMakerVerbosity);

  fLAPPDWaveforms = std::vector<std::vector<double>>(60 * 3, std::vector<double>(256, 0.0));

  // m_data->Stores["ANNIEEvent"]->Print(false);

  // cout<<"LAPPDWaveformInputLabel: "<<LAPPDWaveformInputLabel<<endl;

  std::map<unsigned long, vector<Waveform<double>>> lappdData; // waveform data
  m_data->Stores["ANNIEEvent"]->Get(LAPPDWaveformInputLabel, lappdData);

  std::map<unsigned long, vector<Waveform<double>>>::iterator it;
  for (it = lappdData.begin(); it != lappdData.end(); it++)
  {

    // get the waveforms from channel number
    unsigned long channel = it->first;
    channel = channel % 1000 + 1000;
    if ((channel % 1000) % 30 == 5)
      continue;
    Waveform<double> waveforms = it->second.at(0);
    vector<double> wav = *waveforms.GetSamples();
    vector<double> wave = wav;
    if (wave.size() != 256)
    {
      cout << "ANNIEEventTreeMaker: Found a bug waveform at channel " << channel << ", size is " << wave.size() << endl;
      continue;
    }

    for (int i = 0; i < wave.size(); i++)
    {
      wave.at(i) = -wave.at(i);
    }

    int LAPPD_ID = static_cast<int>((channel - 1000) / 60);
    Channel *chan = geom->GetChannel(channel);
    int stripno = chan->GetStripNum();
    int stripSide = chan->GetStripSide();

    // fill this waveform to fLAPPDWaveforms[LAPPD_ID][stripno+stripSide*30][n]
    for (int n = 0; n < 256; n++)
    {
      fLAPPDWaveforms[LAPPD_ID * 60 + stripno + stripSide * 30][n] = wave.at(n);
    }
  }
}

void ANNIEEventTreeMaker::FillLAPPDMCHitInfo()
{
  std::map<unsigned long, std::vector<MCLAPPDHit>> *MCLAPPDHits = nullptr;
  m_data->Stores.at("ANNIEEvent")->Get("MCLAPPDHits", MCLAPPDHits);

  int HitCount = MCLAPPDHits->size();
  Log("ANNIEEventTreeMaker: Filling LAPPD MC Hit Info, got " + to_string(HitCount) + " MC hits", v_message, ANNIEEventTreeMakerVerbosity);

  Position detector_center = geom->GetTankCentre();
  double tank_center_x = detector_center.X();
  double tank_center_y = detector_center.Y();
  double tank_center_z = detector_center.Z();

  for (std::pair<unsigned long, std::vector<MCLAPPDHit>> &&apair : *MCLAPPDHits)
  {
    unsigned long chankey = apair.first;
    std::vector<MCLAPPDHit> hits = apair.second;

    for (MCLAPPDHit &ahit : hits)
    {
      std::vector<double> pos = ahit.GetPosition();
      double x = pos.at(0) - tank_center_x;
      double y = pos.at(1) - tank_center_y;
      double z = pos.at(2) - tank_center_z;

      std::vector<double> local_pos = ahit.GetLocalPosition();
      // double local_x = local_pos.at(1);
      // double local_y = local_pos.at(0);
      double parallel = local_pos.at(0);
      double transverse = local_pos.at(1);

      int tubeID = ahit.GetTubeId();
      double hitTime = ahit.GetTime();
      double hitCharge = ahit.GetCharge();

      fLAPPDMCHitTubeIDs.push_back(tubeID);
      fLAPPDMCHitChankeys.push_back(chankey);
      fLAPPDMCHitTime.push_back(hitTime);
      fLAPPDMCHitCharge.push_back(hitCharge);
      fLAPPDMCHitX.push_back(x);
      fLAPPDMCHitY.push_back(y);
      fLAPPDMCHitZ.push_back(z);
      fLAPPDMCHitParallelPos.push_back(parallel);
      fLAPPDMCHitTransversePos.push_back(transverse);
    }
  }
}
