#include "EventSelector.h"

EventSelector::EventSelector():Tool(){}


bool EventSelector::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  fPMTMRDOffset = false;
  fIsMC = true;
  fPMTMRDOffset = 745;
  fRecoPDG = -1;

  //Get the tool configuration variables
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("MRDRecoCut", fMRDRecoCut);
  m_variables.Get("MCFVCut", fMCFVCut);
  m_variables.Get("MCPMTVolCut", fMCPMTVolCut);
  m_variables.Get("MCMRDCut", fMCMRDCut);
  m_variables.Get("MCPiKCut", fMCPiKCut);
  m_variables.Get("MCIsMuonCut", fMCIsMuonCut);
  m_variables.Get("MCIsElectronCut", fMCIsElectronCut);
  m_variables.Get("MCIsSingleRingCut", fMCIsSingleRingCut);
  m_variables.Get("MCIsMultiRingCut", fMCIsMultiRingCut);
  m_variables.Get("MCProjectedMRDHit", fMCProjectedMRDHit);
  m_variables.Get("MCEnergyCut", fMCEnergyCut);
  m_variables.Get("Emin",Emin);
  m_variables.Get("Emax",Emax);
  m_variables.Get("NHitCut", fNHitCut);
  m_variables.Get("NHitmin", fNHitmin);
  m_variables.Get("PromptTrigOnly", fPromptTrigOnly);
  m_variables.Get("RecoFVCut", fRecoFVCut);
  m_variables.Get("RecoPMTVolCut", fRecoPMTVolCut);
  m_variables.Get("PMTMRDCoincCut",fPMTMRDCoincCut);
  m_variables.Get("PMTMRDOffset",fPMTMRDOffset);
  m_variables.Get("NoVeto",fNoVetoCut);
  m_variables.Get("Veto",fVetoCut);
  m_variables.Get("ThroughGoing",fThroughGoing);
  m_variables.Get("TriggerWord",fTriggerWord);
  m_variables.Get("SaveStatusToStore", fSaveStatusToStore);
  m_variables.Get("IsMC",fIsMC);
  m_variables.Get("RecoPDG",fRecoPDG);
  m_variables.Get("TriggerExtendedWindow",fTriggerExtended);
  m_variables.Get("BeamOK",fBeamOK);
  m_variables.Get("CutConfiguration",fCutConfigurationName);

  if (!fIsMC){fMCFVCut = false; fMCPMTVolCut = false; fMCMRDCut = false; fMCPiKCut = false; fMCIsMuonCut = false; fMCIsElectronCut = false; fMCIsSingleRingCut = false; fMCIsMultiRingCut = false; fMCProjectedMRDHit = false; fMCEnergyCut = false; fPromptTrigOnly = false;}

  /// Construct the other objects we'll be needing at event level,
  
  // Make the RecoEvent Store if it doesn't exist
  int recoeventexists = m_data->Stores.count("RecoEvent");
  if(recoeventexists==0) m_data->Stores["RecoEvent"] = new BoostStore(false,2);
  //TODO: Have an event selection mask filled based on what cuts are run
  //m_data->Stores.at("RecoEvent")->Set("EvtSelectionMask", fEvtSelectionMask); 
  
  auto get_geometry= m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",fGeometry);
  if(!get_geometry){
    Log("EventSelector Tool: Error retrieving Geometry from ANNIEEvent!",v_error,verbosity); 
    return false; 
  }

  vec_pmtclusters_charge = new std::vector<double>; 
  vec_pmtclusters_time = new std::vector<double>; 
  vec_mrdclusters_time = new std::vector<double>; 

  m_data->CStore.Get("ChannelNumToTankPMTSPEChargeMap",ChannelNumToTankPMTSPEChargeMap);

  m_data->CStore.Set("CutConfiguration",fCutConfigurationName);


  return true;
}


