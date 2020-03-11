#include "PhaseIITreeMaker.h"

PhaseIITreeMaker::PhaseIITreeMaker():Tool(){}


bool PhaseIITreeMaker::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  
  m_variables.Get("verbose", verbosity);
  m_variables.Get("HitInfo_fill", HitInfo_fill);
  m_variables.Get("fillCleanEventsOnly", fillCleanEventsOnly);
  m_variables.Get("MCTruth_fill", MCTruth_fill);
  m_variables.Get("Reco_fill", Reco_fill);
  m_variables.Get("RecoDebug_fill", RecoDebug_fill);
  m_variables.Get("muonTruthRecoDiff_fill", muonTruthRecoDiff_fill);

  m_variables.Get("SiPMPulseInfo_fill",SiPMPulseInfo_fill);
  m_variables.Get("ClusterProcessing",ClusterProcessing);
  m_variables.Get("TriggerProcessing",TriggerProcessing);

  std::string output_filename;
  m_variables.Get("OutputFile", output_filename);
  fOutput_tfile = new TFile(output_filename.c_str(), "recreate");
  fPhaseIITankClusterTree = new TTree("phaseIIClusterTree", "ANNIE Phase II Tank Cluster Tree");
  fPhaseIITrigTree = new TTree("phaseIITriggerTree", "ANNIE Phase II Ntuple Trigger Tree");

  m_data->CStore.Get("AuxChannelNumToTypeMap",AuxChannelNumToTypeMap);
  m_data->CStore.Get("ChannelNumToTankPMTSPEChargeMap",ChannelKeyToSPEMap);

  auto get_geometry= m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",geom);
  if(!get_geometry){
  	Log("DigitBuilder Tool: Error retrieving Geometry from ANNIEEvent!",v_error,verbosity); 
  	return false; 
  }
 
  if(ClusterProcessing){
     fPhaseIITankClusterTree->Branch("runNumber",&fRunNumber,"runNumber/I");
     fPhaseIITankClusterTree->Branch("subrunNumber",&fSubrunNumber,"subrunNumber/I");
     fPhaseIITankClusterTree->Branch("runType",&fRunType,"runType/I");
     fPhaseIITankClusterTree->Branch("startTime",&fStartTime,"startTime/l");

     //Some lower level information to save
     fPhaseIITankClusterTree->Branch("eventNumber",&fEventNumber,"eventNumber/I");
     fPhaseIITankClusterTree->Branch("eventTimeTank",&fEventTimeTank,"eventTimeTank/l");
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
  } 
  
  //SiPM Pulse Info; load into both trees for now...
  if(SiPMPulseInfo_fill){
    fPhaseIITankClusterTree->Branch("SiPMhitQ",&fSiPMHitQ);
    fPhaseIITankClusterTree->Branch("SiPMhitT",&fSiPMHitT);
    fPhaseIITankClusterTree->Branch("SiPMhitAmplitude",&fSiPMHitAmplitude);
    fPhaseIITankClusterTree->Branch("SiPMNum",&fSiPMNum);
  }

  if(TriggerProcessing){
    //Metadata for standard Events
    fPhaseIITrigTree->Branch("runNumber",&fRunNumber,"runNumber/I");
    fPhaseIITrigTree->Branch("subrunNumber",&fSubrunNumber,"subrunNumber/I");
    fPhaseIITrigTree->Branch("runType",&fRunType,"runType/I");
    fPhaseIITrigTree->Branch("startTime",&fStartTime,"startTime/I");

    //Some lower level information to save
    fPhaseIITrigTree->Branch("eventNumber",&fEventNumber,"eventNumber/l");
    fPhaseIITrigTree->Branch("eventTimeTank",&fEventTimeTank,"eventTimeTank/l");
    fPhaseIITrigTree->Branch("nhits",&fNHits,"nhits/I");

    //Event Staus Flag Information
    if(fillCleanEventsOnly){
      fPhaseIITrigTree->Branch("eventStatusApplied",&fEventStatusApplied,"eventStatusApplied/I");
      fPhaseIITrigTree->Branch("eventStatusFlagged",&fEventStatusFlagged,"eventStatusFlagged/I");
    }
    //Hit information (PMT and LAPPD)
    if(SiPMPulseInfo_fill){
      fPhaseIITrigTree->Branch("SiPMhitQ",&fSiPMHitQ);
      fPhaseIITrigTree->Branch("SiPMhitT",&fSiPMHitT);
      fPhaseIITrigTree->Branch("SiPMhitAmplitude",&fSiPMHitAmplitude);
      fPhaseIITrigTree->Branch("SiPMNum",&fSiPMNum);
    }

    if(HitInfo_fill){
      fPhaseIITrigTree->Branch("filter",&fIsFiltered);
      fPhaseIITrigTree->Branch("hitX",&fHitX);
      fPhaseIITrigTree->Branch("hitY",&fHitY);
      fPhaseIITrigTree->Branch("hitZ",&fHitZ);
      fPhaseIITrigTree->Branch("hitT",&fHitT);
      fPhaseIITrigTree->Branch("hitQ",&fHitQ);
      fPhaseIITrigTree->Branch("hitPE",&fHitPE);
      fPhaseIITrigTree->Branch("hitType", &fHitType);
      fPhaseIITrigTree->Branch("hitDetID", &fHitDetID);
    }

    //Reconstructed variables after full Muon Reco Analysis
    if(Reco_fill){
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
  
    //MC truth information for muons
    //Output to tree when MCTruth_fill = 1 in config
    if (MCTruth_fill){
      fPhaseIITrigTree->Branch("triggerNumber",&fMCTriggerNum,"triggerNumber/I");
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
      fPhaseIITrigTree->Branch("trueMuonEnergy",&fTrueMuonEnergy, "trueMuonEnergy/D");
      fPhaseIITrigTree->Branch("trueTrackLengthInWater",&fTrueTrackLengthInWater,"trueTrackLengthInWater/D");
      fPhaseIITrigTree->Branch("trueTrackLengthInMRD",&fTrueTrackLengthInMRD,"trueTrackLengthInMRD/D");
      fPhaseIITrigTree->Branch("Pi0Count",&fPi0Count,"Pi0Count/I");
      fPhaseIITrigTree->Branch("PiPlusCount",&fPiPlusCount,"PiPlusCount/I");
      fPhaseIITrigTree->Branch("PiMinusCount",&fPiMinusCount,"PiMinusCount/I");
      fPhaseIITrigTree->Branch("K0Count",&fK0Count,"K0Count/I");
      fPhaseIITrigTree->Branch("KPlusCount",&fKPlusCount,"KPlusCount/I");
      fPhaseIITrigTree->Branch("KMinusCount",&fKMinusCount,"KMinusCount/I");
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
  }
  return true;
}

bool PhaseIITreeMaker::Execute(){
  Log("===========================================================================================",v_debug,verbosity);
  Log("PhaseIITreeMaker Tool: Executing",v_debug,verbosity);


  // Reset variables
  this->ResetVariables();
  // Get a pointer to the ANNIEEvent Store

  //  If only clean events are built, return true for dirty events
  if(fillCleanEventsOnly){
    auto get_flagsapp = m_data->Stores.at("RecoEvent")->Get("EventFlagApplied",fEventStatusApplied);
    auto get_flags = m_data->Stores.at("RecoEvent")->Get("EventFlagged",fEventStatusFlagged); 
    if(!get_flagsapp || !get_flags) {
      Log("PhaseITreeMaker tool: No Event status applied or flagged bitmask!!", v_error, verbosity);
      return false;	
    }
    // check if event passes the cut
    if((fEventStatusFlagged) != 0) {
      Log("PhaseIITreeMaker Tool: Event was flagged with one of the active cuts.",v_debug, verbosity);
      return true;	
    }
  }



  if(ClusterProcessing){
    Log("PhaseIITreeMaker Tool: Beginning cluster processing",v_debug,verbosity);
    //bool get_clusters = m_data->Stores.at("ANNIEEvent")->Get("ClusterMap",m_all_clusters);
    bool get_clusters = m_data->CStore.Get("ClusterMap",m_all_clusters);
    if(!get_clusters){
      std::cout << "BeamClusterAnalysis tool: No clusters found!" << std::endl;
      return false;
    }
    Log("PhaseIITreeMaker Tool: Accessing pairs in all_clusters map",v_debug,verbosity);
    int cluster_num = 0;
    for (std::pair<double,std::vector<Hit>>&& cluster_pair : *m_all_clusters) {
      Log("PhaseIITreeMaker Tool: Resetting variables prior to getting run level info",v_debug,verbosity);
      this->ResetVariables();
      fClusterNumber = cluster_num;

      //Standard run level information
      Log("PhaseIITreeMaker Tool: Getting run level information from ANNIEEvent",v_debug,verbosity);
      m_data->Stores.at("ANNIEEvent")->Get("RunNumber",fRunNumber);
      m_data->Stores.at("ANNIEEvent")->Get("SubrunNumber",fSubrunNumber);
      m_data->Stores.at("ANNIEEvent")->Get("RunType",fRunType);
      m_data->Stores.at("ANNIEEvent")->Get("RunStartTime",fStartTime);
  
      // ANNIE Event number
      m_data->Stores.at("ANNIEEvent")->Get("EventTimeTank",fEventTimeTank);
      m_data->Stores.at("ANNIEEvent")->Get("EventNumber",fEventNumber);

      std::vector<Hit> cluster_hits = cluster_pair.second;
      fClusterTime = cluster_pair.first;
      if(HitInfo_fill){
        Log("PhaseIITreeMaker Tool: Loading tank cluster hits into cluster tree",v_debug,verbosity);
        this->LoadTankClusterHits(cluster_hits);
      }

      bool good_class = this->LoadTankClusterClassifiers(cluster_pair.first);
      if(!good_class){
        if(verbosity>3) Log("PhaseIITreeMaker Tool: No cluster classifiers.  Continuing tree",v_debug,verbosity);
      }
      if(SiPMPulseInfo_fill) this->LoadSiPMHits();
      fPhaseIITankClusterTree->Fill();
      cluster_num += 1;
    }

  }
  if(TriggerProcessing) {
    // ANNIE Event number
    m_data->Stores.at("ANNIEEvent")->Get("EventNumber",fEventNumber);

    // Read hits and load into ntuple
    if(HitInfo_fill){
      this->LoadAllTankHits();
    }
    if(SiPMPulseInfo_fill) this->LoadSiPMHits();

    bool got_reco = false;
    if(Reco_fill) got_reco = this->FillRecoInfo();

    bool gotmctruth = false;
    if(MCTruth_fill)  gotmctruth = this->FillMCTruthInfo();

    if (muonTruthRecoDiff_fill) this->FillTruthRecoDiffInfo(gotmctruth,got_reco);
    if (got_reco && gotmctruth && (verbosity>4)) this->RecoSummary();

    // FIll tree with all reconstruction information
    if (RecoDebug_fill) this->FillRecoDebugInfo();

    fPhaseIITrigTree->Fill();
  }
  return true;
}

bool PhaseIITreeMaker::Finalise(){
	fOutput_tfile->cd();
	fPhaseIITrigTree->Write();
	fPhaseIITankClusterTree->Write();
	fOutput_tfile->Close();
	if(verbosity>0) cout<<"PhaseIITreeMaker exitting"<<endl;
  return true;
}

void PhaseIITreeMaker::ResetVariables() {
  // tree variables
  fEventNumber = -9999;
  fNHits = -9999;

  if(SiPMPulseInfo_fill){
    fSiPMHitQ.clear();
    fSiPMHitT.clear();
    fSiPMHitAmplitude.clear();
    fSiPMNum.clear();
  }

  if(ClusterProcessing){
    fClusterMaxPE = -9999;
    fClusterChargePointX = -9999;
    fClusterChargePointY = -9999;
    fClusterChargePointZ = -9999;
    fClusterChargeBalance = -9999;
    fClusterTime = -9999;
    fClusterCharge = -9999;
    fClusterNumber = -9999; 
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

  if(Reco_fill){
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

  if(HitInfo_fill){
    fIsFiltered.clear();
    fHitX.clear();
    fHitY.clear();
    fHitZ.clear();
    fHitT.clear();
    fHitQ.clear();
    fHitPE.clear();
    fHitType.clear();
    fHitDetID.clear();
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
    Direction ClusterChargePoint = ClusterChargePoints.at(cluster_time);
    fClusterChargePointX = ClusterChargePoint.X();
    fClusterChargePointY = ClusterChargePoint.Y();
    fClusterChargePointZ = ClusterChargePoint.Z();
    fClusterChargeBalance = ClusterChargeBalances.at(cluster_time);
  }
  return good_classifiers;
}

void PhaseIITreeMaker::LoadTankClusterHits(std::vector<Hit> cluster_hits){
  Position detector_center=geom->GetTankCentre();
  double tank_center_x = detector_center.X();
  double tank_center_y = detector_center.Y();
  double tank_center_z = detector_center.Z();

  fClusterCharge = 0;
  fClusterPE = 0;
  fClusterHits = 0;
  for (int i = 0; i<cluster_hits.size(); i++){
    int channel_key = cluster_hits.at(i).GetTubeId();
    std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(channel_key);
    if(it != ChannelKeyToSPEMap.end()){ //Charge to SPE conversion is available
      Detector* this_detector = geom->ChannelToDetector(channel_key);
      Position det_position = this_detector->GetDetectorPosition();
      double hit_charge = cluster_hits.at(i).GetCharge();
      double hit_PE = hit_charge / ChannelKeyToSPEMap.at(channel_key);
      fHitX.push_back((det_position.X()-tank_center_x));
      fHitY.push_back((det_position.Y()-tank_center_y));
      fHitZ.push_back((det_position.Z()-tank_center_z));
      fHitQ.push_back(hit_charge);
      fHitPE.push_back(hit_PE);
      fHitT.push_back(cluster_hits.at(i).GetTime());
      fHitDetID.push_back(channel_key);
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


void PhaseIITreeMaker::LoadSiPMHits() {
  std::map<unsigned long, std::vector<CalibratedADCWaveform<double> > >
    calibrated_auxwaveform_map;
  std::map<unsigned long, std::vector< std::vector<ADCPulse>> > aux_pulse_map;
  m_data->Stores.at("ANNIEEvent")->Get("CalibratedADCAuxData", calibrated_auxwaveform_map);
  m_data->Stores.at("ANNIEEvent")->Get("RecoADCAuxHits", aux_pulse_map);

  for (const auto& temp_pair : aux_pulse_map) {
    const auto& channel_key = temp_pair.first;
    //For now, only calibrate the SiPM waveforms
    int sipm_number = -1;
    if(AuxChannelNumToTypeMap->at(channel_key) != "SiPM1") sipm_number = 1;
    if(AuxChannelNumToTypeMap->at(channel_key) != "SiPM2") sipm_number = 2;
    else continue;

    std::vector< std::vector<ADCPulse>> sipm_minibuffers = temp_pair.second;
    size_t num_minibuffers = sipm_minibuffers.size();  //Should be size 1 in FrankDAQ mode
    for (size_t mb = 0; mb < num_minibuffers; ++mb) {
      std::vector<ADCPulse> thisbuffer_pulses = sipm_minibuffers.at(mb);
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

void PhaseIITreeMaker::LoadAllTankHits() {
  std::map<unsigned long, std::vector<Hit>>* Hits = nullptr;
  bool got_hits = m_data->Stores["ANNIEEvent"]->Get("Hits", Hits);
  if (!got_hits){
    std::cout << "No Hits store in ANNIEEvent. Continuing to build tree " << std::endl;
    return;
  }
  Position detector_center=geom->GetTankCentre();
  double tank_center_x = detector_center.X();
  double tank_center_y = detector_center.Y();
  double tank_center_z = detector_center.Z();
  fNHits = 0;
  for(std::pair<unsigned long, std::vector<Hit>>&& apair : *Hits){
    unsigned long channel_key = apair.first;
    std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(channel_key);
    if(it != ChannelKeyToSPEMap.end()){ //Charge to SPE conversion is available
      std::vector<Hit>& ThisPMTHits = apair.second;
      fNHits+=ThisPMTHits.size();
      for (Hit &ahit : ThisPMTHits){
        double hit_charge = ahit.GetCharge();
        double hit_PE  = hit_charge / ChannelKeyToSPEMap.at(channel_key);
        Detector* this_detector = geom->ChannelToDetector(channel_key);
        Position det_position = this_detector->GetDetectorPosition();
        fHitX.push_back((det_position.X()-tank_center_x));
        fHitY.push_back((det_position.Y()-tank_center_y));
        fHitZ.push_back((det_position.Z()-tank_center_z));
        fHitT.push_back(ahit.GetTime());
        fHitQ.push_back(hit_charge);
        fHitPE.push_back(hit_PE);
        fHitDetID.push_back(ahit.GetTubeId());
        fHitType.push_back(RecoDigit::PMT8inch); // 0 For PMTs
      }
    }
  }
  return;
}

bool PhaseIITreeMaker::FillRecoInfo() {
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
 


  RecoVertex* truevtx = 0;
  auto get_muonMC = m_data->Stores.at("RecoEvent")->Get("TrueVertex",truevtx);
  auto get_muonMCEnergy = m_data->Stores.at("RecoEvent")->Get("TrueMuonEnergy",fTrueMuonEnergy);
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
  return successful_load;
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
            << " TrueMuonEnergy= " << fTrueMuonEnergy << std::endl
            << "           " << fTrueDirX << ", " << fTrueDirY << ", " << fTrueDirZ << ") " << std::endl;
  std::cout << "  recoVtx=(" << fRecoVtxX << ", " << fRecoVtxY << ", " << fRecoVtxZ <<", "<< fRecoVtxTime << std::endl
            << "           " << fRecoDirX << ", " << fRecoDirY << ", " << fRecoDirZ << ") " << std::endl;
  std::cout << "  DeltaR = "<<deltaR<<"[cm]"<<"\t"<<"  DeltaAngle = "<<DeltaAngle<<" [degree]"<<std::endl;
  std::cout << "  FOM = " << fRecoVtxFOM << std::endl;
  std::cout << "  RecoStatus = " << fRecoStatus <<std::endl;
  std::cout << std::endl;
}

