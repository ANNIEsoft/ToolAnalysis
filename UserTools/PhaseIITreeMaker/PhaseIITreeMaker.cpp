#include "PhaseIITreeMaker.h"

#include <algorithm>
#include <cctype>

PhaseIITreeMaker::PhaseIITreeMaker():Tool(){}


bool PhaseIITreeMaker::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  
  hasGenie = false;
  hasBNBtimingMC = false;
  MCWaveform = false;
  ApplyDeadMask = false;

  m_variables.Get("verbose", verbosity);
  m_variables.Get("IsData",isData);
  m_variables.Get("PMTWaveformSim",MCWaveform);
  m_variables.Get("ApplyDeadMask",ApplyDeadMask);
  m_variables.Get("HasGenie",hasGenie);
  m_variables.Get("HasBNBtimingMC",hasBNBtimingMC);
  m_variables.Get("TankHitInfo_fill", TankHitInfo_fill);
  m_variables.Get("MRDHitInfo_fill", MRDHitInfo_fill);
  m_variables.Get("fillCleanEventsOnly", fillCleanEventsOnly);
  m_variables.Get("MCTruth_fill", MCTruth_fill);
  m_variables.Get("MRDReco_fill", MRDReco_fill);
  m_variables.Get("TankReco_fill", TankReco_fill);
  m_variables.Get("Reweight_fill", Reweight_fill);
  m_variables.Get("RecoDebug_fill", RecoDebug_fill);
  m_variables.Get("SimpleReco_fill", SimpleReco_fill);
  m_variables.Get("RingCounting_fill", RingCounting_fill);
  m_variables.Get("muonTruthRecoDiff_fill", muonTruthRecoDiff_fill);
  m_variables.Get("LAPPDData_fill", LAPPDData_fill);
  m_variables.Get("LAPPDReco_fill", LAPPDReco_fill);

  m_variables.Get("SiPMPulseInfo_fill",SiPMPulseInfo_fill);
  m_variables.Get("TankClusterProcessing",TankClusterProcessing);
  m_variables.Get("MRDClusterProcessing",MRDClusterProcessing);
  m_variables.Get("TriggerProcessing",TriggerProcessing);
  
  m_variables.Get("Digit_fill",Digit_fill);

  m_variables.Get("MuonFitter_fill", MuonFitter_fill);

  // ---------------------------------------------------------------- cMrdMlReco
  // MRDRecoSource: Legacy (default, unchanged behaviour), MrdMLReco (ML-only),
  // or Both (legacy and mrd_ml_* branches side by side).
  MRDRecoSource = "Legacy";
  m_variables.Get("MRDRecoSource", MRDRecoSource);
  if(MRDRecoSource == "Legacy"){
    MrdMLReco_fill = false;
  } else if(MRDRecoSource == "MrdMLReco" || MRDRecoSource == "Both"){
    MrdMLReco_fill = true;
  } else {
    Log("PhaseIITreeMaker Tool: MRDRecoSource must be exactly 'Legacy', "
        "'MrdMLReco', or 'Both'; received '" + MRDRecoSource + "'.", v_error, verbosity);
    return false;
  }
  if(MrdMLReco_fill && !TriggerProcessing){
    Log("PhaseIITreeMaker Tool: MRDRecoSource=" + MRDRecoSource +
        " requires TriggerProcessing=1 (cMRD ML outputs are trigger-level).", v_error, verbosity);
    return false;
  }
  if(MRDRecoSource == "MrdMLReco" && (MRDReco_fill || MuonFitter_fill || SimpleReco_fill)){
    Log("PhaseIITreeMaker Tool: MRDRecoSource=MrdMLReco is ML-only. Set MRDReco_fill, "
        "MuonFitter_fill and SimpleReco_fill to 0, or use MRDRecoSource=Both.", v_error, verbosity);
    return false;
  }
  if(MrdMLReco_fill){
    // Producer handshake: cMrdMlReco::Initialise() publishes its OutputMode.
    // Its absence means cMrdMlReco is missing or placed after this tool.
    auto normalize = [](std::string m){
      std::transform(m.begin(), m.end(), m.begin(),
                     [](unsigned char ch){ return static_cast<char>(std::tolower(ch)); });
      if(m == "smoke-test" || m == "smoke_test" || m == "smoketest") m = "debug";
      return m;
    };
    std::string producer_mode;
    if(!m_data->CStore.Get("MrdMLOutputMode", producer_mode) || producer_mode.empty()){
      Log("PhaseIITreeMaker Tool: MRDRecoSource=" + MRDRecoSource + " but CStore "
          "\"MrdMLOutputMode\" is missing: cMrdMlReco must precede PhaseIITreeMaker "
          "in the ToolChain.", v_error, verbosity);
      return false;
    }
    producer_mode = normalize(producer_mode);
    // Optional override: a debug producer may be written with production
    // branches, never the reverse.
    std::string requested_mode = producer_mode;
    m_variables.Get("MrdMLOutputMode", requested_mode);
    requested_mode = normalize(requested_mode);
    if(requested_mode != "production" && requested_mode != "debug"){
      Log("PhaseIITreeMaker Tool: MrdMLOutputMode must be 'production' or 'debug'; "
          "received '" + requested_mode + "'.", v_error, verbosity);
      return false;
    }
    if(requested_mode == "debug" && producer_mode != "debug"){
      Log("PhaseIITreeMaker Tool: MrdMLOutputMode=debug requested but cMrdMlReco runs "
          "with OutputMode=" + producer_mode + "; debug keys will not exist.", v_error, verbosity);
      return false;
    }
    MrdMLOutputMode = requested_mode;
    MrdMLDebug_fill = (MrdMLOutputMode == "debug");
  }
  Log("PhaseIITreeMaker Tool: MRD reconstruction source = " + MRDRecoSource +
      (MrdMLReco_fill ? ", mrd_ml branch set = " + MrdMLOutputMode : std::string()),
      v_message, verbosity);

  std::string output_filename;
  m_variables.Get("OutputFile", output_filename);
  fOutput_tfile = new TFile(output_filename.c_str(), "recreate");
  fPhaseIITankClusterTree = new TTree("phaseIITankClusterTree", "ANNIE Phase II Tank Cluster Tree");
  fPhaseIIMRDClusterTree = new TTree("phaseIIMRDClusterTree", "ANNIE Phase II MRD Cluster Tree");
  fPhaseIITrigTree = new TTree("phaseIITriggerTree", "ANNIE Phase II Ntuple Trigger Tree");

  m_data->CStore.Get("AuxChannelNumToTypeMap",AuxChannelNumToTypeMap);
  m_data->CStore.Get("ChannelNumToTankPMTSPEChargeMap",ChannelKeyToSPEMap);
  m_data->CStore.Get("pmt_tubeid_to_channelkey_data",pmtid_to_channelkey);
  m_data->CStore.Get("channelkey_to_pmtid",channelkey_to_pmtid);
  m_data->CStore.Get("channelkey_to_mrdpmtid",channelkey_to_mrdpmtid);
  m_data->CStore.Get("mrdpmtid_to_channelkey_data",mrdpmtid_to_channelkey_data);
  m_data->CStore.Get("channelkey_to_faccpmtid",channelkey_to_faccpmtid);

  auto get_geometry= m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",geom);
  if(!get_geometry){
  	Log("PhaseIITreeMaker Tool: Error retrieving Geometry from ANNIEEvent!",v_error,verbosity); 
  	return false; 
  }
 
  if(TankClusterProcessing){
    fPhaseIITankClusterTree->Branch("runNumber",&fRunNumber,"runNumber/I");
    fPhaseIITankClusterTree->Branch("subrunNumber",&fSubrunNumber,"subrunNumber/I");
    fPhaseIITankClusterTree->Branch("runType",&fRunType,"runType/I");
    fPhaseIITankClusterTree->Branch("startTime",&fStartTime_Tree,"startTime/l");

    //Some lower level information to save
    fPhaseIITankClusterTree->Branch("eventNumber",&fEventNumber,"eventNumber/I");
    fPhaseIITankClusterTree->Branch("eventTimeTank",&fEventTimeTank_Tree,"eventTimeTank/l");
    fPhaseIITankClusterTree->Branch("clusterNumber",&fClusterNumber,"clusterNumber/I");
    fPhaseIITankClusterTree->Branch("clusterTime",&fClusterTime,"clusterTime/D");
    fPhaseIITankClusterTree->Branch("clusterCharge",&fClusterCharge,"clusterCharge/D");
    fPhaseIITankClusterTree->Branch("clusterPE",&fClusterPE,"clusterPE/D");
    fPhaseIITankClusterTree->Branch("clusterMaxPE",&fClusterMaxPE,"clusterMaxPE/D");
    fPhaseIITankClusterTree->Branch("clusterChargePointX",&fClusterChargePointX,"clusterChargePointX/D");
    fPhaseIITankClusterTree->Branch("clusterChargePointY",&fClusterChargePointY,"clusterChargePointY/D");
    fPhaseIITankClusterTree->Branch("clusterChargePointZ",&fClusterChargePointZ,"clusterChargePointZ/D");
    fPhaseIITankClusterTree->Branch("clusterChargeBalance",&fClusterChargeBalance,"clusterChargeBalance/D");
    fPhaseIITankClusterTree->Branch("clusterHits",&fClusterHits,"clusterHits/i");
    fPhaseIITankClusterTree->Branch("trigword",&fTriggerword);
    fPhaseIITankClusterTree->Branch("TankMRDCoinc",&fTankMRDCoinc);
    fPhaseIITankClusterTree->Branch("NoVeto",&fNoVeto);
    fPhaseIITankClusterTree->Branch("ADCSamples",&fADCWaveformSamples);
    fPhaseIITankClusterTree->Branch("ADCChankeys",&fADCWaveformChankeys);
    fPhaseIITankClusterTree->Branch("Extended",&fExtended);
    fPhaseIITankClusterTree->Branch("beam_pot",&fPot);
    fPhaseIITankClusterTree->Branch("beam_ok",&fBeamok);
    if(TankHitInfo_fill){
      fPhaseIITankClusterTree->Branch("filter",&fIsFiltered);
      fPhaseIITankClusterTree->Branch("hitX",&fHitX);
      fPhaseIITankClusterTree->Branch("hitY",&fHitY);
      fPhaseIITankClusterTree->Branch("hitZ",&fHitZ);
      fPhaseIITankClusterTree->Branch("hitT",&fHitT);
      fPhaseIITankClusterTree->Branch("hitQ",&fHitQ);
      fPhaseIITankClusterTree->Branch("hitPE",&fHitPE);
      fPhaseIITankClusterTree->Branch("hitType", &fHitType);
      fPhaseIITankClusterTree->Branch("hitDetID", &fHitDetID);
      fPhaseIITankClusterTree->Branch("hitChankey",&fHitChankey);
      fPhaseIITankClusterTree->Branch("hitChankeyMC",&fHitChankeyMC);
    }
    //SiPM Pulse Info; load into both trees for now...
    if(SiPMPulseInfo_fill){
      fPhaseIITankClusterTree->Branch("SiPMhitQ",&fSiPMHitQ);
      fPhaseIITankClusterTree->Branch("SiPMhitT",&fSiPMHitT);
      fPhaseIITankClusterTree->Branch("SiPMhitAmplitude",&fSiPMHitAmplitude);
      fPhaseIITankClusterTree->Branch("SiPMNum",&fSiPMNum);
      fPhaseIITankClusterTree->Branch("SiPM1NPulses",&fSiPM1NPulses,"SiPM1NPulses/I");
      fPhaseIITankClusterTree->Branch("SiPM2NPulses",&fSiPM2NPulses,"SiPM2NPulses/I");
    }
    if(LAPPDData_fill)
    {
      fPhaseIITankClusterTree->Branch("LAPPD_ID",&fLAPPD_ID);
      fPhaseIITankClusterTree->Branch("LAPPD_Beamgate_ns",&fLAPPD_Beamgate_ns);
      fPhaseIITankClusterTree->Branch("LAPPD_Timestamp_ns",&fLAPPD_Timestamp_ns);
      fPhaseIITankClusterTree->Branch("LAPPD_Beamgate_Raw",&fLAPPD_Beamgate_Raw);
      fPhaseIITankClusterTree->Branch("LAPPD_Timestamp_Raw",&fLAPPD_Timestamp_Raw);
      fPhaseIITankClusterTree->Branch("LAPPD_Offset",&fLAPPD_Offset);
      fPhaseIITankClusterTree->Branch("LAPPD_TSCorrection",&fLAPPD_TSCorrection);
      fPhaseIITankClusterTree->Branch("LAPPD_BGCorrection",&fLAPPD_BGCorrection);
      fPhaseIITankClusterTree->Branch("LAPPD_OSInMinusPS",&fLAPPD_OSInMinusPS);

      fPhaseIITankClusterTree->Branch("GroupedTriggerTime",&fGroupedTriggerTime);
      fPhaseIITankClusterTree->Branch("GroupedTriggerWord",&fGroupedTriggerWord);

      if(LAPPDReco_fill)
      {
        fPhaseIITankClusterTree->Branch("LAPPD_PulseIDs",&fLAPPD_IDs);
        fPhaseIITankClusterTree->Branch("LAPPD_ChannelID",&fChannelID);
        fPhaseIITankClusterTree->Branch("LAPPD_PeakTime",&fPulsePeakTime);
        fPhaseIITankClusterTree->Branch("LAPPD_PeakAmp",&fPulsePeakAmp);
        fPhaseIITankClusterTree->Branch("LAPPD_Charge",&fPulseCharge);
        fPhaseIITankClusterTree->Branch("LAPPD_PulseStart",&fPulseStart);
        fPhaseIITankClusterTree->Branch("LAPPD_PulseEnd",&fPulseEnd);
        fPhaseIITankClusterTree->Branch("LAPPD_PulseWidth",&fPulseWidth);
        fPhaseIITankClusterTree->Branch("LAPPD_PulseSide",&fPulseSide);
        fPhaseIITankClusterTree->Branch("LAPPD_PulseStripNum",&fPulseStripNum);

        fPhaseIITankClusterTree->Branch("LAPPDID_Hit",&fLAPPDHit_IDs);
        fPhaseIITankClusterTree->Branch("LAPPDHitChannel",&fLAPPDHitChannel);
        fPhaseIITankClusterTree->Branch("LAPPDHitStrip",&fLAPPDHitStrip);
        fPhaseIITankClusterTree->Branch("LAPPDHitTime",&fLAPPDHitTime);
        fPhaseIITankClusterTree->Branch("LAPPDHitAmp",&fLAPPDHitAmp);
        fPhaseIITankClusterTree->Branch("LAPPDHitParallelPos",&fLAPPDHitParallelPos);
        fPhaseIITankClusterTree->Branch("LAPPDHitTransversePos",&fLAPPDHitTransversePos);
        fPhaseIITankClusterTree->Branch("LAPPDHitP1StartTime",&fLAPPDHitP1StartTime);
        fPhaseIITankClusterTree->Branch("LAPPDHitP2StartTime",&fLAPPDHitP2StartTime);
        fPhaseIITankClusterTree->Branch("LAPPDHitP1EndTime",&fLAPPDHitP1EndTime);
        fPhaseIITankClusterTree->Branch("LAPPDHitP2EndTime",&fLAPPDHitP2EndTime);
      }
    } 
    //MuonFitter reco track length, vtx, energy; juju
    if (MuonFitter_fill)
    {
      fPhaseIITankClusterTree->Branch("recoMuonVtxX", &fRecoMuonVtxX, "recoMuonVtxX/D");
      fPhaseIITankClusterTree->Branch("recoMuonVtxY", &fRecoMuonVtxY, "recoMuonVtxY/D");
      fPhaseIITankClusterTree->Branch("recoMuonVtxZ", &fRecoMuonVtxZ, "recoMuonVtxZ/D");
      fPhaseIITankClusterTree->Branch("recoTankTrack", &fRecoTankTrack, "recoTankTrack/D");
      fPhaseIITankClusterTree->Branch("recoMuonKE", &fRecoMuonKE, "recoMuonKE/D");
    }
    // MC BNB spill structure timing - AssignBunchTimingMC tool
    if(hasBNBtimingMC){
      fPhaseIITankClusterTree->Branch("bunchTimes",&fbunchTimes,"bunchTimes/D");
    }
  } 

  if(MRDClusterProcessing){
    fPhaseIIMRDClusterTree->Branch("runNumber",&fRunNumber,"runNumber/I");
    fPhaseIIMRDClusterTree->Branch("subrunNumber",&fSubrunNumber,"subrunNumber/I");
    fPhaseIIMRDClusterTree->Branch("runType",&fRunType,"runType/I");
    fPhaseIIMRDClusterTree->Branch("startTime",&fStartTime_Tree,"startTime/l");


    //Some lower level information to save
    fPhaseIIMRDClusterTree->Branch("eventNumber",&fEventNumber,"eventNumber/I");
    fPhaseIIMRDClusterTree->Branch("eventTimeMRD",&fEventTimeMRD_Tree,"eventTimeMRD/l");
    fPhaseIIMRDClusterTree->Branch("eventTimeTank",&fEventTimeTank_Tree,"eventTimeTank/l");
    fPhaseIIMRDClusterTree->Branch("clusterNumber",&fMRDClusterNumber,"clusterNumber/I");
    fPhaseIIMRDClusterTree->Branch("clusterTime",&fMRDClusterTime,"clusterTime/D");
    fPhaseIIMRDClusterTree->Branch("clusterTimeSigma",&fMRDClusterTimeSigma,"clusterTimeSigma/D");
    fPhaseIIMRDClusterTree->Branch("clusterHits",&fMRDClusterHits,"clusterHits/i");
    fPhaseIIMRDClusterTree->Branch("trigword",&fTriggerword);
    fPhaseIIMRDClusterTree->Branch("TankMRDCoinc",&fTankMRDCoinc);
    fPhaseIIMRDClusterTree->Branch("NoVeto",&fNoVeto);
    fPhaseIIMRDClusterTree->Branch("Extended",&fExtended);
    fPhaseIIMRDClusterTree->Branch("beam_pot",&fPot);
    fPhaseIIMRDClusterTree->Branch("beam_ok",&fBeamok);

    if(MRDHitInfo_fill){
      fPhaseIIMRDClusterTree->Branch("MRDhitT",&fMRDHitT);
      fPhaseIIMRDClusterTree->Branch("MRDhitDetID", &fMRDHitDetID);
      fPhaseIIMRDClusterTree->Branch("MRDhitChankey", &fMRDHitChankey);
      fPhaseIIMRDClusterTree->Branch("MRDhitChankeyMC", &fMRDHitChankeyMC);
      fPhaseIIMRDClusterTree->Branch("FMVhitT",&fFMVHitT);
      fPhaseIIMRDClusterTree->Branch("FMVhitDetID",&fFMVHitDetID);
      fPhaseIIMRDClusterTree->Branch("FMVhitChankey",&fFMVHitChankey);
      fPhaseIIMRDClusterTree->Branch("FMVhitChankeyMC",&fFMVHitChankeyMC);
      fPhaseIIMRDClusterTree->Branch("vetoHit",&fVetoHit,"vetoHit/I");
    }
    if(MRDReco_fill){
      fPhaseIIMRDClusterTree->Branch("numClusterTracks",&fNumClusterTracks,"numClusterTracks/I");
    //Push back some properties
      fPhaseIIMRDClusterTree->Branch("MRDTrackAngle",&fMRDTrackAngle);
      fPhaseIIMRDClusterTree->Branch("MRDTrackAngleError",&fMRDTrackAngleError);
      fPhaseIIMRDClusterTree->Branch("MRDPenetrationDepth",&fMRDPenetrationDepth);
      fPhaseIIMRDClusterTree->Branch("MRDTrackLength",&fMRDTrackLength);
      fPhaseIIMRDClusterTree->Branch("MRDEntryPointRadius",&fMRDEntryPointRadius);
      fPhaseIIMRDClusterTree->Branch("MRDEnergyLoss",&fMRDEnergyLoss);
      fPhaseIIMRDClusterTree->Branch("MRDEnergyLossError",&fMRDEnergyLossError);
      fPhaseIIMRDClusterTree->Branch("MRDTrackStartX",&fMRDTrackStartX);
      fPhaseIIMRDClusterTree->Branch("MRDTrackStartY",&fMRDTrackStartY);
      fPhaseIIMRDClusterTree->Branch("MRDTrackStartZ",&fMRDTrackStartZ);
      fPhaseIIMRDClusterTree->Branch("MRDTrackStopX",&fMRDTrackStopX);
      fPhaseIIMRDClusterTree->Branch("MRDTrackStopY",&fMRDTrackStopY);
      fPhaseIIMRDClusterTree->Branch("MRDTrackStopZ",&fMRDTrackStopZ);
      fPhaseIIMRDClusterTree->Branch("MRDSide",&fMRDSide);
      fPhaseIIMRDClusterTree->Branch("MRDStop",&fMRDStop);
      fPhaseIIMRDClusterTree->Branch("MRDThrough",&fMRDThrough);
    }

    if(LAPPDData_fill)
    {
      fPhaseIIMRDClusterTree->Branch("LAPPD_ID",&fLAPPD_ID);
      fPhaseIIMRDClusterTree->Branch("LAPPD_Beamgate_ns",&fLAPPD_Beamgate_ns);
      fPhaseIIMRDClusterTree->Branch("LAPPD_Timestamp_ns",&fLAPPD_Timestamp_ns);
      fPhaseIIMRDClusterTree->Branch("LAPPD_Beamgate_Raw",&fLAPPD_Beamgate_Raw);
      fPhaseIIMRDClusterTree->Branch("LAPPD_Timestamp_Raw",&fLAPPD_Timestamp_Raw);
      fPhaseIIMRDClusterTree->Branch("LAPPD_Offset",&fLAPPD_Offset);
      fPhaseIIMRDClusterTree->Branch("LAPPD_TSCorrection",&fLAPPD_TSCorrection);
      fPhaseIIMRDClusterTree->Branch("LAPPD_BGCorrection",&fLAPPD_BGCorrection);
      fPhaseIIMRDClusterTree->Branch("LAPPD_OSInMinusPS",&fLAPPD_OSInMinusPS);

      fPhaseIIMRDClusterTree->Branch("GroupedTriggerTime",&fGroupedTriggerTime);
      fPhaseIIMRDClusterTree->Branch("GroupedTriggerWord",&fGroupedTriggerWord);

      if(LAPPDReco_fill)
      {
        fPhaseIIMRDClusterTree->Branch("LAPPD_PulseIDs",&fLAPPD_IDs);
        fPhaseIIMRDClusterTree->Branch("LAPPD_ChannelID",&fChannelID);
        fPhaseIIMRDClusterTree->Branch("LAPPD_PeakTime",&fPulsePeakTime);
        fPhaseIIMRDClusterTree->Branch("LAPPD_PeakAmp",&fPulsePeakAmp);
        fPhaseIIMRDClusterTree->Branch("LAPPD_Charge",&fPulseCharge);
        fPhaseIIMRDClusterTree->Branch("LAPPD_PulseStart",&fPulseStart);
        fPhaseIIMRDClusterTree->Branch("LAPPD_PulseEnd",&fPulseEnd);
        fPhaseIIMRDClusterTree->Branch("LAPPD_PulseWidth",&fPulseWidth);
        fPhaseIIMRDClusterTree->Branch("LAPPD_PulseSide",&fPulseSide);
        fPhaseIIMRDClusterTree->Branch("LAPPD_PulseStripNum",&fPulseStripNum);

        fPhaseIIMRDClusterTree->Branch("LAPPDID_Hit",&fLAPPDHit_IDs);
        fPhaseIIMRDClusterTree->Branch("LAPPDHitChannel",&fLAPPDHitChannel);
        fPhaseIIMRDClusterTree->Branch("LAPPDHitStrip",&fLAPPDHitStrip);
        fPhaseIIMRDClusterTree->Branch("LAPPDHitTime",&fLAPPDHitTime);
        fPhaseIIMRDClusterTree->Branch("LAPPDHitAmp",&fLAPPDHitAmp);
        fPhaseIIMRDClusterTree->Branch("LAPPDHitParallelPos",&fLAPPDHitParallelPos);
        fPhaseIIMRDClusterTree->Branch("LAPPDHitTransversePos",&fLAPPDHitTransversePos);
        fPhaseIIMRDClusterTree->Branch("LAPPDHitP1StartTime",&fLAPPDHitP1StartTime);
        fPhaseIIMRDClusterTree->Branch("LAPPDHitP2StartTime",&fLAPPDHitP2StartTime);
        fPhaseIIMRDClusterTree->Branch("LAPPDHitP1EndTime",&fLAPPDHitP1EndTime);
        fPhaseIIMRDClusterTree->Branch("LAPPDHitP2EndTime",&fLAPPDHitP2EndTime);
      }

    } 

  }

  if(TriggerProcessing){
    if(MrdMLReco_fill){
      // cMrdMlReco production branches
      fPhaseIITrigTree->Branch("mrd_ml_failure_code", &fMrdMlFailureCode, "mrd_ml_failure_code/I");
      fPhaseIITrigTree->Branch("mrd_ml_reco_skip_reason", &fMrdMlRecoSkipReason, "mrd_ml_reco_skip_reason/I");
      fPhaseIITrigTree->Branch("mrd_ml_gate_passed", &fMrdMlGatePassed, "mrd_ml_gate_passed/O");
      fPhaseIITrigTree->Branch("mrd_ml_reconstruction_available", &fMrdMlReconstructionAvailable, "mrd_ml_reconstruction_available/O");
      fPhaseIITrigTree->Branch("mrd_ml_mrd_input_source", &fMrdMlMRDInputSource);
      fPhaseIITrigTree->Branch("mrd_ml_number_mrd_time_clusters", &fMrdMlNumberMrdTimeClusters, "mrd_ml_number_mrd_time_clusters/I");
      fPhaseIITrigTree->Branch("mrd_ml_selected_mrd_cluster_index", &fMrdMlSelectedMrdClusterIndex, "mrd_ml_selected_mrd_cluster_index/I");
      fPhaseIITrigTree->Branch("mrd_ml_selected_tank_cluster_index", &fMrdMlSelectedTankClusterIndex, "mrd_ml_selected_tank_cluster_index/I");
      fPhaseIITrigTree->Branch("mrd_ml_input_tank_hits", &fMrdMlInputTankHits, "mrd_ml_input_tank_hits/I");
      fPhaseIITrigTree->Branch("mrd_ml_reco_used_tank_bypass", &fMrdMlRecoUsedTankBypass, "mrd_ml_reco_used_tank_bypass/O");
      fPhaseIITrigTree->Branch("mrd_ml_tank_mrd_association_found", &fMrdMlTankMrdAssociationFound, "mrd_ml_tank_mrd_association_found/O");
      fPhaseIITrigTree->Branch("mrd_ml_tank_mrd_selected_tank_cluster_index", &fMrdMlTankMrdSelectedTankClusterIndex, "mrd_ml_tank_mrd_selected_tank_cluster_index/I");
      fPhaseIITrigTree->Branch("mrd_ml_tank_mrd_selected_mrd_subevent_id", &fMrdMlTankMrdSelectedMrdSubEventID, "mrd_ml_tank_mrd_selected_mrd_subevent_id/I");
      fPhaseIITrigTree->Branch("mrd_ml_tank_mrd_timing_residual_ns", &fMrdMlTankMrdTimingResidualNs, "mrd_ml_tank_mrd_timing_residual_ns/D");
      fPhaseIITrigTree->Branch("mrd_ml_has_target_muon_probability", &fMrdMlHasTargetMuonProbability, "mrd_ml_has_target_muon_probability/D");
      fPhaseIITrigTree->Branch("mrd_ml_reconstructable_probability", &fMrdMlReconstructableProbability, "mrd_ml_reconstructable_probability/D");
      fPhaseIITrigTree->Branch("mrd_ml_event_category", &fMrdMlEventCategory, "mrd_ml_event_category/I");
      fPhaseIITrigTree->Branch("mrd_ml_topology_class", &fMrdMlTopologyClass, "mrd_ml_topology_class/I");
      fPhaseIITrigTree->Branch("mrd_ml_topology_probabilities", &fMrdMlTopologyProbabilities);
      fPhaseIITrigTree->Branch("mrd_ml_stopped_probability", &fMrdMlStoppedProbability, "mrd_ml_stopped_probability/D");
      fPhaseIITrigTree->Branch("mrd_ml_exit_face_class", &fMrdMlExitFaceClass, "mrd_ml_exit_face_class/I");
      fPhaseIITrigTree->Branch("mrd_ml_mrd_track_length_cm", &fMrdMlMRDTrackLengthCm, "mrd_ml_mrd_track_length_cm/D");
      fPhaseIITrigTree->Branch("mrd_ml_mrd_track_length_sigma_cm", &fMrdMlMRDTrackLengthSigmaCm, "mrd_ml_mrd_track_length_sigma_cm/D");
      fPhaseIITrigTree->Branch("mrd_ml_cos_theta", &fMrdMlCosTheta, "mrd_ml_cos_theta/D");
      fPhaseIITrigTree->Branch("mrd_ml_cos_theta_sigma", &fMrdMlCosThetaSigma, "mrd_ml_cos_theta_sigma/D");
      fPhaseIITrigTree->Branch("mrd_ml_theta_rad", &fMrdMlThetaRad, "mrd_ml_theta_rad/D");
      fPhaseIITrigTree->Branch("mrd_ml_theta_sigma_rad", &fMrdMlThetaSigmaRad, "mrd_ml_theta_sigma_rad/D");
      fPhaseIITrigTree->Branch("mrd_ml_theta_sigma_valid", &fMrdMlThetaSigmaValid, "mrd_ml_theta_sigma_valid/O");
      fPhaseIITrigTree->Branch("mrd_ml_phi_rad", &fMrdMlPhiRad, "mrd_ml_phi_rad/D");
      fPhaseIITrigTree->Branch("mrd_ml_penetration_cm", &fMrdMlPenetrationCm, "mrd_ml_penetration_cm/D");
      fPhaseIITrigTree->Branch("mrd_ml_penetration_sigma_cm", &fMrdMlPenetrationSigmaCm, "mrd_ml_penetration_sigma_cm/D");
      fPhaseIITrigTree->Branch("mrd_ml_mrd_energy_loss_mev", &fMrdMlMRDEnergyLossMeV, "mrd_ml_mrd_energy_loss_mev/D");
      fPhaseIITrigTree->Branch("mrd_ml_mrd_energy_loss_sigma_mev", &fMrdMlMRDEnergyLossSigmaMeV, "mrd_ml_mrd_energy_loss_sigma_mev/D");
      fPhaseIITrigTree->Branch("mrd_ml_muon_ke_mev", &fMrdMlMuonKEMeV, "mrd_ml_muon_ke_mev/D");
      fPhaseIITrigTree->Branch("mrd_ml_muon_ke_sigma_mev", &fMrdMlMuonKESigmaMeV, "mrd_ml_muon_ke_sigma_mev/D");
      fPhaseIITrigTree->Branch("mrd_ml_cherenkov_angle_rad", &fMrdMlCherenkovAngleRad, "mrd_ml_cherenkov_angle_rad/D");
      fPhaseIITrigTree->Branch("mrd_ml_cherenkov_angle_sigma_rad", &fMrdMlCherenkovAngleSigmaRad, "mrd_ml_cherenkov_angle_sigma_rad/D");
      fPhaseIITrigTree->Branch("mrd_ml_tank_vertex_cm", &fMrdMlTankVertexCm);
      fPhaseIITrigTree->Branch("mrd_ml_tank_vertex_sigma_cm", &fMrdMlTankVertexSigmaCm);
      fPhaseIITrigTree->Branch("mrd_ml_tank_exit_cm", &fMrdMlTankExitCm);
      fPhaseIITrigTree->Branch("mrd_ml_tank_exit_sigma_cm", &fMrdMlTankExitSigmaCm);
      fPhaseIITrigTree->Branch("mrd_ml_mrd_entry_cm", &fMrdMlMRDEntryCm);
      fPhaseIITrigTree->Branch("mrd_ml_mrd_entry_sigma_cm", &fMrdMlMRDEntrySigmaCm);
      fPhaseIITrigTree->Branch("mrd_ml_mrd_exit_cm", &fMrdMlMRDExitCm);
      fPhaseIITrigTree->Branch("mrd_ml_mrd_exit_sigma_cm", &fMrdMlMRDExitSigmaCm);
      fPhaseIITrigTree->Branch("mrd_ml_selected_endpoint_cm", &fMrdMlSelectedEndpointCm);
      fPhaseIITrigTree->Branch("mrd_ml_selected_endpoint_sigma_cm", &fMrdMlSelectedEndpointSigmaCm);
      fPhaseIITrigTree->Branch("mrd_ml_mrd_scatter_rad", &fMrdMlMRDScatterRad);
      fPhaseIITrigTree->Branch("mrd_ml_mrd_scatter_sigma_rad", &fMrdMlMRDScatterSigmaRad);
      fPhaseIITrigTree->Branch("mrd_ml_tank_scatter_rad", &fMrdMlTankScatterRad);
      fPhaseIITrigTree->Branch("mrd_ml_tank_scatter_sigma_rad", &fMrdMlTankScatterSigmaRad);
      fPhaseIITrigTree->Branch("mrd_ml_schema_version", &fMrdMlSchemaVersion, "mrd_ml_schema_version/I");
      fPhaseIITrigTree->Branch("mrd_ml_output_mode", &fMrdMlOutputMode);
      fPhaseIITrigTree->Branch("mrd_ml_model_release", &fMrdMlModelRelease);
      if(MrdMLDebug_fill){
        // cMrdMlReco debug branches
        fPhaseIITrigTree->Branch("mrd_ml_protocol_version", &fMrdMlProtocolVersion, "mrd_ml_protocol_version/I");
        fPhaseIITrigTree->Branch("mrd_ml_status_text", &fMrdMlStatusText);
        fPhaseIITrigTree->Branch("mrd_ml_error_text", &fMrdMlErrorText);
        fPhaseIITrigTree->Branch("mrd_ml_input_build_ok", &fMrdMlInputBuildOK, "mrd_ml_input_build_ok/O");
        fPhaseIITrigTree->Branch("mrd_ml_gate_attempted", &fMrdMlGateAttempted, "mrd_ml_gate_attempted/O");
        fPhaseIITrigTree->Branch("mrd_ml_gate_inference_ok", &fMrdMlGateInferenceOK, "mrd_ml_gate_inference_ok/O");
        fPhaseIITrigTree->Branch("mrd_ml_reco_attempted", &fMrdMlRecoAttempted, "mrd_ml_reco_attempted/O");
        fPhaseIITrigTree->Branch("mrd_ml_reco_inference_ok", &fMrdMlRecoInferenceOK, "mrd_ml_reco_inference_ok/O");
        fPhaseIITrigTree->Branch("mrd_ml_daemon_restart_count", &fMrdMlDaemonRestartCount, "mrd_ml_daemon_restart_count/I");
        fPhaseIITrigTree->Branch("mrd_ml_selected_mrd_cluster_start_time_ns", &fMrdMlSelectedMrdClusterStartTimeNs, "mrd_ml_selected_mrd_cluster_start_time_ns/D");
        fPhaseIITrigTree->Branch("mrd_ml_selected_mrd_cluster_end_time_ns", &fMrdMlSelectedMrdClusterEndTimeNs, "mrd_ml_selected_mrd_cluster_end_time_ns/D");
        fPhaseIITrigTree->Branch("mrd_ml_selected_mrd_cluster_time_span_ns", &fMrdMlSelectedMrdClusterTimeSpanNs, "mrd_ml_selected_mrd_cluster_time_span_ns/D");
        fPhaseIITrigTree->Branch("mrd_ml_input_mrd_raw_hits", &fMrdMlInputMRDRawHits, "mrd_ml_input_mrd_raw_hits/I");
        fPhaseIITrigTree->Branch("mrd_ml_input_mrd_selected_raw_hits", &fMrdMlInputMRDSelectedRawHits, "mrd_ml_input_mrd_selected_raw_hits/I");
        fPhaseIITrigTree->Branch("mrd_ml_input_mrd_geometry_valid_hits_before_dead_mask", &fMrdMlInputMRDGeometryValidHitsBeforeDeadMask, "mrd_ml_input_mrd_geometry_valid_hits_before_dead_mask/I");
        fPhaseIITrigTree->Branch("mrd_ml_input_mrd_known_dead_removed", &fMrdMlInputMRDKnownDeadRemoved, "mrd_ml_input_mrd_known_dead_removed/I");
        fPhaseIITrigTree->Branch("mrd_ml_input_mrd_live_hits_before_dedup", &fMrdMlInputMRDLiveHitsBeforeDedup, "mrd_ml_input_mrd_live_hits_before_dedup/I");
        fPhaseIITrigTree->Branch("mrd_ml_input_mrd_live_unique_hits", &fMrdMlInputMRDLiveUniqueHits, "mrd_ml_input_mrd_live_unique_hits/I");
        fPhaseIITrigTree->Branch("mrd_ml_input_mrd_dedup_removed", &fMrdMlInputMRDDedupRemoved, "mrd_ml_input_mrd_dedup_removed/I");
        fPhaseIITrigTree->Branch("mrd_ml_input_mrd_unassigned_hits", &fMrdMlInputMRDUnassignedHits, "mrd_ml_input_mrd_unassigned_hits/I");
        fPhaseIITrigTree->Branch("mrd_ml_input_mrd_layers", &fMrdMlInputMRDLayers, "mrd_ml_input_mrd_layers/I");
        fPhaseIITrigTree->Branch("mrd_ml_input_mrd_first_layer", &fMrdMlInputMRDFirstLayer, "mrd_ml_input_mrd_first_layer/I");
        fPhaseIITrigTree->Branch("mrd_ml_input_mrd_last_layer", &fMrdMlInputMRDLastLayer, "mrd_ml_input_mrd_last_layer/I");
        fPhaseIITrigTree->Branch("mrd_ml_tank_input_source", &fMrdMlTankInputSource);
        fPhaseIITrigTree->Branch("mrd_ml_number_tank_clusters", &fMrdMlNumberTankClusters, "mrd_ml_number_tank_clusters/I");
        fPhaseIITrigTree->Branch("mrd_ml_selected_tank_cluster_time_ns", &fMrdMlSelectedTankClusterTimeNs, "mrd_ml_selected_tank_cluster_time_ns/D");
        fPhaseIITrigTree->Branch("mrd_ml_selected_tank_cluster_charge_pe", &fMrdMlSelectedTankClusterChargePE, "mrd_ml_selected_tank_cluster_charge_pe/D");
        fPhaseIITrigTree->Branch("mrd_ml_selected_tank_cluster_time_span_ns", &fMrdMlSelectedTankClusterTimeSpanNs, "mrd_ml_selected_tank_cluster_time_span_ns/D");
        fPhaseIITrigTree->Branch("mrd_ml_tank_packet_reordered_by_time", &fMrdMlTankPacketReorderedByTime, "mrd_ml_tank_packet_reordered_by_time/O");
        fPhaseIITrigTree->Branch("mrd_ml_additional_dead_mrd_chankeys", &fMrdMlAdditionalDeadMRDChankeys);
        fPhaseIITrigTree->Branch("mrd_ml_reco_structurally_eligible", &fMrdMlRecoStructurallyEligible, "mrd_ml_reco_structurally_eligible/O");
        fPhaseIITrigTree->Branch("mrd_ml_physics_is_raw_unstandardized", &fMrdMlPhysicsIsRawUnstandardized, "mrd_ml_physics_is_raw_unstandardized/O");
        fPhaseIITrigTree->Branch("mrd_ml_tank_mrd_association_applied", &fMrdMlTankMrdAssociationApplied, "mrd_ml_tank_mrd_association_applied/O");
        fPhaseIITrigTree->Branch("mrd_ml_tank_mrd_compatible_pair_count", &fMrdMlTankMrdCompatiblePairCount, "mrd_ml_tank_mrd_compatible_pair_count/I");
        fPhaseIITrigTree->Branch("mrd_ml_tank_mrd_expected_offset_ns", &fMrdMlTankMrdExpectedOffsetNs, "mrd_ml_tank_mrd_expected_offset_ns/D");
        fPhaseIITrigTree->Branch("mrd_ml_tank_mrd_coincidence_window_ns", &fMrdMlTankMrdCoincidenceWindowNs, "mrd_ml_tank_mrd_coincidence_window_ns/D");
        fPhaseIITrigTree->Branch("mrd_ml_tank_mrd_tank_mean_time_ns", &fMrdMlTankMrdTankMeanTimeNs, "mrd_ml_tank_mrd_tank_mean_time_ns/D");
        fPhaseIITrigTree->Branch("mrd_ml_tank_mrd_mrd_mean_time_ns", &fMrdMlTankMrdMrdMeanTimeNs, "mrd_ml_tank_mrd_mrd_mean_time_ns/D");
        fPhaseIITrigTree->Branch("mrd_ml_tank_mrd_dt_ns", &fMrdMlTankMrdDtNs, "mrd_ml_tank_mrd_dt_ns/D");
        fPhaseIITrigTree->Branch("mrd_ml_event_category_confidence", &fMrdMlEventCategoryConfidence, "mrd_ml_event_category_confidence/D");
        fPhaseIITrigTree->Branch("mrd_ml_event_category_margin", &fMrdMlEventCategoryMargin, "mrd_ml_event_category_margin/D");
        fPhaseIITrigTree->Branch("mrd_ml_event_category_probabilities", &fMrdMlEventCategoryProbabilities);
        fPhaseIITrigTree->Branch("mrd_ml_single_muon_compatibility_probability", &fMrdMlSingleMuonCompatibilityProbability, "mrd_ml_single_muon_compatibility_probability/D");
        fPhaseIITrigTree->Branch("mrd_ml_single_muon_compatibility_validated", &fMrdMlSingleMuonCompatibilityValidated, "mrd_ml_single_muon_compatibility_validated/O");
        fPhaseIITrigTree->Branch("mrd_ml_topology_confidence", &fMrdMlTopologyConfidence, "mrd_ml_topology_confidence/D");
        fPhaseIITrigTree->Branch("mrd_ml_topology_margin", &fMrdMlTopologyMargin, "mrd_ml_topology_margin/D");
        fPhaseIITrigTree->Branch("mrd_ml_topology_p_stopped", &fMrdMlTopologyPStopped, "mrd_ml_topology_p_stopped/D");
        fPhaseIITrigTree->Branch("mrd_ml_topology_p_back_exit", &fMrdMlTopologyPBackExit, "mrd_ml_topology_p_back_exit/D");
        fPhaseIITrigTree->Branch("mrd_ml_topology_p_side_exit", &fMrdMlTopologyPSideExit, "mrd_ml_topology_p_side_exit/D");
        fPhaseIITrigTree->Branch("mrd_ml_exit_face_confidence", &fMrdMlExitFaceConfidence, "mrd_ml_exit_face_confidence/D");
        fPhaseIITrigTree->Branch("mrd_ml_exit_face_margin", &fMrdMlExitFaceMargin, "mrd_ml_exit_face_margin/D");
        fPhaseIITrigTree->Branch("mrd_ml_exit_face_probabilities", &fMrdMlExitFaceProbabilities);
        fPhaseIITrigTree->Branch("mrd_ml_endpoint_expert_used", &fMrdMlEndpointExpertUsed, "mrd_ml_endpoint_expert_used/I");
        fPhaseIITrigTree->Branch("mrd_ml_sin_phi", &fMrdMlSinPhi, "mrd_ml_sin_phi/D");
        fPhaseIITrigTree->Branch("mrd_ml_cos_phi", &fMrdMlCosPhi, "mrd_ml_cos_phi/D");
        fPhaseIITrigTree->Branch("mrd_ml_phi_vector_norm", &fMrdMlPhiVectorNorm, "mrd_ml_phi_vector_norm/D");
        fPhaseIITrigTree->Branch("mrd_ml_stop_endpoint_cm", &fMrdMlStopEndpointCm);
        fPhaseIITrigTree->Branch("mrd_ml_stop_endpoint_sigma_cm", &fMrdMlStopEndpointSigmaCm);
        fPhaseIITrigTree->Branch("mrd_ml_back_exit_endpoint_cm", &fMrdMlBackExitEndpointCm);
        fPhaseIITrigTree->Branch("mrd_ml_back_exit_endpoint_sigma_cm", &fMrdMlBackExitEndpointSigmaCm);
        fPhaseIITrigTree->Branch("mrd_ml_side_exit_endpoint_cm", &fMrdMlSideExitEndpointCm);
        fPhaseIITrigTree->Branch("mrd_ml_side_exit_endpoint_sigma_cm", &fMrdMlSideExitEndpointSigmaCm);
        fPhaseIITrigTree->Branch("mrd_ml_quality_baseline", &fMrdMlQualityBaseline, "mrd_ml_quality_baseline/O");
        fPhaseIITrigTree->Branch("mrd_ml_quality_topology", &fMrdMlQualityTopology, "mrd_ml_quality_topology/O");
        fPhaseIITrigTree->Branch("mrd_ml_quality_direction", &fMrdMlQualityDirection, "mrd_ml_quality_direction/O");
        fPhaseIITrigTree->Branch("mrd_ml_quality_endpoint", &fMrdMlQualityEndpoint, "mrd_ml_quality_endpoint/O");
        fPhaseIITrigTree->Branch("mrd_ml_quality_tier", &fMrdMlQualityTier, "mrd_ml_quality_tier/I");
      }
    }

    //Metadata for standard Events
    fPhaseIITrigTree->Branch("runNumber",&fRunNumber,"runNumber/I");
    fPhaseIITrigTree->Branch("subrunNumber",&fSubrunNumber,"subrunNumber/I");
    fPhaseIITrigTree->Branch("runType",&fRunType,"runType/I");
    fPhaseIITrigTree->Branch("startTime",&fStartTime_Tree,"startTime/l");
    
    
 //DIGITS   
  if(Digit_fill){
      fPhaseIITrigTree->Branch("digitX",&fdigitX);
      fPhaseIITrigTree->Branch("digitY",&fdigitY);
      fPhaseIITrigTree->Branch("digitZ",&fdigitZ);
      fPhaseIITrigTree->Branch("digitT",&fdigitT);
      fPhaseIITrigTree->Branch("NDigitsPMTs",&fNDigitsPMTs);
      fPhaseIITrigTree->Branch("NDigitsLAPPDs",&fNDigitsLAPPDs);
      
    }
 //DIGITS   

    //Some lower level information to save
    fPhaseIITrigTree->Branch("eventNumber",&fEventNumber,"eventNumber/I");
    fPhaseIITrigTree->Branch("eventTimeTank",&fEventTimeTank_Tree,"eventTimeTank/l");
    fPhaseIITrigTree->Branch("eventTimeMRD",&fEventTimeMRD_Tree,"eventTimeMRD/l");
    fPhaseIITrigTree->Branch("nhits",&fNHits,"nhits/I");

    //CTC & event selection related variables
    fPhaseIITrigTree->Branch("trigword",&fTriggerword);
    fPhaseIITrigTree->Branch("HasTank",&fHasTank);
    fPhaseIITrigTree->Branch("HasMRD",&fHasMRD);
    fPhaseIITrigTree->Branch("TankMRDCoinc",&fTankMRDCoinc);
    fPhaseIITrigTree->Branch("NoVeto",&fNoVeto);

    //Extended window information
    fPhaseIITrigTree->Branch("Extended",&fExtended);
    
    //Beam information
    fPhaseIITrigTree->Branch("beam_pot",&fPot);
    fPhaseIITrigTree->Branch("beam_ok",&fBeamok);

    //Event Staus Flag Information
    if(fillCleanEventsOnly){
      fPhaseIITrigTree->Branch("eventStatusApplied",&fEventStatusApplied,"eventStatusApplied/I");
      fPhaseIITrigTree->Branch("eventStatusFlagged",&fEventStatusFlagged,"eventStatusFlagged/I");
    }
    //Hit information (PMT and LAPPD)
    if(SiPMPulseInfo_fill){
      fPhaseIITrigTree->Branch("SiPM1NPulses",&fSiPM1NPulses,"SiPM1NPulses/I");
      fPhaseIITrigTree->Branch("SiPM2NPulses",&fSiPM2NPulses,"SiPM2NPulses/I");
      fPhaseIITrigTree->Branch("SiPMhitQ",&fSiPMHitQ);
      fPhaseIITrigTree->Branch("SiPMhitT",&fSiPMHitT);
      fPhaseIITrigTree->Branch("SiPMhitAmplitude",&fSiPMHitAmplitude);
      fPhaseIITrigTree->Branch("SiPMNum",&fSiPMNum);
    }

    if(TankHitInfo_fill){
      fPhaseIITrigTree->Branch("filter",&fIsFiltered);
      fPhaseIITrigTree->Branch("hitX",&fHitX);
      fPhaseIITrigTree->Branch("hitY",&fHitY);
      fPhaseIITrigTree->Branch("hitZ",&fHitZ);
      fPhaseIITrigTree->Branch("hitT",&fHitT);
      fPhaseIITrigTree->Branch("hitQ",&fHitQ);
      fPhaseIITrigTree->Branch("hitPE",&fHitPE);
      fPhaseIITrigTree->Branch("hitType", &fHitType);
      fPhaseIITrigTree->Branch("hitDetID", &fHitDetID);
      fPhaseIITrigTree->Branch("hitChankey", &fHitChankey);
      fPhaseIITrigTree->Branch("hitChankeyMC",&fHitChankeyMC);
    }

    if(MRDHitInfo_fill){
      fPhaseIITrigTree->Branch("MRDhitT",&fMRDHitT);
      fPhaseIITrigTree->Branch("MRDhitDetID", &fMRDHitDetID);
      fPhaseIITrigTree->Branch("MRDhitChankey", &fMRDHitChankey);
      fPhaseIITrigTree->Branch("MRDhitChankeyMC", &fMRDHitChankeyMC);
      fPhaseIITrigTree->Branch("FMVhitT",&fFMVHitT);
      fPhaseIITrigTree->Branch("FMVhitDetID",&fFMVHitDetID);
      fPhaseIITrigTree->Branch("FMVhitChankey",&fFMVHitChankey);
      fPhaseIITrigTree->Branch("FMVhitChankeyMC",&fFMVHitChankeyMC);
      fPhaseIITrigTree->Branch("vetoHit",&fVetoHit,"vetoHit/I");
    }
    if(MRDReco_fill){
      fPhaseIITrigTree->Branch("numMRDTracks",&fNumClusterTracks,"numMRDTracks/I");
    //Push back some properties
      fPhaseIITrigTree->Branch("MRDTrackAngle",&fMRDTrackAngle);
      fPhaseIITrigTree->Branch("MRDTrackAngleError",&fMRDTrackAngleError);
      fPhaseIITrigTree->Branch("MRDPenetrationDepth",&fMRDPenetrationDepth);
      fPhaseIITrigTree->Branch("MRDTrackLength",&fMRDTrackLength);
      fPhaseIITrigTree->Branch("MRDEntryPointRadius",&fMRDEntryPointRadius);
      fPhaseIITrigTree->Branch("MRDEnergyLoss",&fMRDEnergyLoss);
      fPhaseIITrigTree->Branch("MRDEnergyLossError",&fMRDEnergyLossError);
      fPhaseIITrigTree->Branch("MRDTrackStartX",&fMRDTrackStartX);
      fPhaseIITrigTree->Branch("MRDTrackStartY",&fMRDTrackStartY);
      fPhaseIITrigTree->Branch("MRDTrackStartZ",&fMRDTrackStartZ);
      fPhaseIITrigTree->Branch("MRDTrackStopX",&fMRDTrackStopX);
      fPhaseIITrigTree->Branch("MRDTrackStopY",&fMRDTrackStopY);
      fPhaseIITrigTree->Branch("MRDTrackStopZ",&fMRDTrackStopZ);
      fPhaseIITrigTree->Branch("MRDSide",&fMRDSide);
      fPhaseIITrigTree->Branch("MRDStop",&fMRDStop);
      fPhaseIITrigTree->Branch("MRDThrough",&fMRDThrough);
    }

    //Reconstructed variables after full Muon Reco Analysis
    if(TankReco_fill){
      fPhaseIITrigTree->Branch("recoVtxX",&fRecoVtxX,"recoVtxX/D");
      fPhaseIITrigTree->Branch("recoVtxY",&fRecoVtxY,"recoVtxY/D");
      fPhaseIITrigTree->Branch("recoVtxZ",&fRecoVtxZ,"recoVtxZ/D");
      fPhaseIITrigTree->Branch("recoVtxTime",&fRecoVtxTime,"recoVtxTime/D");
      fPhaseIITrigTree->Branch("recoDirX",&fRecoDirX,"recoDirX/D");
      fPhaseIITrigTree->Branch("recoDirY",&fRecoDirY,"recoDirY/D");
      fPhaseIITrigTree->Branch("recoDirZ",&fRecoDirZ,"recoDirZ/D");
      fPhaseIITrigTree->Branch("recoAngle",&fRecoAngle,"recoAngle/D");
      fPhaseIITrigTree->Branch("recoPhi",&fRecoPhi,"recoPhi/D");
      fPhaseIITrigTree->Branch("recoVtxFOM",&fRecoVtxFOM,"recoVtxFOM/D");
      fPhaseIITrigTree->Branch("recoStatus",&fRecoStatus,"recoStatus/I");
    }

    //Michael's Simple Reconstruction
    if(SimpleReco_fill){
      fPhaseIITrigTree->Branch("simpleRecoFlag",&fSimpleFlag,"simpleRecoFlag/I");
      fPhaseIITrigTree->Branch("simpleRecoEnergy",&fSimpleEnergy,"simpleRecoEnergy/D");
      fPhaseIITrigTree->Branch("simpleRecoVtxX",&fSimpleVtxX,"simpleRecoVtxX/D");
      fPhaseIITrigTree->Branch("simpleRecoVtxY",&fSimpleVtxY,"simpleRecoVtxY/D");
      fPhaseIITrigTree->Branch("simpleRecoVtxZ",&fSimpleVtxZ,"simpleRecoVtxZ/D");
      fPhaseIITrigTree->Branch("simpleRecoStopVtxX",&fSimpleStopVtxX,"simpleRecoStopVtxX/D");
      fPhaseIITrigTree->Branch("simpleRecoStopVtxY",&fSimpleStopVtxY,"simpleRecoStopVtxY/D");
      fPhaseIITrigTree->Branch("simpleRecoStopVtxZ",&fSimpleStopVtxZ,"simpleRecoStopVtxZ/D");
      fPhaseIITrigTree->Branch("simpleRecoCosTheta",&fSimpleCosTheta,"simpleRecoCosTheta/D");
      fPhaseIITrigTree->Branch("simpleRecoPt",&fSimplePt,"simpleRecoPt/D");
      fPhaseIITrigTree->Branch("simpleRecoFV",&fSimpleFV,"simpleRecoFV/I");
      fPhaseIITrigTree->Branch("simpleRecoMrdEnergyLoss",&fSimpleMrdEnergyLoss,"simpleRecoMrdEnergyLoss/D");
      fPhaseIITrigTree->Branch("simpleRecoTrackLengthInMRD",&fSimpleTrackLengthInMRD,"simpleRecoTrackLengthInMRD/D");
      fPhaseIITrigTree->Branch("simpleRecoMRDStartX",&fSimpleMRDStartX,"simpleRecoMRDStartX/D");
      fPhaseIITrigTree->Branch("simpleRecoMRDStartY",&fSimpleMRDStartY,"simpleRecoMRDStartY/D");
      fPhaseIITrigTree->Branch("simpleRecoMRDStartZ",&fSimpleMRDStartZ,"simpleRecoMRDStartZ/D");
      fPhaseIITrigTree->Branch("simpleRecoMRDStopX",&fSimpleMRDStopX,"simpleRecoMRDStopX/D");
      fPhaseIITrigTree->Branch("simpleRecoMRDStopY",&fSimpleMRDStopY,"simpleRecoMRDStopY/D");
      fPhaseIITrigTree->Branch("simpleRecoMRDStopZ",&fSimpleMRDStopZ,"simpleRecoMRDStopZ/D");
      fPhaseIITrigTree->Branch("simpleRecoTrackLengthInTank",&fSimpleTrackLengthInTank,"simpleRecoTrackLengthInTank/D");
    }
 
    //Ring Counting
    if(RingCounting_fill){
      fPhaseIITrigTree->Branch("RCSRPred",&fRCSRPred,"RCSRPred/D");
      fPhaseIITrigTree->Branch("RCMRPred",&fRCMRPred,"RCMRPred/D");
    }
 
    //MC truth information for muons
    //Output to tree when MCTruth_fill = 1 in config
    if (MCTruth_fill){
      fTrueNeutCapVtxX = new std::vector<double>;
      fTrueNeutCapVtxY = new std::vector<double>;
      fTrueNeutCapVtxZ = new std::vector<double>;
      fTrueNeutCapNucleus = new std::vector<double>;
      fTrueNeutCapTime = new std::vector<double>;
      fTrueNeutCapGammas = new std::vector<double>;
      fTrueNeutCapE = new std::vector<double>;
      fTrueNeutCapGammaE = new std::vector<double>;
      fTruePrimaryPdgs = new std::vector<int>;
      fPhaseIITrigTree->Branch("triggerNumber",&fiMCTriggerNum,"triggerNumber/I");
      fPhaseIITrigTree->Branch("mcEntryNumber",&fMCEventNum,"mcEntryNumber/I");
      fPhaseIITrigTree->Branch("trueVtxX",&fTrueVtxX,"trueVtxX/D");
      fPhaseIITrigTree->Branch("trueVtxY",&fTrueVtxY,"trueVtxY/D");
      fPhaseIITrigTree->Branch("trueVtxZ",&fTrueVtxZ,"trueVtxZ/D");
      fPhaseIITrigTree->Branch("trueVtxTime",&fTrueVtxTime,"trueVtxTime/D");
      fPhaseIITrigTree->Branch("trueDirX",&fTrueDirX,"trueDirX/D");
      fPhaseIITrigTree->Branch("trueDirY",&fTrueDirY,"trueDirY/D");
      fPhaseIITrigTree->Branch("trueDirZ",&fTrueDirZ,"trueDirZ/D");
      fPhaseIITrigTree->Branch("trueAngle",&fTrueAngle,"trueAngle/D");
      fPhaseIITrigTree->Branch("truePhi",&fTruePhi,"truePhi/D");
      fPhaseIITrigTree->Branch("trueMuonEnergy",&fTrueMuonEnergy,"trueMuonEnergy/D");
      fPhaseIITrigTree->Branch("truePrimaryPdg",&fTruePrimaryPdg,"truePrimaryPdg/I");
      fPhaseIITrigTree->Branch("trueTrackLengthInWater",&fTrueTrackLengthInWater,"trueTrackLengthInWater/D");
      fPhaseIITrigTree->Branch("trueTrackLengthInMRD",&fTrueTrackLengthInMRD,"trueTrackLengthInMRD/D");
      fPhaseIITrigTree->Branch("trueMultiRing",&fTrueMultiRing,"trueMultiRing/I");
      fPhaseIITrigTree->Branch("Pi0Count",&fPi0Count,"Pi0Count/I");
      fPhaseIITrigTree->Branch("PiPlusCount",&fPiPlusCount,"PiPlusCount/I");
      fPhaseIITrigTree->Branch("PiMinusCount",&fPiMinusCount,"PiMinusCount/I");
      fPhaseIITrigTree->Branch("K0Count",&fK0Count,"K0Count/I");
      fPhaseIITrigTree->Branch("KPlusCount",&fKPlusCount,"KPlusCount/I");
      fPhaseIITrigTree->Branch("KMinusCount",&fKMinusCount,"KMinusCount/I");
      fPhaseIITrigTree->Branch("truePrimaryPdgs",&fTruePrimaryPdgs);
      fPhaseIITrigTree->Branch("trueNeutCapVtxX",&fTrueNeutCapVtxX);
      fPhaseIITrigTree->Branch("trueNeutCapVtxY",&fTrueNeutCapVtxY);
      fPhaseIITrigTree->Branch("trueNeutCapVtxZ",&fTrueNeutCapVtxZ);
      fPhaseIITrigTree->Branch("trueNeutCapNucleus",&fTrueNeutCapNucleus);
      fPhaseIITrigTree->Branch("trueNeutCapTime",&fTrueNeutCapTime);
      fPhaseIITrigTree->Branch("trueNeutCapGammas",&fTrueNeutCapGammas);
      fPhaseIITrigTree->Branch("trueNeutCapE",&fTrueNeutCapE);
      fPhaseIITrigTree->Branch("trueNeutCapGammaE",&fTrueNeutCapGammaE);
      fPhaseIITrigTree->Branch("trueNeutrinoEnergy",&fTrueNeutrinoEnergy,"trueNeutrinoEnergy/D");
      fPhaseIITrigTree->Branch("trueNuPDG",&fTrueNuPDG,"trueNuPDG/I");
      fPhaseIITrigTree->Branch("trueNeutrinoMomentum_X",&fTrueNeutrinoMomentum_X,"trueNeutrinoMomentum_X/D");
      fPhaseIITrigTree->Branch("trueNeutrinoMomentum_Y",&fTrueNeutrinoMomentum_Y,"trueNeutrinoMomentum_Y/D");
      fPhaseIITrigTree->Branch("trueNeutrinoMomentum_Z",&fTrueNeutrinoMomentum_Z,"trueNeutrinoMomentum_Z/D");
      fPhaseIITrigTree->Branch("trueNuIntxVtx_X",&fTrueNuIntxVtx_X,"trueNuIntxVtx_X/D");
      fPhaseIITrigTree->Branch("trueNuIntxVtx_Y",&fTrueNuIntxVtx_Y,"trueNuIntxVtx_Y/D");
      fPhaseIITrigTree->Branch("trueNuIntxVtx_Z",&fTrueNuIntxVtx_Z,"trueNuIntxVtx_Z/D");
      fPhaseIITrigTree->Branch("trueNuIntxVtx_T",&fTrueNuIntxVtx_T,"trueNuIntxVtx_T/D");
      fPhaseIITrigTree->Branch("trueFSLVtx_X",&fTrueFSLVtx_X,"trueFSLVtx_X/D");
      fPhaseIITrigTree->Branch("trueFSLVtx_Y",&fTrueFSLVtx_Y,"trueFSLVtx_Y/D");
      fPhaseIITrigTree->Branch("trueFSLVtx_Z",&fTrueFSLVtx_Z,"trueFSLVtx_Z/D");
      fPhaseIITrigTree->Branch("trueFSLMomentum_X",&fTrueFSLMomentum_X,"trueFSLMomentum_X/D");
      fPhaseIITrigTree->Branch("trueFSLMomentum_Y",&fTrueFSLMomentum_Y,"trueFSLMomentum_Y/D");
      fPhaseIITrigTree->Branch("trueFSLMomentum_Z",&fTrueFSLMomentum_Z,"trueFSLMomentum_Z/D");
      fPhaseIITrigTree->Branch("trueFSLTime",&fTrueFSLTime,"trueFSLTime/D");
      fPhaseIITrigTree->Branch("trueFSLMass",&fTrueFSLMass,"trueFSLMass/D");
      fPhaseIITrigTree->Branch("trueFSLPdg",&fTrueFSLPdg,"trueFSLPdg/I");
      fPhaseIITrigTree->Branch("trueFSLEnergy",&fTrueFSLEnergy,"trueFSLEnergy/D");
      fPhaseIITrigTree->Branch("trueQ2",&fTrueQ2,"trueQ2/D");
      fPhaseIITrigTree->Branch("trueCC",&fTrueCC,"trueCC/I");
      fPhaseIITrigTree->Branch("trueNC",&fTrueNC,"trueNC/I");
      fPhaseIITrigTree->Branch("trueQEL",&fTrueQEL,"trueQEL/I");
      fPhaseIITrigTree->Branch("trueRES",&fTrueRES,"trueRES/I");
      fPhaseIITrigTree->Branch("trueDIS",&fTrueDIS,"trueDIS/I");
      fPhaseIITrigTree->Branch("trueCOH",&fTrueCOH,"trueCOH/I");
      fPhaseIITrigTree->Branch("trueMEC",&fTrueMEC,"trueMEC/I");
      fPhaseIITrigTree->Branch("trueNeutrons",&fTrueNeutrons,"trueNeutrons/I");
      fPhaseIITrigTree->Branch("trueProtons",&fTrueProtons,"trueProtons/I");
      fPhaseIITrigTree->Branch("truePi0",&fTruePi0,"truePi0/I");
      fPhaseIITrigTree->Branch("truePiPlus",&fTruePiPlus,"truePiPlus/I");
      fPhaseIITrigTree->Branch("truePiPlusCher",&fTruePiPlusCher,"truePiPlusCher/I");
      fPhaseIITrigTree->Branch("truePiMinus",&fTruePiMinus,"truePiMinus/I");
      fPhaseIITrigTree->Branch("truePiMinusCher",&fTruePiMinusCher,"truePiMinusCher/I");
      fPhaseIITrigTree->Branch("trueKPlus",&fTrueKPlus,"trueKPlus/I");
      fPhaseIITrigTree->Branch("trueKPlusCher",&fTrueKPlusCher,"trueKPlusCher/I"); 
      fPhaseIITrigTree->Branch("trueKMinus",&fTrueKMinus,"trueKMinus/I");
      fPhaseIITrigTree->Branch("trueKMinusCher",&fTrueKMinusCher,"trueKMinusCher/I"); 
      fPhaseIITrigTree->Branch("trueBJx",&fTrueBJx,"trueBJx/D");
      fPhaseIITrigTree->Branch("truey",&fTruey,"truey/D");
      fPhaseIITrigTree->Branch("trueq0",&fTrueq0,"trueq0/D");
      fPhaseIITrigTree->Branch("trueq3",&fTrueq3,"trueq3/D");
      fPhaseIITrigTree->Branch("trueTargetZ",&fTrueTarget,"trueTargetZ/I");
      fPhaseIITrigTree->Branch("trueW2",&fTrueW2,"trueW2/D");
    }

    if (Reweight_fill){
      fPhaseIITrigTree->Branch("XSecWeights",&fxsec_weights);
      fPhaseIITrigTree->Branch("FluxWeights",&fflux_weights);
      fPhaseIITrigTree->Branch("weight_All0_UBGenie",&fAll0);
      fPhaseIITrigTree->Branch("weight_All1_UBGenie",&fAll1);
      fPhaseIITrigTree->Branch("weight_All2_UBGenie",&fAll2);
      fPhaseIITrigTree->Branch("weight_All3_UBGenie",&fAll3);
      fPhaseIITrigTree->Branch("weight_All4_UBGenie",&fAll4);
      fPhaseIITrigTree->Branch("weight_All5_UBGenie",&fAll5);
      fPhaseIITrigTree->Branch("weight_AxFFCCQEshape_UBGenie",&fAxFFCCQEshape);
      fPhaseIITrigTree->Branch("weight_DecayAngMEC_UBGenie",&fDecayAngMEC);
      fPhaseIITrigTree->Branch("weight_NormCCCOH_UBGenie",&fNormCCCOH);
      fPhaseIITrigTree->Branch("weight_Norm_NCCOH_UBGenie",&fNorm_NCCOH);
      fPhaseIITrigTree->Branch("weight_RPA_CCQE_UBGenie",&fRPA_CCQE);
      fPhaseIITrigTree->Branch("weight_RootinoFix_UBGenie",&fRootinoFix);
      fPhaseIITrigTree->Branch("weight_ThetaDelta2NRad_UBGenie",&fThetaDelta2NRad);
      fPhaseIITrigTree->Branch("weight_Theta_Delta2Npi_UBGenie",&fTheta_Delta2Npi);
      fPhaseIITrigTree->Branch("weight_TunedCentralValue_UBGenie",&fTunedCentralValue);
      fPhaseIITrigTree->Branch("weight_VecFFCCQEshape_UBGenie",&fVecFFCCQEshape);
      fPhaseIITrigTree->Branch("weight_XSecShape_CCMEC_UBGenie",&fXSecShape_CCMEC); 
      fPhaseIITrigTree->Branch("weight_horncurrent_FluxUnisim",&fhorncurrent);
      fPhaseIITrigTree->Branch("weight_expskin_FluxUnisim",&fexpskin);
      fPhaseIITrigTree->Branch("weight_piplus_PrimaryHadronSWCentralSplineVariation",&fpiplus);
      fPhaseIITrigTree->Branch("weight_piminus_PrimaryHadronSWCentralSplineVariation",&fpiminus);
      fPhaseIITrigTree->Branch("weight_kplus_PrimaryHadronFeynmanScaling",&fkplus);
      fPhaseIITrigTree->Branch("weight_kzero_PrimaryHadronSanfordWang",&fkzero);
      fPhaseIITrigTree->Branch("weight_kminus_PrimaryHadronNormalization",&fkminus);
      fPhaseIITrigTree->Branch("weight_pioninexsec_FluxUnisim",&fpioninexsec);
      fPhaseIITrigTree->Branch("weight_pionqexsec_FluxUnisim",&fpionqexsec);
      fPhaseIITrigTree->Branch("weight_piontotxsec_FluxUnisim",&fpiontotxsec);
      fPhaseIITrigTree->Branch("weight_nucleoninexsec_FluxUnisim",&fnucleoninexsec);
      fPhaseIITrigTree->Branch("weight_nucleonqexsec_FluxUnisim",&fnucleonqexsec);
      fPhaseIITrigTree->Branch("weight_nucleontotxsec_FluxUnisim",&fnucleontotxsec);
    } 

    //MuonFitter reco track length, vtx, energy; juju
    if (MuonFitter_fill)
    {
      fPhaseIITrigTree->Branch("recoMuonVtxX", &fRecoMuonVtxX, "recoMuonVtxX/D");
      fPhaseIITrigTree->Branch("recoMuonVtxY", &fRecoMuonVtxY, "recoMuonVtxY/D");
      fPhaseIITrigTree->Branch("recoMuonVtxZ", &fRecoMuonVtxZ, "recoMuonVtxZ/D");
      fPhaseIITrigTree->Branch("recoTankTrack", &fRecoTankTrack, "recoTankTrack/D");
      fPhaseIITrigTree->Branch("recoMuonKE", &fRecoMuonKE, "recoMuonKE/D");
      fPhaseIITrigTree->Branch("numMrdLayers", &fNumMrdLayers, "numMrdLayers/I");
    }
  
    // Reconstructed variables from each step in Muon Reco Analysis
    // Currently output when RecoDebug_fill = 1 in config 
    if (RecoDebug_fill){
      fPhaseIITrigTree->Branch("seedVtxX",&fSeedVtxX); 
      fPhaseIITrigTree->Branch("seedVtxY",&fSeedVtxY); 
      fPhaseIITrigTree->Branch("seedVtxZ",&fSeedVtxZ);
      fPhaseIITrigTree->Branch("seedVtxFOM",&fSeedVtxFOM); 
      fPhaseIITrigTree->Branch("seedVtxTime",&fSeedVtxTime,"seedVtxTime/D");
    
      fPhaseIITrigTree->Branch("pointPosX",&fPointPosX,"pointPosX/D");
      fPhaseIITrigTree->Branch("pointPosY",&fPointPosY,"pointPosY/D");
      fPhaseIITrigTree->Branch("pointPosZ",&fPointPosZ,"pointPosZ/D");
      fPhaseIITrigTree->Branch("pointPosTime",&fPointPosTime,"pointPosTime/D");
      fPhaseIITrigTree->Branch("pointPosFOM",&fPointPosFOM,"pointPosFOM/D");
      fPhaseIITrigTree->Branch("pointPosStatus",&fPointPosStatus,"pointPosStatus/I");
      
      fPhaseIITrigTree->Branch("pointDirX",&fPointDirX,"pointDirX/D");
      fPhaseIITrigTree->Branch("pointDirY",&fPointDirY,"pointDirY/D");
      fPhaseIITrigTree->Branch("pointDirZ",&fPointDirZ,"pointDirZ/D");
      fPhaseIITrigTree->Branch("pointDirTime",&fPointDirTime,"pointDirTime/D");
      fPhaseIITrigTree->Branch("pointDirStatus",&fPointDirStatus,"pointDirStatus/I");
      fPhaseIITrigTree->Branch("pointDirFOM",&fPointDirFOM,"pointDirFOM/D");
    
      fPhaseIITrigTree->Branch("pointVtxPosX",&fPointVtxPosX,"pointVtxPosX/D");
      fPhaseIITrigTree->Branch("pointVtxPosY",&fPointVtxPosY,"pointVtxPosY/D");
      fPhaseIITrigTree->Branch("pointVtxPosZ",&fPointVtxPosZ,"pointVtxPosZ/D");
      fPhaseIITrigTree->Branch("pointVtxTime",&fPointVtxTime,"pointVtxTime/D");
      fPhaseIITrigTree->Branch("pointVtxDirX",&fPointVtxDirX,"pointVtxDirX/D");
      fPhaseIITrigTree->Branch("pointVtxDirY",&fPointVtxDirY,"pointVtxDirY/D");
      fPhaseIITrigTree->Branch("pointVtxDirZ",&fPointVtxDirZ,"pointVtxDirZ/D");
      fPhaseIITrigTree->Branch("pointVtxFOM",&fPointVtxFOM,"pointVtxFOM/D");
      fPhaseIITrigTree->Branch("pointVtxStatus",&fPointVtxStatus,"pointVtxStatus/I");
    } 

    // Difference in MC Truth and Muon Reconstruction Analysis
    // Output to tree when muonTruthRecoDiff_fill = 1 in config
    if (muonTruthRecoDiff_fill){
      fPhaseIITrigTree->Branch("deltaVtxX",&fDeltaVtxX,"deltaVtxX/D");
      fPhaseIITrigTree->Branch("deltaVtxY",&fDeltaVtxY,"deltaVtxY/D");
      fPhaseIITrigTree->Branch("deltaVtxZ",&fDeltaVtxZ,"deltaVtxZ/D");
      fPhaseIITrigTree->Branch("deltaVtxR",&fDeltaVtxR,"deltaVtxR/D");
      fPhaseIITrigTree->Branch("deltaVtxT",&fDeltaVtxT,"deltaVtxT/D");
      fPhaseIITrigTree->Branch("deltaParallel",&fDeltaParallel,"deltaParallel/D");
      fPhaseIITrigTree->Branch("deltaPerpendicular",&fDeltaPerpendicular,"deltaPerpendicular/D");
      fPhaseIITrigTree->Branch("deltaAzimuth",&fDeltaAzimuth,"deltaAzimuth/D");
      fPhaseIITrigTree->Branch("deltaZenith",&fDeltaZenith,"deltaZenith/D");
      fPhaseIITrigTree->Branch("deltaAngle",&fDeltaAngle,"deltaAngle/D");
    }

    if(LAPPDData_fill)
    {
      fPhaseIITrigTree->Branch("LAPPD_ID",&fLAPPD_ID);
      fPhaseIITrigTree->Branch("LAPPD_Beamgate_ns",&fLAPPD_Beamgate_ns);
      fPhaseIITrigTree->Branch("LAPPD_Timestamp_ns",&fLAPPD_Timestamp_ns);
      fPhaseIITrigTree->Branch("LAPPD_Beamgate_Raw",&fLAPPD_Beamgate_Raw);
      fPhaseIITrigTree->Branch("LAPPD_Timestamp_Raw",&fLAPPD_Timestamp_Raw);
      fPhaseIITrigTree->Branch("LAPPD_Offset",&fLAPPD_Offset);
      fPhaseIITrigTree->Branch("LAPPD_TSCorrection",&fLAPPD_TSCorrection);
      fPhaseIITrigTree->Branch("LAPPD_BGCorrection",&fLAPPD_BGCorrection);
      fPhaseIITrigTree->Branch("LAPPD_OSInMinusPS",&fLAPPD_OSInMinusPS);

      fPhaseIITrigTree->Branch("GroupedTriggerTime",&fGroupedTriggerTime);
      fPhaseIITrigTree->Branch("GroupedTriggerWord",&fGroupedTriggerWord);

      if(LAPPDReco_fill)
      {
        fPhaseIITrigTree->Branch("LAPPD_PulseIDs",&fLAPPD_IDs);
        fPhaseIITrigTree->Branch("LAPPD_ChannelID",&fChannelID);
        fPhaseIITrigTree->Branch("LAPPD_PeakTime",&fPulsePeakTime);
        fPhaseIITrigTree->Branch("LAPPD_PeakAmp",&fPulsePeakAmp);
        fPhaseIITrigTree->Branch("LAPPD_Charge",&fPulseCharge);
        fPhaseIITrigTree->Branch("LAPPD_PulseStart",&fPulseStart);
        fPhaseIITrigTree->Branch("LAPPD_PulseEnd",&fPulseEnd);
        fPhaseIITrigTree->Branch("LAPPD_PulseWidth",&fPulseWidth);
        fPhaseIITrigTree->Branch("LAPPD_PulseSide",&fPulseSide);
        fPhaseIITrigTree->Branch("LAPPD_PulseStripNum",&fPulseStripNum);

        fPhaseIITrigTree->Branch("LAPPDID_Hit",&fLAPPDHit_IDs);
        fPhaseIITrigTree->Branch("LAPPDHitChannel",&fLAPPDHitChannel);
        fPhaseIITrigTree->Branch("LAPPDHitStrip",&fLAPPDHitStrip);
        fPhaseIITrigTree->Branch("LAPPDHitTime",&fLAPPDHitTime);
        fPhaseIITrigTree->Branch("LAPPDHitAmp",&fLAPPDHitAmp);
        fPhaseIITrigTree->Branch("LAPPDHitParallelPos",&fLAPPDHitParallelPos);
        fPhaseIITrigTree->Branch("LAPPDHitTransversePos",&fLAPPDHitTransversePos);
        fPhaseIITrigTree->Branch("LAPPDHitP1StartTime",&fLAPPDHitP1StartTime);
        fPhaseIITrigTree->Branch("LAPPDHitP2StartTime",&fLAPPDHitP2StartTime);
        fPhaseIITrigTree->Branch("LAPPDHitP1EndTime",&fLAPPDHitP1EndTime);
        fPhaseIITrigTree->Branch("LAPPDHitP2EndTime",&fLAPPDHitP2EndTime);
      }
    } 
  }
  return true;
}