bool EventSelector::Execute(){
  // Reset everything
  this->Reset();
  
  // see if "ANNIEEvent" exists
  auto get_annieevent = m_data->Stores.count("ANNIEEvent");
  if(!get_annieevent){
  	Log("EventSelector Tool: No ANNIEEvent store!",v_error,verbosity); 
  	return false;
  };

  // ANNIE Event number
  m_data->Stores.at("ANNIEEvent")->Get("EventNumber",fEventNumber);
  
  if (fIsMC){
  // MC entry number
  m_data->Stores.at("ANNIEEvent")->Get("MCEventNum",fMCEventNum);  
  
  // MC trigger number
  m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum",fMCTriggernum); 
  
  std::string logmessage = "EventSelector Tool: Processing MCEntry "+to_string(fMCEventNum)+
  ", MCTrigger "+to_string(fMCTriggernum) + ", Event "+to_string(fEventNumber);
  Log(logmessage,v_message,verbosity);
  }

  // Retrive digits from RecoEvent
  auto has_reco = m_data->Stores.at("RecoEvent")->Get("RecoDigit",fDigitList);  ///> Get digits from "RecoEvent" 
  if(not has_reco){
  	Log("EventSelector  Tool: Error retrieving RecoDigits,no digit from the RecoEvent!",v_warning,verbosity); 
  	/*return false;*/
  }

  // BEGIN CUTS USING TRUTH INFORMATION //

  bool get_truevtx, get_truestopvtx;
  if (fIsMC){
    // get truth vertex information 
    get_truevtx = m_data->Stores.at("RecoEvent")->Get("TrueVertex", fMuonStartVertex);
    if(!get_truevtx){ 
      Log("EventSelector Tool: Error retrieving TrueVertex from RecoEvent!",v_error,verbosity); 
      return false; 
    }
  
    get_truestopvtx = m_data->Stores.at("RecoEvent")->Get("TrueStopVertex", fMuonStopVertex);
    if(!get_truestopvtx){ 
      Log("EventSelector Tool: Error retrieving TrueStopVertex from RecoEvent!",v_error,verbosity); 
      return false; 
    }
   
    //Get MC version of MRD hits
    get_mrd = m_data->Stores.at("ANNIEEvent")->Get("TDCData",TDCData_MC);
    if (!get_mrd) {
      Log("EventSelector Tool: Error retrieving TDCData, true from ANNIEEvent!",v_error,verbosity);
    }
  } else {
    //Get data version of MRD hits
    get_mrd = m_data->Stores.at("ANNIEEvent")->Get("TDCData",TDCData);
    if (!get_mrd) {
      Log("EventSelector Tool: Error retrieving TDCData, true from ANNIEEvent!",v_error,verbosity);
    }

  }

  int fTrigger;
  auto get_trigger = m_data->Stores.at("ANNIEEvent")->Get("TriggerWord",fTrigger);
  if (not get_trigger){
      Log("EventSelector Tool: Error retrieving Triggerword, true from ANNIEEvent!",v_error,verbosity);
      return false;
  }

  bool IsSingleRing = false, IsMultiRing = false, HasProjectedMRDHit = false, passNoPiK = false, passMCFVCut = false, passMCPMTCut = false, passMCMRDCut = false, IsInsideEnergyWindow = false, IsElectron = false, IsMuon = false, isPromptTrigger=false;
 
  if (fIsMC){

    isPromptTrigger = this->PromptTriggerCheck();
    m_data->Stores.at("RecoEvent")->Set("PromptEvent",isPromptTrigger);
    
    IsSingleRing = this->EventSelectionByMCSingleRing();
    m_data->Stores.at("RecoEvent")->Set("MCSingleRingEvent",IsSingleRing);

    IsMultiRing = this->EventSelectionByMCMultiRing();
    m_data->Stores.at("RecoEvent")->Set("MCMultiRingEvent",IsMultiRing);

    HasProjectedMRDHit = this->EventSelectionByMCProjectedMRDHit();
    //information about projected MRD hit already stored in the RecoEvent store by MCRecoEventLoader

    passNoPiK = this->EventSelectionNoPiK();
    m_data->Stores.at("RecoEvent")->Set("MCNoPiK",passNoPiK);

    passMCFVCut = this->EventSelectionByFV(true);
    m_data->Stores.at("RecoEvent")->Set("MCFV",passMCFVCut);

    passMCPMTCut = this->EventSelectionByPMTVol(true);
    m_data->Stores.at("RecoEvent")->Set("MCPMTVol",passMCPMTCut);
 
    passMCMRDCut= this->EventSelectionByMCTruthMRD();
    m_data->Stores.at("RecoEvent")->Set("MCMRDStop",passMCMRDCut);

    IsInsideEnergyWindow = this->EnergyCutCheck(Emin,Emax);
    m_data->Stores.at("RecoEvent")->Set("MCEnergyCut",IsInsideEnergyWindow);

    IsMuon = this->ParticleCheck(13);
    m_data->Stores.at("RecoEvent")->Set("MCIsMuon",IsMuon);

    IsElectron = this->ParticleCheck(11);
    m_data->Stores.at("RecoEvent")->Set("MCIsElectron",IsElectron);

  }

  bool HasEnoughHits = false;
  if (has_reco){
    HasEnoughHits = this->NHitCountCheck(fNHitmin);
    m_data->Stores.at("RecoEvent")->Set("NHitCut",HasEnoughHits);  
  }

  bool passPMTMRDCoincCut = this->EventSelectionByPMTMRDCoinc();
  m_data->Stores.at("RecoEvent")->Set("PMTMRDCoinc",passPMTMRDCoincCut);

  bool passVetoCut = this->EventSelectionByVetoCut();
  m_data->Stores.at("RecoEvent")->Set("NoVeto",passVetoCut);

  bool passTriggerCut = this->EventSelectionByTrigger(fTrigger,fTriggerWord);
  m_data->Stores.at("RecoEvent")->Set("TriggerCut",passTriggerCut);

  //bool passThroughGoingCut = this->EventSelectionByThroughGoing();
  bool passThroughGoingCut = true;
  m_data->Stores.at("RecoEvent")->Set("ThroughGoing",passThroughGoingCut);

  std::vector<double> cluster_reco_pdg;
  bool passRecoPDGCut = this->EventSelectionByRecoPDG(fRecoPDG, cluster_reco_pdg);
  m_data->Stores.at("RecoEvent")->Set("RecoPDGVector",cluster_reco_pdg);
  m_data->Stores.at("RecoEvent")->Set("PDG",fRecoPDG);

  bool passExtendedCut = this->EventSelectionByTriggerExtended();
  m_data->Stores.at("RecoEvent")->Set("TriggerExtended",passExtendedCut);

  bool passBeamOKCut = this->EventSelectionByBeamOK();
  m_data->Stores.at("RecoEvent")->Set("BeamOK",passBeamOKCut);

  // Fill the EventSelection mask for the cuts that are supposed to be applied
  if (fMCPiKCut){
    fEventApplied |= EventSelector::kFlagMCPiK; 
    if(!passNoPiK) fEventFlagged |= EventSelector::kFlagMCPiK;
  }  

  if(fMCFVCut || fMCPMTVolCut){
    if (fMCFVCut){
      fEventApplied |= EventSelector::kFlagMCFV; 
      if(!passMCFVCut) fEventFlagged |= EventSelector::kFlagMCFV;
    }
    if (fMCPMTVolCut){
      fEventApplied |= EventSelector::kFlagMCPMTVol; 
      if(!passMCPMTCut) fEventFlagged |= EventSelector::kFlagMCPMTVol;
    }
  }

  if(fMCMRDCut){
    fEventApplied |= EventSelector::kFlagMCMRD; 
    if(!passMCMRDCut) fEventFlagged |= EventSelector::kFlagMCMRD;
  }

  if(fPromptTrigOnly){
    fEventApplied |= EventSelector::kFlagPromptTrig; 
    if(!isPromptTrigger) fEventFlagged |= EventSelector::kFlagPromptTrig;
  }

  if(fNHitCut){
    fEventApplied |= EventSelector::kFlagNHit;
    if(!HasEnoughHits) fEventFlagged |= EventSelector::kFlagNHit;
  }

  if (fMCEnergyCut){
    fEventApplied |= EventSelector::kFlagMCEnergyCut;
    if (!IsInsideEnergyWindow) fEventFlagged |= EventSelector::kFlagMCEnergyCut;
  }

  if (fMCIsMuonCut){
    fEventApplied |= EventSelector::kFlagMCIsMuon;
    if (!IsMuon) fEventFlagged |= EventSelector::kFlagMCIsMuon;
  }

  if (fMCIsElectronCut){
    fEventApplied |= EventSelector::kFlagMCIsElectron;
    if (!IsElectron) fEventFlagged |= EventSelector::kFlagMCIsElectron;
  }

  if (fMCIsSingleRingCut){
    fEventApplied |= EventSelector::kFlagMCIsSingleRing;
    if (!IsSingleRing) fEventFlagged |= EventSelector::kFlagMCIsSingleRing;
  }

  if (fMCIsMultiRingCut){
    fEventApplied |= EventSelector::kFlagMCIsMultiRing;
    if (!IsMultiRing) fEventFlagged |= EventSelector::kFlagMCIsMultiRing;
  }

  if (fMCProjectedMRDHit){
    fEventApplied |= EventSelector::kFlagMCProjectedMRDHit;
    if (!HasProjectedMRDHit) fEventFlagged |= EventSelector::kFlagMCProjectedMRDHit;
  }

  // BEGIN CUTS USING RECONSTRUCTED INFORMATION //
  if(fRecoFVCut || fRecoPMTVolCut){
	// Retrive Reconstructed vertex from RecoEvent 
	auto get_ok = m_data->Stores.at("RecoEvent")->Get("ExtendedVertex",fRecoVertex);  ///> Get reconstructed vertex 
    if(not get_ok){
  	  Log("EventSelector Tool: Error retrieving Extended vertex from RecoEvent!",v_error,verbosity); 
  	  return false;
    }
    if(fRecoFVCut){
      fEventApplied |= EventSelector::kFlagRecoFV; 
      bool passRecoFVCut= this->EventSelectionByFV(false);
      if(!passRecoFVCut) fEventFlagged |= EventSelector::kFlagRecoFV;
    }
    if(fRecoPMTVolCut){
      fEventApplied |= EventSelector::kFlagRecoPMTVol; 
      bool passRecoPMTVolCut= this->EventSelectionByPMTVol(false);
      if(!passRecoPMTVolCut) fEventFlagged |= EventSelector::kFlagRecoPMTVol;
    }
  }

  //FIXME: This isn't working according to Jingbo
  if(fMRDRecoCut){
    fEventApplied |= EventSelector::kFlagRecoMRD; 
    std::cout << "EventSelector Tool: Currently not implemented. Setting to false" << std::endl;
    Log("EventSelector Tool: MRDReco not implemented.  Setting cut bit to false",v_message,verbosity);
    bool passMRDRecoCut = false;
    //bool passMRDRecoCut = this->EventSelectionByMRDReco(); 
    if(!passMRDRecoCut) fEventFlagged |= EventSelector::kFlagRecoMRD; 
  }

  //Fast check whether the times of MRD and PMT clusters are coincident
  if(fPMTMRDCoincCut){
    fEventApplied |= EventSelector::kFlagPMTMRDCoinc;
    if (!passPMTMRDCoincCut) fEventFlagged |= EventSelector::kFlagPMTMRDCoinc;
  }

  if (fNoVetoCut){
    fEventApplied |= EventSelector::kFlagNoVeto;
    if (!passVetoCut) fEventFlagged |= EventSelector::kFlagNoVeto;
  }

  if (fVetoCut){
    fEventApplied |= EventSelector::kFlagVeto;
    if (passVetoCut) fEventFlagged |= EventSelector::kFlagVeto;
  }

  if (fThroughGoing){
    fEventApplied |= EventSelector::kFlagThroughGoing;
    if (!passThroughGoingCut) fEventFlagged |= EventSelector::kFlagThroughGoing;
  }

  if (fTriggerWord > 0){
    fEventApplied |= EventSelector::kFlagTrigger;
    if (!passTriggerCut) fEventFlagged |= EventSelector::kFlagTrigger;
  }

  if (fTriggerExtended){
    fEventApplied |= EventSelector::kFlagExtended;
    if (!passExtendedCut) fEventFlagged |= EventSelector::kFlagExtended;
  }

  if (fBeamOK){
    fEventApplied |= EventSelector::kFlagBeamOK;
    if (!passBeamOKCut) fEventFlagged |= EventSelector::kFlagBeamOK;
  }

  if (fRecoPDG != -1){
    fEventApplied |= EventSelector::kFlagRecoPDG;
    if (!passRecoPDGCut) fEventFlagged |= EventSelector::kFlagRecoPDG;
  }
  
  if(fEventFlagged != EventSelector::kFlagNone) fEventCutStatus = false;
  if(fEventCutStatus){  
    Log("EventSelector Tool: Event is clean according to current event selection.",v_message,verbosity);
  }
  if(fSaveStatusToStore) {
    m_data->Stores.at("RecoEvent")->Set("EventCutStatus", fEventCutStatus);
    m_data->Stores.at("ANNIEEvent")->Set("EventCutStatus", fEventCutStatus);
  }
  m_data->Stores.at("RecoEvent")->Set("EventFlagApplied", fEventApplied);
  m_data->Stores.at("RecoEvent")->Set("EventFlagged", fEventFlagged);

  if (verbosity >= v_debug){
    std::cout << "EventSelector tool: fEventApplied: "<< fEventApplied << ", fEventFlagged: " << fEventFlagged << std::endl;
    std::cout << "EventSelector tool: Bit representation: fEventApplied: " << std::bitset<32>(fEventApplied) << ", fEventFlagged: " << std::bitset<32>(fEventFlagged) << std::endl;
  }

  if (verbosity > 1) std::cout <<"EventCutStatus: "<<fEventCutStatus<<std::endl;

  //std::cout << "EventSelector tool: Bit representation: fEventApplied: " << std::bitset<32>(fEventApplied) << ", fEventFlagged: " << std::bitset<32>(fEventFlagged) << std::endl;
  //std::cout <<"EventCutStatus: "<<fEventCutStatus<<std::endl;


  return true;
}


