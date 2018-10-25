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
  m_variables.Get("MCTruthCut", fMCTruthCut);

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
  m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum",fMCTriggerNum); 
  
  // ANNIE Event number
  m_data->Stores.at("ANNIEEvent")->Get("EventNumber",fEventNumber);
  
  std::string logmessage = "EventSelector Tool: Processing MCEntry "+to_string(fMCEventNum)+
  	", MCTrigger "+to_string(fMCTriggerNum) + ", Event "+to_string(fEventNumber);
	Log(logmessage,v_message,verbosity);
  
  auto get_geometry= m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",fGeometry);
	if(!get_geometry){
		Log("EventSelector Tool: Error retrieving Geometry from ANNIEEvent!",v_error,verbosity); 
		return false; 
	}

	// Retrieve particle information from ANNIEEvent
  auto get_mcparticles = m_data->Stores.at("ANNIEEvent")->Get("MCParticles",fMCParticles);
	if(!get_mcparticles){ 
		Log("EventSelector:: Tool: Error retrieving MCParticles from ANNIEEvent!",v_error,verbosity); 
		return false; 
	}
	
  /// Find true neutrino vertex which is defined by the start point of the Primary muon
  this->FindTrueVertexFromMC();
  
  if(fMCTruthCut){
    bool passMCTruth= this->EventSelectionByMCTruthInfo();
    if(!passMCTruth) fEventCutStatus = false; 
  }
  
  //FIXME: This isn't working according to Jingbo
  if(fMRDRecoCut){
    std::cout << "EventSelector Tool: Currently not implemented. Setting to false" << std::endl;
    Log("EventSelector Tool: MRDReco not implemented.  Setting cut bit to false",v_message,verbosity);
    bool passMRDRecoCut = false;
    //bool passMRDRecoCut = this->EventSelectionByMRDReco(); 
    if(!passMRDRecoCut) fEventCutStatus = false; 
  }
  // Event selection successfully run!
  //If event passes cuts, store truth vertex info.
  if(fEventCutStatus) this->PushTrueVertex(true);

  m_data->Stores.at("RecoEvent")->Set("EventCutStatus", fEventCutStatus);
  return true;
}


bool EventSelector::Finalise(){
  delete fMuonStartVertex;
  delete fMuonStopVertex;
  if(verbosity>0) cout<<"EventSelector exitting"<<endl;
  return true;
}

RecoVertex* EventSelector::FindTrueVertexFromMC() {
  
  // loop over the MCParticles to find the highest enery primary muon
  // MCParticles is a std::vector<MCParticle>
  MCParticle primarymuon;  // primary muon
  bool mufound=false;
  if(fMCParticles){
    Log("EventSelector::  Tool: Num MCParticles = "+to_string(fMCParticles->size()),v_message,verbosity);
    for(int particlei=0; particlei<fMCParticles->size(); particlei++){
      MCParticle aparticle = fMCParticles->at(particlei);
      //if(v_debug<verbosity) aparticle.Print();       // print if we're being *really* verbose
      if(aparticle.GetParentPdg()!=0) continue;      // not a primary particle
      if(aparticle.GetPdgCode()!=13) continue;       // not a muon
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
  double muonstarttime = primarymuon.GetStartTime().GetNs() + primarymuon.GetStopTime().GetTpsec()/1000 ;
  Position muonstoppos = primarymuon.GetStopVertex();    // only true if the muon is primary
  double muonstoptime = primarymuon.GetStopTime().GetNs() + primarymuon.GetStopTime().GetTpsec()/1000;
  
  Direction muondirection = primarymuon.GetStartDirection();
  double muonenergy = primarymuon.GetStartEnergy();
  // set true vertex
  // change unit
  muonstartpos.UnitToCentimeter(); // convert unit from meter to centimeter
  muonstoppos.UnitToCentimeter(); // convert unit from meter to centimeter
  // change coordinate for muon start vertex
  muonstartpos.SetY(muonstartpos.Y()+14.46469);
  muonstartpos.SetZ(muonstartpos.Z()-168.1);
  fMuonStartVertex->SetVertex(muonstartpos, muonstarttime);
  fMuonStartVertex->SetDirection(muondirection);
  //  charge coordinate for muon stop vertex
  muonstoppos.SetY(muonstoppos.Y()+14.46469);
  muonstoppos.SetZ(muonstoppos.Z()-168.1);
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


bool EventSelector::EventSelectionByMCTruthInfo() {
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