bool PhaseIITreeMaker::Execute(){
  Log("===========================================================================================",v_debug,verbosity);
  Log("PhaseIITreeMaker Tool: Executing",v_debug,verbosity);

/*
  std::map<std::string,bool> checkDataStreams;
      m_data->Stores.at("ANNIEEvent")->Get("DataStreams",checkDataStreams);
      //print out the data streams 
      if(checkDataStreams["LAPPD"]==1){
        m_data->Stores.at("ANNIEEvent")->Get("LAPPDTimeStamps_ns", LAPPDTimeStamps_ns);
        m_data->Stores.at("ANNIEEvent")->Get("LAPPDOffsets", LAPPDOffsets);
        //check they have the same size, if not print in log
        if(LAPPDTimeStamps_ns.size() != LAPPDOffsets.size()){
          Log("PhaseIITreeMaker Tool: LAPPD Time Stamps and Offsets are not the same size!",v_message,verbosity);
        }else{
          cout<<"LAPPDTimeStamps_ns size is: "<<LAPPDTimeStamps_ns.size()<<endl;
          cout<<"LAPPDOffsets size is: "<<LAPPDOffsets.size()<<endl;
          //loop the map LAPPDOffsets, print the value + key
          for (auto const& x : LAPPDOffsets){
            cout << x.first << " : " << x.second << " = " << x.first + x.second << endl;
            gotLAPPDNumber++;
          }
        cout<<"gotLAPPDNumber = "<<gotLAPPDNumber<<endl;
        }
      }*/

  // Reset variables
  this->ResetVariables();

  // RecoEvent flags are optional for lean chains without EventSelector (e.g. a
  // cMRD-only chain).  ResetVariables() keeps the sentinels when unavailable.
  auto LoadOptionalRecoEventFlags = [this]() {
    auto recoevent_it = m_data->Stores.find("RecoEvent");
    if (recoevent_it == m_data->Stores.end() || recoevent_it->second == nullptr) return;
    bool pmtmrdcoinc = false;
    bool noveto = false;
    if (recoevent_it->second->Get("PMTMRDCoinc",pmtmrdcoinc)) fTankMRDCoinc = pmtmrdcoinc ? 1 : 0;
    if (recoevent_it->second->Get("NoVeto",noveto)) fNoVeto = noveto ? 1 : 0;
  };
  // Get a pointer to the ANNIEEvent Store
  if (LAPPDData_fill) this->LoadLAPPDData();

  //  If only clean events are built, return true for dirty events
  if(fillCleanEventsOnly){
    auto get_flagsapp = m_data->Stores.at("RecoEvent")->Get("EventFlagApplied",fEventStatusApplied);
    auto get_flags = m_data->Stores.at("RecoEvent")->Get("EventFlagged",fEventStatusFlagged); 
    //auto get_cutstatus = m_data->Stores.at("RecoEvent")->Get("EventCutStatus",fEventCutStatus);
    if(!get_flagsapp || !get_flags) {
      Log("PhaseITreeMaker tool: No Event status applied or flagged bitmask!!", v_error, verbosity);
      return false;	
    }
    // check if event passes the cut
    if((fEventStatusFlagged) != 0) {
    //  if (!fEventCutStatus){
      Log("PhaseIITreeMaker Tool: Event was flagged with one of the active cuts.",v_debug, verbosity);
      return true;	
    }
  }

  if(TankClusterProcessing){
    Log("PhaseIITreeMaker Tool: Beginning Tank cluster processing",v_debug,verbosity);
    //bool get_clusters = m_data->Stores.at("ANNIEEvent")->Get("ClusterMap",m_all_clusters);
    bool get_clusters = false;
    if (isData){
      get_clusters = m_data->CStore.Get("ClusterMap",m_all_clusters);
      if(!get_clusters){
        std::cout << "PhaseIITreeMaker tool: No clusters found!" << std::endl;
        return false;
      }
    } else {
      if (MCWaveform){
        get_clusters = m_data->CStore.Get("ClusterMap",m_all_clusters);
        if(!get_clusters){
          std::cout << "PhaseIITreeMaker tool: MCWaveform --> no clusters found!" << std::endl;
          return false;
        }
      } else {
      get_clusters = m_data->CStore.Get("ClusterMapMC",m_all_clusters_MC);
      if (!get_clusters){
        std::cout <<"PhaseIITreeMaker tool: No clusters found (MC)!" << std::endl;
        return false;
      }
    }
    }
    get_clusters = m_data->CStore.Get("ClusterMapDetkey",m_all_clusters_detkeys);
    if (!get_clusters){
      std::cout <<"PhaseIITreeMaker tool: No cluster detkeys found!" << std::endl;
      return false;
    }
    Log("PhaseIITreeMaker Tool: Accessing pairs in all_clusters map",v_debug,verbosity);

    int cluster_num = 0;
    int cluster_size = 0;
    if (isData) {
        cluster_size = (int) m_all_clusters->size();
    } else {
        if (MCWaveform) {
            cluster_size = (int) m_all_clusters->size();
        } else {
            cluster_size = (int) m_all_clusters_MC->size();
        }
    }
      
    std::map<double,std::vector<Hit>>::iterator it_cluster_pair;
    std::map<double,std::vector<MCHit>>::iterator it_cluster_pair_mc;
    bool loop_map = true;
    if (isData) {
        it_cluster_pair = (*m_all_clusters).begin();
    } else {
        if (MCWaveform) {
            it_cluster_pair = (*m_all_clusters).begin();
        } else {
            it_cluster_pair_mc = (*m_all_clusters_MC).begin();
        }
    }

    if (cluster_size == 0) loop_map = false;
    while (loop_map){
    //for (std::pair<double,std::vector<Hit>>&& cluster_pair : *m_all_clusters) {
      Log("PhaseIITreeMaker Tool: Resetting variables prior to getting run level info",v_debug,verbosity);
      this->ResetVariables();
      if (LAPPDData_fill) this->LoadLAPPDData();
      
      fClusterNumber = cluster_num;

      //Standard run level information
      Log("PhaseIITreeMaker Tool: tank cluster, Getting run level information from ANNIEEvent",v_debug,verbosity);
            // m_data->Stores.at("ANNIEEvent")->Print(false);

      m_data->Stores.at("ANNIEEvent")->Get("RunNumber",fRunNumber);
      m_data->Stores.at("ANNIEEvent")->Get("SubrunNumber",fSubrunNumber);
      m_data->Stores.at("ANNIEEvent")->Get("RunType",fRunType);
      m_data->Stores.at("ANNIEEvent")->Get("RunStartTime",fStartTime);

      uint64_t primaryTrigTime = 0;
      m_data->Stores.at("ANNIEEvent")->Get("PrimaryTriggerTime", primaryTrigTime);
  
      fStartTime_Tree = (ULong64_t) primaryTrigTime;
      // ANNIE Event number
      m_data->Stores.at("ANNIEEvent")->Get("EventTimeTank",fEventTimeTank);
      fEventTimeTank_Tree = (ULong64_t) fEventTimeTank;
      m_data->Stores.at("ANNIEEvent")->Get("EventNumber",fEventNumber);
      uint32_t trigword_tmp;
      m_data->Stores.at("ANNIEEvent")->Get("TriggerWord",trigword_tmp);
      fTriggerword = int(trigword_tmp);
      m_data->Stores["ANNIEEvent"]->Get("TriggerExtended",fExtended);
      BeamStatus beamstat;
      bool get_beamstat = m_data->Stores["ANNIEEvent"]->Get("BeamStatus",beamstat);

      if(get_beamstat)
      {
      if (beamstat.ok()) fBeamok = 1;
      else fBeamok = 0;
      fPot = beamstat.pot();
      }else{
      bool gotfBeamok = m_data->Stores["ANNIEEvent"]->Get("beam_good",fBeamok);
      bool gotpot = m_data->Stores["ANNIEEvent"]->Get("beam_E_TOR875",fPot);
      if(!gotfBeamok) fBeamok = 0;
      if(!gotpot) fPot = -99999;
      }

      LoadOptionalRecoEventFlags();

      //Number of ADC samples
      std::map<unsigned long, std::vector<Waveform<unsigned short>>> raw_waveform_map;
      bool has_raw = m_data->Stores["ANNIEEvent"]->Get("RawADCData",raw_waveform_map);
      if (!has_raw) {
        Log("PhaseIITreeMaker tool: Did not find RawADCData in ANNIEEvent! Abort",v_warning,verbosity);
        /*return false;*/
      }

      if (has_raw){
        for (auto& temp_pair : raw_waveform_map) {
          const auto& achannel_key = temp_pair.first;
          auto& araw_waveforms = temp_pair.second;
          for (unsigned int i=0; i< araw_waveforms.size(); i++){
            auto samples = araw_waveforms.at(i).GetSamples();
            int size_sample = samples->size();
            fADCWaveformChankeys.push_back(achannel_key);
            fADCWaveformSamples.push_back(size_sample);
          }
        }
      }else {
        //Try to get summary information on the ADC waveform samples in case no raw info is saved
        std::map<unsigned long, std::vector<int>> raw_acqsize_map;
        has_raw = m_data->Stores["ANNIEEvent"]->Get("RawAcqSize",raw_acqsize_map);
        if (!has_raw) {
          Log("RunValidation tool: Did not find RawAcqSize in ANNIEEvent! Abort",v_error,verbosity);
          if (isData) return false;
        }
        int size_of_window = 2000;
        if (has_raw){
          for (auto& temp_pair : raw_acqsize_map) {
            const auto& achannel_key = temp_pair.first;
            auto& araw_acqsize = temp_pair.second;
            for (unsigned int i=0; i< araw_acqsize.size(); i++){
              int size_sample = araw_acqsize.at(i);
              fADCWaveformChankeys.push_back(achannel_key);
              fADCWaveformSamples.push_back(size_sample);
            }
          }
        }
      }
      if (isData){
        std::vector<Hit> cluster_hits = it_cluster_pair->second;
        fClusterTime = it_cluster_pair->first;
        if(TankHitInfo_fill){
          Log("PhaseIITreeMaker Tool: Loading tank cluster hits into cluster tree",v_debug,verbosity);
          this->LoadTankClusterHits(cluster_hits);
        }

        bool good_class = this->LoadTankClusterClassifiers(it_cluster_pair->first);
        if(!good_class){
          if(verbosity>3) Log("PhaseIITreeMaker Tool: No cluster classifiers.  Continuing tree",v_debug,verbosity);
        }
      } else {
        if (MCWaveform){
          std::vector<Hit> cluster_hits = it_cluster_pair->second;
          fClusterTime = it_cluster_pair->first;
          if(TankHitInfo_fill){
            Log("PhaseIITreeMaker Tool: Loading tank cluster hits into cluster tree",v_debug,verbosity);
            this->LoadTankClusterHits(cluster_hits);
          }

          bool good_class = this->LoadTankClusterClassifiers(it_cluster_pair->first);
          if(!good_class){
            if(verbosity>3) Log("PhaseIITreeMaker Tool: No cluster classifiers.  Continuing tree",v_debug,verbosity);
          }
        } else {
        std::vector<MCHit> cluster_hits = it_cluster_pair_mc->second;
        fClusterTime = it_cluster_pair_mc->first;
        std::vector<unsigned long> cluster_detkeys = m_all_clusters_detkeys->at(it_cluster_pair_mc->first);
        if(TankHitInfo_fill){
          Log("PhaseIITreeMaker Tool: Loading tank cluster hits into cluster tree",v_debug,verbosity);
          this->LoadTankClusterHitsMC(cluster_hits,cluster_detkeys);
        }
        bool good_class = this->LoadTankClusterClassifiers(it_cluster_pair_mc->first);
        if(!good_class){
          if(verbosity>3) Log("PhaseIITreeMaker Tool: No cluster classifiers.  Continuing tree",v_debug,verbosity);
        }
        bool good_bunch = this->LoadBNBtimingMC(it_cluster_pair_mc->first);
        if(!good_bunch){
          if(verbosity>v_debug) Log("PhaseIITreeMaker Tool: No BNB timing (MC). Continuing tree",v_debug,verbosity);
        }
        }
      }

      if(SiPMPulseInfo_fill) this->LoadSiPMHits();
      if (MuonFitter_fill)
      {
        Position tmp_vtx(-999,-999,-999);
        m_data->CStore.Get("FittedMuonVertex", tmp_vtx);
        fRecoMuonVtxX = tmp_vtx.X();
        fRecoMuonVtxY = tmp_vtx.Y();
        fRecoMuonVtxZ = tmp_vtx.Z();
        m_data->CStore.Get("FittedTrackLengthInWater", fRecoTankTrack);
        m_data->CStore.Get("RecoMuonKE", fRecoMuonKE);
      }
      fPhaseIITankClusterTree->Fill();
      cluster_num += 1;
      if (isData) {
          it_cluster_pair++;
          if (it_cluster_pair == (*m_all_clusters).end()) {
              loop_map = false;
          }
      } else {
          if (MCWaveform) {
              it_cluster_pair++;
              if (it_cluster_pair == (*m_all_clusters).end()) {
                  loop_map = false;
              }
          } else {
              it_cluster_pair_mc++;
              if (it_cluster_pair_mc == (*m_all_clusters_MC).end()) {
                  loop_map = false;
              }
          }
      }

    }
  }
  
  if(MRDClusterProcessing){
    Log("PhaseIITreeMaker Tool: Beginning MRD cluster processing",v_debug,verbosity);

    bool get_clusters = m_data->CStore.Get("MrdTimeClusters",MrdTimeClusters);
    if(!get_clusters){
      std::cout << "PhaseIITreeMaker tool: No MRD clusters found! Did you run the TimeClustering tool?" << std::endl;
      return false;
    }
	
    int num_mrd_clusters;
    m_data->CStore.Get("NumMrdTimeClusters",num_mrd_clusters);

    if (num_mrd_clusters > 0){
      m_data->CStore.Get("MrdDigitTimes",mrddigittimesthisevent);
      m_data->CStore.Get("MrdDigitPmts",mrddigitpmtsthisevent);
      m_data->CStore.Get("MrdDigitChankeys",mrddigitchankeysthisevent);
    }
    

    //Check if there's a veto hit in the acquisition
    int TrigHasVetoHit = 0;
    bool has_tdc = false;
    if (isData) has_tdc = m_data->Stores["ANNIEEvent"]->Get("TDCData",TDCData);  // a std::map<ChannelKey,vector<TDCHit>>
    else has_tdc = m_data->Stores["ANNIEEvent"]->Get("TDCData",TDCData_MC);     
 if (!has_tdc){
        std::cout << "No TDCData store in ANNIEEvent." << std::endl;
        //return false;
     }
    
    int cluster_num = 0;
    for (int i=0; i<(int)MrdTimeClusters.size(); i++){
      Log("PhaseIITreeMaker Tool: Resetting variables prior to getting MRD cluster info",v_debug,verbosity);
      int tdcdata_size = (isData)? TDCData->size() : TDCData_MC->size();
      this->ResetVariables();
      uint32_t trigword_temp;
      m_data->Stores.at("ANNIEEvent")->Get("TriggerWord",trigword_temp);
      fTriggerword = int(trigword_temp);
      m_data->Stores["ANNIEEvent"]->Get("TriggerExtended",fExtended);
      BeamStatus beamstat;
      bool get_beamstat = m_data->Stores["ANNIEEvent"]->Get("BeamStatus",beamstat);

      if(get_beamstat)
      {
      if (beamstat.ok()) fBeamok = 1;
      else fBeamok = 0;
      fPot = beamstat.pot();
      }else{
      bool gotfBeamok = m_data->Stores["ANNIEEvent"]->Get("beam_good",fBeamok);
      bool gotpot = m_data->Stores["ANNIEEvent"]->Get("beam_E_TOR875",fPot);
      if(!gotfBeamok) fBeamok = 0;
      if(!gotpot) fPot = -99999;
      }

      LoadOptionalRecoEventFlags();
      fMRDClusterHits = 0;
      if (has_tdc){
        if(tdcdata_size>0){
          Log("PhaseIITreeMaker tool: Looping over FACC/MRD hits... looking for Veto activity",v_debug,verbosity);
          if (isData){
            for(auto&& anmrdpmt : (*TDCData)){
              unsigned long chankey = anmrdpmt.first;
              std::vector<Hit> mrdhits = anmrdpmt.second;
              Detector* thedetector = geom->ChannelToDetector(chankey);
              unsigned long detkey = thedetector->GetDetectorID();
              if(thedetector->GetDetectorElement()=="Veto") {
                TrigHasVetoHit=1; // this is a veto hit, not an MRD hit.
                for (int j = 0; j < (int) mrdhits.size(); j++){
                  fFMVHitT.push_back(mrdhits.at(j).GetTime());
                  fFMVHitDetID.push_back(detkey);
                  fFMVHitChankey.push_back(chankey);
                  fFMVHitChankeyMC.push_back(chankey);
                }
              }
            }
          } else {  
            for(auto&& anmrdpmt : (*TDCData_MC)){
              unsigned long chankey = anmrdpmt.first;
              Detector* thedetector = geom->ChannelToDetector(chankey);
              unsigned long detkey = thedetector->GetDetectorID();
              if(thedetector->GetDetectorElement()=="Veto") {
                TrigHasVetoHit=1; // this is a veto hit, not an MRD hit.
                int wcsimid = channelkey_to_faccpmtid.at(chankey)-1;
                unsigned long chankey_data = wcsimid;
                std::vector<MCHit> mrdhits = anmrdpmt.second;
                for (int j = 0; j < (int) mrdhits.size(); j++){
                  fFMVHitT.push_back(mrdhits.at(j).GetTime());
                  fFMVHitDetID.push_back(detkey);
                  fFMVHitChankey.push_back(chankey_data);  
                  fFMVHitChankeyMC.push_back(chankey);
                }
              }
            }
          }
        }
      }
      fVetoHit = TrigHasVetoHit;
      std::vector<int> ThisClusterIndices = MrdTimeClusters.at(i);
      for (int j=0; j < (int) ThisClusterIndices.size(); j++){
        Detector *thedetector = geom->ChannelToDetector(mrddigitchankeysthisevent.at(ThisClusterIndices.at(j)));
        unsigned long detkey = thedetector->GetDetectorID();

        fMRDHitT.push_back(mrddigittimesthisevent.at(ThisClusterIndices.at(j)));
        fMRDHitDetID.push_back(detkey);
        if (isData) fMRDHitChankey.push_back(mrddigitchankeysthisevent.at(ThisClusterIndices.at(j)));
        else {
          int wcsimid = channelkey_to_mrdpmtid.at(mrddigitchankeysthisevent.at(ThisClusterIndices.at(j)))-1;
          unsigned long chankey_data = mrdpmtid_to_channelkey_data[wcsimid];
          fMRDHitChankey.push_back(chankey_data);
        }
        fMRDHitChankeyMC.push_back(mrddigitchankeysthisevent.at(ThisClusterIndices.at(j)));
        fMRDClusterHits+=1;
      }

      fMRDClusterNumber = cluster_num;
      ComputeMeanAndVariance(fMRDHitT, fMRDClusterTime, fMRDClusterTimeSigma);
      //FIXME: calculate fMRDClusterTime

      //Standard run level information
      Log("PhaseIITreeMaker Tool: MRD cluster, Getting run level information from ANNIEEvent",v_debug,verbosity);
      
           // // m_data->Stores.at("ANNIEEvent")->Print(false);

      m_data->Stores.at("ANNIEEvent")->Get("RunNumber",fRunNumber);
      m_data->Stores.at("ANNIEEvent")->Get("SubrunNumber",fSubrunNumber);
      m_data->Stores.at("ANNIEEvent")->Get("RunType",fRunType);
      m_data->Stores.at("ANNIEEvent")->Get("RunStartTime",fStartTime);
      uint64_t primaryTrigTime = 0;
      m_data->Stores.at("ANNIEEvent")->Get("PrimaryTriggerTime", primaryTrigTime);
      fStartTime_Tree = (ULong64_t) primaryTrigTime;
      m_data->Stores.at("ANNIEEvent")->Get("EventNumber",fEventNumber);
      m_data->Stores.at("ANNIEEvent")->Get("EventTimeTank",fEventTimeTank);
      fEventTimeTank_Tree = (ULong64_t) fEventTimeTank;
      m_data->Stores.at("ANNIEEvent")->Get("EventTimeMRD",fEventTimeMRD);  
      fEventTimeMRD_Tree = (ULong64_t) fEventTimeMRD.GetNs();

      // ANNIE Event number
      //bool got_mrdtime = m_data->Stores.at("ANNIEEvent")->Get("EventTime",mrd_timestamp);
      //if(got_mrdtime) fEventTimeMRD = static_cast<uint64_t>(mrd_timestamp->GetNs());

      if(MRDReco_fill){
        fNumClusterTracks = this->LoadMRDTrackReco(i);
        //Get the track info
      }
      if (LAPPDData_fill) this->LoadLAPPDData();

      fPhaseIIMRDClusterTree->Fill();
      cluster_num += 1;
    }
  }
 
  if(TriggerProcessing) {

    Log("PhaseIITreeMaker Tool: trigger, Getting run level information from ANNIEEvent",v_debug,verbosity);
          // m_data->Stores.at("ANNIEEvent")->Print(false);

    this->ResetVariables();

    m_data->Stores.at("ANNIEEvent")->Get("RunNumber",fRunNumber);
    m_data->Stores.at("ANNIEEvent")->Get("SubrunNumber",fSubrunNumber);
    m_data->Stores.at("ANNIEEvent")->Get("RunType",fRunType);
    m_data->Stores.at("ANNIEEvent")->Get("RunStartTime",fStartTime);
      uint64_t primaryTrigTime = 0;
      m_data->Stores.at("ANNIEEvent")->Get("PrimaryTriggerTime", primaryTrigTime);
      fStartTime_Tree = (ULong64_t) primaryTrigTime;

    // ANNIE Event number
    m_data->Stores.at("ANNIEEvent")->Get("EventNumber",fEventNumber);
    m_data->Stores.at("ANNIEEvent")->Get("EventTimeTank",fEventTimeTank);
    fEventTimeTank_Tree = (ULong64_t) fEventTimeTank;

    uint32_t trigword_temp;

    m_data->Stores.at("ANNIEEvent")->Get("TriggerWord",trigword_temp);
    fTriggerword = int(trigword_temp);
    m_data->Stores["ANNIEEvent"]->Get("TriggerExtended",fExtended);
      BeamStatus beamstat;
      bool get_beamstat = m_data->Stores["ANNIEEvent"]->Get("BeamStatus",beamstat);

      if(get_beamstat)
      {
      if (beamstat.ok()) fBeamok = 1;
      else fBeamok = 0;
      fPot = beamstat.pot();
      }else{
      bool gotfBeamok = m_data->Stores["ANNIEEvent"]->Get("beam_good",fBeamok);
      bool gotpot = m_data->Stores["ANNIEEvent"]->Get("beam_E_TOR875",fPot);
      if(!gotfBeamok) fBeamok = 0;
      if(!gotpot) fPot = -99999;
      }
 
    // Optional event metadata: keep ResetVariables() sentinels if unavailable.
    fDataStreams.clear();
    bool got_data_streams = m_data->Stores.at("ANNIEEvent")->Get("DataStreams",fDataStreams);
    LoadOptionalRecoEventFlags();
    if (got_data_streams) {
      auto tank_stream_it = fDataStreams.find("Tank");
      auto mrd_stream_it = fDataStreams.find("MRD");
      if (tank_stream_it != fDataStreams.end()) fHasTank = tank_stream_it->second ? 1 : 0;
      if (mrd_stream_it != fDataStreams.end()) fHasMRD = mrd_stream_it->second ? 1 : 0;
    }


    if (m_data->Stores.at("ANNIEEvent")->Get("EventTimeMRD",fEventTimeMRD))
      fEventTimeMRD_Tree = (ULong64_t) fEventTimeMRD.GetNs();
    // bool got_mrdtime = m_data->Stores.at("ANNIEEvent")->Get("EventTime",mrd_timestamp);
    //if(got_mrdtime) fEventTimeMRD = static_cast<uint64_t>(mrd_timestamp->GetNs());

    // Read hits and load into ntuple
    if(TankHitInfo_fill){
      this->LoadAllTankHits(isData, MCWaveform);
    }
    if(SiPMPulseInfo_fill) this->LoadSiPMHits();
 
    if(MRDHitInfo_fill) this->LoadAllMRDHits(isData);
    
    //DIGITS
    if(Digit_fill) this->LoadDigitHits();
    //DIGITS
    /*
    if(Digit_fill){
       // get digits from RecoDigit store
       std::vector<RecoDigit>* digitList;
       auto get_digits=m_data->Stores.at("RecoEvent")->Get("RecoDigit", digitList);
       if(not get_digits){
         Log("PhaseIITreeMaker Tool: Failed to retrieve the RecoDigit from RecoEvent Store!",v_error,verbosity);
         return false;
       }
       // Extract the PMT & LAPPD digit information
       // ===============================
       int totalPMTs =0; // number of digits from PMT hits in the event
       int totalLAPPDs = 0; // number of digits from LAPPD hits in the event
       //loop through all digits
       for(RecoDigit &adigit : *digitList){
	   digitX.push_back(adigit.GetPosition().X());
	   digitY.push_back(adigit.GetPosition().Y());
	   digitZ.push_back(adigit.GetPosition().Z());
	   digitT.push_back(adigit.GetCalTime());
	   if(adigit.GetDigitType()==0){totalPMTs+=1;} //when the digit type is zero we have a PMT digit
	   else{totalLAPPDs+=1;}// when it is 1 we have LAPPD
       }
       Log("PhaseIITreeMaker Tool: Got "+to_string(totalPMTs)+" PMT digits; "+to_string(digitT.size()) +" total digits so far",v_debug,verbosity);
       Log("PhaseIITreeMaker Tool: Got "+to_string(totalLAPPDs)+" LAPPD digits; "+to_string(digitT.size()) +" total digits",v_debug,verbosity);
    }
*/

    if(MRDReco_fill){
      fNumClusterTracks=0;
      // Missing MRD clustering means zero legacy tracks for this trigger; it
      // must not drop the trigger (or, with MRDRecoSource=Both, its mrd_ml_*
      // outputs) from the tree.
      MrdTimeClusters.clear();
      bool get_clusters = m_data->CStore.Get("MrdTimeClusters",MrdTimeClusters);
      if(!get_clusters){
        Log("PhaseIITreeMaker tool: No MrdTimeClusters for this trigger; writing "
            "numMRDTracks=0 and continuing.", v_debug, verbosity);
      } else {
        for(int i=0; i < (int) MrdTimeClusters.size(); i++) fNumClusterTracks += this->LoadMRDTrackReco(i);
      }
    }

    if(SimpleReco_fill) this->FillSimpleRecoInfo();

    if(RingCounting_fill) this->FillRingCountingInfo();

    if(Reweight_fill) this->FillWeightInfo();

    bool got_reco = false;
    if(TankReco_fill) got_reco = this->FillTankRecoInfo();

    bool gotmctruth = false;
    if(MCTruth_fill)  gotmctruth = this->FillMCTruthInfo();

    if (muonTruthRecoDiff_fill) this->FillTruthRecoDiffInfo(gotmctruth,got_reco);
    if (got_reco && gotmctruth && (verbosity>4)) this->RecoSummary();

    // FIll tree with all reconstruction information
    if (RecoDebug_fill) this->FillRecoDebugInfo();

    if (LAPPDData_fill) this->LoadLAPPDData();

    if (MuonFitter_fill)
    {
      Position tmp_vtx(-999,-999,-999);
      m_data->CStore.Get("FittedMuonVertex", tmp_vtx);
      fRecoMuonVtxX = tmp_vtx.X();
      fRecoMuonVtxY = tmp_vtx.Y();
      fRecoMuonVtxZ = tmp_vtx.Z();
      m_data->CStore.Get("FittedTrackLengthInWater", fRecoTankTrack);
      m_data->CStore.Get("RecoMuonKE", fRecoMuonKE);
      m_data->CStore.Get("NLyers", fNumMrdLayers);
    }

    if(MrdMLReco_fill && !LoadMrdMLReco()) return false;
    fPhaseIITrigTree->Fill();
  }
  return true;
}