bool EventSelector::Finalise(){
  if(verbosity>0) cout<<"EventSelector exitting"<<endl;
  delete vec_pmtclusters_charge;
  delete vec_pmtclusters_time;
  delete vec_mrdclusters_time;
  return true;
}

bool EventSelector::EventSelectionNoPiK() {
  
  // Get the pion and kaon counts from the store.  If any count is greater
  // than zero, the fEventCutStatus is set to "false" for a failed event.
  //Fill in pion counts for this event
  int pi0count, pipcount, pi0mcount, K0count, Kpcount, Kmcount;
  m_data->Stores.at("RecoEvent")->Get("MCPi0Count", pi0count);
  m_data->Stores.at("RecoEvent")->Get("MCPiPlusCount", pipcount);
  m_data->Stores.at("RecoEvent")->Get("MCPiMinusCount", pi0mcount);
  m_data->Stores.at("RecoEvent")->Get("MCK0Count", K0count);
  m_data->Stores.at("RecoEvent")->Get("MCKPlusCount", Kpcount);
  m_data->Stores.at("RecoEvent")->Get("MCKMinusCount", Kmcount);
  int sum = pi0count + pipcount + pi0mcount + K0count + Kpcount + Kmcount;
  if (sum > 0){
    Log("EventSelector: A primary pion or kaon was found. Total count: " +to_string(sum),v_message,verbosity);
    return false;
  } else{
    return true;
  }
}

