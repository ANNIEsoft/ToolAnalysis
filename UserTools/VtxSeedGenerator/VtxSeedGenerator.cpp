#include "VtxSeedGenerator.h"

VtxSeedGenerator::VtxSeedGenerator():Tool(){}


bool VtxSeedGenerator::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(verbosity) cout<<"Initializing Tool VtxSeedGenerator"<<endl;
  // Initialise variables

  fNumSeeds = 500;
  fThisDigit = 0;
  fLastEntry = 0; 
  fCounter = 0;
  fSeedType = RecoDigit::All;	 	
  	
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  
  /// Get the Tool configuration variables
	m_variables.Get("SeedType",fSeedType);
	m_variables.Get("NumberOfSeeds", fNumSeeds);
	m_variables.Get("verbosity", verbosity);
  
  // Make the ANNIEEvent Store if it doesn't exist
	// =============================================
	int recoeventexists = m_data->Stores.count("RecoEvent");
	if(recoeventexists==0){
		cerr<<"VtxSeedGenerator tool need to run after DigitBuilder tool!"<<endl;
		return false; 
	}
		
	// Create an object to store the true MC neutrino vertex
	fTrueVertex = new RecoVertex();
	vSeedVtxList = new std::vector<RecoVertex>;
		
  return true;
}

bool VtxSeedGenerator::Execute(){
	Log("===========================================================================================",v_debug,verbosity);
	
	Log("VtxSeedGenerator Tool: Executing",v_debug,verbosity);
	
	get_ok = m_data->Stores.count("RecoEvent");
	if(!get_ok){
		Log("VtxSeedGenerator Tool: No RecoVertex store!",v_error,verbosity);
		return false;
	};
	
	// First, see if this is a delayed trigger in the event
	get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum",fMCTriggernum);
	if(not get_ok){ Log("VtxSeedGenerator  Tool: Error retrieving MCTriggernum from ANNIEEvent!",v_error,verbosity); return false; }
	// if so, truth analysis is probablys not interested in this trigger. Primary muon will not be in the listed tracks.
	if(fMCTriggernum>0){ 
		Log("VtxSeedGenerator s Tool: Skipping delayed trigger",v_debug,verbosity); 
		return true;
	}
		
	// Load digits
	get_ok = m_data->Stores.at("RecoEvent")->Get("RecoDigit",fDigitList);  ///> Get digits from "RecoEvent" 
  if(not get_ok){ 
  	Log("VtxSeedGenerator  Tool: Error retrieving RecoDigits,no digit from the RecoEvent store!",v_error,verbosity); 
  	return true;
  }
 
	// Reset
	fTrueVertex->Reset();
	// Find true neutrino vertex and push to "RecoEvent" store
  this->FindTrueVertex();
  this->PushTrueVertex(true);
  // Generate vertex candidates and push to "RecoEvent" store
  this->GenerateVertexSeeds(fNumSeeds);
  this->PushVertexSeeds(true);
  
  return true;
}

bool VtxSeedGenerator::Finalise(){

  return true;
}

RecoVertex* VtxSeedGenerator::FindTrueVertex() {
  // Retrieve particle information from ANNIEEvent
  get_ok = m_data->Stores.at("ANNIEEvent")->Get("MCParticles",fMCParticles);
	if(not get_ok){ Log("VtxSeedGenerator Tool: Error retrieving MCParticles,true tracks from ANNIEEvent!",v_error,verbosity); return 0; }
	
	// loop over the MCParticles to find the highest enery primary muon
	// MCParticles is a std::vector<MCParticle>
	MCParticle primarymuon;  // primary muon
	bool mufound=false;
	if(fMCParticles){
		Log("VtxSeedGenerator  Tool: Num MCParticles = "+to_string(fMCParticles->size()),v_message,verbosity);
		for(int particlei=0; particlei<fMCParticles->size(); particlei++){
			MCParticle aparticle = fMCParticles->at(particlei);
			//if(v_debug<verbosity) aparticle.Print();       // print if we're being *really* verbose
			if(aparticle.GetParentPdg()!=0) continue;      // not a primary particle
			if(aparticle.GetPdgCode()!=13) continue;       // not a muon
			primarymuon = aparticle;                       // note the particle
			mufound=true;                                  // note that we found it
			break;                                         // won't have more than one primary muon
		}
	} else {
		Log("VtxSeedGenerator  Tool: No MCParticles in the event!",v_error,verbosity);
	}
	if(not mufound){
		Log("VtxSeedGenerator  Tool: No muon in this event",v_warning,verbosity);
		return 0;
	}
	
	// retrieve desired information from the particle
	const Position neutrinovtx = primarymuon.GetStartVertex();    // only true if the muon is primary
	const Direction muondirection = primarymuon.GetStartDirection();
	double muonenergy = primarymuon.GetStartEnergy();
	logmessage = "VtxSeedGenerator  Tool: Interaction Vertex is at ("+to_string(neutrinovtx.X())
		+", "+to_string(neutrinovtx.Y())+", "+to_string(neutrinovtx.Z())+")\n"
		+"Primary muon has energy "+to_string(muonenergy)+"GeV and direction ("
		+to_string(muondirection.X())+", "+to_string(muondirection.Y())+", "+to_string(muondirection.Z())+")";
	Log(logmessage,v_debug,verbosity);
	// set true vertex
	fTrueVertex->SetVertex(neutrinovtx);
  fTrueVertex->SetDirection(muondirection);
  return fTrueVertex;
}