bool PhaseIITreeMaker::Finalise(){
  
  fOutput_tfile->cd();
  fPhaseIITrigTree->Write();
  fPhaseIIMRDClusterTree->Write();
  fPhaseIITankClusterTree->Write();
  // The metadata tree is part of the cMrdMlReco release contract: an
  // incomplete one is still written for inspection, but Finalise() fails.
  const bool mrdml_metadata_ok = !MrdMLReco_fill || WriteMrdMLMetadata();
  fOutput_tfile->Close();
  if(verbosity>0) cout<<"PhaseIITreeMaker exitting"<<endl;

  if(!mrdml_metadata_ok) return false;
  return true;
}

void PhaseIITreeMaker::ResetVariables() {
  // tree variables
  fStartTime_Tree = 9999;
  fEventNumber = -9999;
  fEventTimeTank_Tree = 9999;
  fNHits = -9999;
  fNDigitsPMTs = 0;
  fNDigitsLAPPDs = 0;
  fVetoHit = -9999;
  fEventTimeMRD_Tree = 9999;
  fTriggerword = -1;
  fTankMRDCoinc = -99999;
  fNoVeto = -99999;
  fHasTank = -99999;
  fHasMRD = -99999;
  fPot = -99999;
  fBeamok = -99999;
  fExtended = -99999;

  if(SiPMPulseInfo_fill){
    fSiPM1NPulses = -9999;
    fSiPM2NPulses = -9999;
    fSiPMHitQ.clear();
    fSiPMHitT.clear();
    fSiPMHitAmplitude.clear();
    fSiPMNum.clear();
  }

  if(hasBNBtimingMC){
      fbunchTimes = -9999;
  }

  if(TankClusterProcessing){
    fClusterMaxPE = -9999;
    fClusterChargePointX = -9999;
    fClusterChargePointY = -9999;
    fClusterChargePointZ = -9999;
    fClusterChargeBalance = -9999;
    fClusterTime = -9999;
    fClusterCharge = -9999;
    fClusterNumber = -9999;
    fADCWaveformSamples.clear();
    fADCWaveformChankeys.clear();
  } 
  if(MCTruth_fill){ 
    fMCEventNum = -9999;
    fMCTriggerNum = -9999;
    fTrueVtxX = -9999;
    fTrueVtxY = -9999;
    fTrueVtxZ = -9999;
    fTrueVtxTime = -9999;
    fTrueMuonEnergy = -9999;
    fTrueTrackLengthInWater = -9999;
    fTrueTrackLengthInMRD = -9999;
    fTrueDirX = -9999;
    fTrueDirY = -9999;
    fTrueDirZ = -9999;
    fTrueAngle = -9999;
    fTruePhi = -9999;
    fPi0Count = -9999;
    fPiPlusCount = -9999;
    fPiMinusCount = -9999;
    fK0Count = -9999;
    fKPlusCount = -9999;
    fKMinusCount = -9999;
    fTrueMultiRing = -9999;
    fTruePrimaryPdgs->clear();
    fTrueNeutCapVtxX->clear();
    fTrueNeutCapVtxY->clear();
    fTrueNeutCapVtxZ->clear();
    fTrueNeutCapNucleus->clear();
    fTrueNeutCapTime->clear();
    fTrueNeutCapGammas->clear();
    fTrueNeutCapE->clear();
    fTrueNeutCapGammaE->clear();
    fTrueNeutrinoEnergy = -9999;
    fTrueNuPDG = -9999;
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
    fTrueProtons = -9999;
    fTrueNeutrons = -9999;
    fTruePi0 = -9999;
    fTruePiPlus = -9999;
    fTruePiPlusCher = -9999;
    fTruePiMinus = -9999;
    fTruePiMinusCher = -9999;
    fTrueKPlus = -9999;
    fTrueKPlusCher = -9999;
    fTrueKMinus = -9999;
    fTrueKMinusCher = -9999;
    fTrueW2 = -9999;
    fTrueBJx = -9999;
    fTruey = -9999;
    fTrueTarget = -9999;
    fTrueq0 = -9999;
    fTrueq3 = -9999;
  }

  if (Reweight_fill){
    fAll0.clear();
    fAll1.clear();
    fAll2.clear();
    fAll3.clear();
    fAll4.clear();
    fAll5.clear();
    fAxFFCCQEshape.clear();
    fDecayAngMEC.clear();
    fNormCCCOH.clear();
    fNorm_NCCOH.clear();
    fRPA_CCQE.clear();
    fRootinoFix.clear();
    fThetaDelta2NRad.clear();
    fTheta_Delta2Npi.clear();
    fTunedCentralValue.clear();
    fVecFFCCQEshape.clear();
    fXSecShape_CCMEC.clear();
    fpiplus.clear();
    fpiminus.clear();
    fkplus.clear();
    fkzero.clear();
    fkminus.clear();
    fhorncurrent.clear();
    fpioninexsec.clear();
    fpionqexsec.clear();
    fpiontotxsec.clear();
    fexpskin.clear();
    fnucleoninexsec.clear();
    fnucleonqexsec.clear();
    fnucleontotxsec.clear();
  }

  if (RecoDebug_fill){ 
    fSeedVtxX.clear();
    fSeedVtxY.clear();
    fSeedVtxZ.clear();
    fSeedVtxFOM.clear();
    fSeedVtxTime = -9999;
    fPointPosX = -9999;
    fPointPosY = -9999;
    fPointPosZ = -9999;
    fPointPosTime = -9999;
    fPointPosFOM = -9999;
    fPointPosStatus = -9999;
    fPointDirX = -9999;
    fPointDirY = -9999;
    fPointDirZ = -9999;
    fPointDirTime = -9999;
    fPointDirFOM = -9999;
    fPointDirStatus = -9999;
    fPointVtxPosX = -9999;
    fPointVtxPosY = -9999;
    fPointVtxPosZ = -9999;
    fPointVtxDirX = -9999;
    fPointVtxDirY = -9999;
    fPointVtxDirZ = -9999;
    fPointVtxTime = -9999;
    fPointVtxStatus = -9999;
    fPointVtxFOM = -9999;
  }

  if(TankReco_fill){
    fRecoVtxX = -9999;
    fRecoVtxY = -9999;
    fRecoVtxZ = -9999;
    fRecoStatus = -9999;
    fRecoVtxTime = -9999;
    fRecoVtxFOM = -9999;
    fRecoDirX = -9999;
    fRecoDirY = -9999;
    fRecoDirZ = -9999;
    fRecoAngle = -9999;
    fRecoPhi = -9999;
  }

  if(MRDClusterProcessing){
    fMRDClusterNumber = -9999;
    fMRDClusterTime = -9999;
    fMRDClusterTimeSigma = -9999;
    fMRDClusterHits = -9999;
  }
  if(MRDHitInfo_fill){
    fMRDHitT.clear();
    fMRDHitDetID.clear();
    fMRDHitChankey.clear();
    fMRDHitChankeyMC.clear();
    fFMVHitT.clear();
    fFMVHitDetID.clear();
    fFMVHitChankey.clear();
    fFMVHitChankeyMC.clear();
  }
  if(MRDReco_fill){
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
  }
  if(SimpleReco_fill){
    fSimpleFlag = -9999;
    fSimpleEnergy = -9999;
    fSimpleVtxX = -9999;
    fSimpleVtxY = -9999;
    fSimpleVtxZ = -9999;
    fSimpleStopVtxX = -9999;
    fSimpleStopVtxY = -9999;
    fSimpleStopVtxZ = -9999;
    fSimpleCosTheta = -9999;
    fSimplePt = -9999;
    fSimpleFV = -9999;
    fSimpleMrdEnergyLoss = -9999;
    fSimpleTrackLengthInMRD = -9999;
    fSimpleTrackLengthInTank = -9999;
    fSimpleMRDStartX = -9999;
    fSimpleMRDStartY = -9999;
    fSimpleMRDStartZ = -9999;
    fSimpleMRDStopX = -9999;
    fSimpleMRDStopY = -9999;
    fSimpleMRDStopZ = -9999;
  }
  if(RingCounting_fill){
    fRCSRPred = -9999;
    fRCMRPred = -9999;
  }
  if(TankHitInfo_fill){
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
  }
  
  if (muonTruthRecoDiff_fill){ 
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
  }

  if(LAPPDData_fill){
    LAPPDDataMap.clear();
    LAPPDBeamgate_ns.clear();
    LAPPDTimeStamps_ns.clear();
    LAPPDTimeStampsRaw.clear();
    LAPPDBeamgatesRaw.clear();
    LAPPDOffsets.clear();
    LAPPDTSCorrection.clear();
    LAPPDBGCorrection.clear();
    LAPPDOSInMinusPS.clear();

    fLAPPD_ID.clear();
    fLAPPD_Beamgate_ns.clear();
    fLAPPD_Timestamp_ns.clear();
    fLAPPD_Beamgate_Raw.clear();
    fLAPPD_Timestamp_Raw.clear();
    fLAPPD_Offset.clear();
    fLAPPD_TSCorrection.clear();
    fLAPPD_BGCorrection.clear();
    fLAPPD_OSInMinusPS.clear();

    GroupedTrigger.clear();

    fGroupedTriggerTime.clear();
    fGroupedTriggerWord.clear();

    lappdPulses.clear();
    lappdHits.clear();

    fLAPPD_IDs.clear();
    fChannelID.clear();
    fPulsePeakTime.clear();
    fPulseCharge.clear();
    fPulsePeakAmp.clear();
    fPulseStart.clear();
    fPulseEnd.clear();
    fPulseWidth.clear();
    fPulseSide.clear();
    fPulseStripNum.clear();

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

  }

  //DIGITS
  if (Digit_fill){
    fdigitX.clear();
    fdigitY.clear();
    fdigitZ.clear();
    fdigitT.clear();
    }
   //DIGITS 

  if (MuonFitter_fill)
  {
    fRecoMuonVtxX = -9999;
    fRecoMuonVtxY = -9999;
    fRecoMuonVtxZ = -9999;
    fRecoTankTrack = -9999;
    fRecoMuonKE = -9999;
    fNumMrdLayers = -9999;
  }  
}