bool EventSelector::PromptTriggerCheck() {
  /// First, see if this is a delayed trigger in the event
  auto get_mctrigger = m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum",fMCTriggernum);
  if(!get_mctrigger){ 
    Log("EventSelector Tool: Error retrieving MCTriggernum from ANNIEEvent!",v_error,verbosity); 
    return false; 
  }	
  
  /// if so, truth analysis is probably not interested in this trigger. Primary muon will not be in the listed tracks.
  bool splitSubtriggers = false;
  m_data->CStore.Get("SplitSubTriggers",splitSubtriggers);
  if(splitSubtriggers && (fMCTriggernum>0)){ 
    Log("EventSelector Tool: This event is not a prompt trigger",v_message,verbosity); 
    return false;
  }
  return true;
}

bool EventSelector::NHitCountCheck(int NHitCut) {
  if(verbosity>3){
    std::cout << "SIZE OF DIGIT LIST FOR EVENT IS: " << fDigitList->size() << std::endl;
  }
  
  int size_pmt_digits = 0;

  for (unsigned int i_digit = 0; i_digit < fDigitList->size(); i_digit++){
    RecoDigit thisdigit = fDigitList->at(i_digit);
    int digittype = thisdigit.GetDigitType();
    if (digittype == 0) size_pmt_digits++;
  }

  if(size_pmt_digits<NHitCut) {
    Log("EventSelector Tool: Event has less than 4 digits",v_message,verbosity);
    return false;
  }
  else {
  return true;
  }
}

// This function isn't working now, because the MRDTracks store must have been changed. 
// We have to contact Marcus and ask how we can retieve the MRD track information. 
bool EventSelector::EventSelectionByMRDReco() {
  /// Get number of subentries
  int NumMrdTracks = 0;
  double mrdtracklength = 0.;
  double MaximumMRDTrackLength = 0;
  int mrdlayershit = 0;
  bool mrdtrackisstoped = false;
  int longesttrackEntryNumber = -9999;
  
  /// check if "MRDTracks" store exists
	int mrdtrackexists = m_data->Stores.count("MRDTracks");
	if(mrdtrackexists == 0) {
	  Log("EventSelector Tool: MRDTracks store doesn't exist!",v_message,verbosity);
	  return false;
	} 	
  
  auto getmrdtracks = m_data->Stores.at("MRDTracks")->Get("NumMrdTracks",NumMrdTracks);
  if(!getmrdtracks) {
    Log("EventSelector Tool: Error retrieving MRDTracks Store!",v_error,verbosity); 
  	return false;	
  }
  logmessage = "EventSelector Tool: Found " + to_string(NumMrdTracks) + " MRD tracks";
  Log(logmessage,v_message,verbosity);
  
  // If mrd track isn't found
  if(NumMrdTracks == 0) {
    Log("EventSelector Tool: Found no MRD Tracks",v_message,verbosity);
    return false;
  }
  // If mrd track is found
  // MRD tracks are saved in the "MRDTracks" Store. Each entry is an MRD track
  
  for(int entrynum=0; entrynum<NumMrdTracks; entrynum++) {
    //m_data->Stores["MRDTracks"]->GetEntry(entrynum);
    m_data->Stores.at("MRDTracks")->Get("TrackLength",mrdtracklength);
    m_data->Stores.at("MRDTracks")->Get("LayersHit",mrdlayershit);
    if(MaximumMRDTrackLength<mrdtracklength) {
      MaximumMRDTrackLength = mrdtracklength;
      longesttrackEntryNumber = entrynum;
    }
  }
  // check if the longest track is stopped inside MRD
  //m_data->Stores["MRDTracks"]->GetEntry(longesttrackEntryNumber);
  m_data->Stores.at("MRDTracks")->Get("IsMrdStopped",mrdtrackisstoped);
  if(!mrdtrackisstoped) return false;
  return true;
}


bool EventSelector::EventSelectionByFV(bool isMC) {
  if(isMC && !fMuonStartVertex) return false;
  if(!isMC && !fRecoVertex) return false;
  RecoVertex* checkedVertex;
  if(isMC){
      Log("EventSelector Tool: Checking FV cut for true muon vertex",v_debug,verbosity); 
      checkedVertex=fMuonStartVertex;
  } else {
      Log("EventSelector Tool: Checking FV cut for reconstructed muon vertex",v_debug,verbosity); 
    checkedVertex=fRecoVertex;
  }
  double checkedVtxX, checkedVtxY, checkedVtxZ;
  Position vtxPos = checkedVertex->GetPosition();
  Direction vtxDir = checkedVertex->GetDirection();
  checkedVtxX = vtxPos.X();
  checkedVtxY = vtxPos.Y();
  checkedVtxZ = vtxPos.Z();
  //std::cout<<"checkedVtxX, Y, Z = "<<checkedVtxX<<", "<<checkedVtxY<<", "<<checkedVtxZ<<std::endl;
  double tankradius = ANNIEGeometry::Instance()->GetCylRadius();
  double fidcutradius = 0.8 * tankradius;
  double fidcuty = 50.;
  double fidcutz = 0.;
  if(checkedVtxZ > fidcutz) return false;
  if( (TMath::Sqrt(TMath::Power(checkedVtxX, 2) + TMath::Power(checkedVtxZ,2)) > fidcutradius) 
  	  || (TMath::Abs(checkedVtxY) > fidcuty) 
  	  || (checkedVtxZ > fidcutz) ){
  Log("EventSelector Tool: This event is not contained inside the FV",v_message,verbosity); 
  return false;
  }	
 
  return true;
}

bool EventSelector::EventSelectionByPMTVol(bool isMC) {
  if(isMC && !fMuonStartVertex) return false;
  if(!isMC && !fRecoVertex) return false;
  RecoVertex* checkedVertex;
  if(isMC){
      Log("EventSelector Tool: Checking PMT volume cut for true muon vertex",v_debug,verbosity); 
      checkedVertex=fMuonStartVertex;
  } else {
      Log("EventSelector Tool: Checking PMT volume cut for reconstructed muon vertex",v_debug,verbosity); 
    checkedVertex=fRecoVertex;
  }
  double checkedVtxX, checkedVtxY, checkedVtxZ;
  Position vtxPos = checkedVertex->GetPosition();
  Direction vtxDir = checkedVertex->GetDirection();
  checkedVtxX = vtxPos.X();
  checkedVtxY = vtxPos.Y();
  checkedVtxZ = vtxPos.Z();
  double fidcutradius = fGeometry->GetPMTEnclosedRadius()*100.;
  double fidcuty = fGeometry->GetPMTEnclosedHalfheight()*100.;
  if( (TMath::Sqrt(TMath::Power(checkedVtxX, 2) + TMath::Power(checkedVtxZ,2)) > fidcutradius) 
  	  || (TMath::Abs(checkedVtxY) > fidcuty)){
  Log("EventSelector Tool: This event is not contained within the PMT volume",v_message,verbosity); 
  return false;
  }	
 
  return true;
}

