#include "DigitBuilderROOT.h"

DigitBuilderROOT::DigitBuilderROOT():Tool(){}


bool DigitBuilderROOT::Initialise(std::string configfile, DataModel &data){
  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
	m_variables.Get("InputFile",NtupleFile);
	m_variables.Get("verbosity",verbosity);

  Log("---------DigitBuilderROOT Tool initializing ---------- " ,v_message,verbosity);

  int recoeventexists = m_data->Stores.count("RecoEvent");
  if(recoeventexists==0) m_data->Stores["RecoEvent"] = new BoostStore(false,2);

	int annieeventexists = m_data->Stores.count("ANNIEEvent");
	if(annieeventexists==0) m_data->Stores["ANNIEEvent"] = new BoostStore(false,2);

  P2Chain = new TChain("phaseII");
  P2Chain->Add(NtupleFile.c_str());
 
  fMuonVertex = new RecoVertex();
	fDigitList = new std::vector<RecoDigit>;
  
  fDigitX = new std::vector<double>;
  fDigitY = new std::vector<double>;
  fDigitZ = new std::vector<double>;
  fDigitT = new std::vector<double>;
  fDigitQ = new std::vector<double>;    
  fDigitType  = new std::vector<int>;
  fDigitDetID = new std::vector<int>;
  Log("DigitBuilderROOT Tool: Setting all branch addresses now",v_debug,verbosity);
  //Hit information (PMT and LAPPD)
  //Always output in Phase II Reco Tree
  P2Chain->SetBranchAddress("McEntryNumber",&fMCEventNum);
  P2Chain->SetBranchAddress("triggerNumber",&fMCTriggerNum);
  P2Chain->SetBranchAddress("eventNumber",&fEventNumber);
  P2Chain->SetBranchAddress("nhits",&fNhits);
  P2Chain->SetBranchAddress("digitX",&fDigitX);
  P2Chain->SetBranchAddress("digitY",&fDigitY);
  P2Chain->SetBranchAddress("digitZ",&fDigitZ);
  P2Chain->SetBranchAddress("digitT",&fDigitT);
  P2Chain->SetBranchAddress("digitQ",&fDigitQ);
  P2Chain->SetBranchAddress("digitType", &fDigitType);
  P2Chain->SetBranchAddress("digitDetID", &fDigitDetID);

  //Muon Truth Information
  P2Chain->SetBranchAddress("trueVtxX",&fTrueVtxX);
  P2Chain->SetBranchAddress("trueVtxY",&fTrueVtxY);
  P2Chain->SetBranchAddress("trueVtxZ",&fTrueVtxZ);
  P2Chain->SetBranchAddress("trueVtxTime",&fTrueVtxTime);
  P2Chain->SetBranchAddress("trueDirX",&fTrueDirX);
  P2Chain->SetBranchAddress("trueDirY",&fTrueDirY);
  P2Chain->SetBranchAddress("trueDirZ",&fTrueDirZ);

  TotalEntries = P2Chain->GetEntries();
  EntryNum = 0;
  return true;
}


bool DigitBuilderROOT::Execute(){
  this->Reset();
  Log("DigitBuilderROOT Tool: Getting EntryNum " + std::to_string(EntryNum),v_debug,verbosity);
  P2Chain->GetEvent(EntryNum);

  Log("DigitBuilderROOT Tool: loading true muon information from phaseII ntuple",v_debug,verbosity);
  //Load the True Muon Vertex information
  Position muonstartpos(fTrueVtxX, fTrueVtxY, fTrueVtxZ);
  Direction muondirection(fTrueDirX, fTrueDirY, fTrueDirZ);
  double muonstarttime = fTrueVtxTime;
  fMuonVertex->SetVertex(muonstartpos, muonstarttime);
  fMuonVertex->SetDirection(muondirection);
  this->PushTrueVertex(true);

  Log("DigitBuilderROOT Tool: Looping through and loading all digits",v_debug,verbosity);
  //Loop through all Digits and load their information as RecoDigits
  for(int j=0;j<fDigitType->size();j++){
    int region = -999;
    double calT = fDigitT->at(j);
    double calQ = fDigitQ->at(j);
    Position pos_reco(fDigitX->at(j), fDigitY->at(j),fDigitZ->at(j));
    int digitType = fDigitType->at(j);
    int digitID = fDigitDetID->at(j);
		RecoDigit aDigit(region, pos_reco, calT, calQ,digitType,digitID);
    fDigitList->push_back(aDigit); 
  }
  this->PushRecoDigits(true);
  EntryNum++;
	if(EntryNum==TotalEntries){
    m_data->vars.Set("StopLoop",1);
    Log("DigitBuilderROOT Tool: All entries in ROOT file read.",v_message,verbosity);
  }
  // MC entry number
  m_data->Stores.at("ANNIEEvent")->Set("MCEventNum",fMCEventNum);  
  
  // MC trigger number
  m_data->Stores.at("ANNIEEvent")->Set("MCTriggernum",fMCTriggerNum); 
  
  // ANNIE Event number
  m_data->Stores.at("ANNIEEvent")->Set("EventNumber",fEventNumber);
  
  fEventCutStatus = true; //FIXME: Need to read EventCutStatus into PhaseIITreeMaker
  m_data->Stores.at("RecoEvent")->Set("EventCutStatus", fEventCutStatus);
  return true;
}


bool DigitBuilderROOT::Finalise(){
  delete P2Chain;
  delete fMuonVertex;
  delete fDigitList;
  delete fDigitX;
  delete fDigitY;
  delete fDigitZ;
  delete fDigitT;
  delete fDigitQ;
  delete fDigitType;
  delete fDigitDetID;
  return true;
}

void DigitBuilderROOT::Reset(){
  fMuonVertex->Reset();
  fDigitX->clear();
  fDigitY->clear();
  fDigitZ->clear();
  fDigitT->clear();
  fDigitQ->clear();
  fDigitType->clear();
  fDigitDetID->clear();
  fDigitList->clear();
}

void DigitBuilderROOT::PushTrueVertex(bool savetodisk) {
  Log("DigitBuilderROOT Tool: Push true vertex to the RecoEvent store",v_message,verbosity);
  m_data->Stores.at("RecoEvent")->Set("TrueVertex", fMuonVertex, savetodisk); 
}

void DigitBuilderROOT::PushRecoDigits(bool savetodisk) {
	Log("DigitBuilder Tool: Push reconstructed digits to the RecoEvent store",v_message,verbosity);
	m_data->Stores.at("RecoEvent")->Set("RecoDigit", fDigitList, savetodisk);  ///> Add digits to RecoEvent
}