bool PhaseIITreeMaker::LoadTankClusterClassifiers(double cluster_time){
  //Save classifiers to ANNIEEvent
  Log("PhaseITreeMaker tool: Getting cluster classifiers", v_debug, verbosity);

  bool got_ccp = m_data->Stores.at("ANNIEEvent")->Get("ClusterChargePoints", ClusterChargePoints);
  bool got_ccb = m_data->Stores.at("ANNIEEvent")->Get("ClusterChargeBalances", ClusterChargeBalances);
  bool got_cmpe = m_data->Stores.at("ANNIEEvent")->Get("ClusterMaxPEs", ClusterMaxPEs);
  bool good_classifiers = got_ccp && got_ccb && got_cmpe;
  if(!good_classifiers){
    Log("PhaseITreeMaker tool: One of the charge cluster classifiers is not available", v_debug, verbosity);
  } else { 
    Log("PhaseITreeMaker tool: Setting fCluster variables to classifier parameters", v_debug, verbosity);
    fClusterMaxPE = ClusterMaxPEs.at(cluster_time);
    Position ClusterChargePoint = ClusterChargePoints.at(cluster_time);
    fClusterChargePointX = ClusterChargePoint.X();
    fClusterChargePointY = ClusterChargePoint.Y();
    fClusterChargePointZ = ClusterChargePoint.Z();
    fClusterChargeBalance = ClusterChargeBalances.at(cluster_time);
  }
  return good_classifiers;
}