bool EventSelector::EventSelectionByMCTruthMRD() {
  if(!fMuonStopVertex) return false;
  // mrd cut
  double muonStopX, muonStopY, muonStopZ;
  muonStopX = fMuonStopVertex->GetPosition().X();
  muonStopY = fMuonStopVertex->GetPosition().Y();
  muonStopZ = fMuonStopVertex->GetPosition().Z();
  double mrdStartZ = fGeometry->GetMrdStart()*100-168.1;
  double mrdEndZ = fGeometry->GetMrdEnd()*100-168.1;
  double mrdHeightY = fGeometry->GetMrdHeight()*100;
  double mrdWidthX = fGeometry->GetMrdWidth()*100;
  Log("EventSelector tool: Read in MuonStop (X,Y,Z) = ("+std::to_string(muonStopX)+","+std::to_string(muonStopY)+","+std::to_string(muonStopZ)+")",2,verbosity);
  if(muonStopZ<mrdStartZ || muonStopZ>mrdEndZ
  	|| muonStopX<-1.0*mrdWidthX || muonStopX>mrdWidthX
  	|| muonStopY<-1.0*mrdHeightY || muonStopY>mrdHeightY) {
    Log("EventSelector Tool: This MC Event's muon does not stop in the MRD",v_message,verbosity); 
    return false;	
  }
  return true;
}

bool EventSelector::EnergyCutCheck(double Emin, double Emax) {

  double energy;
  m_data->Stores.at("RecoEvent")->Get("TrueMuonEnergy",energy);
  if (energy > Emax || energy < Emin) return false;
  return true;

}

bool EventSelector::ParticleCheck(int pdg_number) {

  int pdg;
  m_data->Stores.at("RecoEvent")->Get("PdgPrimary",pdg);
  if (pdg!=pdg_number) return false;
  return true;

}

bool EventSelector::EventSelectionByMCSingleRing() {

  int nrings;
  m_data->Stores.at("RecoEvent")->Get("NRings",nrings);
  if (nrings!=1) return false;
  return true;

}

bool EventSelector::EventSelectionByMCMultiRing() {

  int nrings;
  m_data->Stores.at("RecoEvent")->Get("NRings",nrings);
  if (nrings <=1 ) return false;
  return true;

}

bool EventSelector::EventSelectionByMCProjectedMRDHit() {

  bool has_projected_mrd_hit;
  m_data->Stores.at("RecoEvent")->Get("ProjectedMRDHit",has_projected_mrd_hit);
  return has_projected_mrd_hit;

}

