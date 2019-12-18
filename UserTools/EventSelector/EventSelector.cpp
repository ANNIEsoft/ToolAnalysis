#include "EventSelector.h"

EventSelector::EventSelector():Tool(){}


bool EventSelector::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

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
  m_variables.Get("SaveStatusToStore", fSaveStatusToStore);

  /// Construct the other objects we'll be needing at event level,
  
  // Make the RecoEvent Store if it doesn't exist
  int recoeventexists = m_data->Stores.count("RecoEvent");
  if(recoeventexists==0) m_data->Stores["RecoEvent"] = new BoostStore(false,2);
  //TODO: Have an event selection mask filled based on what cuts are run
  //m_data->Stores.at("RecoEvent")->Set("EvtSelectionMask", fEvtSelectionMask); 
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

  // MC entry number
  m_data->Stores.at("ANNIEEvent")->Get("MCEventNum",fMCEventNum);  
  
  // MC trigger number
  m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum",fMCTriggernum); 
  
  // ANNIE Event number
  m_data->Stores.at("ANNIEEvent")->Get("EventNumber",fEventNumber);
  
  std::string logmessage = "EventSelector Tool: Processing MCEntry "+to_string(fMCEventNum)+
  ", MCTrigger "+to_string(fMCTriggernum) + ", Event "+to_string(fEventNumber);
  Log(logmessage,v_message,verbosity);
  
  auto get_geometry= m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",fGeometry);
  if(!get_geometry){
    Log("EventSelector Tool: Error retrieving Geometry from ANNIEEvent!",v_error,verbosity); 
    return false; 
  }

  // Retrive digits from RecoEvent
  auto get_ok = m_data->Stores.at("RecoEvent")->Get("RecoDigit",fDigitList);  ///> Get digits from "RecoEvent" 
  if(not get_ok){
  	Log("EventSelector  Tool: Error retrieving RecoDigits,no digit from the RecoEvent!",v_error,verbosity); 
  	return false;
  }

  // BEGIN CUTS USING TRUTH INFORMATION //

  // get truth vertex information 
  auto get_truevtx = m_data->Stores.at("RecoEvent")->Get("TrueVertex", fMuonStartVertex);
  if(!get_truevtx){ 
      Log("EventSelector Tool: Error retrieving TrueVertex from RecoEvent!",v_error,verbosity); 
      return false; 
  }
  
  auto get_truestopvtx = m_data->Stores.at("RecoEvent")->Get("TrueStopVertex", fMuonStopVertex);
  if(!get_truestopvtx){ 
      Log("EventSelector Tool: Error retrieving TrueStopVertex from RecoEvent!",v_error,verbosity); 
      return false; 
  }
 
  bool IsSingleRing = this->EventSelectionByMCSingleRing();
  m_data->Stores.at("RecoEvent")->Set("SingleRingEvent",IsSingleRing);

  bool IsMultiRing = this->EventSelectionByMCMultiRing();
  m_data->Stores.at("RecoEvent")->Set("MultiRingEvent",IsMultiRing);

  bool HasProjectedMRDHit = this->EventSelectionByMCProjectedMRDHit();
  //information about projected MRD hit already stored in the RecoEvent store by MCRecoEventLoader

  // Fill the EventSelection mask for the cuts that are supposed to be applied
  if (fMCPiKCut){
    bool passNoPiK = this->EventSelectionNoPiK();
    fEventApplied |= EventSelector::kFlagMCPiK; 
    if(!passNoPiK) fEventFlagged |= EventSelector::kFlagMCPiK;
  }  

  if(fMCFVCut || fMCPMTVolCut){
    if (fMCFVCut){
      bool passMCFVCut= this->EventSelectionByFV(true);
      fEventApplied |= EventSelector::kFlagMCFV; 
      if(!passMCFVCut) fEventFlagged |= EventSelector::kFlagMCFV;
    }
    if (fMCPMTVolCut){
      bool passMCPMTCut= this->EventSelectionByPMTVol(true);
      fEventApplied |= EventSelector::kFlagMCPMTVol; 
      if(!passMCPMTCut) fEventFlagged |= EventSelector::kFlagMCPMTVol;
    }
  }

  if(fMCMRDCut){
    bool passMCMRDCut= this->EventSelectionByMCTruthMRD();
    fEventApplied |= EventSelector::kFlagMCMRD; 
    if(!passMCMRDCut) fEventFlagged |= EventSelector::kFlagMCMRD;
  }

  if(fPromptTrigOnly){
    bool isPromptTrigger= this->PromptTriggerCheck();
    fEventApplied |= EventSelector::kFlagPromptTrig; 
    if(!isPromptTrigger) fEventFlagged |= EventSelector::kFlagPromptTrig;
  }

  if(fNHitCut){
    bool HasEnoughHits = this->NHitCountCheck(fNHitmin);
    fEventApplied |= EventSelector::kFlagNHit;
    if(!HasEnoughHits) fEventFlagged |= EventSelector::kFlagNHit;
  }

  if (fMCEnergyCut){
    bool IsInsideEnergyWindow = this->EnergyCutCheck(Emin,Emax);
    fEventApplied |= EventSelector::kFlagMCEnergyCut;
    if (!IsInsideEnergyWindow) fEventFlagged |= EventSelector::kFlagMCEnergyCut;
  }

  if (fMCIsMuonCut){
    bool IsMuon = this->ParticleCheck(13);
    fEventApplied |= EventSelector::kFlagMCIsMuon;
    if (!IsMuon) fEventFlagged |= EventSelector::kFlagMCIsMuon;
  }

  if (fMCIsElectronCut){
    bool IsElectron = this->ParticleCheck(11);
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

  if(fEventFlagged != EventSelector::kFlagNone) fEventCutStatus = false;
  if(fEventCutStatus){  
    Log("EventSelector Tool: Event is clean according to current event selection.",v_message,verbosity);
  }
  if(fSaveStatusToStore) m_data->Stores.at("RecoEvent")->Set("EventCutStatus", fEventCutStatus);
  m_data->Stores.at("RecoEvent")->Set("EventFlagApplied", fEventApplied);
  m_data->Stores.at("RecoEvent")->Set("EventFlagged", fEventFlagged);
  return true;
}