bool PhaseIITreeMaker::LoadBNBtimingMC(double cluster_time){
  Log("PhaseITreeMaker tool: Getting BNB timing (MC)", v_debug, verbosity);
  bool got_bnb_times = m_data->Stores.at("ANNIEEvent")->Get("bunchTimes", bunchTimes);
  if(!got_bnb_times){
    Log("PhaseITreeMaker tool: BNB timing (MC) not found!", v_debug, verbosity);
  } else { 
    Log("PhaseITreeMaker tool: Setting fbunchTimes variables", v_debug, verbosity);
    fbunchTimes = bunchTimes.at(cluster_time);
  }
  return got_bnb_times;
}

void PhaseIITreeMaker::LoadTankClusterHits(std::vector<Hit> cluster_hits){
  Position detector_center=geom->GetTankCentre();
  double tank_center_x = detector_center.X();
  double tank_center_y = detector_center.Y();
  double tank_center_z = detector_center.Z();

  fClusterCharge = 0;
  fClusterPE = 0;
  fClusterHits = 0;
  for (int i = 0; i<(int)cluster_hits.size(); i++){
    int channel_key = cluster_hits.at(i).GetTubeId();
	Detector* this_detector = geom->ChannelToDetector(channel_key);
	if (ApplyDeadMask && this_detector->GetStatus() == detectorstatus::OFF) {
      continue;
    }
    std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(channel_key);
    if(it != ChannelKeyToSPEMap.end()){ //Charge to SPE conversion is available
      unsigned long detkey = this_detector->GetDetectorID();
      Position det_position = this_detector->GetDetectorPosition();
      double hit_charge = cluster_hits.at(i).GetCharge();
      double hit_PE = hit_charge / ChannelKeyToSPEMap.at(channel_key);
      fHitX.push_back((det_position.X()-tank_center_x));
      fHitY.push_back((det_position.Y()-tank_center_y));
      fHitZ.push_back((det_position.Z()-tank_center_z));
      fHitQ.push_back(hit_charge);
      fHitPE.push_back(hit_PE);
      fHitT.push_back(cluster_hits.at(i).GetTime());
      fHitDetID.push_back(detkey);
      fHitChankey.push_back(channel_key);
      fHitType.push_back(RecoDigit::PMT8inch);
      fClusterCharge+=hit_charge;
      fClusterPE+=hit_PE;
      fClusterHits += 1;
    } else {
      if(verbosity>4){
        std::cout << "FOUND A HIT FOR CHANNELKEY " << channel_key << "BUT NO CONVERSION " <<
            "TO PE AVAILABLE.  SKIPPING PE." << std::endl;
      }
    }
  }
  return;
}

void PhaseIITreeMaker::LoadTankClusterHitsMC(std::vector<MCHit> cluster_hits, std::vector<unsigned long> cluster_detkeys){
   Position detector_center=geom->GetTankCentre();
   double tank_center_x = detector_center.X();
   double tank_center_y = detector_center.Y();
   double tank_center_z = detector_center.Z();

   fClusterCharge = 0;
   fClusterPE = 0;
   fClusterHits = 0;
   for (int i = 0; i < (int) cluster_hits.size(); i++){
     unsigned long detkey = cluster_detkeys.at(i);
     int channel_key = (int) detkey; 
     int tubeid = cluster_hits.at(i).GetTubeId();
     unsigned long utubeid = (unsigned long) tubeid;
     int wcsimid = channelkey_to_pmtid.at(utubeid);
     unsigned long detkey_data = pmtid_to_channelkey[wcsimid];
     int channel_key_data = (int) detkey_data;
	 Detector* this_detector = geom->ChannelToDetector(tubeid);
     if (ApplyDeadMask && this_detector->GetStatus() == detectorstatus::OFF) {
       continue;
     }
     std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(channel_key_data);
     if(it != ChannelKeyToSPEMap.end()){ //Charge to SPE conversion is available
       Position det_position = this_detector->GetDetectorPosition();
       unsigned long detkey = this_detector->GetDetectorID();
       double hit_PE = cluster_hits.at(i).GetCharge();
       double hit_charge = hit_PE * ChannelKeyToSPEMap.at(channel_key_data);
       fHitX.push_back((det_position.X()-tank_center_x));
       fHitY.push_back((det_position.Y()-tank_center_y));
       fHitZ.push_back((det_position.Z()-tank_center_z));
       fHitQ.push_back(hit_charge);
       fHitPE.push_back(hit_PE);
       fHitT.push_back(cluster_hits.at(i).GetTime());
       //if (cluster_hits.at(i).GetTime()>2000.) std::cout <<"found cluster hit >  2us! Time: "<<cluster_hits.at(i).GetTime()<<std::endl;
       //fHitDetID.push_back(tubeid);
       //fHitChankey.push_back(channel_key_data);
       fHitDetID.push_back(detkey);
       fHitChankey.push_back(channel_key_data);
       fHitChankeyMC.push_back(channel_key);
       fHitType.push_back(RecoDigit::PMT8inch);
       fClusterCharge+=hit_charge;
       fClusterPE+=hit_PE;
       fClusterHits += 1;
     } else {
       if(verbosity>4){
         std::cout << "FOUND A HIT FOR CHANNELKEY " << channel_key_data << "(MC detkey: "<<channel_key<<", chankey = "<<utubeid<<", wcsimid = "<<wcsimid<<") BUT NO CONVERSION " <<
             "TO PE AVAILABLE.  SKIPPING PE." << std::endl;
       }
     }
   }
   return;
 }

void PhaseIITreeMaker::LoadSiPMHits() {
  std::map<unsigned long, std::vector< std::vector<ADCPulse>> > aux_pulse_map;
  m_data->Stores.at("ANNIEEvent")->Get("RecoADCAuxHits", aux_pulse_map);
  fSiPM1NPulses = 0;
  fSiPM2NPulses = 0;
  for (const auto& temp_pair : aux_pulse_map) {
    const auto& channel_key = temp_pair.first;
    //For now, only calibrate the SiPM waveforms
    int sipm_number = -1;
    if(AuxChannelNumToTypeMap->at(channel_key) == "SiPM1"){
      sipm_number = 1;
    } else if (AuxChannelNumToTypeMap->at(channel_key) == "SiPM2"){
      sipm_number = 2;
    } else continue;

    std::vector< std::vector<ADCPulse>> sipm_minibuffers = temp_pair.second;
    size_t num_minibuffers = sipm_minibuffers.size();  //Should be size 1 in FrankDAQ mode
    for (size_t mb = 0; mb < num_minibuffers; ++mb) {
      std::vector<ADCPulse> thisbuffer_pulses = sipm_minibuffers.at(mb);
      if(sipm_number == 1) fSiPM1NPulses += thisbuffer_pulses.size();
      if(sipm_number == 2) fSiPM2NPulses += thisbuffer_pulses.size();
      for (size_t i = 0; i < thisbuffer_pulses.size(); i++){
        ADCPulse apulse = thisbuffer_pulses.at(i);
        fSiPMHitAmplitude.push_back(apulse.amplitude());
        fSiPMHitT.push_back(apulse.peak_time());
        fSiPMHitQ.push_back(apulse.charge());
        fSiPMNum.push_back(sipm_number);
      }
    }
  }
}

