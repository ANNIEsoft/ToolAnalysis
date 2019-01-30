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
  fUseMinuit = true;
  
  /// Get the Tool configuration variables
	m_variables.Get("UseTrueVertexAsSeed",fUseTrueVertexAsSeed);
	m_variables.Get("UseMinuitForPos",fUseMinuit);
	m_variables.Get("verbosity", verbosity);
	
	/// Create Simple position and point position
	/// Note that the objects created by "new" must be added to the "RecoEvent" store. 
	/// The last tool SaveRecoEvent will delete these pointers and free the memory.
	/// If these pointers are not added to any store, the user has to delete the pointers
	/// and free the memory. 
	fSimplePosition = new RecoVertex();
	fPointPosition = new RecoVertex();

        // Now Create the list of Seed FOMs
        vSeedFOMList = new std::vector<double>;
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
    // return vertex
    if (fUseMinuit){
      fPointPosition  = (RecoVertex*)(this->FitPointPosition(fTrueVertex));
    } else {
      Log("VtxPointPositionFinder Tool: You've chosen to use MC truth information but not use Minuit for position fit... returning True MC vertex as Point Position",v_message,verbosity);
    RecoVertex* newVertex = new RecoVertex();
    newVertex->CloneVertex(fTrueVertex);
    fPointPosition = (RecoVertex*)(newVertex);
    this->PushPointPosition(fPointPosition, true);
    }
  
  // Do point position reconstruction using Seeds
  } else {
  	// Get vertex seed candidates from the store
  	std::vector<RecoVertex>* vSeedVtxList = 0;
  	auto get_seedlist = m_data->Stores.at("RecoEvent")->Get("vSeedVtxList", vSeedVtxList);
  	if(!get_seedlist){ 
		  Log("VtxPointPositionFinder Tool: Error retrieving vertex seeds from RecoEvent!",v_error,verbosity);
		  Log("VtxPointPositionFinder Tool: Needs to run VtxSeedGenerator first!",v_error,verbosity);
		  return false;
	  }
  	// Loop over all the seeds and find the best one
  	fSimplePosition = this->FindSimplePosition(vSeedVtxList);
  	// Push selected simple position to RecoEvent store
  	this->PushSimplePosition(fSimplePosition, true);
    
    // If sing Minuit, Use the found simple position as the seed
    if (fUseMinuit){ 
      fPointPosition  = (RecoVertex*)(this->FitPointPosition(fSimplePosition));
      this->PushPointPosition(fPointPosition, true);	
    } else { 
      //Simple position is the point position seed for pointVtx
      RecoVertex* newVertex = new RecoVertex();
      newVertex->CloneVertex(fSimplePosition);
      fPointPosition = (RecoVertex*)(newVertex);
      this->PushPointPosition(fPointPosition, true);
    }
  }

  return true;
}

bool VtxPointPositionFinder::Finalise(){
	// memory has to be freed in the Finalise() function
	delete fSimplePosition; fSimplePosition = 0;
	delete fPointPosition; fPointPosition = 0;
	delete vSeedFOMList; vSeedFOMList = 0;
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
  //fit with Minuit
  MinuitOptimizer* myOptimizer = new MinuitOptimizer();
  myOptimizer->SetPrintLevel(0);
  myOptimizer->SetMeanTimeCalculatorType(1); //
  VertexGeometry* myvtxgeo = VertexGeometry::Instance();
  myOptimizer->LoadVertexGeometry(myvtxgeo); //Load vertex geometry
  myOptimizer->LoadVertex(myVertex); //Load vertex seed
  myOptimizer->FitPointPositionWithMinuit(); //scan the point position in 4D space
  // Fitted vertex must be copied to a new vertex pointer that is created in this class 
  // Once the optimizer is deleted, the fitted vertex is lost. 
  // copy vertex to fPointPosition
  RecoVertex* newVertex = new RecoVertex();
  newVertex->CloneVertex(myOptimizer->GetFittedVertex());
  //newVertex->SetFOM(myOptimizer->GetFittedVertex()->GetFOM(),1,1);
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

RecoVertex* VtxPointPositionFinder::FindSimplePosition(std::vector<RecoVertex>* vSeedVtxList) {
  double vtxX = -999, vtxY = -999, vtxZ = -999;
  double vtxTime = 0.0;
  double vtxFOM = 0.0;
  double bestFOM = -1.0;
  unsigned int nlast = vSeedVtxList->size();
  
  //Find best time with Minuit
  MinuitOptimizer* myOptimizer = new MinuitOptimizer();
  myOptimizer->SetPrintLevel(0);
  myOptimizer->SetMeanTimeCalculatorType(1);
  VertexGeometry* myvtxgeo = VertexGeometry::Instance();
  myOptimizer->LoadVertexGeometry(myvtxgeo); //Load vertex geometry
  RecoVertex* vSeed = 0;
  RecoVertex* newVertex = new RecoVertex(); // Note: pointer must be deleted by the invoker
  
  for( unsigned int n=0; n<nlast; n++ ){
  	vSeed = &(vSeedVtxList->at(n));
    myOptimizer->LoadVertex(vSeed); //Load vertex seed
    myOptimizer->FitPointTimeWithMinuit(); //scan time for fixed position
    vtxFOM = myOptimizer->GetFittedVertex()->GetFOM();
    vSeedFOMList->push_back(vtxFOM);
    if( vtxFOM>bestFOM ){
      newVertex->CloneVertex(myOptimizer->GetFittedVertex());
      bestFOM = vtxFOM;
    }
  }
  //Push the vSeedFOMList to the Store
  this->PushVertexSeedFOMList(true);
  if(verbosity>0 && newVertex->GetPass()==1) {
    std::cout << "  simple position: " << std::endl
              << "    (vx,vy,vz)=(" << newVertex->GetPosition().X() << "," << newVertex->GetPosition().Y() << "," << newVertex->GetPosition().Z() << ") " << std::endl
              << "      vtime=" << newVertex->GetTime() << " itr=" << newVertex->GetIterations() << " fom=" << newVertex->GetFOM() << std::endl;
  }
  if( newVertex->GetPass()==0 ) Log("   <warning> simple position calculation failed! ", v_warning, verbosity);
	delete myOptimizer; myOptimizer = 0;
	return newVertex;
}

void VtxPointPositionFinder::Reset() {
	fSimplePosition->Reset();
	fPointPosition->Reset();
	vSeedFOMList->clear();
}

// Add simple position to RecoEvent store
void VtxPointPositionFinder::PushSimplePosition(RecoVertex* thevtx, bool savetodisk) {  
  // push vertex to RecoEvent store
  Log("VtxPointPositionFinder Tool: Push simple position to the RecoEvent store",v_message,verbosity);
	m_data->Stores.at("RecoEvent")->Set("SimplePosition", thevtx, savetodisk);  ///> RecoEvent store is responsible to free the memory
}

// Add point position to RecoEvent store
void VtxPointPositionFinder::PushPointPosition(RecoVertex* thevtx, bool savetodisk) {  
  // push vertex to RecoEvent store
  Log("VtxPointPositionFinder Tool: Push point position to the RecoEvent store",v_message,verbosity);
	m_data->Stores.at("RecoEvent")->Set("PointPosition", thevtx, savetodisk);  ///> RecoEvent store is responsible to free the memory
}

void VtxPointPositionFinder::PushVertexSeedFOMList(bool savetodisk) {
  m_data->Stores.at("RecoEvent")->Set("vSeedFOMList", vSeedFOMList, savetodisk); 
}
