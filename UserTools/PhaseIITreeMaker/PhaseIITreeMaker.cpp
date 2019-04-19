#include "PhaseIITreeMaker.h"

PhaseIITreeMaker::PhaseIITreeMaker():Tool(){}


bool PhaseIITreeMaker::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  
  m_variables.Get("verbose", verbosity);
  m_variables.Get("fillCleanEventsOnly", fillCleanEventsOnly);
  m_variables.Get("muonMCTruth_fill", muonMCTruth_fill);
  m_variables.Get("muonRecoDebug_fill", muonRecoDebug_fill);
  m_variables.Get("muonTruthRecoDiff_fill", muonTruthRecoDiff_fill);
  m_variables.Get("pionKaonCount_fill", pionKaonCount_fill);

  std::string output_filename;
  m_variables.Get("OutputFile", output_filename);
  fOutput_tfile = new TFile(output_filename.c_str(), "recreate");
  fRecoTree = new TTree("phaseII", "ANNIE Phase II Reconstruction Tree");
  //Metadata for Events
  fRecoTree->Branch("mcEntryNumber",&fMCEventNum,"mcEntryNumber/I");
  fRecoTree->Branch("triggerNumber",&fMCTriggerNum,"triggerNumber/I");
  fRecoTree->Branch("eventNumber",&fEventNumber,"eventNumber/I");
  fRecoTree->Branch("runNumber",&fRunNumber,"runNumber/I");
  fRecoTree->Branch("subrunNumber",&fSubrunNumber,"subrunNumber/I");

  //Event Staus Flag Information
  fRecoTree->Branch("eventStatusApplied",&fEventStatusApplied,"eventStatusApplied/I");
  fRecoTree->Branch("eventStatusFlagged",&fEventStatusFlagged,"eventStatusFlagged/I");
  
  //Hit information (PMT and LAPPD)
  //Always output in Phase II Reco Tree
  fRecoTree->Branch("nhits",&fNhits,"nhits/I");
  fRecoTree->Branch("filter",&fIsFiltered);
  fRecoTree->Branch("digitX",&fDigitX);
  fRecoTree->Branch("digitY",&fDigitY);
  fRecoTree->Branch("digitZ",&fDigitZ);
  fRecoTree->Branch("digitT",&fDigitT);
  fRecoTree->Branch("digitQ",&fDigitQ);
  fRecoTree->Branch("digitType", &fDigitType);
  fRecoTree->Branch("digitDetID", &fDigitDetID);

  //Reconstructed variables after full Muon Reco Analysis
  //Always output in Phase II Reco Tree
  fRecoTree->Branch("recoVtxX",&fRecoVtxX,"recoVtxX/D");
  fRecoTree->Branch("recoVtxY",&fRecoVtxY,"recoVtxY/D");
  fRecoTree->Branch("recoVtxZ",&fRecoVtxZ,"recoVtxZ/D");
  fRecoTree->Branch("recoVtxTime",&fRecoVtxTime,"recoVtxTime/D");
  fRecoTree->Branch("recoDirX",&fRecoDirX,"recoDirX/D");
  fRecoTree->Branch("recoDirY",&fRecoDirY,"recoDirY/D");
  fRecoTree->Branch("recoDirZ",&fRecoDirZ,"recoDirZ/D");
  fRecoTree->Branch("recoAngle",&fRecoAngle,"recoAngle/D");
  fRecoTree->Branch("recoPhi",&fRecoPhi,"recoPhi/D");
  fRecoTree->Branch("recoVtxFOM",&fRecoVtxFOM,"recoVtxFOM/D");
  fRecoTree->Branch("recoStatus",&fRecoStatus,"recoStatus/I");
  
  //MC truth information for muons
  //Output to tree when muonMCTruth_fill = 1 in config
  if (muonMCTruth_fill){
    fRecoTree->Branch("trueVtxX",&fTrueVtxX,"trueVtxX/D");
    fRecoTree->Branch("trueVtxY",&fTrueVtxY,"trueVtxY/D");
    fRecoTree->Branch("trueVtxZ",&fTrueVtxZ,"trueVtxZ/D");
    fRecoTree->Branch("trueVtxTime",&fTrueVtxTime,"trueVtxTime/D");
    fRecoTree->Branch("trueDirX",&fTrueDirX,"trueDirX/D");
    fRecoTree->Branch("trueDirY",&fTrueDirY,"trueDirY/D");
    fRecoTree->Branch("trueDirZ",&fTrueDirZ,"trueDirZ/D");
    fRecoTree->Branch("trueAngle",&fTrueAngle,"trueAngle/D");
    fRecoTree->Branch("truePhi",&fTruePhi,"truePhi/D");
    fRecoTree->Branch("trueMuonEnergy",&fTrueMuonEnergy, "trueMuonEnergy/D");
    fRecoTree->Branch("trueTrackLengthInWater",&fTrueTrackLengthInWater,"trueTrackLengthInWater/D");
    fRecoTree->Branch("trueTrackLengthInMRD",&fTrueTrackLengthInMRD,"trueTrackLengthInMRD/D");
  }
  
  // Reconstructed variables from each step in Muon Reco Analysis
  // Currently output when muonRecoDebug_fill = 1 in config 
  if (muonRecoDebug_fill){
    fRecoTree->Branch("seedVtxX",&fSeedVtxX); 
    fRecoTree->Branch("seedVtxY",&fSeedVtxY); 
    fRecoTree->Branch("seedVtxZ",&fSeedVtxZ);
    fRecoTree->Branch("seedVtxFOM",&fSeedVtxFOM); 
    fRecoTree->Branch("seedVtxTime",&fSeedVtxTime,"seedVtxTime/D");
    
    fRecoTree->Branch("pointPosX",&fPointPosX,"pointPosX/D");
    fRecoTree->Branch("pointPosY",&fPointPosY,"pointPosY/D");
    fRecoTree->Branch("pointPosZ",&fPointPosZ,"pointPosZ/D");
    fRecoTree->Branch("pointPosTime",&fPointPosTime,"pointPosTime/D");
    fRecoTree->Branch("pointPosFOM",&fPointPosFOM,"pointPosFOM/D");
    fRecoTree->Branch("pointPosStatus",&fPointPosStatus,"pointPosStatus/I");
    
    fRecoTree->Branch("pointDirX",&fPointDirX,"pointDirX/D");
    fRecoTree->Branch("pointDirY",&fPointDirY,"pointDirY/D");
    fRecoTree->Branch("pointDirZ",&fPointDirZ,"pointDirZ/D");
    fRecoTree->Branch("pointDirTime",&fPointDirTime,"pointDirTime/D");
    fRecoTree->Branch("pointDirStatus",&fPointDirStatus,"pointDirStatus/I");
    fRecoTree->Branch("pointDirFOM",&fPointDirFOM,"pointDirFOM/D");
    
    fRecoTree->Branch("pointVtxPosX",&fPointVtxPosX,"pointVtxPosX/D");
    fRecoTree->Branch("pointVtxPosY",&fPointVtxPosY,"pointVtxPosY/D");
    fRecoTree->Branch("pointVtxPosZ",&fPointVtxPosZ,"pointVtxPosZ/D");
    fRecoTree->Branch("pointVtxTime",&fPointVtxTime,"pointVtxTime/D");
    fRecoTree->Branch("pointVtxDirX",&fPointVtxDirX,"pointVtxDirX/D");
    fRecoTree->Branch("pointVtxDirY",&fPointVtxDirY,"pointVtxDirY/D");
    fRecoTree->Branch("pointVtxDirZ",&fPointVtxDirZ,"pointVtxDirZ/D");
    fRecoTree->Branch("pointVtxFOM",&fPointVtxFOM,"pointVtxFOM/D");
    fRecoTree->Branch("pointVtxStatus",&fPointVtxStatus,"pointVtxStatus/I");
  } 

  // Difference in MC Truth and Muon Reconstruction Analysis
  // Output to tree when muonTruthRecoDiff_fill = 1 in config
  if (muonTruthRecoDiff_fill){
    fRecoTree->Branch("deltaVtxX",&fDeltaVtxX,"deltaVtxX/D");
    fRecoTree->Branch("deltaVtxY",&fDeltaVtxY,"deltaVtxY/D");
    fRecoTree->Branch("deltaVtxZ",&fDeltaVtxZ,"deltaVtxZ/D");
    fRecoTree->Branch("deltaVtxR",&fDeltaVtxR,"deltaVtxR/D");
    fRecoTree->Branch("deltaVtxT",&fDeltaVtxT,"deltaVtxT/D");
    fRecoTree->Branch("deltaParallel",&fDeltaParallel,"deltaParallel/D");
    fRecoTree->Branch("deltaPerpendicular",&fDeltaPerpendicular,"deltaPerpendicular/D");
    fRecoTree->Branch("deltaAzimuth",&fDeltaAzimuth,"deltaAzimuth/D");
    fRecoTree->Branch("deltaZenith",&fDeltaZenith,"deltaZenith/D");
    fRecoTree->Branch("deltaAngle",&fDeltaAngle,"deltaAngle/D");
  } 

  // Pion and kaon counts as found in MC Truth based on PDG codes
  if (pionKaonCount_fill){
    fRecoTree->Branch("Pi0Count",&fPi0Count,"Pi0Count/I");
    fRecoTree->Branch("PiPlusCount",&fPiPlusCount,"PiPlusCount/I");
    fRecoTree->Branch("PiMinusCount",&fPiMinusCount,"PiMinusCount/I");
    fRecoTree->Branch("K0Count",&fK0Count,"K0Count/I");
    fRecoTree->Branch("KPlusCount",&fKPlusCount,"KPlusCount/I");
    fRecoTree->Branch("KMinusCount",&fKMinusCount,"KMinusCount/I");
  }

  return true;
}