void PhaseIITreeMaker::LoadAllMRDHits(bool IsData){

  fVetoHit = 0;
  bool get_ok = false;
  if (IsData) get_ok = m_data->Stores["ANNIEEvent"]->Get("TDCData",TDCData);  // a std::map<ChannelKey,vector<TDCHit>>
  else get_ok = m_data->Stores["ANNIEEvent"]->Get("TDCData",TDCData_MC);
  if (!get_ok){
    std::cout << "PhaseIITreeMaker tool: No TDCData store in ANNIEEvent." << std::endl;
    return;
  }
  int tdcdata_size = (IsData)? TDCData->size() : TDCData_MC->size();
  if(tdcdata_size>0){
    Log("PhaseIITreeMaker tool: Looping over FACC/MRD hits... looking for Veto activity",v_debug,verbosity);
    std::map<unsigned long,std::vector<Hit>>::iterator it_mrd_data;
    std::map<unsigned long,std::vector<MCHit>>::iterator it_mrd_mc;
    if (IsData) it_mrd_data = (*TDCData).begin();
    else it_mrd_mc = (*TDCData_MC).begin();
    bool loop_mrd = true;
    while (loop_mrd){
    //for(auto&& anmrdpmt : (*TDCData)){
      unsigned long chankey = 0;
      if (IsData) chankey = it_mrd_data->first;
      else chankey = it_mrd_mc->first;
      Detector* thedetector = geom->ChannelToDetector(chankey);
      unsigned long detkey = thedetector->GetDetectorID();
      std::vector<Hit> mrdhits;
      std::vector<MCHit> mrdhits_mc;
      if (IsData) mrdhits = it_mrd_data->second;
      else mrdhits_mc = it_mrd_mc->second;
      if(thedetector->GetDetectorElement()=="Veto") {
        fVetoHit=1; // this is a veto hit, not an MRD hit.
        if (IsData){
          for (int j = 0; j < (int) mrdhits.size(); j++){
            fFMVHitT.push_back(mrdhits.at(j).GetTime());
            fFMVHitDetID.push_back(detkey);
            fFMVHitChankey.push_back(chankey);
            fFMVHitChankeyMC.push_back(chankey);
          }
        } else {
          int wcsimid = channelkey_to_faccpmtid.at(chankey)-1;
          unsigned long chankey_data = wcsimid;
          for (int j = 0; j < (int) mrdhits_mc.size(); j++){
            fFMVHitT.push_back(mrdhits_mc.at(j).GetTime());
            fFMVHitDetID.push_back(detkey);
            fFMVHitChankey.push_back(chankey_data);
            fFMVHitChankeyMC.push_back(chankey);
          }
        }
      } else {
        if (IsData){
          for(int j = 0; j < (int) mrdhits.size(); j++){
            fMRDHitT.push_back(mrdhits.at(j).GetTime());
            fMRDHitDetID.push_back(detkey);
            fMRDHitChankey.push_back(chankey);
            fMRDHitChankeyMC.push_back(chankey);
          }
        } else {
         int wcsimid = channelkey_to_mrdpmtid.at(chankey)-1;
         unsigned long chankey_data = mrdpmtid_to_channelkey_data[wcsimid];
          for(int j = 0; j < (int) mrdhits_mc.size(); j++){
            fMRDHitT.push_back(mrdhits_mc.at(j).GetTime());
            fMRDHitDetID.push_back(detkey);
            fMRDHitChankey.push_back(chankey_data);
            fMRDHitChankeyMC.push_back(chankey);
          }
        }
      }
      if (IsData){
        it_mrd_data++; 
        if (it_mrd_data == (*TDCData).end()) loop_mrd = false;
      } else {
        it_mrd_mc++;
        if (it_mrd_mc == (*TDCData_MC).end()) loop_mrd = false;
      }
    }
  }
  return;
}


//DIGITS  
void PhaseIITreeMaker::LoadDigitHits(){
    // get digits from RecoDigit store
       std::vector<RecoDigit>* digitList;
       bool get_digits=false;
       get_digits=m_data->Stores.at("RecoEvent")->Get("RecoDigit", digitList);
       if(!get_digits){
         Log("PhaseIITreeMaker Tool: Failed to retrieve the RecoDigit from RecoEvent Store!",v_error,verbosity);
         return;
          }
   // Extract the PMT & LAPPD digit information
   // ===============================
  //loop through all digits
        for(RecoDigit &adigit : *digitList){
	   fdigitX.push_back(adigit.GetPosition().X());
	   fdigitY.push_back(adigit.GetPosition().Y());
	   fdigitZ.push_back(adigit.GetPosition().Z());
	   fdigitT.push_back(adigit.GetCalTime());
	   if(adigit.GetDigitType()==0){fNDigitsPMTs+=1;} //when the digit type is zero we have a PMT digit
	   else{fNDigitsLAPPDs+=1;}// when it is 1 we have LAPPD
           }
        Log("PhaseIITreeMaker Tool: Got "+to_string(fNDigitsPMTs)+" PMT digits; "+to_string(fdigitT.size()) +" total digits so far",v_debug,verbosity);
        Log("PhaseIITreeMaker Tool: Got "+to_string(fNDigitsLAPPDs)+" LAPPD digits; "+to_string(fdigitT.size()) +" total digits",v_debug,verbosity);
   return;
}       
//DIGITS

int PhaseIITreeMaker::LoadMRDTrackReco(int SubEventID) {
  //Check for valid track criteria
  m_data->Stores["MRDTracks"]->Get("MRDTracks",theMrdTracks);
  m_data->Stores["MRDTracks"]->Get("NumMrdTracks",numtracksinev);
  // Loop over reconstructed tracks


  Position StartVertex;
  Position StopVertex;
  double TrackLength = -9999;
  double TrackAngle = -9999;
  double TrackAngleError = -9999;
  double PenetrationDepth = -9999;
  Position MrdEntryPoint;
  double EnergyLoss = -9999; //in MeV
  double EnergyLossError = -9999;
  double EntryPointRadius = -9999;
  bool IsMrdPenetrating;
  bool IsMrdStopped;
  bool IsMrdSideExit;

  int NumClusterTracks = 0;
  for(int tracki=0; tracki<numtracksinev; tracki++){
    BoostStore* thisTrackAsBoostStore = &(theMrdTracks->at(tracki));
    int TrackEventID = -1; 
    //get track properties that are needed for the through-going muon selection
	thisTrackAsBoostStore->Get("MrdSubEventID",TrackEventID);
    if(TrackEventID!= SubEventID) continue;
 

    //If we're here, this track is associated with this cluster
    thisTrackAsBoostStore->Get("StartVertex",StartVertex);
    thisTrackAsBoostStore->Get("StopVertex",StopVertex);
    thisTrackAsBoostStore->Get("TrackAngle",TrackAngle);
    thisTrackAsBoostStore->Get("TrackAngleError",TrackAngleError);
    thisTrackAsBoostStore->Get("PenetrationDepth",PenetrationDepth);
    thisTrackAsBoostStore->Get("MrdEntryPoint",MrdEntryPoint);
    thisTrackAsBoostStore->Get("EnergyLoss",EnergyLoss);
    thisTrackAsBoostStore->Get("EnergyLossError",EnergyLossError);
    thisTrackAsBoostStore->Get("IsMrdPenetrating",IsMrdPenetrating);        // bool
    thisTrackAsBoostStore->Get("IsMrdStopped",IsMrdStopped);                // bool
    thisTrackAsBoostStore->Get("IsMrdSideExit",IsMrdSideExit);
    TrackLength = sqrt(pow((StopVertex.X()-StartVertex.X()),2)+pow(StopVertex.Y()-StartVertex.Y(),2)+pow(StopVertex.Z()-StartVertex.Z(),2)) * 100.0;
    EntryPointRadius = sqrt(pow(MrdEntryPoint.X(),2) + pow(MrdEntryPoint.Y(),2)) * 100.0; // convert to cm
    PenetrationDepth = PenetrationDepth*100.0;


    //Push back some properties
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
    NumClusterTracks+=1;
  }
  return NumClusterTracks;
}

void PhaseIITreeMaker::LoadAllTankHits(bool isData, bool MCWaveform) {
    std::map<unsigned long, std::vector<Hit>>* Hits = nullptr;
    std::map<unsigned long, std::vector<MCHit>>* MCHits = nullptr;
    bool got_hits = false;

    if (isData) {
        got_hits = m_data->Stores["ANNIEEvent"]->Get("Hits", Hits);
    } else {
        if (MCWaveform) {
            got_hits = m_data->Stores["ANNIEEvent"]->Get("Hits", Hits);
        } else {
            got_hits = m_data->Stores["ANNIEEvent"]->Get("MCHits", MCHits);
        }
    }

    if (!got_hits) {
        std::cout << "No Hits store in ANNIEEvent. Continuing to build tree." << std::endl;
        return;
    }

    Position detector_center = geom->GetTankCentre();
    double tank_center_x = detector_center.X();
    double tank_center_y = detector_center.Y();
    double tank_center_z = detector_center.Z();
    fNHits = 0;

    bool loop_tank = true;
    int hits_size = (isData || MCWaveform) ? Hits->size() : MCHits->size();
    if (hits_size == 0) loop_tank = false;

    auto it_tank_data = (isData || MCWaveform) ? Hits->begin() : std::map<unsigned long, std::vector<Hit>>::iterator();
    auto it_tank_mc = (!isData && !MCWaveform) ? MCHits->begin() : std::map<unsigned long, std::vector<MCHit>>::iterator();

    while (loop_tank) {
        unsigned long channel_key = (isData || MCWaveform) ? it_tank_data->first : it_tank_mc->first;
        Detector* this_detector = geom->ChannelToDetector(channel_key);
        Position det_position = this_detector->GetDetectorPosition();
        unsigned long detkey = this_detector->GetDetectorID();
        unsigned long channel_key_data = channel_key;

        if (!isData && !MCWaveform) {
            int wcsimid = channelkey_to_pmtid.at(channel_key);
            channel_key_data = pmtid_to_channelkey[wcsimid];
        }

		bool SPE_available = false;

        if (ApplyDeadMask && this_detector->GetStatus() == detectorstatus::OFF) {
            goto skip_channel;  // do not save the hits information for a Dead PMT (if the mask is on), jump to skip_channel
        }

        SPE_available = (isData || MCWaveform) ? 
                             (ChannelKeyToSPEMap.find(channel_key) != ChannelKeyToSPEMap.end()) : 
                             (ChannelKeyToSPEMap.find(channel_key_data) != ChannelKeyToSPEMap.end());

        if (SPE_available) {
            if (isData || MCWaveform) {
                std::vector<Hit> ThisPMTHits = it_tank_data->second;
                fNHits += ThisPMTHits.size();
                for (Hit &ahit : ThisPMTHits) {
                    double hit_charge = ahit.GetCharge();
                    double hit_PE = hit_charge / ChannelKeyToSPEMap.at(channel_key);
                    fHitX.push_back(det_position.X() - tank_center_x);
                    fHitY.push_back(det_position.Y() - tank_center_y);
                    fHitZ.push_back(det_position.Z() - tank_center_z);
                    fHitT.push_back(ahit.GetTime());
                    fHitQ.push_back(hit_charge);
                    fHitPE.push_back(hit_PE);
                    fHitDetID.push_back(detkey);
                    fHitChankey.push_back(channel_key);
                    fHitChankeyMC.push_back(channel_key);
                    fHitType.push_back(RecoDigit::PMT8inch);
                }
            } else {
                std::vector<MCHit> ThisPMTHits = it_tank_mc->second;
                fNHits += ThisPMTHits.size();
                for (MCHit &ahit : ThisPMTHits) {
                    double hit_PE = ahit.GetCharge();
                    double hit_charge = hit_PE * ChannelKeyToSPEMap.at(channel_key_data);
                    fHitX.push_back(det_position.X() - tank_center_x);
                    fHitY.push_back(det_position.Y() - tank_center_y);
                    fHitZ.push_back(det_position.Z() - tank_center_z);
                    fHitT.push_back(ahit.GetTime());
                    fHitQ.push_back(hit_charge);
                    fHitPE.push_back(hit_PE);
                    fHitDetID.push_back(detkey);
                    fHitChankey.push_back(channel_key_data);
                    fHitChankeyMC.push_back(channel_key);
                    fHitType.push_back(RecoDigit::PMT8inch);
                }
            }
        }

        skip_channel:   // skip the block above if the PMT is dead, advance the iterator
        if (isData || MCWaveform) {
            it_tank_data++;
            if (it_tank_data == Hits->end()) loop_tank = false;
        } else {
            it_tank_mc++;
            if (it_tank_mc == MCHits->end()) loop_tank = false;
        }
    }
    return;
}

bool PhaseIITreeMaker::FillTankRecoInfo() {
  bool got_reco_info = true;
  auto* reco_event = m_data->Stores["RecoEvent"];
  if (!reco_event) {
    Log("Error: The PhaseITreeMaker tool could not find the RecoEvent Store", v_error, verbosity);
    got_reco_info = false;
  }
  // Read reconstructed Vertex
  RecoVertex* recovtx = 0;
  auto get_extendedvtx = m_data->Stores.at("RecoEvent")->Get("ExtendedVertex",recovtx); 
  if(!get_extendedvtx) {
    Log("Warning: The PhaseITreeMaker tool could not find ExtendedVertex. Continuing to build tree", v_message, verbosity);
    got_reco_info = false;
  } else {
    fRecoVtxX = recovtx->GetPosition().X();
    fRecoVtxY = recovtx->GetPosition().Y();
    fRecoVtxZ = recovtx->GetPosition().Z();
    fRecoVtxTime = recovtx->GetTime();
    fRecoVtxFOM = recovtx->GetFOM();
    fRecoDirX = recovtx->GetDirection().X();
    fRecoDirY = recovtx->GetDirection().Y();
    fRecoDirZ = recovtx->GetDirection().Z();
    fRecoAngle = TMath::ACos(fRecoDirZ);
    if (fRecoDirX>0.0){
      fRecoPhi = atan(fRecoDirY/fRecoDirX);
    }
    if (fRecoDirX<0.0){
      fRecoPhi = atan(fRecoDirY/fRecoDirX);
      if( fRecoDirY>0.0) fRecoPhi += TMath::Pi();
      if( fRecoDirY<=0.0) fRecoPhi -= TMath::Pi();
    }
    if (fRecoDirX==0.0){
      if( fRecoDirY>0.0) fRecoPhi = 0.5*TMath::Pi();
      else if( fRecoDirY<0.0) fRecoPhi = -0.5*TMath::Pi();
      else fRecoPhi = 0;
    }
    fRecoStatus = recovtx->GetStatus();
  }
  return got_reco_info;
}

void PhaseIITreeMaker::FillRingCountingInfo() {
  auto* reco_event = m_data->Stores["RecoEvent"];
  if (!reco_event) {
    Log("Error: The PhaseIITreeMaker tool could not find the RecoEvent Store", v_error, verbosity);
  }
  double RCSRPred;
  double RCMRPred;
  auto get_sr = m_data->Stores["RecoEvent"]->Get("RingCountingSRPrediction",RCSRPred);
  auto get_mr = m_data->Stores["RecoEvent"]->Get("RingCountingMRPrediction",RCMRPred);

  if(get_sr && get_mr){
    fRCSRPred = RCSRPred;
    fRCMRPred = RCMRPred;
  }
  else Log("Error: PhaseIITreeMaker tool could not find RingCountingPrediction from RecoEvent Store", v_error, verbosity); 
  return;
}

void PhaseIITreeMaker::FillSimpleRecoInfo() {
  auto* reco_event = m_data->Stores["RecoEvent"];
  if (!reco_event) {
    Log("Error: The PhaseITreeMaker tool could not find the RecoEvent Store", v_error, verbosity);
  }
  int SimpleRecoFlag;
  bool SimpleRecoFV;
  double SimpleRecoEnergy, SimpleRecoCosTheta, SimpleRecoPt, SimpleRecoMrdEnergyLoss, SimpleRecoTrackLengthInMRD;
  double SimpleRecoTrackLengthInTank;
  Position SimpleRecoVtx;
  Position SimpleRecoStopVtx;
  Position SimpleRecoMRDStart;
  Position SimpleRecoMRDStop;
  auto get_flag = m_data->Stores["RecoEvent"]->Get("SimpleRecoFlag",SimpleRecoFlag);
  auto get_energy = m_data->Stores["RecoEvent"]->Get("SimpleRecoEnergy",SimpleRecoEnergy);
  auto get_vtx = m_data->Stores["RecoEvent"]->Get("SimpleRecoVtx",SimpleRecoVtx);
  auto get_stopvtx = m_data->Stores["RecoEvent"]->Get("SimpleRecoStopVtx",SimpleRecoStopVtx);
  auto get_cos = m_data->Stores["RecoEvent"]->Get("SimpleRecoCosTheta",SimpleRecoCosTheta);
  auto get_pt = m_data->Stores["RecoEvent"]->Get("SimpleRecoPt",SimpleRecoPt);
  auto get_fv = m_data->Stores["RecoEvent"]->Get("SimpleRecoFV",SimpleRecoFV);
  auto get_mrdenergy = m_data->Stores["RecoEvent"]->Get("SimpleRecoMrdEnergyLoss",SimpleRecoMrdEnergyLoss);
  auto get_mrdtrack = m_data->Stores["RecoEvent"]->Get("SimpleRecoTrackLengthInMRD",SimpleRecoTrackLengthInMRD);
  auto get_tanktrack = m_data->Stores["RecoEvent"]->Get("SimpleRecoTrackLengthInTank",SimpleRecoTrackLengthInTank);
  auto get_mrdstart = m_data->Stores["RecoEvent"]->Get("SimpleRecoMRDStart",SimpleRecoMRDStart);
  auto get_mrdstop = m_data->Stores["RecoEvent"]->Get("SimpleRecoMRDStop",SimpleRecoMRDStop); 

  if(get_flag && get_energy && get_vtx && get_stopvtx && get_cos && get_pt && get_fv && get_mrdenergy && get_mrdtrack && get_mrdstart && get_mrdstop){
    fSimpleFlag = SimpleRecoFlag;
    fSimpleEnergy = SimpleRecoEnergy;
    fSimpleVtxX = SimpleRecoVtx.X();
    fSimpleVtxY = SimpleRecoVtx.Y();
    fSimpleVtxZ = SimpleRecoVtx.Z();
    fSimpleStopVtxX = SimpleRecoStopVtx.X();
    fSimpleStopVtxY = SimpleRecoStopVtx.Y();
    fSimpleStopVtxZ = SimpleRecoStopVtx.Z();
    fSimpleCosTheta = SimpleRecoCosTheta;
    fSimplePt = SimpleRecoPt;
    fSimpleFV = (SimpleRecoFV)? 1 : 0;
    fSimpleMrdEnergyLoss = SimpleRecoMrdEnergyLoss;
    fSimpleTrackLengthInMRD = SimpleRecoTrackLengthInMRD;
    fSimpleTrackLengthInTank = SimpleRecoTrackLengthInTank;
    fSimpleMRDStartX = SimpleRecoMRDStart.X();
    fSimpleMRDStartY = SimpleRecoMRDStart.Y();
    fSimpleMRDStartZ = SimpleRecoMRDStart.Z();
    fSimpleMRDStopX = SimpleRecoMRDStop.X();
    fSimpleMRDStopY = SimpleRecoMRDStop.Y();
    fSimpleMRDStopZ = SimpleRecoMRDStop.Z();
  }
  return;
}

void PhaseIITreeMaker::FillRecoDebugInfo() {
  // Read Seed candidates   
  std::vector<RecoVertex>* seedvtxlist = 0;
  auto get_seedvtxlist = m_data->Stores.at("RecoEvent")->Get("vSeedVtxList",seedvtxlist);  ///> Get List of seeds from "RecoEvent"
  if(get_seedvtxlist){
    for( auto& seed : *seedvtxlist ){
      fSeedVtxX.push_back(seed.GetPosition().X());
      fSeedVtxY.push_back(seed.GetPosition().Y());
      fSeedVtxZ.push_back(seed.GetPosition().Z());
      fSeedVtxTime = seed.GetTime();
    }
  } else {  
	  Log("PhaseIITreeMaker  Tool: No Seed List found.  Continuing to build tree ",v_message,verbosity); 
  }
  std::vector<double>* seedfomlist = 0;
  auto get_seedfomlist = m_data->Stores.at("RecoEvent")->Get("vSeedFOMList",seedfomlist);  ///> Get List of seed FOMs from "RecoEvent" 
  if(get_seedfomlist){
    for( auto& seedFOM : *seedfomlist ){
      fSeedVtxFOM.push_back(seedFOM);
    }
  } else {  
	  Log("PhaseIITreeMaker  Tool: No Seed FOM List found.  Continuing to build tree ",v_message,verbosity); 
  }
  
  // Read PointPosition-fitted Vertex   
  RecoVertex* pointposvtx = 0;
  auto get_pointposdata = m_data->Stores.at("RecoEvent")->Get("PointPosition",pointposvtx);
  if(get_pointposdata){ 
    fPointPosX = pointposvtx->GetPosition().X();
    fPointPosY = pointposvtx->GetPosition().Y();
    fPointPosZ = pointposvtx->GetPosition().Z();
    fPointPosTime = pointposvtx->GetTime();
    fPointPosFOM = pointposvtx->GetFOM();
    fPointPosStatus = pointposvtx->GetStatus();
  } else{
    Log("PhaseIITreeMaker Tool: No PointPosition Tool data found.  Continuing to build remaining tree",v_message,verbosity);
  }

  // Read PointDirection-fitted Vertex
  RecoVertex* pointdirvtx = 0;
  auto get_pointdirdata = m_data->Stores.at("RecoEvent")->Get("PointDirection",pointdirvtx);
  if(get_pointdirdata){ 
    fPointDirX = pointdirvtx->GetDirection().X();
    fPointDirY = pointdirvtx->GetDirection().Y();
    fPointDirZ = pointdirvtx->GetDirection().Z();
    fPointDirTime = pointdirvtx->GetTime();
    fPointDirFOM = pointdirvtx->GetFOM();
    fPointDirStatus = pointdirvtx->GetStatus();
  } else{
    Log("PhaseIITreeMaker Tool: No PointDirection Tool data found.  Continuing to build remaining tree",v_message,verbosity);
  }
  
  // Read PointVertex Tool's fitted Vertex
  RecoVertex* pointvtx = 0;
  auto get_pointvtxdata = m_data->Stores.at("RecoEvent")->Get("PointVertex",pointvtx);
  if(get_pointvtxdata){ 
    fPointVtxPosX = pointvtx->GetPosition().X();
    fPointVtxPosY = pointvtx->GetPosition().Y();
    fPointVtxPosZ = pointvtx->GetPosition().Z();
    fPointVtxDirX = pointvtx->GetDirection().X();
    fPointVtxDirY = pointvtx->GetDirection().Y();
    fPointVtxDirZ = pointvtx->GetDirection().Z();
    fPointVtxTime = pointvtx->GetTime();
    fPointVtxFOM = pointvtx->GetFOM();
    fPointVtxStatus = pointvtx->GetStatus();
  } else{
    Log("PhaseIITreeMaker Tool: No PointVertex Tool data found.  Continuing to build remaining tree",v_message,verbosity);
  }
}