bool EventSelector::Finalise(){
  if(verbosity>0) cout<<"EventSelector exitting"<<endl;
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
  if(fMCTriggernum>0){ 
    Log("EventSelector Tool: This event is not a prompt trigger",v_message,verbosity); 
    return false;
  }
  return true;
}

bool EventSelector::NHitCountCheck(int NHitCut) {
  if(verbosity>3){
    std::cout << "SIZE OF DIGIT LIST FOR EVENT IS: " << fDigitList->size() << std::endl;
  }
  if(fDigitList->size()<NHitCut) {
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
  double fidcutradius = fGeometry.GetPMTEnclosedRadius()*100.;
  double fidcuty = fGeometry.GetPMTEnclosedHalfheight()*100.;
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
  double mrdStartZ = fGeometry.GetMrdStart()*100-168.1;
  double mrdEndZ = fGeometry.GetMrdEnd()*100-168.1;
  double mrdHeightY = fGeometry.GetMrdHeight()*100;                                                                                     
  double mrdWidthX = fGeometry.GetMrdWidth()*100;
  Log("EventSelector tool: Read in MuonStop (X,Y,Z) = ("+std::to_string(muonStopX)+","+std::to_string(muonStopY)+","+std::to_string(muonStopZ)+")");
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

void EventSelector::Reset() {
  // Reset 
  fEventApplied = EventSelector::kFlagNone;
  fEventFlagged = EventSelector::kFlagNone;
  fEventCutStatus = true; 
} 