bool EventSelector::EventSelectionByPMTMRDCoinc() {

  if (fIsMC){
    bool has_clustered_pmt = m_data->CStore.Get("ClusterMapMC",m_all_clusters_MC);
    if (not has_clustered_pmt) { Log("EventSelector Tool: Error retrieving ClusterMapMC from CStore, did you run ClusterFinder beforehand?",v_error,verbosity); return false; }
  } else {
    bool has_clustered_pmt = m_data->CStore.Get("ClusterMap",m_all_clusters);
    if (not has_clustered_pmt) { Log("EventSelector Tool: Error retrieving ClusterMap from CStore, did you run ClusterFinder beforehand?",v_error,verbosity); return false; }
  }

  bool has_clustered_mrd = m_data->CStore.Get("MrdTimeClusters",MrdTimeClusters);
  if (not has_clustered_mrd) { Log("EventSelector Tool: Error retrieving MrdTimeClusters map from CStore, did you run TimeClustering beforehand?",v_error,verbosity); return false; }
  if (MrdTimeClusters.size()!=0){
    has_clustered_mrd = m_data->CStore.Get("MrdDigitTimes",MrdDigitTimes);
    if (not has_clustered_mrd) { Log("EventSelector Tool: Error retrieving MrdDigitTimes map from CStore, did you run TimeClustering beforehand?",v_error,verbosity); return false; }
    has_clustered_mrd = m_data->CStore.Get("MrdDigitChankeys",MrdDigitChankeys);
    if (not has_clustered_mrd) { Log("EventDisplay Tool: Error retrieving MrdDigitChankeys, did you run TimeClustering beforehand",v_error,verbosity); return false;}
  }
  
  int pmt_cluster_size;
  if (fIsMC) pmt_cluster_size = (int) m_all_clusters_MC->size();
  else pmt_cluster_size = (int) m_all_clusters->size();
  m_data->Stores["RecoEvent"]->Set("NumPMTClusters",pmt_cluster_size);
  vec_pmtclusters_charge->clear();
  vec_pmtclusters_time->clear();
  m_data->Stores["RecoEvent"]->Set("PMTClustersCharge",vec_pmtclusters_charge,true);
  m_data->Stores["RecoEvent"]->Set("PMTClustersTime",vec_pmtclusters_time,true);
  vec_mrdclusters_time->clear();
  m_data->Stores["RecoEvent"]->Set("MRDClustersTime",vec_mrdclusters_time,true);
  if (verbosity > 1) std::cout <<"pmt_cluster_size: "<<pmt_cluster_size<<", mrd cluster size: "<<MrdTimeClusters.size()<<std::endl;

  bool prompt_cluster = false;
  pmt_time = 0;
  double max_charge = 0;
  n_hits = 0;

  pmt_time = -1;

  if (fIsMC){
    if (m_all_clusters_MC->size()){
      double cluster_time;
      for(std::pair<double,std::vector<MCHit>>&& apair : *m_all_clusters_MC){
        std::vector<MCHit>&MCHits = apair.second;
        double time_temp = 0;
        double charge_temp = 0;
        for (unsigned int i_hit = 0; i_hit < MCHits.size(); i_hit++){
          time_temp+=MCHits.at(i_hit).GetTime();
          charge_temp+=MCHits.at(i_hit).GetCharge();
        }
        if (MCHits.size()>0) time_temp/=MCHits.size();
        vec_pmtclusters_charge->push_back(charge_temp);
        vec_pmtclusters_time->push_back(time_temp);
        if (time_temp > 2000.) continue;	//not a prompt event
        if (charge_temp > max_charge){
          max_charge = charge_temp;
          prompt_cluster = true;
          pmt_time = time_temp;
          n_hits = int(MCHits.size());
        }
      }
    }
  } else {
    if (m_all_clusters->size()){
      double cluster_time;
      for(std::pair<double,std::vector<Hit>>&& apair : *m_all_clusters){
        std::vector<Hit>&Hits = apair.second;
        double time_temp = 0;
        double charge_temp = 0;
        for (unsigned int i_hit = 0; i_hit < Hits.size(); i_hit++){
          time_temp+=Hits.at(i_hit).GetTime();
          int tube = Hits.at(i_hit).GetTubeId();
          double charge_pe = Hits.at(i_hit).GetCharge()/ChannelNumToTankPMTSPEChargeMap->at(tube);
          charge_temp+=charge_pe;
        }
        if (Hits.size()>0) time_temp/=Hits.size();
        vec_pmtclusters_charge->push_back(charge_temp);
        vec_pmtclusters_time->push_back(time_temp);
        if (time_temp > 2000.) continue;	//not a prompt event
        if (charge_temp > max_charge){
          max_charge = charge_temp;
          prompt_cluster = true;
          pmt_time = time_temp;
          n_hits = int(Hits.size());
        }
      }
    }
  }

  if (verbosity > 1) {
    std::cout <<"Maximum charge in PMT cluster: "<<max_charge<<std::endl;
    std::cout <<"Number of PMT hits in muon cluster: "<<n_hits<<std::endl;
  }

  m_data->Stores["RecoEvent"]->Set("PMTClustersCharge",vec_pmtclusters_charge,true);
  m_data->Stores["RecoEvent"]->Set("PMTClustersTime",vec_pmtclusters_time,true);

  std::vector<double> mrd_meantimes;
  if (verbosity > 1) std::cout <<"MrdTimeClusters.size(): "<<MrdTimeClusters.size()<<std::endl;
  for(unsigned int thiscluster=0; thiscluster<MrdTimeClusters.size(); thiscluster++){
 
    std::vector<int> hitmrd_times;
    std::vector<int> single_mrdcluster = MrdTimeClusters.at(thiscluster);
    int numdigits = single_mrdcluster.size();
    double mrd_meantime = 0.;
    for(int thisdigit=0;thisdigit<numdigits;thisdigit++){
      int digit_value = single_mrdcluster.at(thisdigit);
      unsigned long chankey = MrdDigitChankeys.at(digit_value);
      Detector *thedetector = fGeometry->ChannelToDetector(chankey);
      unsigned long detkey = thedetector->GetDetectorID();
      if (thedetector->GetDetectorElement()=="MRD") {
        double mrdtimes=MrdDigitTimes.at(digit_value);
        hitmrd_times.push_back(mrdtimes);
        mrd_meantime += mrdtimes;
      }
    }
    if (hitmrd_times.size()>0) mrd_meantime /= hitmrd_times.size();
    mrd_meantimes.push_back(mrd_meantime);
  }
  if (verbosity > 1) std::cout <<"mrd_meantimes.size(): "<<mrd_meantimes.size()<<std::endl;

  vec_mrdclusters_time->clear();
  for (int i=0; i<(int)mrd_meantimes.size(); i++){
    vec_mrdclusters_time->push_back(mrd_meantimes.at(i));
  }
  m_data->Stores["RecoEvent"]->Set("MRDClustersTime",vec_mrdclusters_time, true);
  
  if (fIsMC){
    if (MrdTimeClusters.size() == 0 || m_all_clusters_MC->size() == 0) return false;
  } else {
    if (MrdTimeClusters.size() == 0 || m_all_clusters->size() == 0) return false;
  }

  pmtmrd_coinc_min = fPMTMRDOffset - 50;
  pmtmrd_coinc_max = fPMTMRDOffset + 50;

  std::vector<int> vector_mrd_coincidence;

  bool coincidence = false;
  for (int i_mrd = 0; i_mrd < int(mrd_meantimes.size()); i_mrd++){
    double time_diff = mrd_meantimes.at(i_mrd) - pmt_time;
    if (verbosity > 0) std::cout <<"MRD time: "<<mrd_meantimes.at(i_mrd)<<", PMT time: "<<pmt_time<<", difference: "<<time_diff<<std::endl;
    Log("EventSelector tool: MRD/Tank coincidene candidate "+std::to_string(i_mrd)+ " has time difference: "+std::to_string(time_diff),v_message,verbosity);
    if (verbosity > 1) std::cout <<"max_charge: "<<max_charge<<", n_hits: "<<n_hits<<std::endl;
    Log("EventSelector tool: MRD/Tank coincidene candidate "+std::to_string(i_mrd)+ " has time difference: "+std::to_string(time_diff),1,verbosity);
    
    if (time_diff > pmtmrd_coinc_min && time_diff < pmtmrd_coinc_max && max_charge > 200 && n_hits >= 20){
      coincidence = true;
      vector_mrd_coincidence.push_back(i_mrd);
    }
  }
  
  m_data->Stores["RecoEvent"]->Set("MRDCoincidenceCluster",vector_mrd_coincidence);

  return coincidence;

}

bool EventSelector::EventSelectionByVetoCut(){

 bool has_veto = false;
 if (pmt_time > 0.){
 if (fIsMC) {
    if (get_mrd){
    if(TDCData_MC){
    if (TDCData_MC->size()==0){
      Log("EventSelector tool: TDC data is empty in this event.",v_message,verbosity);
    } else {
      for (auto&& anmrdpmt : (*TDCData_MC)){
        unsigned long chankey = anmrdpmt.first;
        Detector* thedetector = fGeometry->ChannelToDetector(chankey);
        unsigned long detkey = thedetector->GetDetectorID();
        if (thedetector->GetDetectorElement()=="Veto") {
          std::vector<MCHit> fmv_hits = anmrdpmt.second;
          for (int i_hit=0; i_hit < fmv_hits.size(); i_hit++){
            MCHit fmv_hit = fmv_hits.at(i_hit);
            double time_diff = fmv_hit.GetTime()-pmt_time;
            if (time_diff > (pmtmrd_coinc_min-50) && time_diff < (pmtmrd_coinc_max+50)){
              has_veto = true;
           }
          }
        }
      }
    }
  } else {
    Log("EventSelector tool: No TDC data available in this event.",v_message,verbosity);
  }
 }
}
else {
  if (get_mrd){
  if(TDCData){
    if (TDCData->size()==0){
      Log("EventSelector tool: TDC data is empty in this event.",v_message,verbosity);
    } else {
      for (auto&& anmrdpmt : (*TDCData)){
        unsigned long chankey = anmrdpmt.first;
        Detector* thedetector = fGeometry->ChannelToDetector(chankey);
        unsigned long detkey = thedetector->GetDetectorID();
        if (thedetector->GetDetectorElement()=="Veto") {
          std::vector<Hit> fmv_hits = anmrdpmt.second;
          for (int i_hit=0; i_hit < fmv_hits.size(); i_hit++){
            Hit fmv_hit = fmv_hits.at(i_hit);
            double time_diff = fmv_hit.GetTime()-pmt_time;
            if (time_diff > (pmtmrd_coinc_min+75) && time_diff < (pmtmrd_coinc_max+75)){	//TODO: Check this 75ns offset
              has_veto = true;
           }
          }
        }
      }
    }
  } else {
    Log("EventSelector tool: No TDC data available in this event.",v_message,verbosity);
  }
  }
  }
  }

  return (!has_veto);	//Successful selection means no veto hit 

}