bool PhaseIITreeMaker::FillMCTruthInfo() {
  bool successful_load = true;
  // MC entry number
  m_data->Stores.at("ANNIEEvent")->Get("MCEventNum",fMCEventNum);  
  // MC trigger number
  m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum",fMCTriggerNum); 
  std::string logmessage = "  Retriving information for MCEntry "+to_string(fMCEventNum)+
  	", MCTrigger "+ to_string(fMCTriggerNum) + ", EventNumber " + to_string(fEventNumber);
  Log(logmessage, v_message, verbosity);
 
  fiMCTriggerNum = (int) fMCTriggerNum;

  std::map<std::string, std::vector<double>> MCNeutCap;
  bool get_neutcap = m_data->Stores.at("ANNIEEvent")->Get("MCNeutCap",MCNeutCap);
  if (!get_neutcap){
    Log("PhaseIITreeMaker: Did not find MCNeutCap in ANNIEEvent Store!",v_warning,verbosity);
  }
  std::map<std::string, std::vector<std::vector<double>>> MCNeutCapGammas;
  bool get_neutcap_gammas = m_data->Stores.at("ANNIEEvent")->Get("MCNeutCapGammas",MCNeutCapGammas);
  if (!get_neutcap_gammas){
    Log("PhaseIITreeMaker: Did not find MCNeutCapGammas in ANNIEEvent Store!",v_warning,verbosity);
  }

  for (std::map<std::string, std::vector<std::vector<double>>>::iterator it = MCNeutCapGammas.begin(); it!= MCNeutCapGammas.end(); it++){
  std::vector<std::vector<double>> mcneutgammas = it->second;
  for (int i_cap=0; i_cap < (int) mcneutgammas.size(); i_cap++){
    std::vector<double> capgammas = mcneutgammas.at(i_cap);
    for (int i_gamma=0; i_gamma < (int) capgammas.size(); i_gamma++){
      std::cout <<"gamma # "<<i_gamma<<", energy: "<<capgammas.at(i_gamma)<<std::endl;
    }
  }
  }

  RecoVertex* truevtx = 0;
  auto get_muonMC = m_data->Stores.at("RecoEvent")->Get("TrueVertex",truevtx);
  auto get_muonMCEnergy = m_data->Stores.at("RecoEvent")->Get("TrueMuonEnergy",fTrueMuonEnergy);
  auto get_pdg = m_data->Stores.at("RecoEvent")->Get("PdgPrimary",fTruePrimaryPdg);
  if(get_muonMC && get_muonMCEnergy){ 
    fTrueVtxX = truevtx->GetPosition().X();
    fTrueVtxY = truevtx->GetPosition().Y();
    fTrueVtxZ = truevtx->GetPosition().Z();
    fTrueVtxTime = truevtx->GetTime();
    fTrueDirX = truevtx->GetDirection().X();
    fTrueDirY = truevtx->GetDirection().Y();
    fTrueDirZ = truevtx->GetDirection().Z();
    double TrueAngRad = TMath::ACos(fTrueDirZ);
    fTrueAngle = TrueAngRad/(TMath::Pi()/180.0); // radians->degrees
    if (fTrueDirX>0.0){
      fTruePhi = atan(fTrueDirY/fTrueDirX);
    }
    if (fTrueDirX<0.0){
      fTruePhi = atan(fTrueDirY/fTrueDirX);
      if( fTrueDirY>0.0) fTruePhi += TMath::Pi();
      if( fTrueDirY<=0.0) fTruePhi -= TMath::Pi();
    }
    if (fTrueDirX==0.0){
      if( fTrueDirY>0.0) fTruePhi = 0.5*TMath::Pi();
      else if( fTrueDirY<0.0) fTruePhi = -0.5*TMath::Pi();
      else fTruePhi = 0;
    }
  } else {
    Log("PhaseIITreeMaker Tool: Missing MC Energy/Vertex info; is this MC?  Continuing to build remaining tree",v_message,verbosity);
    successful_load = false;
  }
  double waterT, MRDT;
  auto get_tankTrackLength = m_data->Stores.at("RecoEvent")->Get("TrueTrackLengthInWater",waterT); 
  auto get_MRDTrackLength = m_data->Stores.at("RecoEvent")->Get("TrueTrackLengthInMRD",MRDT); 
  if (get_tankTrackLength && get_MRDTrackLength){
    fTrueTrackLengthInWater = waterT;
    fTrueTrackLengthInMRD = MRDT;
  } else {
    Log("PhaseIITreeMaker Tool: True track lengths missing. Continuing to build tree",v_message,verbosity);
    successful_load = false;
  }

  bool IsMultiRing = false;
  bool get_multi = m_data->Stores.at("RecoEvent")->Get("MCMultiRingEvent",IsMultiRing);
  if (get_multi){
    fTrueMultiRing = (IsMultiRing)? 1 : 0;
  } else {
    Log("PhaseIITreeMaker Tool: True Multi Ring information missing. Continuing to build tree",v_message,verbosity);
    successful_load = false;
  }

  std::vector<int> primary_pdgs;
  bool has_primaries = m_data->Stores.at("RecoEvent")->Get("PrimaryPdgs",primary_pdgs);
  if (has_primaries){
    for (int i_part=0; i_part < (int) primary_pdgs.size(); i_part++){
      fTruePrimaryPdgs->push_back(primary_pdgs.at(i_part));
    }
  } else {
    Log("PhaseIITreeMaker Tool: Primary Pdgs information missing. Continuing to build tree",v_message,verbosity);
    successful_load = false;
  }

  int pi0count, pipcount, pimcount, K0count, Kpcount, Kmcount;
  auto get_pi0 = m_data->Stores.at("RecoEvent")->Get("MCPi0Count",pi0count);
  auto get_pim = m_data->Stores.at("RecoEvent")->Get("MCPiMinusCount",pimcount);
  auto get_pip = m_data->Stores.at("RecoEvent")->Get("MCPiPlusCount",pipcount);
  auto get_k0 = m_data->Stores.at("RecoEvent")->Get("MCK0Count",K0count);
  auto get_km = m_data->Stores.at("RecoEvent")->Get("MCKMinusCount",Kmcount);
  auto get_kp = m_data->Stores.at("RecoEvent")->Get("MCKPlusCount",Kpcount);
  if(get_pi0 && get_pim && get_pip && get_k0 && get_km && get_kp) {
    // set values in tree to thouse grabbed from the RecoEvent Store
    fPi0Count = pi0count;
    fPiPlusCount = pipcount;
    fPiMinusCount = pimcount;
    fK0Count = K0count;
    fKPlusCount = Kpcount;
    fKMinusCount = Kmcount;
  } else {
    Log("PhaseIITreeMaker Tool: Missing MC Pion/Kaon count information. Continuing to build remaining tree",v_message,verbosity);
    successful_load = false;
  }

  if (MCNeutCap.count("CaptVtxX")>0){
    std::vector<double> n_vtxx = MCNeutCap["CaptVtxX"];
    std::vector<double> n_vtxy = MCNeutCap["CaptVtxY"];
    std::vector<double> n_vtxz = MCNeutCap["CaptVtxZ"];
    std::vector<double> n_parent = MCNeutCap["CaptParent"];
    std::vector<double> n_ngamma = MCNeutCap["CaptNGamma"];
    std::vector<double> n_totale = MCNeutCap["CaptTotalE"];
    std::vector<double> n_time = MCNeutCap["CaptTime"];
    std::vector<double> n_nuc = MCNeutCap["CaptNucleus"];
 
    for (int i_cap=0; i_cap < (int) n_vtxx.size(); i_cap++){
      fTrueNeutCapVtxX->push_back(n_vtxx.at(i_cap));
      fTrueNeutCapVtxY->push_back(n_vtxy.at(i_cap));
      fTrueNeutCapVtxZ->push_back(n_vtxz.at(i_cap));
      fTrueNeutCapNucleus->push_back(n_nuc.at(i_cap));
      fTrueNeutCapTime->push_back(n_time.at(i_cap));
      fTrueNeutCapGammas->push_back(n_ngamma.at(i_cap));
      fTrueNeutCapE->push_back(n_totale.at(i_cap));
    }
  }

  std::cout <<"MCNeutCapGammas count CaptGammas: "<<MCNeutCapGammas.count("CaptGammas")<<std::endl;
  if (MCNeutCapGammas.count("CaptGammas")>0){
    std::vector<std::vector<double>> cap_energies = MCNeutCapGammas["CaptGammas"];
    std::cout <<"cap_energies size: "<<cap_energies.size()<<std::endl;
    for (int i_cap = 0; i_cap < (int) cap_energies.size(); i_cap++){
      for (int i_gamma=0; i_gamma < cap_energies.at(i_cap).size(); i_gamma++){
        std::cout <<"gamma energy: "<<cap_energies.at(i_cap).at(i_gamma)<<std::endl;
        fTrueNeutCapGammaE->push_back(cap_energies.at(i_cap).at(i_gamma));
      }
    }
  }

  //Load genie information
  if (hasGenie){
    double TrueNeutrinoEnergy, TrueQ2, TrueNuIntxVtx_X, TrueNuIntxVtx_Y, TrueNuIntxVtx_Z, TrueNuIntxVtx_T;
    double TrueFSLeptonMass, TrueFSLeptonEnergy, TrueFSLeptonTime;
    bool TrueCC, TrueNC, TrueQEL, TrueDIS, TrueCOH, TrueMEC, TrueRES;
    int fsNeutrons, fsProtons, fsPi0, fsPiPlus, fsPiPlusCher, fsPiMinus, fsPiMinusCher;
    int fsKPlus, fsKPlusCher, fsKMinus, fsKMinusCher, TrueNuPDG, TrueFSLeptonPdg;
    int TrueTarget;
    double TrueW2, TrueBJx, Truey, Trueq0, Trueq3;
    double TrueNuIntxVtxDisToEdge;
    Position TrueFSLeptonVtx;
    Direction TrueFSLeptonMomentum;
    Direction TrueNeutrinoMomentum;
    bool get_neutrino_energy = m_data->Stores["GenieInfo"]->Get("NeutrinoEnergy",TrueNeutrinoEnergy);
    bool get_neutrino_mom = m_data->Stores["GenieInfo"]->Get("NeutrinoMomentum",TrueNeutrinoMomentum);
    bool get_neutrino_vtxx = m_data->Stores["GenieInfo"]->Get("NuIntxVtx_X",TrueNuIntxVtx_X);
    bool get_neutrino_vtxy = m_data->Stores["GenieInfo"]->Get("NuIntxVtx_Y",TrueNuIntxVtx_Y);
    bool get_neutrino_vtxz = m_data->Stores["GenieInfo"]->Get("NuIntxVtx_Z",TrueNuIntxVtx_Z);
    bool get_neutrino_vtxt = m_data->Stores["GenieInfo"]->Get("NuIntxVtx_T",TrueNuIntxVtx_T);
    bool get_q2 = m_data->Stores["GenieInfo"]->Get("EventQ2",TrueQ2);
    bool get_cc = m_data->Stores["GenieInfo"]->Get("IsWeakCC",TrueCC);
    bool get_nc = m_data->Stores["GenieInfo"]->Get("IsWeakNC",TrueNC);
    bool get_qel = m_data->Stores["GenieInfo"]->Get("IsQuasiElastic",TrueQEL);
    bool get_res = m_data->Stores["GenieInfo"]->Get("IsResonant",TrueRES);
    bool get_dis = m_data->Stores["GenieInfo"]->Get("IsDeepInelastic",TrueDIS);
    bool get_coh = m_data->Stores["GenieInfo"]->Get("IsCoherent",TrueCOH);
    bool get_mec = m_data->Stores["GenieInfo"]->Get("IsMEC",TrueMEC);
    bool get_n = m_data->Stores["GenieInfo"]->Get("NumFSNeutrons",fsNeutrons);
    bool get_p = m_data->Stores["GenieInfo"]->Get("NumFSProtons",fsProtons);
    bool get_pi0 = m_data->Stores["GenieInfo"]->Get("NumFSPi0",fsPi0);
    bool get_piplus = m_data->Stores["GenieInfo"]->Get("NumFSPiPlus",fsPiPlus);
    bool get_pipluscher = m_data->Stores["GenieInfo"]->Get("NumFSPiPlusCher",fsPiPlusCher);
    bool get_piminus = m_data->Stores["GenieInfo"]->Get("NumFSPiMinus",fsPiMinus);
    bool get_piminuscher = m_data->Stores["GenieInfo"]->Get("NumFSPiMinusCher",fsPiMinusCher);
    bool get_kplus = m_data->Stores["GenieInfo"]->Get("NumFSKPlus",fsKPlus);
    bool get_kpluscher = m_data->Stores["GenieInfo"]->Get("NumFSKPlusCher",fsKPlusCher);
    bool get_kminus = m_data->Stores["GenieInfo"]->Get("NumFSKMinus",fsKMinus);
    bool get_kminuscher = m_data->Stores["GenieInfo"]->Get("NumFSKMinusCher",fsKMinusCher);
    bool get_fsl_vtx = m_data->Stores["GenieInfo"]->Get("FSLeptonVertex",TrueFSLeptonVtx);
    bool get_fsl_momentum = m_data->Stores["GenieInfo"]->Get("FSLeptonMomentum",TrueFSLeptonMomentum);
    bool get_fsl_time = m_data->Stores["GenieInfo"]->Get("FSLeptonTime",TrueFSLeptonTime);
    bool get_fsl_mass = m_data->Stores["GenieInfo"]->Get("FSLeptonMass",TrueFSLeptonMass);
    bool get_fsl_pdg = m_data->Stores["GenieInfo"]->Get("FSLeptonPdg",TrueFSLeptonPdg);
    bool get_fsl_energy = m_data->Stores["GenieInfo"]->Get("FSLeptonEnergy",TrueFSLeptonEnergy);
    bool get_w = m_data->Stores["GenieInfo"]->Get("EventW2",TrueW2);
    bool get_bjx = m_data->Stores["GenieInfo"]->Get("EventBjx",TrueBJx);
    bool get_y = m_data->Stores["GenieInfo"]->Get("Eventy",Truey);
    bool get_targetZ = m_data->Stores["GenieInfo"]->Get("TargetZ",TrueTarget);
    bool get_q0 = m_data->Stores["GenieInfo"]->Get("Eventq0",Trueq0);
    bool get_q3 = m_data->Stores["GenieInfo"]->Get("Eventq3",Trueq3);
    bool get_nu_pdg = m_data->Stores["GenieInfo"]->Get("NeutrinoPDG",TrueNuPDG);
    std::cout <<"get_neutrino_energy: "<<get_neutrino_energy<<"get_neutrino_vtxx: "<<get_neutrino_vtxx<<"get_neutrino_vtxy: "<<get_neutrino_vtxy<<"get_neutrino_vtxz: "<<get_neutrino_vtxz<<"get_neutrino_time: "<<get_neutrino_vtxt<<std::endl;
    std::cout <<"get_q2: "<<get_q2<<", get_cc: "<<get_cc<<", get_qel: "<<get_qel<<", get_res: "<<get_res<<", get_dis: "<<get_dis<<", get_coh: "<<get_coh<<", get_mec: "<<get_mec<<std::endl;
    std::cout <<"get_n: "<<get_n<<", get_p: "<<get_p<<", get_pi0: "<<get_pi0<<", get_piplus: "<<get_piplus<<", get_pipluscher: "<<get_pipluscher<<", get_piminus: "<<get_piminus<<", get_piminuscher: "<<get_piminuscher<<", get_kplus: "<<get_kplus<<", get_kpluscher: "<<get_kpluscher<<", get_kminus: "<<get_kminus<<", get_kminuscher: "<<get_kminuscher<<std::endl;
    std::cout <<"get_fsl_vtx: "<<get_fsl_vtx<<", get_fsl_momentum: "<<get_fsl_momentum<<", get_fsl_time: "<<get_fsl_time<<", get_fsl_mass: "<<get_fsl_mass<<", get_fsl_pdg: "<<get_fsl_pdg<<", get_fsl_energy: "<<get_fsl_energy<<std::endl;
    if (get_neutrino_energy && get_neutrino_mom && get_neutrino_vtxx && get_neutrino_vtxy && get_neutrino_vtxz && get_neutrino_vtxt && get_q2 && get_cc && get_nc && get_qel && get_res && get_dis && get_coh && get_mec && get_n && get_p && get_pi0 && get_piplus && get_pipluscher && get_piminus && get_piminuscher && get_kplus && get_kpluscher && get_kminus && get_kminuscher && get_fsl_vtx && get_fsl_momentum && get_fsl_time && get_fsl_mass && get_fsl_pdg && get_fsl_energy && get_bjx && get_y && get_targetZ && get_q0 && get_q3 && get_w ){
      fTrueNeutrinoEnergy = TrueNeutrinoEnergy;
      fTrueNuPDG = TrueNuPDG;
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
      fTrueW2 = TrueW2;
      fTrueBJx = TrueBJx;
      fTruey = Truey;
      fTrueTarget = TrueTarget;
      fTrueq0 = Trueq0;
      fTrueq3 = Trueq3;
      fTrueCC = (TrueCC)? 1 : 0;
      fTrueNC = (TrueNC)? 1 : 0;
      fTrueQEL = (TrueQEL)? 1 : 0;
      fTrueRES = (TrueRES)? 1 : 0;
      fTrueDIS = (TrueDIS)? 1 : 0;
      fTrueCOH = (TrueCOH)? 1 : 0;
      fTrueMEC = (TrueMEC)? 1 : 0;
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
    } else {
      Log("PhaseIITreeMaker tool: Did not find GENIE information. Continuing building remaining tree",v_message,verbosity);
      successful_load = false;
    }
  } // end if hasGenie

  return successful_load;
}

void PhaseIITreeMaker::FillWeightInfo() {
  bool get_xsec_weights = m_data->Stores.at("ANNIEEvent")->Get("xsec_weights",fxsec_weights);
  bool get_flux_weights = m_data->Stores.at("ANNIEEvent")->Get("flux_weights",fflux_weights);
  if (get_xsec_weights && get_flux_weights){
    fAll0 = fxsec_weights["All0"];
    fAll1 = fxsec_weights["All1"];
    fAll2 = fxsec_weights["All2"];
    fAll3 = fxsec_weights["All3"];
    fAll4 = fxsec_weights["All4"];
    fAll5 = fxsec_weights["All5"];
    fAxFFCCQEshape = fxsec_weights["AxFFCCQEshape"];
    fDecayAngMEC = fxsec_weights["DecayAngMEC"];
    fNormCCCOH = fxsec_weights["NormCCCOH"];
    fNorm_NCCOH = fxsec_weights["Norm_NCCOH"];
    fRPA_CCQE = fxsec_weights["RPA_CCQE"];
    fRootinoFix = fxsec_weights["RootinoFix"];
    fThetaDelta2NRad = fxsec_weights["ThetaDelta2NRad"];
    fTheta_Delta2Npi = fxsec_weights["Theta_Delta2Npi"];
    fTunedCentralValue = fxsec_weights["TunedCentralValue"];
    fVecFFCCQEshape = fxsec_weights["VecFFCCQEshape"];
    fXSecShape_CCMEC = fxsec_weights["XSecShape_CCMEC"];
    fhorncurrent = fflux_weights["horncurrent_FluxUnisim"];
    fexpskin = fflux_weights["expskin_FluxUnisim"];
    fpiplus = fflux_weights["piplus_PrimaryHadronSWCentralSplineVariation"];
    fpiminus = fflux_weights["piminus_PrimaryHadronSWCentralSplineVariation"];
    fkplus = fflux_weights["kplus_PrimaryHadronFeynmanScaling"];
    fkzero = fflux_weights["kzero_PrimaryHadronSanfordWang"];
    fkminus = fflux_weights["kminus_PrimaryHadronNormalization"];
    fpioninexsec = fflux_weights["pioninexsec_FluxUnisim"];
    fpionqexsec = fflux_weights["pionqexsec_FluxUnisim"];
    fpiontotxsec = fflux_weights["piontotxsec_FluxUnisim"];
    fnucleoninexsec = fflux_weights["nucleoninexsec_FluxUnisim"];
    fnucleonqexsec = fflux_weights["nucleonqexsec_FluxUnisim"];
    fnucleontotxsec = fflux_weights["nucleontotxsec_FluxUnisim"];
  }
}

void PhaseIITreeMaker::FillTruthRecoDiffInfo(bool successful_mcload,bool successful_recoload) {
  if (!successful_mcload || !successful_recoload) {
    Log("PhaseIITreeMaker Tool: Error loading True Muon Vertex or Extended Vertex information.  Continuing to build remaining tree",v_message,verbosity);
  } else {
    //Make sure MCTruth Information is loaded from store
    //Let's fill in stuff from the RecoSummary
    fDeltaVtxX = fRecoVtxX - fTrueVtxX;
    fDeltaVtxY = fRecoVtxY - fTrueVtxY;
    fDeltaVtxZ = fRecoVtxZ - fTrueVtxZ;
    fDeltaVtxT = fRecoVtxTime - fTrueVtxTime;
    fDeltaVtxR = sqrt(pow(fDeltaVtxX,2) + pow(fDeltaVtxY,2) + pow(fDeltaVtxZ,2)); 
    fDeltaParallel = fDeltaVtxX*fRecoDirX + fDeltaVtxY*fRecoDirY + fDeltaVtxZ*fRecoDirZ;
    fDeltaPerpendicular = sqrt(pow(fDeltaVtxR,2) - pow(fDeltaParallel,2));
    fDeltaAzimuth = (fRecoAngle - fTrueAngle)/(TMath::Pi()/180.0);
    fDeltaZenith = (fRecoPhi - fTruePhi)/(TMath::Pi()/180.0); 
    double cosphi = fTrueDirX*fRecoDirX+fTrueDirY*fRecoDirY+fTrueDirZ*fRecoDirZ;
    double phi = TMath::ACos(cosphi); // radians
    double TheAngle = phi/(TMath::Pi()/180.0); // radians->degrees
    fDeltaAngle = TheAngle;
  }
}

void PhaseIITreeMaker::RecoSummary() {

  // get reconstruction output
  double dx = fRecoVtxX - fTrueVtxX;
  double dy = fRecoVtxY - fTrueVtxY;
  double dz = fRecoVtxZ - fTrueVtxZ;
  double dt = fRecoVtxTime - fTrueVtxTime;
  double deltaR = sqrt(dx*dx + dy*dy + dz*dz);
  double cosphi = 0., phi = 0., DeltaAngle = 0.;
  cosphi = fTrueDirX*fRecoDirX+fTrueDirY*fRecoDirY+fTrueDirZ*fRecoDirZ;
  phi = TMath::ACos(cosphi); // radians
  DeltaAngle = phi/(TMath::Pi()/180.0); // radians->degrees
  std::cout << "============================================================================"<<std::endl;
  std::cout << " Event number " << fEventNumber << std::endl;
  std::cout << "  trueVtx=(" << fTrueVtxX << ", " << fTrueVtxY << ", " << fTrueVtxZ <<", "<< fTrueVtxTime<< std::endl
            << " TrueMuonEnergy= " << fTrueMuonEnergy << " Primary Pdg = " << fTruePrimaryPdg << std::endl
            << "           " << fTrueDirX << ", " << fTrueDirY << ", " << fTrueDirZ << ") " << std::endl;
  std::cout << "  recoVtx=(" << fRecoVtxX << ", " << fRecoVtxY << ", " << fRecoVtxZ <<", "<< fRecoVtxTime << std::endl
            << "           " << fRecoDirX << ", " << fRecoDirY << ", " << fRecoDirZ << ") " << std::endl;
  std::cout << "  DeltaR = "<<deltaR<<"[cm]"<<"\t"<<"  DeltaAngle = "<<DeltaAngle<<" [degree]"<<std::endl;
  std::cout << "  FOM = " << fRecoVtxFOM << std::endl;
  std::cout << "  RecoStatus = " << fRecoStatus <<std::endl;
  std::cout << std::endl;
}

void PhaseIITreeMaker::FillLAPPDData(){

for (std::map<uint64_t, PsecData>::iterator it = LAPPDDataMap.begin(); it != LAPPDDataMap.end(); ++it) {
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
  }

  //cout<<"Grouped Trigger Size: "<<GroupedTrigger.size()<<endl;
  for(std::map<uint64_t, uint32_t>::iterator it = GroupedTrigger.begin(); it != GroupedTrigger.end(); ++it) {
    //cout<<"Grouped Trigger: "<<it->first<<" "<<it->second<<endl;
    uint64_t key = it->first;
    uint32_t value = it->second;

    fGroupedTriggerTime.push_back(key);
    fGroupedTriggerWord.push_back(value);
  }

}

void PhaseIITreeMaker::LoadLAPPDData()
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
  
  
  m_data->Stores["ANNIEEvent"]->Get("GroupedTrigger", GroupedTrigger);
  if(LAPPDDataMap.size() != 0)
    {
      FillLAPPDData();
      //print the content of fDataStreams, and the size of data map
      //cout<<"Found LAPPDData, LAPPDDataMap Size: "<<LAPPDDataMap.size()<<endl;
      if(LAPPDReco_fill){
      FillLAPPDPulse();
      FillLAPPDHit();
      }
    }
}

void PhaseIITreeMaker::FillLAPPDPulse()
{
  bool gotPulse = m_data->Stores["ANNIEEvent"]->Get("LAPPDPulses", lappdPulses);
  if(gotPulse)
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
        fPulseCharge.push_back(thisPulse.GetCharge());
        fPulsePeakAmp.push_back(thisPulse.GetPeak());
        fPulseStart.push_back(thisPulse.GetLowRange());
        fPulseEnd.push_back(thisPulse.GetHiRange());
        fPulseWidth.push_back(thisPulse.GetHiRange() - thisPulse.GetLowRange());
      }
      for(int i = 0; i<pulse1.size();i++)
      {
        fPulseSide.push_back(1);
        LAPPDPulse thisPulse = pulse1.at(i);
        fLAPPD_IDs.push_back(thisPulse.GetTubeId());
        fChannelID.push_back(thisPulse.GetChannelID());
        fPulseStripNum.push_back(stripno);
        fPulsePeakTime.push_back(thisPulse.GetTime());
        fPulseCharge.push_back(thisPulse.GetCharge());
        fPulsePeakAmp.push_back(thisPulse.GetPeak());
        fPulseStart.push_back(thisPulse.GetLowRange());
        fPulseEnd.push_back(thisPulse.GetHiRange());
        fPulseWidth.push_back(thisPulse.GetHiRange() - thisPulse.GetLowRange());
      }

    }

  }

}


