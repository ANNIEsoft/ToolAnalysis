#include "VtxPointPositionFinder.h"

VtxPointPositionFinder::VtxPointPositionFinder():Tool(){}
VtxPointPositionFinder::~VtxPointPositionFinder(){}

bool VtxPointPositionFinder::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  
  fUseTrueVertexAsSeed = false;
  
  /// Get the Tool configuration variables
	m_variables.Get("UseTrueVertexAsSeed",fUseTrueVertexAsSeed);
	m_variables.Get("verbosity", verbosity);
	
	/// Create Simple position and point position
	/// Note that the objects created by "new" must be added to the "RecoEvent" store. 
	/// The last tool SaveRecoEvent will delete these pointers and free the memory.
	/// If these pointers are not added to any store, the user has to delete the pointers
	/// and free the memory. 
	fSimplePosition = new RecoVertex();
	fPointPosition = new RecoVertex();

  return true;
}

bool VtxPointPositionFinder::Execute(){
	Log("===========================================================================================",v_debug,verbosity);
	
	Log("VtxPointPositionFinder Tool: Executing",v_debug,verbosity);
	
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
  	Log("VtxPointPositionFinder  Tool: Error retrieving RecoDigits,no digit from the RecoEvent!",v_error,verbosity); 
  	return false;
  }
  
  // Load digits to VertexGeometry
  VertexGeometry* myvtxgeo = VertexGeometry::Instance();
  //myvtxgeo->Clear();
  myvtxgeo->LoadDigits(fDigitList);
	// Do point position reconstruction using MC truth information
  if( fUseTrueVertexAsSeed ){
    Log("VtxPointPositionFinder Tool: Run vertex reconstruction using MC truth information",v_message,verbosity);
    // get truth vertex information 
    get_ok = m_data->Stores.at("RecoEvent")->Get("TrueVertex", fTrueVertex);
	  if(not get_ok){ 
		  Log("VtxPointPositionFinder Tool: Error retrieving TrueVertex from RecoEvent!",v_error,verbosity); 
		  return false; 
	  }
	  Position vtxPos = fTrueVertex->GetPosition();
	  // Direction vtxDir = fTrueVertex->GetDirection();
	  double vtxTime = fTrueVertex->GetTime();
	  // Set vertex seed
    RecoVertex* vtx = new RecoVertex();
    // Use true vertex information
	  vtx->SetVertex(vtxPos, vtxTime);
    //vtx->SetDirection(vtxDir);
    // return vertex
    fPointPosition  = (RecoVertex*)(this->FitPointPosition(vtx));
    // Push fitted vertex to RecoEvent store
    this->PushPointPosition(fPointPosition, true);
    delete vtx; vtx = 0;
  }
  
  else {
    	
  }

  return true;
}

bool VtxPointPositionFinder::Finalise(){
	// memory has to be freed in the Finalise() function
	delete fSimplePosition; fSimplePosition = 0;
	delete fPointPosition; fPointPosition = 0;
	if(verbosity>0) cout<<"VtxPointPositionFinder exitting"<<endl;
  return true;
}

/// fit point position 
/// Note that it's highly discourged to return a pointer a local stack object. 
/// You'll be ended up with potential memory leak issue. 
/// In the old framework ANNIEReco, I spent lots of time dealing with the memory
/// leak. 
/// This function returns such a pointer, which means the user has to free the memory
/// I don't like this type of function, but there are many functions like this in
/// the code originally from WCSimAnalysis. Now we just keep them as they are and 
/// we'll come back to fix this later.  (Jingbo Wang, Aug 24, 2018)
RecoVertex* VtxPointPositionFinder::FitPointPosition(RecoVertex* myVertex) {
	Log("DEBUG [VertexFinderOpt::FitPointPosition]", v_message, verbosity);
  //fit with Minuit
  MinuitOptimizer* myOptimizer = new MinuitOptimizer();
  myOptimizer->SetPrintLevel(1);
  VertexGeometry* myvtxgeo = VertexGeometry::Instance();
  myOptimizer->LoadVertexGeometry(myvtxgeo); //Load vertex geometry
  myOptimizer->LoadVertex(myVertex); //Load vertex seed
  myOptimizer->FitPointPositionWithMinuit(); //scan the point position in 4D space
  // Fitted vertex must be copied to a new vertex pointer that is created in this class 
  // Once the optimizer is deleted, the fitted vertex is lost. 
  // copy vertex to fPointPosition
  RecoVertex* newVertex = new RecoVertex();
  newVertex->CloneVertex(myOptimizer->GetFittedVertex());
  newVertex->SetFOM(myOptimizer->GetFittedVertex()->GetFOM(),1,1);
  // print vertex
  // ============
  if(verbosity >0) {
  std::cout << "  set point position: " << std::endl
  	        << "  status = "<<newVertex->GetStatus()<<std::endl
            <<"     (vx,vy,vz)=(" << newVertex->GetPosition().X() << "," << newVertex->GetPosition().Y() << "," << newVertex->GetPosition().Z() << ") " << std::endl
            << "      vtime=" << newVertex->GetTime() << " itr=" << newVertex->GetIterations() << " fom=" << newVertex->GetFOM() << std::endl;
  }
  delete myOptimizer; myOptimizer = 0;
  return newVertex;
}

// Add point position to RecoEvent store
void VtxPointPositionFinder::PushPointPosition(RecoVertex* vtx, bool savetodisk) {  
  // push vertex to RecoEvent store
  Log("VtxPointPositionFinder Tool: Push point position to the RecoEvent store",v_message,verbosity);
	m_data->Stores.at("RecoEvent")->Set("PointPosition", fPointPosition, savetodisk);  ///> RecoEvent store is responsible to free the memory
}

void VtxPointPositionFinder::Reset() {
	fSimplePosition->Reset();
	fPointPosition->Reset();
}