bool EventSelector::EventSelectionByThroughGoing(){

	bool throughgoing = false;
        double z0 = 0.;
        double z1 = 0.0508;
        double z2 = 0.0728;
        std::vector<double> fmv_firstlayer_y{-1.987,-1.68,-1.373,-1.066,-0.758999,-0.451999,-0.144999,0.162001,0.469001,0.77601,1.083,1.39,1.697};

	bool passVetoCut;
	m_data->Stores.at("RecoEvent")->Get("NoVeto",passVetoCut);

	std::vector<double> *vec_mrdclusters_time;
	m_data->Stores["RecoEvent"]->Get("MRDClustersTime",vec_mrdclusters_time);

	std::vector<unsigned long> fmv_hit_chkey;
	std::vector<double> fmv_hit_t;


	if (get_mrd){
		if(TDCData){
			if (TDCData->size()==0){
      				Log("EventSelector tool: TDC data is empty in this event.",v_message,verbosity);
    			} else {
      				for (auto&& anmrdpmt : (*TDCData)){
        				unsigned long chankey = anmrdpmt.first;
        				Detector* thedetector = fGeometry->ChannelToDetector(chankey);
        				unsigned long detkey = thedetector->GetDetectorID();
        				if (thedetector->GetDetectorElement()=="Veto") {
          					std::vector<Hit> fmv_hits = anmrdpmt.second;
						for (int i_hit=0; i_hit < (int) fmv_hits.size(); i_hit++){
							Hit afmvhit = fmv_hits.at(i_hit);
							fmv_hit_chkey.push_back(chankey);
							fmv_hit_t.push_back(afmvhit.GetTime());
						}
					}
				}
			}
		}
	}

	if (!passVetoCut && n_hits > 70){	// Need coincident veto hit for through-going candidates
		std::vector<BoostStore>* theMrdTracks;   // the reconstructed tracks
		int numtracksinev;
  		m_data->Stores["MRDTracks"]->Get("MRDTracks",theMrdTracks);
  		m_data->Stores["MRDTracks"]->Get("NumMrdTracks",numtracksinev);
		if (numtracksinev == 1){
			Position StartVertex;
			Position StopVertex;
			BoostStore* thisTrackAsBoostStore = &(theMrdTracks->at(0));
    			thisTrackAsBoostStore->Get("StartVertex",StartVertex);
    			thisTrackAsBoostStore->Get("StopVertex",StopVertex);
			double mrd_start_x = StartVertex.X();
			double mrd_start_y = StartVertex.Y();
			double mrd_start_z = StartVertex.Z();
			double mrd_stop_x = StopVertex.X();
			double mrd_stop_y = StopVertex.Y();
			double mrd_stop_z = StopVertex.Z();
			
			//Find intersection of extrapolated MRD track with FMV
			double x_layer1, y_layer1, x_layer2, y_layer2;
			std::vector<double> mrd_start{mrd_start_x,mrd_start_y,mrd_start_z};
			std::vector<double> mrd_stop{mrd_stop_x,mrd_stop_y,mrd_stop_z};
			FindPaddleIntersection(mrd_start,mrd_stop,x_layer1,y_layer1,z1);
			FindPaddleIntersection(mrd_start,mrd_stop,x_layer2,y_layer2,z2);
			unsigned long hit_chankey_layer1 = 999999;
                        unsigned long hit_chankey_layer2 = 999999;
                        FindPaddleChankey(x_layer1, y_layer1, 1, hit_chankey_layer1);
                        FindPaddleChankey(x_layer2, y_layer2, 2, hit_chankey_layer2);

                        bool coincident_layer1=false;
                        bool coincident_layer2=false;
                        if (hit_chankey_layer1 == 999999){
                                std::cout <<"Did not find paddle with the desired intersection properties for FMV layer 1."<<std::endl;
                        }
                        if (hit_chankey_layer2 == 999999){
                                std::cout << "Did not find paddle with the desired intersection properties for FMV layer 2."<<std::endl;
                        }
			for (int i_fmv=0; i_fmv < (int) fmv_hit_chkey.size(); i_fmv++){
                                unsigned long chkey = fmv_hit_chkey.at(i_fmv);
                                if (verbosity > 2) std::cout <<"chkey: "<<chkey<<std::endl;
                                if (chkey <= 12){
                                if (chkey == hit_chankey_layer1){
					coincident_layer1=true;
				} else if (y_layer1-fmv_firstlayer_y.at(chkey) > -0.4 &&
                                        y_layer1-fmv_firstlayer_y.at(chkey)< 0.6 &&
                                        fabs(x_layer1)<1.6) {
						coincident_layer1 = true;
					}
				} else if (chkey > 12){
					if (chkey == hit_chankey_layer2){
						coincident_layer2=true;
					} else if (y_layer2-fmv_firstlayer_y.at(chkey-13) > -0.4 &&
                                        y_layer2-fmv_firstlayer_y.at(chkey-13)< 0.6 &&
                                        fabs(x_layer1)<1.6) {
						coincident_layer2=true;
                                        }
                                }
			}

			if (coincident_layer1 || coincident_layer2) throughgoing = true;

		}
	}

	if (verbosity > 1) std::cout <<"EventSelector tool: Through-going: "<<throughgoing<<std::endl;

	return throughgoing;
}

bool EventSelector::EventSelectionByTrigger(int current_trigger, int reference_trigger){

  bool correct_triggerword = false;
  if (reference_trigger == current_trigger) correct_triggerword = true;
  if (verbosity > 1) std::cout <<"current_trigger: "<<current_trigger<<", reference trigger: "<<reference_trigger<<", correct_trigger: "<<correct_triggerword<<std::endl;
  return correct_triggerword;

}