void VtxSeedGenerator::Reset() {
  fTrueVertex->Reset();
}

bool VtxSeedGenerator::GenerateVertexSeeds(int NSeeds) {
	double VtxX1 = 0.0;
  double VtxY1 = 0.0;
  double VtxZ1 = 0.0;
  double VtxTime1 = 0.0;
  double VtxX2 = 0.0;
  double VtxY2 = 0.0;
  double VtxZ2 = 0.0;
  double VtxTime2 = 0.0;
	// reset list of seeds
  vSeedVtxList->clear();
  // always calculate the simple vertex first
  this->CalcSimpleVertex(VtxX1,VtxY1,VtxZ1,VtxTime1);
  
  // add this vertex to seed list
  RecoVertex vtxseed;
  vtxseed.SetVertex(VtxX1,VtxY1,VtxZ1,VtxTime1);
  vSeedVtxList->push_back(vtxseed);
  // check limit
  if( NSeeds<=1 ) return false;
  
  // form list of golden digits
  // Digit quality cut can be applied here. 
  // However, the DataCleaner class is a better place to do the cuts. 
  // Here only the digit type (PMT or LAPPD or all) is specified. 
  vSeedDigitList.clear();  
  RecoDigit digit;
  for( fThisDigit=0; fThisDigit<fDigitList->size(); fThisDigit++ ){
  	digit = fDigitList->at(fThisDigit);
    if( digit.GetFilterStatus() ){ 
      if( digit.GetDigitType() == fSeedType){ 
        vSeedDigitList.push_back(fThisDigit);
      }
    }
  }
  // check if the digits are enough to calculate the vertex
  if( vSeedDigitList.size()<=4 ) {
    Log("VtxSeedGenerator Tool: The number of digits is less than 4! ",v_message,verbosity);
    return false;
  }
  	
  // generate a new list of seeds
  double x0 = 0.0;
  double y0 = 0.0;
  double z0 = 0.0;
  double t0 = 0.0;

  double x1 = 0.0;
  double y1 = 0.0;
  double z1 = 0.0;
  double t1 = 0.0; 

  double x2 = 0.0;
  double y2 = 0.0;
  double z2 = 0.0;
  double t2 = 0.0;

  double x3 = 0.0;
  double y3 = 0.0;
  double z3 = 0.0;
  double t3 = 0.0;
  
  unsigned int counter = 0;
  unsigned int NSeedsTarget = NSeeds;
  
  while( vSeedVtxList->size()<NSeedsTarget && counter<100*NSeedsTarget ){
    counter++;

    // choose next four digits
    this->ChooseNextQuadruple(x0,y0,z0,t0,
                              x1,y1,z1,t1,
                              x2,y2,z2,t2,
                              x3,y3,z3,t3);
		
    // find common vertex
    ANNIEGeometry::FindVertex(x0,y0,z0,t0,
                              x1,y1,z1,t1,
                              x2,y2,z2,t2,
                              x3,y3,z3,t3,
                              VtxX1,VtxY1,VtxZ1,VtxTime1,
                              VtxX2,VtxY2,VtxZ2,VtxTime2);

//    std::cout << "   result: (x,y,z,t)=(" << VtxX1 << "," << VtxY1 << "," << VtxZ1 << "," << VtxTime1 << ") " << std::endl
//              << "   result: (x,y,z,t)=(" << VtxX2 << "," << VtxY2 << "," << VtxZ2 << "," << VtxTime2 << ") " << std::endl;
//    std::cout << std::endl;

    if(VtxX1==-99999.9 && VtxX2==-99999.9) continue;
    bool inside_det;

    // add first digit
    if( ANNIEGeometry::Instance()->InsideDetector(VtxX1,VtxY1,VtxZ1) ){
      vtxseed.SetVertex(VtxX1,VtxY1,VtxZ1,VtxTime1);
      vSeedVtxList->push_back(vtxseed);
    }
    
    // add second digit
    if( ANNIEGeometry::Instance()->InsideDetector(VtxX2,VtxY2,VtxZ2) ){
			vtxseed.SetVertex(VtxX2,VtxY2,VtxZ2,VtxTime2);
      vSeedVtxList->push_back(vtxseed);
    }
  }
  this->PushVertexSeeds(true);
  return true;
}

