#include "DigitBuilderROOT.h"

DigitBuilderROOT::DigitBuilderROOT():Tool(){}


bool DigitBuilderROOT::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
	m_variables.Get("InputFile",NtupleFile);

  P2Chain = new TChain("PhaseII");
  P2Chain->Add(NtupleFile.c_str());
  return true;
 
  fMuonVertex = new RecoVertex();
	fDigitList = new std::vector<RecoDigit>;
  
  //Hit information (PMT and LAPPD)
  //Always output in Phase II Reco Tree
  P2Chain->SetBranchAddress("nhits",&fNhits);
  P2Chain->SetBranchAddress("filter",&fIsFiltered);
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
}


bool DigitBuilderROOT::Execute(){
  this->Reset();
  P2Chain->GetEvent(EntryNum);

  //Load the True Muon Vertex information
  Position muonstartpos(fTrueVtxX, fTrueVtxY, fTrueVtxZ);
  Direction muondirection(fTrueDirX, fTrueDirY, fTrueDirZ);
  double muonstarttime = fTrueVtxTime;
  fMuonVertex->SetVertex(muonstartpos, muonstarttime);
  fMuonVertex->SetDirection(muondirection);

  //Loop through all Digits and load their information as RecoDigits
  for(int j=0;j<fDigitType.size();j++){
    int region = -999;
    double calT = fDigitT.at(j);
    double calQ = fDigitQ.at(j);
    int isFiltered = fIsFiltered.at(j);
    Position pos_reco(fDigitX.at(j), fDigitY.at(j),fDigitZ.at(j));
    int digitType = fDigitType.at(j);
    int digitID = fDigitDetID.at(j);
		RecoDigit aDigit(region, pos_reco, calT, calQ,digitType,digitID);
    aDigit.SetFilter(isFiltered);
    fDigitList->push_back(aDigit); 
  }
	if(EntryNum==TotalEntries) m_data->vars.Set("StopLoop",1);
	if(verbosity>1) std::cout<<"done loading Phase II ntuple entry"<<std::endl;
  return true;
}


bool DigitBuilderROOT::Finalise(){
  delete P2Chain;
  delete fMuonVertex;
  delete fDigitList;
  return true;
}

void DigitBuilderROOT::Reset(){
  fMuonVertex->Reset();
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
