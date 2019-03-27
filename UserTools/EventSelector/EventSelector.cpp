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
  m_variables.Get("PromptTrigOnly", fPromptTrigOnly);

  /// Construct the other objects we'll be setting at event level,
  fMuonStartVertex = new RecoVertex();
  fMuonStopVertex = new RecoVertex();
  
  // Make the RecoDigit Store if it doesn't exist
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

  // see if "RecoEvent" exists
  auto get_recoevent = m_data->Stores.count("RecoEvent");
  if(!get_recoevent){
  	Log("EventSelector Tool: No RecoEvent store!",v_error,verbosity); 
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

  // get truth vertex information 
  auto get_truevtx = m_data->Stores.at("RecoEvent")->Get("TrueVertex", fMuonStartVertex);
	if(!get_truevtx){ 
	  Log("VtxExtendedVertexFinder Tool: Error retrieving TrueVertex from RecoEvent!",v_error,verbosity); 
	  return false; 
	}
  auto get_truestopvtx = m_data->Stores.at("RecoEvent")->Get("TrueStopVertex", fMuonStopVertex);
	if(!get_truestopvtx){ 
	  Log("VtxExtendedVertexFinder Tool: Error retrieving TrueStopVertex from RecoEvent!",v_error,verbosity); 
	  return false; 
	}
  
  if (fMCPiKCut){
    bool passNoPiK = this->EventSelectionNoPiK();
    if(!passNoPiK) fEventCutStatus = false;
  }  
  if(fMCFVCut){
    bool passMCTruth= this->EventSelectionByMCTruthFV();
    if(!passMCTruth) fEventCutStatus = false; 
  }
  if(fMCMRDCut){
    bool passMCTruth= this->EventSelectionByMCTruthMRD();
    if(!passMCTruth) fEventCutStatus = false; 
  }
  if(fPromptTrigOnly){
    bool isPromptTrigger= this->PromptTriggerCheck();
    if(!isPromptTrigger) fEventCutStatus = false; 
  }
	
  //FIXME: This isn't working according to Jingbo
  if(fMRDRecoCut){
    std::cout << "EventSelector Tool: Currently not implemented. Setting to false" << std::endl;
    Log("EventSelector Tool: MRDReco not implemented.  Setting cut bit to false",v_message,verbosity);
    bool passMRDRecoCut = false;
    //bool passMRDRecoCut = this->EventSelectionByMRDReco(); 
    if(!passMRDRecoCut) fEventCutStatus = false; 
  }

  m_data->Stores.at("RecoEvent")->Set("EventCutStatus", fEventCutStatus);
  return true;
}


bool EventSelector::Finalise(){
  delete fMuonStartVertex;
  delete fMuonStopVertex;
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
    Log("DigitBuilder Tool: Error retrieving MCTriggernum from ANNIEEvent!",v_error,verbosity); 
    return false; 
  }	
  /// if so, truth analysis is probably not interested in this trigger. Primary muon will not be in the listed tracks.
  if(fMCTriggernum>0){ 
    Log("DigitBuilder Tool: This event is not a prompt trigger",v_debug,verbosity); 
    return false;
  }
  return true;
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


bool EventSelector::EventSelectionByMCTruthFV() {
  if(!fMuonStartVertex) return false;
  double trueVtxX, trueVtxY, trueVtxZ;
  Position vtxPos = fMuonStartVertex->GetPosition();
  Direction vtxDir = fMuonStartVertex->GetDirection();
  trueVtxX = vtxPos.X();
  trueVtxY = vtxPos.Y();
  trueVtxZ = vtxPos.Z();
  // fiducial volume cut
  double tankradius = ANNIEGeometry::Instance()->GetCylRadius();	
  double fidcutradius = 0.8 * tankradius;
  double fidcuty = 50.;
  double fidcutz = 0.;
  if(trueVtxZ > fidcutz) return false;
  if( (TMath::Sqrt(TMath::Power(trueVtxX, 2) + TMath::Power(trueVtxZ,2)) > fidcutradius) 
  	  || (TMath::Abs(trueVtxY) > fidcuty) 
  	  || (trueVtxZ > fidcutz) ){
  return false;
  }	
 
  return true;
}

bool EventSelector::EventSelectionByMCTruthMRD() {
  if(!fMuonStartVertex) return false;
  double trueVtxX, trueVtxY, trueVtxZ;
  Position vtxPos = fMuonStartVertex->GetPosition();
  Direction vtxDir = fMuonStartVertex->GetDirection();
  trueVtxX = vtxPos.X();
  trueVtxY = vtxPos.Y();
  trueVtxZ = vtxPos.Z();
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
    return false;	
  }
  return true;
}

void EventSelector::Reset() {
  // Reset 
  fMuonStartVertex->Reset();
  fMuonStopVertex->Reset();
  fEventCutStatus = true; 
} 