void VtxSeedGenerator::CalcSimpleVertex(double& vtxX, double& vtxY, double& vtxZ, double& vtxTime)
{
  // default vertex
  vtxX = 0.0;
  vtxY = 0.0;
  vtxZ = 0.0;
  vtxTime = 0.0;

  // loop over digits
  double Swx = 0.0;
  double Swy = 0.0;
  double Swz = 0.0;
  double Swt = 0.0;
  double Sw = 0.0;
  double digitq = 0.;
  
  int NDigits = fDigitList->size();
  RecoDigit digit;
  for( int idigit=0; idigit<NDigits; idigit++ ){
  	digit = fDigitList->at(idigit);
  	digitq = digit.GetCalCharge();
    if( digit.GetFilterStatus() ){
      Swx += digitq*digit.GetPosition().X();   
      Swy += digitq*digit.GetPosition().Y();
      Swz += digitq*digit.GetPosition().Z();
      Swt += digitq*digit.GetCalTime();
      Sw  += digitq;
    }
  }
  if( Sw>0.0 ){
    vtxX = Swx/Sw;
    vtxY = Swy/Sw;
    vtxZ = Swz/Sw;
    vtxTime = Swt/Sw;
  }   
  return;
}

void VtxSeedGenerator::ChooseNextQuadruple(double& x0, double& y0, double& z0, double& t0, double& x1, double& y1, double& z1, double& t1, double& x2, double& y2, double& z2, double& t2, double& x3, double& y3, double& z3, double& t3) {
  this->ChooseNextDigit(x0,y0,z0,t0);
  this->ChooseNextDigit(x1,y1,z1,t1);
  this->ChooseNextDigit(x2,y2,z2,t2);
  this->ChooseNextDigit(x3,y3,z3,t3);

  return;
}

void VtxSeedGenerator::ChooseNextDigit(double& xpos, double& ypos, double& zpos, double& time) {
  // default
  xpos=0; ypos=0; zpos=0; time=0;

  // random number generator
  int numEntries = vSeedDigitList.size();

  fCounter++;
  if( fCounter>=fDigitList->size() ) fCounter = 0;
  fThisDigit = vSeedDigitList.at(fLastEntry);

  double r = gRandom->Uniform(); 

  fLastEntry = (int)(r*numEntries);

  // return the new digit
  fThisDigit = vSeedDigitList.at(fLastEntry);
  xpos = fDigitList->at(fThisDigit).GetPosition().X();
  ypos = fDigitList->at(fThisDigit).GetPosition().Y();
  zpos = fDigitList->at(fThisDigit).GetPosition().Z();
  time = fDigitList->at(fThisDigit).GetCalTime();
  
  return;
}

void VtxSeedGenerator::PushTrueVertex(bool savetodisk) {
	m_data->Stores.at("RecoEvent")->Set("TrueVertex", fTrueVertex, savetodisk); 
}

void VtxSeedGenerator::PushVertexSeeds(bool savetodisk) {
  m_data->Stores.at("RecoEvent")->Set("vSeedVtxList", vSeedVtxList, savetodisk); 
}

