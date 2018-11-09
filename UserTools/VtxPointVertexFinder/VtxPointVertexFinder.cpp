#include "VtxPointVertexFinder.h"

VtxPointVertexFinder::VtxPointVertexFinder():Tool(){}


bool VtxPointVertexFinder::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
	fUseTrueVertexAsSeed = false;
  
  /// Get the Tool configuration variables
	m_variables.Get("UseTrueVertexAsSeed",fUseTrueVertexAsSeed);
	m_variables.Get("verbosity", verbosity);
	
	/// The pointer has to be deleted after usage
	fPointVertex = new RecoVertex();
	
  return true;
}


bool VtxPointVertexFinder::Execute(){
	Log("===========================================================================================",v_debug,verbosity);
	Log("VtxPointVertexFinder Tool: Executing",v_debug,verbosity);
  // Reset everything
	this->Reset();
	// check if event passes the cut
  bool EventCutstatus = false;
  auto get_evtstatus = m_data->Stores.at("RecoEvent")->Get("EventCutStatus",EventCutstatus);
  if(!get_evtstatus) {
    Log("Error: The PhaseITreeMaker tool could not find the Event selection status", v_error, verbosity);
    return false;	
  }

  if(!EventCutstatus) {
  	Log("Message: This event doesn't pass the event selection. ", v_message, verbosity);
    return true;	
  }
  
  // MC entry number
  m_data->Stores.at("ANNIEEvent")->Get("MCEventNum",fMCEventNum);  
  
  // MC trigger number
  m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum",fMCTriggerNum); 
  
  // ANNIE Event number
  m_data->Stores.at("ANNIEEvent")->Get("EventNumber",fEventNumber);
	
	// Retrive digits from RecoEvent
	auto get_recodigit = m_data->Stores.at("RecoEvent")->Get("RecoDigit",fDigitList);  ///> Get digits from "RecoEvent" 
  if(!get_recodigit){
  	Log("VtxPointVertexFinder  Tool: Error retrieving RecoDigits,no digit from the RecoEvent!",v_error,verbosity); 
  	return false;
  }
	
	// Load digits to VertexGeometry
  VertexGeometry* myvtxgeo = VertexGeometry::Instance();
  myvtxgeo->LoadDigits(fDigitList);
	// Do extended vertex (muon track) reconstruction using MC truth information
	if( fUseTrueVertexAsSeed ){
    Log("VtxPointVertexFinder Tool: Run direction reconstruction using MC truth information",v_message,verbosity);
    // get truth vertex information 
    auto get_truevertex = m_data->Stores.at("RecoEvent")->Get("TrueVertex", fTrueVertex);
	  if(!get_truevertex){ 
		  Log("VtxPointVertexFinder Tool: Error retrieving TrueVertex from RecoEvent!",v_error,verbosity); 
		  return false; 
	  }
    // return vertex
    fPointVertex  = (RecoVertex*)(this->FitPointVertex(fTrueVertex));
    // Push fitted vertex to RecoEvent store
    this->PushPointVertex(fPointVertex, true);
  }
  else {
  	Log("VtxPointVertexFinder Tool: Run point vertex reconstruction using point direction",v_message,verbosity);
    // get point position
    RecoVertex* pointdirection = 0; 
    auto get_pointdirection = m_data->Stores.at("RecoEvent")->Get("PointDirection", pointdirection);
	  if(!get_pointdirection){ 
		  Log("VtxPointVertexFinder Tool: Error retrieving PointDirection from RecoEvent!",v_error,verbosity); 
		  return false; 
	  }
    // return vertex
    fPointVertex  = (RecoVertex*)(this->FitPointVertex(pointdirection));
    // Push fitted vertex to RecoEvent store
    this->PushPointVertex(fPointVertex, true);    	
  }
  return true;
}


bool VtxPointVertexFinder::Finalise(){

  /// memory has to be freed in the Finalise() function
	/// If the pointer is not delected here, it can be delected in the last tool in the tool chain,
	/// for example the SaveRecoEvent tool
	delete fPointVertex; fPointVertex = 0;
	if(verbosity>0) cout<<"VtxPointVertexFinder exitting"<<endl;
  return true;
}

RecoVertex* VtxPointVertexFinder::FitPointVertex(RecoVertex* myVertex) {
  //fit with Minuit
  MinuitOptimizer* myOptimizer = new MinuitOptimizer();
  myOptimizer->SetPrintLevel(0);
  myOptimizer->SetMeanTimeCalculatorType(1); //Type 1: most probable time
  VertexGeometry* myvtxgeo = VertexGeometry::Instance();
  myOptimizer->LoadVertexGeometry(myvtxgeo); //Load vertex geometry
  myOptimizer->LoadVertex(myVertex); //Load vertex seed
  myOptimizer->FitPointVertexWithMinuit(); //scan the point position in 4D space
  // Fitted vertex must be copied to a new vertex pointer that is created in this class 
  // Once the optimizer is deleted, the fitted vertex is lost. 
  // copy vertex to fPointVertex
  RecoVertex* newVertex = new RecoVertex();
  newVertex->CloneVertex(myOptimizer->GetFittedVertex());
  // print vertex
  // ============
  if(verbosity >0) {
  std::cout << "  set point vertex: " << std::endl
  	        << "  status = "<<newVertex->GetStatus()<<std::endl
            << "    (vx,vy,vz)=(" << newVertex->GetPosition().X() << "," << newVertex->GetPosition().Y() << "," << newVertex->GetPosition().Z() << ") " << std::endl
            << "    (px,py,pz)=(" << newVertex->GetDirection().X() << "," << newVertex->GetDirection().Y() << "," << newVertex->GetDirection().Z() << ") " << std::endl
            << "      angle=" << newVertex->GetConeAngle() << " vtime=" << newVertex->GetTime() << " itr=" << newVertex->GetIterations() << " fom=" << newVertex->GetFOM() << std::endl;
  }
  delete myOptimizer; myOptimizer = 0;
  return newVertex;
}

// Add extended vertex to RecoEvent store
void VtxPointVertexFinder::PushPointVertex(RecoVertex* vtx, bool savetodisk) {  
  // push vertex to RecoEvent store
  Log("VtxPointVertexFinder Tool: Push extended vertex to the RecoEvent store",v_message,verbosity);
	m_data->Stores.at("RecoEvent")->Set("PointVertex", fPointVertex, savetodisk);
}

void VtxPointVertexFinder::Reset() {
	fPointVertex->Reset();
}