void PhaseIITreeMaker::FillLAPPDHit(){
  bool gotHit = m_data->Stores["ANNIEEvent"]->Get("LAPPDHits", lappdHits);

  if(gotHit)
  {
  std::map<unsigned long, vector<LAPPDHit>>::iterator it;
  for (it = lappdHits.begin(); it != lappdHits.end(); it++)
  {
    int stripno = it->first;
    vector<LAPPDHit> stripHits = it->second;
    for (int i = 0; i < stripHits.size(); i++)
    {
      LAPPDHit thisHit = stripHits.at(i);
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
      fLAPPDHitP1StartTime.push_back(thisHit.GetPulse1StartTime());
      fLAPPDHitP2StartTime.push_back(thisHit.GetPulse2StartTime());
      fLAPPDHitP1EndTime.push_back(thisHit.GetPulse1LastTime());
      fLAPPDHitP2EndTime.push_back(thisHit.GetPulse2LastTime());
    }
  }
  }
}

// ============================================================================
// cMrdMlReco support (MRDRecoSource = MrdMLReco or Both)
// ============================================================================

void PhaseIITreeMaker::ResetMrdMLReco(){
  fMrdMlFailureCode = -1;
  fMrdMlRecoSkipReason = 0;
  fMrdMlGatePassed = false;
  fMrdMlReconstructionAvailable = false;
  fMrdMlMRDInputSource.clear();
  fMrdMlNumberMrdTimeClusters = -1;
  fMrdMlSelectedMrdClusterIndex = -1;
  fMrdMlSelectedTankClusterIndex = -1;
  fMrdMlInputTankHits = -1;
  fMrdMlRecoUsedTankBypass = false;
  fMrdMlTankMrdAssociationFound = false;
  fMrdMlTankMrdSelectedTankClusterIndex = -1;
  fMrdMlTankMrdSelectedMrdSubEventID = -1;
  fMrdMlTankMrdTimingResidualNs = -999.0;
  fMrdMlHasTargetMuonProbability = -1.0;
  fMrdMlReconstructableProbability = -1.0;
  fMrdMlEventCategory = -1;
  fMrdMlTopologyClass = -1;
  fMrdMlTopologyProbabilities.clear();
  fMrdMlStoppedProbability = -1.0;
  fMrdMlExitFaceClass = -1;
  fMrdMlMRDTrackLengthCm = -999.0;
  fMrdMlMRDTrackLengthSigmaCm = -999.0;
  fMrdMlCosTheta = -999.0;
  fMrdMlCosThetaSigma = -999.0;
  fMrdMlThetaRad = -999.0;
  fMrdMlThetaSigmaRad = -999.0;
  fMrdMlThetaSigmaValid = false;
  fMrdMlPhiRad = -999.0;
  fMrdMlPenetrationCm = -999.0;
  fMrdMlPenetrationSigmaCm = -999.0;
  fMrdMlMRDEnergyLossMeV = -999.0;
  fMrdMlMRDEnergyLossSigmaMeV = -999.0;
  fMrdMlMuonKEMeV = -999.0;
  fMrdMlMuonKESigmaMeV = -999.0;
  fMrdMlCherenkovAngleRad = -999.0;
  fMrdMlCherenkovAngleSigmaRad = -999.0;
  fMrdMlTankVertexCm.clear();
  fMrdMlTankVertexSigmaCm.clear();
  fMrdMlTankExitCm.clear();
  fMrdMlTankExitSigmaCm.clear();
  fMrdMlMRDEntryCm.clear();
  fMrdMlMRDEntrySigmaCm.clear();
  fMrdMlMRDExitCm.clear();
  fMrdMlMRDExitSigmaCm.clear();
  fMrdMlSelectedEndpointCm.clear();
  fMrdMlSelectedEndpointSigmaCm.clear();
  fMrdMlMRDScatterRad.clear();
  fMrdMlMRDScatterSigmaRad.clear();
  fMrdMlTankScatterRad.clear();
  fMrdMlTankScatterSigmaRad.clear();
  fMrdMlSchemaVersion = -1;
  fMrdMlOutputMode.clear();
  fMrdMlModelRelease.clear();
  fMrdMlProtocolVersion = -1;
  fMrdMlStatusText.clear();
  fMrdMlErrorText.clear();
  fMrdMlInputBuildOK = false;
  fMrdMlGateAttempted = false;
  fMrdMlGateInferenceOK = false;
  fMrdMlRecoAttempted = false;
  fMrdMlRecoInferenceOK = false;
  fMrdMlDaemonRestartCount = 0;
  fMrdMlSelectedMrdClusterStartTimeNs = -999.0;
  fMrdMlSelectedMrdClusterEndTimeNs = -999.0;
  fMrdMlSelectedMrdClusterTimeSpanNs = -999.0;
  fMrdMlInputMRDRawHits = -1;
  fMrdMlInputMRDSelectedRawHits = -1;
  fMrdMlInputMRDGeometryValidHitsBeforeDeadMask = -1;
  fMrdMlInputMRDKnownDeadRemoved = -1;
  fMrdMlInputMRDLiveHitsBeforeDedup = -1;
  fMrdMlInputMRDLiveUniqueHits = -1;
  fMrdMlInputMRDDedupRemoved = -1;
  fMrdMlInputMRDUnassignedHits = -1;
  fMrdMlInputMRDLayers = -1;
  fMrdMlInputMRDFirstLayer = -1;
  fMrdMlInputMRDLastLayer = -1;
  fMrdMlTankInputSource.clear();
  fMrdMlNumberTankClusters = -1;
  fMrdMlSelectedTankClusterTimeNs = -999.0;
  fMrdMlSelectedTankClusterChargePE = -999.0;
  fMrdMlSelectedTankClusterTimeSpanNs = -999.0;
  fMrdMlTankPacketReorderedByTime = false;
  fMrdMlAdditionalDeadMRDChankeys.clear();
  fMrdMlRecoStructurallyEligible = false;
  fMrdMlPhysicsIsRawUnstandardized = false;
  fMrdMlTankMrdAssociationApplied = false;
  fMrdMlTankMrdCompatiblePairCount = 0;
  fMrdMlTankMrdExpectedOffsetNs = -999.0;
  fMrdMlTankMrdCoincidenceWindowNs = -999.0;
  fMrdMlTankMrdTankMeanTimeNs = -999.0;
  fMrdMlTankMrdMrdMeanTimeNs = -999.0;
  fMrdMlTankMrdDtNs = -999.0;
  fMrdMlEventCategoryConfidence = -1.0;
  fMrdMlEventCategoryMargin = -1.0;
  fMrdMlEventCategoryProbabilities.clear();
  fMrdMlSingleMuonCompatibilityProbability = -1.0;
  fMrdMlSingleMuonCompatibilityValidated = false;
  fMrdMlTopologyConfidence = -1.0;
  fMrdMlTopologyMargin = -1.0;
  fMrdMlTopologyPStopped = -1.0;
  fMrdMlTopologyPBackExit = -1.0;
  fMrdMlTopologyPSideExit = -1.0;
  fMrdMlExitFaceConfidence = -1.0;
  fMrdMlExitFaceMargin = -1.0;
  fMrdMlExitFaceProbabilities.clear();
  fMrdMlEndpointExpertUsed = -1;
  fMrdMlSinPhi = -999.0;
  fMrdMlCosPhi = -999.0;
  fMrdMlPhiVectorNorm = -999.0;
  fMrdMlStopEndpointCm.clear();
  fMrdMlStopEndpointSigmaCm.clear();
  fMrdMlBackExitEndpointCm.clear();
  fMrdMlBackExitEndpointSigmaCm.clear();
  fMrdMlSideExitEndpointCm.clear();
  fMrdMlSideExitEndpointSigmaCm.clear();
  fMrdMlQualityBaseline = false;
  fMrdMlQualityTopology = false;
  fMrdMlQualityDirection = false;
  fMrdMlQualityEndpoint = false;
  fMrdMlQualityTier = 0;
}

// Reads Stores["MrdMLReco"] for the current trigger.  Every key of the active
// branch set is required: cMrdMlReco writes all of them for every event, so a
// missing key means a mismatched producer and stops the ToolChain.
bool PhaseIITreeMaker::LoadMrdMLReco(){
  ResetMrdMLReco();
  auto store_it = m_data->Stores.find("MrdMLReco");
  if(store_it == m_data->Stores.end() || store_it->second == nullptr){
    Log("PhaseIITreeMaker Tool: Stores[\"MrdMLReco\"] is missing.", v_error, verbosity);
    return false;
  }
  BoostStore* store = store_it->second;

#define MRDML_GET(KEY, VARIABLE) \
  do { \
    if(!store->Get(KEY, VARIABLE)){ \
      Log(std::string("PhaseIITreeMaker Tool: missing MrdMLReco key ") + KEY, \
          v_error, verbosity); \
      return false; \
    } \
  } while(0)

  MRDML_GET("FailureCode", fMrdMlFailureCode);
  MRDML_GET("RecoSkipReason", fMrdMlRecoSkipReason);
  MRDML_GET("GatePassed", fMrdMlGatePassed);
  MRDML_GET("ReconstructionAvailable", fMrdMlReconstructionAvailable);
  MRDML_GET("MRDInputSource", fMrdMlMRDInputSource);
  MRDML_GET("NumberMrdTimeClusters", fMrdMlNumberMrdTimeClusters);
  MRDML_GET("SelectedMrdClusterIndex", fMrdMlSelectedMrdClusterIndex);
  MRDML_GET("SelectedTankClusterIndex", fMrdMlSelectedTankClusterIndex);
  MRDML_GET("InputTankHits", fMrdMlInputTankHits);
  MRDML_GET("RecoUsedTankBypass", fMrdMlRecoUsedTankBypass);
  MRDML_GET("TankMrdAssociationFound", fMrdMlTankMrdAssociationFound);
  MRDML_GET("TankMrdSelectedTankClusterIndex", fMrdMlTankMrdSelectedTankClusterIndex);
  MRDML_GET("TankMrdSelectedMrdSubEventID", fMrdMlTankMrdSelectedMrdSubEventID);
  MRDML_GET("TankMrdTimingResidualNs", fMrdMlTankMrdTimingResidualNs);
  MRDML_GET("HasTargetMuonProbability", fMrdMlHasTargetMuonProbability);
  MRDML_GET("ReconstructableProbability", fMrdMlReconstructableProbability);
  MRDML_GET("EventCategory", fMrdMlEventCategory);
  MRDML_GET("TopologyClass", fMrdMlTopologyClass);
  MRDML_GET("TopologyProbabilities", fMrdMlTopologyProbabilities);
  MRDML_GET("StoppedProbability", fMrdMlStoppedProbability);
  MRDML_GET("ExitFaceClass", fMrdMlExitFaceClass);
  MRDML_GET("MRDTrackLengthCm", fMrdMlMRDTrackLengthCm);
  MRDML_GET("MRDTrackLengthSigmaCm", fMrdMlMRDTrackLengthSigmaCm);
  MRDML_GET("CosTheta", fMrdMlCosTheta);
  MRDML_GET("CosThetaSigma", fMrdMlCosThetaSigma);
  MRDML_GET("ThetaRad", fMrdMlThetaRad);
  MRDML_GET("ThetaSigmaRad", fMrdMlThetaSigmaRad);
  MRDML_GET("ThetaSigmaValid", fMrdMlThetaSigmaValid);
  MRDML_GET("PhiRad", fMrdMlPhiRad);
  MRDML_GET("PenetrationCm", fMrdMlPenetrationCm);
  MRDML_GET("PenetrationSigmaCm", fMrdMlPenetrationSigmaCm);
  MRDML_GET("MRDEnergyLossMeV", fMrdMlMRDEnergyLossMeV);
  MRDML_GET("MRDEnergyLossSigmaMeV", fMrdMlMRDEnergyLossSigmaMeV);
  MRDML_GET("MuonKEMeV", fMrdMlMuonKEMeV);
  MRDML_GET("MuonKESigmaMeV", fMrdMlMuonKESigmaMeV);
  MRDML_GET("CherenkovAngleRad", fMrdMlCherenkovAngleRad);
  MRDML_GET("CherenkovAngleSigmaRad", fMrdMlCherenkovAngleSigmaRad);
  MRDML_GET("TankVertexCm", fMrdMlTankVertexCm);
  MRDML_GET("TankVertexSigmaCm", fMrdMlTankVertexSigmaCm);
  MRDML_GET("TankExitCm", fMrdMlTankExitCm);
  MRDML_GET("TankExitSigmaCm", fMrdMlTankExitSigmaCm);
  MRDML_GET("MRDEntryCm", fMrdMlMRDEntryCm);
  MRDML_GET("MRDEntrySigmaCm", fMrdMlMRDEntrySigmaCm);
  MRDML_GET("MRDExitCm", fMrdMlMRDExitCm);
  MRDML_GET("MRDExitSigmaCm", fMrdMlMRDExitSigmaCm);
  MRDML_GET("SelectedEndpointCm", fMrdMlSelectedEndpointCm);
  MRDML_GET("SelectedEndpointSigmaCm", fMrdMlSelectedEndpointSigmaCm);
  MRDML_GET("MRDScatterRad", fMrdMlMRDScatterRad);
  MRDML_GET("MRDScatterSigmaRad", fMrdMlMRDScatterSigmaRad);
  MRDML_GET("TankScatterRad", fMrdMlTankScatterRad);
  MRDML_GET("TankScatterSigmaRad", fMrdMlTankScatterSigmaRad);
  MRDML_GET("SchemaVersion", fMrdMlSchemaVersion);
  MRDML_GET("OutputMode", fMrdMlOutputMode);
  MRDML_GET("ModelRelease", fMrdMlModelRelease);
  if(MrdMLDebug_fill){
    MRDML_GET("ProtocolVersion", fMrdMlProtocolVersion);
    MRDML_GET("StatusText", fMrdMlStatusText);
    MRDML_GET("ErrorText", fMrdMlErrorText);
    MRDML_GET("InputBuildOK", fMrdMlInputBuildOK);
    MRDML_GET("GateAttempted", fMrdMlGateAttempted);
    MRDML_GET("GateInferenceOK", fMrdMlGateInferenceOK);
    MRDML_GET("RecoAttempted", fMrdMlRecoAttempted);
    MRDML_GET("RecoInferenceOK", fMrdMlRecoInferenceOK);
    MRDML_GET("DaemonRestartCount", fMrdMlDaemonRestartCount);
    MRDML_GET("SelectedMrdClusterStartTimeNs", fMrdMlSelectedMrdClusterStartTimeNs);
    MRDML_GET("SelectedMrdClusterEndTimeNs", fMrdMlSelectedMrdClusterEndTimeNs);
    MRDML_GET("SelectedMrdClusterTimeSpanNs", fMrdMlSelectedMrdClusterTimeSpanNs);
    MRDML_GET("InputMRDRawHits", fMrdMlInputMRDRawHits);
    MRDML_GET("InputMRDSelectedRawHits", fMrdMlInputMRDSelectedRawHits);
    MRDML_GET("InputMRDGeometryValidHitsBeforeDeadMask", fMrdMlInputMRDGeometryValidHitsBeforeDeadMask);
    MRDML_GET("InputMRDKnownDeadRemoved", fMrdMlInputMRDKnownDeadRemoved);
    MRDML_GET("InputMRDLiveHitsBeforeDedup", fMrdMlInputMRDLiveHitsBeforeDedup);
    MRDML_GET("InputMRDLiveUniqueHits", fMrdMlInputMRDLiveUniqueHits);
    MRDML_GET("InputMRDDedupRemoved", fMrdMlInputMRDDedupRemoved);
    MRDML_GET("InputMRDUnassignedHits", fMrdMlInputMRDUnassignedHits);
    MRDML_GET("InputMRDLayers", fMrdMlInputMRDLayers);
    MRDML_GET("InputMRDFirstLayer", fMrdMlInputMRDFirstLayer);
    MRDML_GET("InputMRDLastLayer", fMrdMlInputMRDLastLayer);
    MRDML_GET("TankInputSource", fMrdMlTankInputSource);
    MRDML_GET("NumberTankClusters", fMrdMlNumberTankClusters);
    MRDML_GET("SelectedTankClusterTimeNs", fMrdMlSelectedTankClusterTimeNs);
    MRDML_GET("SelectedTankClusterChargePE", fMrdMlSelectedTankClusterChargePE);
    MRDML_GET("SelectedTankClusterTimeSpanNs", fMrdMlSelectedTankClusterTimeSpanNs);
    MRDML_GET("TankPacketReorderedByTime", fMrdMlTankPacketReorderedByTime);
    MRDML_GET("AdditionalDeadMRDChankeys", fMrdMlAdditionalDeadMRDChankeys);
    MRDML_GET("RecoStructurallyEligible", fMrdMlRecoStructurallyEligible);
    MRDML_GET("PhysicsIsRawUnstandardized", fMrdMlPhysicsIsRawUnstandardized);
    MRDML_GET("TankMrdAssociationApplied", fMrdMlTankMrdAssociationApplied);
    MRDML_GET("TankMrdCompatiblePairCount", fMrdMlTankMrdCompatiblePairCount);
    MRDML_GET("TankMrdExpectedOffsetNs", fMrdMlTankMrdExpectedOffsetNs);
    MRDML_GET("TankMrdCoincidenceWindowNs", fMrdMlTankMrdCoincidenceWindowNs);
    MRDML_GET("TankMrdTankMeanTimeNs", fMrdMlTankMrdTankMeanTimeNs);
    MRDML_GET("TankMrdMrdMeanTimeNs", fMrdMlTankMrdMrdMeanTimeNs);
    MRDML_GET("TankMrdDtNs", fMrdMlTankMrdDtNs);
    MRDML_GET("EventCategoryConfidence", fMrdMlEventCategoryConfidence);
    MRDML_GET("EventCategoryMargin", fMrdMlEventCategoryMargin);
    MRDML_GET("EventCategoryProbabilities", fMrdMlEventCategoryProbabilities);
    MRDML_GET("SingleMuonCompatibilityProbability", fMrdMlSingleMuonCompatibilityProbability);
    MRDML_GET("SingleMuonCompatibilityValidated", fMrdMlSingleMuonCompatibilityValidated);
    MRDML_GET("TopologyConfidence", fMrdMlTopologyConfidence);
    MRDML_GET("TopologyMargin", fMrdMlTopologyMargin);
    MRDML_GET("TopologyPStopped", fMrdMlTopologyPStopped);
    MRDML_GET("TopologyPBackExit", fMrdMlTopologyPBackExit);
    MRDML_GET("TopologyPSideExit", fMrdMlTopologyPSideExit);
    MRDML_GET("ExitFaceConfidence", fMrdMlExitFaceConfidence);
    MRDML_GET("ExitFaceMargin", fMrdMlExitFaceMargin);
    MRDML_GET("ExitFaceProbabilities", fMrdMlExitFaceProbabilities);
    MRDML_GET("EndpointExpertUsed", fMrdMlEndpointExpertUsed);
    MRDML_GET("SinPhi", fMrdMlSinPhi);
    MRDML_GET("CosPhi", fMrdMlCosPhi);
    MRDML_GET("PhiVectorNorm", fMrdMlPhiVectorNorm);
    MRDML_GET("StopEndpointCm", fMrdMlStopEndpointCm);
    MRDML_GET("StopEndpointSigmaCm", fMrdMlStopEndpointSigmaCm);
    MRDML_GET("BackExitEndpointCm", fMrdMlBackExitEndpointCm);
    MRDML_GET("BackExitEndpointSigmaCm", fMrdMlBackExitEndpointSigmaCm);
    MRDML_GET("SideExitEndpointCm", fMrdMlSideExitEndpointCm);
    MRDML_GET("SideExitEndpointSigmaCm", fMrdMlSideExitEndpointSigmaCm);
    MRDML_GET("QualityBaseline", fMrdMlQualityBaseline);
    MRDML_GET("QualityTopology", fMrdMlQualityTopology);
    MRDML_GET("QualityDirection", fMrdMlQualityDirection);
    MRDML_GET("QualityEndpoint", fMrdMlQualityEndpoint);
    MRDML_GET("QualityTier", fMrdMlQualityTier);
  }

#undef MRDML_GET
  return true;
}

// One-entry tree "mrd_ml_metadata": model / geometry / runtime identity and
// the event-selection policy of the cMrdMlReco job, so the file is
// self-describing without repeating 64-character hashes on every entry.
bool PhaseIITreeMaker::WriteMrdMLMetadata(){
  auto store_it = m_data->Stores.find("MrdMLReco");
  if(store_it == m_data->Stores.end() || store_it->second == nullptr){
    Log("PhaseIITreeMaker Tool: cannot write mrd_ml_metadata, Stores[\"MrdMLReco\"] missing.",
        v_error, verbosity);
    return false;
  }
  BoostStore* store = store_it->second;
  bool complete = true;
  complete = store->Get("SchemaVersion", fMrdMlMetaSchemaVersion) && complete;
  complete = store->Get("OutputMode", fMrdMlMetaOutputMode) && complete;
  complete = store->Get("AdapterVersion", fMrdMlMetaAdapterVersion) && complete;
  complete = store->Get("ModelRelease", fMrdMlMetaModelRelease) && complete;
  complete = store->Get("DaemonVersion", fMrdMlMetaDaemonVersion) && complete;
  complete = store->Get("RuntimeFeatureVersion", fMrdMlMetaRuntimeFeatureVersion) && complete;
  complete = store->Get("TensorFlowVersion", fMrdMlMetaTensorFlowVersion) && complete;
  complete = store->Get("GateModelName", fMrdMlMetaGateModelName) && complete;
  complete = store->Get("ReconstructionModelName", fMrdMlMetaReconstructionModelName) && complete;
  complete = store->Get("GateModelTreeSHA256", fMrdMlMetaGateModelTreeSHA256) && complete;
  complete = store->Get("ReconstructionModelTreeSHA256", fMrdMlMetaReconstructionModelTreeSHA256) && complete;
  complete = store->Get("GateTrainerVersion", fMrdMlMetaGateTrainerVersion) && complete;
  complete = store->Get("ReconstructionTrainerVersion", fMrdMlMetaReconstructionTrainerVersion) && complete;
  complete = store->Get("GeometryVersion", fMrdMlMetaGeometryVersion) && complete;
  complete = store->Get("GeometryManifestSHA256", fMrdMlMetaGeometryManifestSHA256) && complete;
  complete = store->Get("GateHasTargetThreshold", fMrdMlMetaGateHasTargetThreshold) && complete;
  complete = store->Get("GateReconstructableThreshold", fMrdMlMetaGateReconstructableThreshold) && complete;
  complete = store->Get("RequireGateCategoryZero", fMrdMlMetaRequireGateCategoryZero) && complete;
  complete = store->Get("PrefilterMinMRDHits", fMrdMlMetaPrefilterMinMRDHits) && complete;
  if(!complete) Log("PhaseIITreeMaker Tool: mrd_ml_metadata is incomplete (missing "
                    "cMrdMlReco job-level keys); Finalise will fail.", v_error, verbosity);

  TTree* meta = new TTree("mrd_ml_metadata", "cMrdMlReco job identity (one entry)");
  meta->Branch("mrd_reco_source", &MRDRecoSource);
  meta->Branch("branch_set", &MrdMLOutputMode);
  meta->Branch("schema_version", &fMrdMlMetaSchemaVersion, "schema_version/I");
  meta->Branch("output_mode", &fMrdMlMetaOutputMode);
  meta->Branch("adapter_version", &fMrdMlMetaAdapterVersion);
  meta->Branch("model_release", &fMrdMlMetaModelRelease);
  meta->Branch("daemon_version", &fMrdMlMetaDaemonVersion);
  meta->Branch("runtime_feature_version", &fMrdMlMetaRuntimeFeatureVersion);
  meta->Branch("tensorflow_version", &fMrdMlMetaTensorFlowVersion);
  meta->Branch("gate_model_name", &fMrdMlMetaGateModelName);
  meta->Branch("reconstruction_model_name", &fMrdMlMetaReconstructionModelName);
  meta->Branch("gate_model_tree_sha256", &fMrdMlMetaGateModelTreeSHA256);
  meta->Branch("reconstruction_model_tree_sha256", &fMrdMlMetaReconstructionModelTreeSHA256);
  meta->Branch("gate_trainer_version", &fMrdMlMetaGateTrainerVersion);
  meta->Branch("reconstruction_trainer_version", &fMrdMlMetaReconstructionTrainerVersion);
  meta->Branch("geometry_version", &fMrdMlMetaGeometryVersion);
  meta->Branch("geometry_manifest_sha256", &fMrdMlMetaGeometryManifestSHA256);
  meta->Branch("gate_has_target_threshold", &fMrdMlMetaGateHasTargetThreshold, "gate_has_target_threshold/D");
  meta->Branch("gate_reconstructable_threshold", &fMrdMlMetaGateReconstructableThreshold, "gate_reconstructable_threshold/D");
  meta->Branch("require_gate_category_zero", &fMrdMlMetaRequireGateCategoryZero, "require_gate_category_zero/O");
  meta->Branch("prefilter_min_mrd_hits", &fMrdMlMetaPrefilterMinMRDHits, "prefilter_min_mrd_hits/I");
  meta->Fill();
  meta->Write();
  return complete;
}
