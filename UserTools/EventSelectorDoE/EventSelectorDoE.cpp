#include "EventSelectorDoE.h"

EventSelectorDoE::EventSelectorDoE():Tool(){}


bool EventSelectorDoE::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  //Get the tool configuration variables
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("MCTruthCut", fMCTruthCut);
  m_variables.Get("PromptTrigOnly", fPromptTrigOnly);

  /// Construct the other objects we'll be setting at event level,
  fMuonVertex = new RecoVertex();
  
  // Make the RecoDigit Store if it doesn't exist
  int recoeventexists = m_data->Stores.count("RecoEvent");
  if(recoeventexists==0) m_data->Stores["RecoEvent"] = new BoostStore(false,2);
  //TODO: Have an event selection mask filled based on what cuts are run
  //m_data->Stores.at("RecoEvent")->Set("EvtSelectionMask", fEvtSelectionMask); 
  return true;
}


bool EventSelectorDoE::Execute(){
  // Reset everything
  this->Reset();
  
  // see if "ANNIEEvent" exists
  auto get_annieevent = m_data->Stores.count("ANNIEEvent");
  if(!get_annieevent){
  	Log("EventSelectorDoE Tool: No ANNIEEvent store!",v_error,verbosity); 
  	return false;
  };

  // see if "RecoEvent" exists
  auto get_recoevent = m_data->Stores.count("RecoEvent");
  if(!get_recoevent){
  	Log("EventSelectorDoE Tool: No RecoEvent store!",v_error,verbosity); 
  	return false;
  };
  
  // MC entry number
  m_data->Stores.at("ANNIEEvent")->Get("MCEventNum",fMCEventNum);  
  
  // MC trigger number
  m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum",fMCTriggernum); 
  
  //Track length in MRD
  m_data->Stores.at("RecoEvent")->Get("TrackLengthInMRD",fTrackLengthInMrd); 

  // ANNIE Event number
  m_data->Stores.at("ANNIEEvent")->Get("EventNumber",fEventNumber);
 
  auto get_truevtx = m_data->Stores.at("RecoEvent")->Get("TrueVertex", fMuonVertex);
	if(!get_truevtx){ 
		Log("EventSelectorDoE Tool: Error retrieving TrueVertex from RecoEvent!",v_error,verbosity); 
		return false; 
	}
  
  std::string logmessage = "EventSelectorDoE Tool: Processing MCEntry "+to_string(fMCEventNum)+
  	", MCTrigger "+to_string(fMCTriggernum) + ", Event "+to_string(fEventNumber);
	Log(logmessage,v_message,verbosity);
  

  
  if(fMCTruthCut){
    bool passMCTruth= this->EventSelectionByMCTruthInfo();
    if(!passMCTruth) fEventCutStatus = false; 
  }
  if(fPromptTrigOnly){
    bool isPromptTrigger= this->PromptTriggerCheck();
    if(!isPromptTrigger) fEventCutStatus = false; 
  }
  
  // Event selection successfully run!
  //Push the EventCutStatus, which determines if reco is run.
  m_data->Stores.at("RecoEvent")->Set("EventCutStatus", fEventCutStatus);
  return true;
}


bool EventSelectorDoE::Finalise(){
  if(verbosity>0) cout<<"EventSelectorDoE exitting"<<endl;
  delete fMuonVertex;
  return true;
}

bool EventSelectorDoE::PromptTriggerCheck() {
  /// First, see if this is a delayed trigger in the event
  auto get_mctrigger = m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum",fMCTriggernum);
  if(!get_mctrigger){ 
    Log("EventSelectorDoE Tool: Error retrieving MCTriggernum from ANNIEEvent!",v_error,verbosity); 
    return false; 
  }	
  /// if so, truth analysis is probably not interested in this trigger. Primary muon will not be in the listed tracks.
  if(fMCTriggernum>0){ 
    Log("EventSelectorDoE Tool: This event is not a prompt trigger",v_debug,verbosity); 
    return false;
  }
  return true;
}
	

bool EventSelectorDoE::EventSelectionByMCTruthInfo() {
  if(!fMuonVertex) return false;
  double trueVtxX, trueVtxY, trueVtxZ;
  Position vtxPos = fMuonVertex->GetPosition();
  Direction vtxDir = fMuonVertex->GetDirection();
  trueVtxX = vtxPos.X();
  trueVtxY = vtxPos.Y();
  trueVtxZ = vtxPos.Z();
  // fiducial volume cut
  double tankradius = ANNIEGeometry::Instance()->GetCylRadius();	
  double fidcutradius = 0.8 * tankradius;
  double fidcuty = 50.;
  double fidcutz = 0.;
  if(trueVtxZ > fidcutz) return false;

  std::cout << "MADE IT PAST THE ZCUT" << std::endl;
  std::cout << "MUON INFO: X," << trueVtxX << " Y, " << trueVtxY << " Z, " << trueVtxZ << std::endl;
  if( (TMath::Sqrt(TMath::Power(trueVtxX, 2) + TMath::Power(trueVtxZ,2)) > fidcutradius) 
  	  || (TMath::Abs(trueVtxY) > fidcuty)){
    return false;
  }	
  std::cout << "MADE IT PAST THE FIDUCIAL VOLUME CUT" << std::endl; 
  //mrd cut
  if (fTrackLengthInMrd == 0){
    return false;
  }
  return true;
}

void EventSelectorDoE::Reset() {
  // Reset 
  fEventCutStatus = true; 
} 