void EventSelector::Reset() {
  // Reset 
  fEventApplied = EventSelector::kFlagNone;
  fEventFlagged = EventSelector::kFlagNone;
  fEventCutStatus = true; 
}

bool EventSelector::FindPaddleIntersection(std::vector<double> startpos, std::vector<double> endpos, double &x, double &y, double z){

        double DirX = endpos.at(0)-startpos.at(0);
        double DirY = endpos.at(1)-startpos.at(1);
        double DirZ = endpos.at(2)-startpos.at(2);

        if (fabs(DirZ) < 0.001) std::cout << "StartVertex = EndVertex! Track was not fitted well"<<std::endl;

        double frac = (z - startpos.at(2))/DirZ;

        x = startpos.at(0)+frac*DirX;
        y = startpos.at(1)+frac*DirY;

        return true;

} 

bool EventSelector::FindPaddleChankey(double x, double y, int layer, unsigned long &chankey){

        double y_min[13]={-2.139499,-1.832499,-1.525499,-1.218499,-0.911499,-0.604499,-0.297499,0.009501,0.316501,0.623501,0.930501,1.237501,1.544501};
        double y_max[13]={-1.834499,-1.527499,-1.220499,-0.913499,-0.606499,-0.299499,0.007501,0.314501,0.621501,0.928501,1.235501,1.542501,1.849501};
        bool found_chankey = false;
        for (unsigned int i_channel = 0; i_channel < 13; i_channel++){

                if (found_chankey) break;

                unsigned long chankey_tmp = (unsigned long) i_channel;
                if (layer == 2) chankey_tmp += 13;
                double xmin = -1.60;
                double xmax = 1.60;
                double ymin = y_min[i_channel];
                double ymax = y_max[i_channel];

                //Check if expected hit was within the channel or not
                if (xmin <= x && xmax >= x && ymin <= y && ymax >= y){
                  chankey = chankey_tmp;
                  found_chankey = true;
                }
        }
        return found_chankey;
}

bool EventSelector::EventSelectionByRecoPDG(int recoPDG, std::vector<double> & cluster_reco_pdg){

  if (fIsMC){
    bool has_clustered_pmt = m_data->CStore.Get("ClusterMapMC",m_all_clusters_MC);
    if (not has_clustered_pmt) { Log("EventSelector Tool: Error retrieving ClusterMapMC from CStore, did you run ClusterFinder beforehand?",v_error,verbosity); return false; }
  } else {
    bool has_clustered_pmt = m_data->CStore.Get("ClusterMap",m_all_clusters);
    if (not has_clustered_pmt) { Log("EventSelector Tool: Error retrieving ClusterMap from CStore, did you run ClusterFinder beforehand?",v_error,verbosity); return false; }
  }

  std::map<double,double> ClusterMaxPEs;
  std::map<double,Position> ClusterChargePoints;
  std::map<double,double> ClusterChargeBalances;

  bool got_ccp = m_data->Stores.at("ANNIEEvent")->Get("ClusterChargePoints", ClusterChargePoints);
  bool got_ccb = m_data->Stores.at("ANNIEEvent")->Get("ClusterChargeBalances", ClusterChargeBalances);
  bool got_cmpe = m_data->Stores.at("ANNIEEvent")->Get("ClusterMaxPEs", ClusterMaxPEs);

  bool found_pdg = false;

  if (fabs(recoPDG)==2112){
    if (fIsMC){
      if (m_all_clusters_MC->size()){
        for(std::pair<double,std::vector<MCHit>>&& apair : *m_all_clusters_MC){
          double cluster_time = apair.first;
          double charge_balance = ClusterChargeBalances.at(cluster_time);
          std::vector<MCHit>&MCHits = apair.second;
          double time_temp = 0;
          double charge_temp = 0;
          for (unsigned int i_hit = 0; i_hit < MCHits.size(); i_hit++){
            time_temp+=MCHits.at(i_hit).GetTime();
            charge_temp+=MCHits.at(i_hit).GetCharge();
          }
          if (cluster_time > 10000 && charge_balance < 0.4 && charge_temp < 120) {
            cluster_reco_pdg.push_back(cluster_time);
            found_pdg = true;
            std::cout <<"Found neutron candidate for cluster at time = "<<cluster_time<<", with CB = "<<charge_balance <<" and charge "<<charge_temp<<"!!!"<<std::endl;
          } else {
            std::cout <<"Did not pass neutron candidate cuts!!! Time = "<<cluster_time <<", CB = "<<charge_balance << " and charge "<<charge_temp<<"!!!"<<std::endl;
          }
        }
      }
    } else {
      if (m_all_clusters->size()){
        double cluster_time;
        for(std::pair<double,std::vector<Hit>>&& apair : *m_all_clusters){
          double cluster_time = apair.first;
          double charge_balance = ClusterChargeBalances.at(cluster_time);
          std::vector<Hit>&Hits = apair.second;
          double time_temp = 0;
          double charge_temp = 0;
          for (unsigned int i_hit = 0; i_hit < Hits.size(); i_hit++){
            time_temp+=Hits.at(i_hit).GetTime();
            int tube = Hits.at(i_hit).GetTubeId();
            double charge_pe = Hits.at(i_hit).GetCharge()/ChannelNumToTankPMTSPEChargeMap->at(tube);
            charge_temp+=charge_pe;
          }
          if (cluster_time > 10000 && charge_balance < 0.4 && charge_temp < 120) {
            cluster_reco_pdg.push_back(cluster_time);
            found_pdg = true;
            std::cout <<"Found neutron candidate for cluster at time = "<<cluster_time<<", with CB = "<<charge_balance <<"and charge "<<charge_temp<<"!!!"<<std::endl;
          } else {
            std::cout <<"Did not pass neutron candidate cuts!!! Time = "<<cluster_time <<", CB = "<<charge_balance << " and charge "<<charge_temp<<"!!!"<<std::endl;
          }
        }
      }
    }
  }

  return found_pdg;

}
bool EventSelector::EventSelectionByTriggerExtended(){

  int fExtended = 0;
  m_data->Stores["ANNIEEvent"]->Get("TriggerExtended",fExtended);
  if (fExtended == 1) return true;
  else return false;

}

bool EventSelector::EventSelectionByBeamOK(){

  BeamStatus beamstat;
  m_data->Stores["ANNIEEvent"]->Get("BeamStatus",beamstat);
  if (beamstat.ok()) return true;
  else return false;

}