bool PhaseIITreeMaker::Execute(){
  Log("===========================================================================================",v_debug,verbosity);
  Log("PhaseIITreeMaker Tool: Executing",v_debug,verbosity);

  // Reset variables
  this->ResetVariables();
  // Get a pointer to the ANNIEEvent Store
  auto* annie_event = m_data->Stores["RecoEvent"];
  if (!annie_event) {
    Log("Error: The PhaseITreeMaker tool could not find the RecoEvent Store", v_error, verbosity);
    return false;
  }

  // check if event passes the cut
  bool EventCutstatus = false;
  auto get_evtstatus = m_data->Stores.at("RecoEvent")->Get("EventCutStatus",EventCutstatus);
  if(!get_evtstatus) {
    Log("Error: The PhaseITreeMaker tool could not find the Event selection status", v_error, verbosity);
    return false;	
  }

  if(!EventCutstatus && fillCleanEventsOnly) {
  	Log("Message: This event doesn't pass the event selection. ", v_message, verbosity);
    return true;	
  }
  
  // MC entry number
  m_data->Stores.at("ANNIEEvent")->Get("MCEventNum",fMCEventNum);  
  
  // MC trigger number
  m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum",fMCTriggerNum); 
  
  // ANNIE Event number
  m_data->Stores.at("ANNIEEvent")->Get("EventNumber",fEventNumber);

  m_data->Stores.at("ANNIEEvent")->Get("RunNumber",fRunNumber);
  m_data->Stores.at("ANNIEEvent")->Get("SubrunNumber",fSubrunNumber);
  
  std::string logmessage = "  Retriving information for MCEntry "+to_string(fMCEventNum)+
  	", MCTrigger "+ to_string(fMCTriggerNum) + ", EventNumber " + to_string(fEventNumber);
  Log(logmessage, v_message, verbosity);
 

  // Read Event Selector Status information
  auto get_flagsapp = m_data->Stores.at("RecoEvent")->Get("EventFlagApplied",fEventStatusApplied);
  auto get_flags = m_data->Stores.at("RecoEvent")->Get("EventFlagged",fEventStatusFlagged);  
  if(!get_flagsapp || !get_flags) {
    Log("PhaseITreeMaker tool: No Event status applied or flagged bitmask!!", v_error, verbosity);
    return false;	
  }

  // Read digits
  std::vector<RecoDigit>* digitList = nullptr;
  auto get_digits = m_data->Stores.at("RecoEvent")->Get("RecoDigit",digitList);  ///> Get digits from "RecoEvent" 
  if(!get_digits) {
    Log("PhaseITreeMaker tool: no digit list in store!", v_error, verbosity);
    return false;	
  }
  fNhits = digitList->size();
  for( auto& digit : *digitList ){
    fDigitX.push_back(digit.GetPosition().X());
    fDigitY.push_back(digit.GetPosition().Y());
    fDigitZ.push_back(digit.GetPosition().Z());
    fDigitT.push_back(digit.GetCalTime());      
    fDigitQ.push_back(digit.GetCalCharge());
    fDigitType.push_back(digit.GetDigitType());
    fDigitDetID.push_back(digit.GetDetectorID());
  }
  
  // Read reconstructed Vertex
  RecoVertex* recovtx = 0;
  auto get_extendedvtx = m_data->Stores.at("RecoEvent")->Get("ExtendedVertex",recovtx); 
  if(!get_extendedvtx) {
    Log("Warning: The PhaseITreeMaker tool could not find ExtendedVertex. Continuing to build tree", v_message, verbosity);
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
  // Read True Vertex if flag is set
  if(muonMCTruth_fill){
    this->FillMCTruthInfo();
  }

  if (muonRecoDebug_fill){
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

  if (pionKaonCount_fill){
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
    }
  } 

  if (muonTruthRecoDiff_fill){
    bool successload = this->FillMCTruthInfo();
    RecoVertex* recovtx = 0;
    auto get_extendedvtx = m_data->Stores.at("RecoEvent")->Get("ExtendedVertex",recovtx); 
    if (!successload || !get_extendedvtx) {
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

  fRecoTree->Fill();
  this->RecoSummary();
  return true;
}

bool PhaseIITreeMaker::Finalise(){
	fOutput_tfile->cd();
	fRecoTree->Write();
	fOutput_tfile->Close();
	
	if(verbosity>0) cout<<"PhaseIITreeMaker exitting"<<endl;

  return true;
}

void PhaseIITreeMaker::ResetVariables() {
  // tree variables
  fMCEventNum = -9999;
  fMCTriggerNum = -9999;
  fEventNumber = -9999;
  fNhits = -9999;
  fRunNumber = -9999;
  fSubrunNumber = -9999;
  
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
 
  if (muonRecoDebug_fill){ 
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
  fIsFiltered.clear();
  fDigitX.clear();
  fDigitY.clear();
  fDigitZ.clear();
  fDigitT.clear();
  fDigitQ.clear();
  fDigitType.clear();
  fDigitDetID.clear();	
  
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

  if (pionKaonCount_fill){
    fPi0Count = -9999;
    fPiPlusCount = -9999;
    fPiMinusCount = -9999;
    fK0Count = -9999;
    fKPlusCount = -9999;
    fKMinusCount = -9999;
  }
}

bool PhaseIITreeMaker::FillMCTruthInfo() {
  bool successful_load = true;
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
  return successful_load;
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

