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
  m_variables.Get("MCMRDCut", fMCMRDCut);
  m_variables.Get("MCPiKCut", fMCPiKCut);
  m_variables.Get("NHitCut", fNHitCut);
  m_variables.Get("PromptTrigOnly", fPromptTrigOnly);
  m_variables.Get("GetPionKaonInfo", fGetPiKInfo);
  m_variables.Get("xshift",xshift);
  m_variables.Get("yshift",yshift);
  m_variables.Get("zshift",zshift);
  m_variables.Get("RecoFVCut", fRecoFVCut);
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

  if (fMCPiKCut){
    fEventApplied |= EventSelector::kFlagMCPiK; 
    bool passNoPiK = this->EventSelectionNoPiK();
    if(!passNoPiK) fEventFlagged |= EventSelector::kFlagMCPiK;
  }  
  if(fMCFVCut){
    // get truth vertex information 
    auto get_truevtx = m_data->Stores.at("RecoEvent")->Get("TrueVertex", fMuonStartVertex);
	if(!get_truevtx){ 
	  Log("EventSelector Tool: Error retrieving TrueVertex from RecoEvent!",v_error,verbosity); 
	  return false; 
	}
    fEventApplied |= EventSelector::kFlagMCFV; 
    bool passMCFVCut= this->EventSelectionByFV(true);
    if(!passMCFVCut) fEventFlagged |= EventSelector::kFlagMCFV;
; 
  }
  if(fMCMRDCut){
    auto get_truestopvtx = m_data->Stores.at("RecoEvent")->Get("TrueStopVertex", fMuonStopVertex);
    if(!get_truestopvtx){ 
      Log("EventSelector Tool: Error retrieving TrueStopVertex from RecoEvent!",v_error,verbosity); 
      return false; 
    }
    fEventApplied |= EventSelector::kFlagMCMRD; 
    bool passMCMRDCut= this->EventSelectionByMCTruthMRD();
    if(!passMCMRDCut) fEventFlagged |= EventSelector::kFlagMCMRD;
  }

  if(fPromptTrigOnly){
    fEventApplied |= EventSelector::kFlagPromptTrig; 
    bool isPromptTrigger= this->PromptTriggerCheck();
    if(!isPromptTrigger) fEventFlagged |= EventSelector::kFlagPromptTrig;
  }

  if(fNHitCut){
    fEventApplied |= EventSelector::kFlagNHit;
    int DefaultCut = 4; //TODO: could add to the config?
    bool HasEnoughHits = this->NHitCountCheck(DefaultCut);
    if(!HasEnoughHits) fEventFlagged |= EventSelector::kFlagNHit;
  }

  if(fRecoFVCut){
	// Retrive Reconstructed vertex from RecoEvent 
	auto get_ok = m_data->Stores.at("RecoEvent")->Get("ExtendedVertex",fRecoVertex);  ///> Get reconstructed vertex 
    if(not get_ok){
  	  Log("EventSelector Tool: Error retrieving Extended vertex from RecoEvent!",v_error,verbosity); 
  	  return false;
    }
    fEventApplied |= EventSelector::kFlagRecoFV; 
    bool passRecoFVCut= this->EventSelectionByFV(false);
    if(!passRecoFVCut) fEventFlagged |= EventSelector::kFlagRecoFV;
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

void EventSelector::FindPionKaonCountFromMC() {
  
  // loop over the MCParticles to find the highest enery primary muon
  // MCParticles is a std::vector<MCParticle>
  bool pionfound=false;
  bool kaonfound=false;
  int pi0count = 0;
  int pipcount = 0;
  int pimcount = 0;
  int K0count = 0;
  int Kpcount = 0;
  int Kmcount = 0;
  if(fMCParticles){
    Log("EventSelector::  Tool: Num MCParticles = "+to_string(fMCParticles->size()),v_message,verbosity);
    for(int particlei=0; particlei<fMCParticles->size(); particlei++){
      MCParticle aparticle = fMCParticles->at(particlei);
      //if(v_debug<verbosity) aparticle.Print();       // print if we're being *really* verbose
      if(aparticle.GetParentPdg()!=0) continue;      // not a primary particle
      if(aparticle.GetPdgCode()==111){               // is a primary pi0
        pionfound = true;
        pi0count++;
      }
      if(aparticle.GetPdgCode()==211){               // is a primary pi+
        pionfound = true;
        pipcount++;
      }
      if(aparticle.GetPdgCode()==-211){               // is a primary pi-
        pionfound = true;
        pimcount++;
      }
      if(aparticle.GetParentPdg()!=0) continue;      // not a primary particle
      if(aparticle.GetPdgCode()==311){               // is a primary K0
        kaonfound = true;
        K0count++;
      }
      if(aparticle.GetPdgCode()==321){               // is a primary K+
        kaonfound = true;
        Kpcount++;
      }
      if(aparticle.GetPdgCode()==-321){               // is a primary K-
        kaonfound = true;
        Kmcount++;
      }
    }
  } else {
    Log("EventSelector::  Tool: No MCParticles in the event!",v_error,verbosity);
  }
  if(not pionfound){
    Log("EventSelector::  Tool: No primary pions in this event",v_warning,verbosity);
  }
  if(not kaonfound){
    Log("EventSelector::  Tool: No kaons in this event",v_warning,verbosity);
  }
  //Fill in pion counts for this event
  m_data->Stores.at("RecoEvent")->Set("MCPi0Count", pi0count);
  m_data->Stores.at("RecoEvent")->Set("MCPiPlusCount", pipcount);
  m_data->Stores.at("RecoEvent")->Set("MCPiMinusCount", pimcount);
  m_data->Stores.at("RecoEvent")->Set("MCK0Count", K0count);
  m_data->Stores.at("RecoEvent")->Set("MCKPlusCount", Kpcount);
  m_data->Stores.at("RecoEvent")->Set("MCKMinusCount", Kmcount);
}

RecoVertex* EventSelector::FindTrueVertexFromMC() {
  
  // loop over the MCParticles to find the highest enery primary muon
  // MCParticles is a std::vector<MCParticle>
  MCParticle primarymuon;  // primary muon
  bool mufound=false;
  double muStartEnergy = 0;
  if(fMCParticles){
    Log("EventSelector::  Tool: Num MCParticles = "+to_string(fMCParticles->size()),v_message,verbosity);
    for(int particlei=0; particlei<fMCParticles->size(); particlei++){
      MCParticle aparticle = fMCParticles->at(particlei);
      //if(v_debug<verbosity) aparticle.Print();       // print if we're being *really* verbose
      if(aparticle.GetParentPdg()!=0) continue;      // not a primary particle
      if(aparticle.GetPdgCode()!=13) continue;       // not a muon
      if(aparticle.GetStartEnergy()<muStartEnergy) continue; // select muon with higher energy
      else muStartEnergy = aparticle.GetStartEnergy(); 
      primarymuon = aparticle;                       // note the particle
      mufound=true;                                  // note that we found it
      //primarymuon.Print();
      break;                                         // won't have more than one primary muon
    }
  } else {
    Log("EventSelector::  Tool: No MCParticles in the event!",v_error,verbosity);
  }
  if(not mufound){
    Log("EventSelector::  Tool: No muon in this event",v_warning,verbosity);
    return 0;
  }
  
  // retrieve desired information from the particle
  Position muonstartpos = primarymuon.GetStartVertex();    // only true if the muon is primary
  double muonstarttime = primarymuon.GetStartTime();
  Position muonstoppos = primarymuon.GetStopVertex();    // only true if the muon is primary
  double muonstoptime = primarymuon.GetStopTime();
  
  Direction muondirection = primarymuon.GetStartDirection();
  double muonenergy = primarymuon.GetStartEnergy();
  // set true vertex
  // change unit
  muonstartpos.UnitToCentimeter(); // convert unit from meter to centimeter
  muonstoppos.UnitToCentimeter(); // convert unit from meter to centimeter
  // change coordinate for muon start vertex
  muonstartpos.SetX(muonstartpos.X()+xshift);
  muonstartpos.SetY(muonstartpos.Y()+yshift);
  muonstartpos.SetZ(muonstartpos.Z()+zshift);
  fMuonStartVertex->SetVertex(muonstartpos, muonstarttime);
  fMuonStartVertex->SetDirection(muondirection);
  //  charge coordinate for muon stop vertex
  muonstoppos.SetX(muonstoppos.X()+xshift);
  muonstoppos.SetY(muonstoppos.Y()+yshift);
  muonstoppos.SetZ(muonstoppos.Z()+zshift);
  fMuonStopVertex->SetVertex(muonstoppos, muonstoptime); 
  
  logmessage = "  trueVtx = (" +to_string(muonstartpos.X()) + ", " + to_string(muonstartpos.Y()) + ", " + to_string(muonstartpos.Z()) +", "+to_string(muonstarttime)+ "\n"
            + "           " +to_string(muondirection.X()) + ", " + to_string(muondirection.Y()) + ", " + to_string(muondirection.Z()) + ") " + "\n";
  
  Log(logmessage,v_debug,verbosity);
	logmessage = "  muonStop = ("+to_string(muonstoppos.X()) + ", " + to_string(muonstoppos.Y()) + ", " + to_string(muonstoppos.Z()) + ") "+ "\n";
	Log(logmessage,v_debug,verbosity);
  return fMuonStartVertex;
}

void EventSelector::PushTrueVertex(bool savetodisk) {
  Log("EventSelector Tool: Push true vertex to the RecoEvent store",v_message,verbosity);
  m_data->Stores.at("RecoEvent")->Set("TrueVertex", fMuonStartVertex, savetodisk); 
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
  std::cout<<"checkedVtxX, Y, Z = "<<checkedVtxX<<", "<<checkedVtxY<<", "<<checkedVtxZ<<std::endl;
  double tankradius = ANNIEGeometry::Instance()->GetCylRadius();
  double fidcutradius = 0.8 * tankradius;
  double fidcuty = 50.;
  double fidcutz = 0.;
  if(checkedVtxZ > fidcutz) return false;
  if( (TMath::Sqrt(TMath::Power(checkedVtxX, 2) + TMath::Power(checkedVtxZ,2)) > fidcutradius) 
  	  || (TMath::Abs(checkedVtxY) > fidcuty) 
  	  || (checkedVtxZ > fidcutz) ){
  Log("EventSelector Tool: This event did not reconstruct inside the FV",v_message,verbosity); 
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
  cout<<"muonStopX, Y, Z = "<<muonStopX<<", "<<muonStopY<<", "<<muonStopZ<<endl;
  cout<<"mrdStartZ, mrdStopZ = "<<mrdStartZ<<", "<<mrdEndZ<<endl;
  cout<<"mrdWidthX = "<<mrdWidthX<<endl;
  cout<<"mrdHeightY = "<<mrdHeightY<<endl;
  if(muonStopZ<mrdStartZ || muonStopZ>mrdEndZ
  	|| muonStopX<-1.0*mrdWidthX || muonStopX>mrdWidthX
  	|| muonStopY<-1.0*mrdHeightY || muonStopY>mrdHeightY) {
    Log("EventSelector Tool: This MC Event's muon does not stop in the MRD",v_message,verbosity); 
    return false;	
  }
  return true;
}

void EventSelector::Reset() {
  // Reset 
  fEventApplied = EventSelector::kFlagNone;
  fEventFlagged = EventSelector::kFlagNone;
  fEventCutStatus = true; 
} 
