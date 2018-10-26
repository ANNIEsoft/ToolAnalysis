#include "VtxExtendedVertexFinder.h"

VtxExtendedVertexFinder::VtxExtendedVertexFinder():Tool(){}


bool VtxExtendedVertexFinder::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  fUseTrueVertexAsSeed = false;
  /// Get the Tool configuration variables
	m_variables.Get("UseTrueVertexAsSeed",fUseTrueVertexAsSeed);
	m_variables.Get("verbosity", verbosity);
	
	/// Create extended vertex
	/// Note that the objects created by "new" must be added to the "RecoEvent" store. 
	/// The last tool SaveRecoEvent will delete these pointers and free the memory.
	/// If these pointers are not added to any store, the user has to delete the pointers
	/// and free the memory. 
	/// In this tool, the pointer is 
	fExtendedVertex = new RecoVertex();
  
  return true;
}

bool VtxExtendedVertexFinder::Execute(){
	Log("===========================================================================================",v_debug,verbosity);
	Log("VtxExtendedVertexFinder Tool: Executing",v_debug,verbosity);
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
	get_ok = m_data->Stores.at("RecoEvent")->Get("RecoDigit",fDigitList);  ///> Get digits from "RecoEvent" 
  if(not get_ok){
  	Log("VtxExtendedVertexFinder  Tool: Error retrieving RecoDigits,no digit from the RecoEvent!",v_error,verbosity); 
  	return false;
  }
	
	// Load digits to VertexGeometry
  VertexGeometry* myvtxgeo = VertexGeometry::Instance();
  myvtxgeo->LoadDigits(fDigitList);
	// Do extended vertex (muon track) reconstruction using MC truth information
	if( fUseTrueVertexAsSeed ){
    Log("VtxExtendedVertexFinder Tool: Run vertex reconstruction using MC truth information",v_message,verbosity);
    // get truth vertex information 
    auto get_truevtx = m_data->Stores.at("RecoEvent")->Get("TrueVertex", fTrueVertex);
	  if(!get_truevtx){ 
		  Log("VtxExtendedVertexFinder Tool: Error retrieving TrueVertex from RecoEvent!",v_error,verbosity); 
		  return false; 
	  }
    // return vertex
    fExtendedVertex  = (RecoVertex*)(this->FitExtendedVertex(fTrueVertex));
    // Push fitted vertex to RecoEvent store
    this->PushExtendedVertex(fExtendedVertex, true);
  }
  else {
    Log("VtxExtendedVertexFinder Tool: Run extended vertex reconstruction using point vertex",v_message,verbosity);
    // get point vertex
    RecoVertex* pointvertex = 0; 
    auto get_pointvertex = m_data->Stores.at("RecoEvent")->Get("PointVertex", pointvertex);
	  if(!get_pointvertex){ 
		  Log("VtxPointVertexFinder Tool: Error retrieving PointVertex from RecoEvent!",v_error,verbosity); 
		  return false; 
	  }
    // return vertex
    fExtendedVertex  = (RecoVertex*)(this->FitExtendedVertex(pointvertex));
    // Push fitted vertex to RecoEvent store
    this->PushExtendedVertex(fExtendedVertex, true);    	
  }
  return true;
}


bool VtxExtendedVertexFinder::Finalise(){
	// memory has to be freed in the Finalise() function
	delete fExtendedVertex; fExtendedVertex = 0;
	if(verbosity>0) cout<<"VtxExtendedVertexFinder exitting"<<endl;
  return true;
}

RecoVertex* VtxExtendedVertexFinder::FitExtendedVertex(RecoVertex* myVertex) {
  //fit with Minuit
  MinuitOptimizer* myOptimizer = new MinuitOptimizer();
  myOptimizer->SetPrintLevel(0);
  myOptimizer->SetMeanTimeCalculatorType(1); //Type 1: most probable time
  VertexGeometry* myvtxgeo = VertexGeometry::Instance();
  myvtxgeo->LoadDigits(fDigitList);
  myOptimizer->LoadVertexGeometry(myvtxgeo); //Load vertex geometry
  myOptimizer->LoadVertex(myVertex); //Load vertex seed
  myOptimizer->FitExtendedVertexWithMinuit(); //scan the point position in 4D space
  // Fitted vertex must be copied to a new vertex pointer that is created in this class 
  // Once the optimizer is deleted, the fitted vertex is lost. 
  // copy vertex to fExtendedVertex
  RecoVertex* newVertex = new RecoVertex();
  newVertex->CloneVertex(myOptimizer->GetFittedVertex());
  //newVertex->SetFOM(myOptimizer->GetFittedVertex()->GetFOM(),1,1);
  // print vertex
  // ============
  if(verbosity >0) {
  std::cout << "  set extended vertex: " << std::endl
  	        << "  status = "<<newVertex->GetStatus()<<std::endl
            << "     (vx,vy,vz)=(" << newVertex->GetPosition().X() << "," << newVertex->GetPosition().Y() << "," << newVertex->GetPosition().Z() << ") " << std::endl
            << "     vtime=" << newVertex->GetTime() << " itr=" << newVertex->GetIterations() << " fom=" << newVertex->GetFOM() << std::endl;
  }
  delete myOptimizer; myOptimizer = 0;
  return newVertex;
}

// Add extended vertex to RecoEvent store
void VtxExtendedVertexFinder::PushExtendedVertex(RecoVertex* vtx, bool savetodisk) {  
  // push vertex to RecoEvent store
  Log("VtxExtendedVertexFinder Tool: Push extended vertex to the RecoEvent store",v_message,verbosity);
	m_data->Stores.at("RecoEvent")->Set("ExtendedVertex", fExtendedVertex, savetodisk);
}

void VtxExtendedVertexFinder::Reset() {
	fExtendedVertex->Reset();
}
