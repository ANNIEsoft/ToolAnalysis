#include "DigitBuilderDoE.h"

DigitBuilderDoE::DigitBuilderDoE():Tool(){}


bool DigitBuilderDoE::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

	doetreename = "vertextreefiducialmrd";

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
	m_variables.Get("InputFile",doefilename);
  m_variables.Get("TreeName",doetreename);
 
	// Make the ANNIEEvent Store if it doesn't exist
	int annieeventexists = m_data->Stores.count("ANNIEEvent");
	if(annieeventexists==0) m_data->Stores["ANNIEEvent"] = new BoostStore(false,2);

  //Construct objects
	DigitVertices = new std::vector<LorentzVector<PxPyPzE4D<double>>>;
  DigitWhichDet = new std::vector<std::string>;
	DigitCharges = new std::vector<double>;
	DigitIdArray = new std::vector<int>;
	MuonStartVertex = new TLorentzVector;
	MuonStopVertex = new TLorentzVector;
	MuonDirection = new TVector3;	
  TrackLengthInMrd = new double;

  doechain = new TChain(doetreename.c_str());
  doechain->Add(doefilename.c_str());
  NumEvents = doechain->GetEntries();
  std::cout<<"FILE LOADED INTO CHAIN, HAS " << NumEvents << "ENTRIES" << std::endl;

  /// Construct the other objects we'll be setting at event level,
  fMuonStartVertex = new RecoVertex();
  fMuonStopVertex = new RecoVertex();
	fDigitList = new std::vector<RecoDigit>;

  EventNum = 0;
  //Now, link chain's branch addresses with those defined here
  this->LinkChain();
  return true;
}


bool DigitBuilderDoE::Execute(){
  doechain->GetEntry(EventNum);
  this->ClearDigitList();

  //First, load digits from event into RecoDigit classes
  //Loop through and convert information to the right form for the
  //RecoEvent checks that will be done in EventSelectorDoE
  //
  int region=-999.;
	double calT;
	double calQ = 0.;
	Position pos_reco;
  int DigitId;
  int digitType;
  for (int i=0;DigitIdArray->size();i++){
    if(DigitWhichDet->at(i) == "PMT8inch" && DigitCharges->at(i)>5.0){
      //Load up a PMT RecoDigit's goods
      digitType = 0;
      pos_reco.SetX(DigitVertices->at(i).X());
      pos_reco.SetY(DigitVertices->at(i).Y() + 14.46469);
      pos_reco.SetZ(DigitVertices->at(i).Z() - 168.1);
      calQ = DigitCharges->at(i);
      calT = DigitVertices->at(i).T();
      std::cout << "DIGIT PMT TIME: " << calT;
      DigitId = DigitIdArray->at(i);
		  RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType, DigitId);
		  fDigitList->push_back(recoDigit); 
    }
    if(DigitWhichDet->at(i) == "lappd_v0"){
      digitType = 1;
      pos_reco.SetX(DigitVertices->at(i).X());
      pos_reco.SetY(DigitVertices->at(i).Y() + 14.46469);
      pos_reco.SetZ(DigitVertices->at(i).Z() - 168.1);
      calQ = 1; //Charges not well modeled yet; don't use
      calT = DigitVertices->at(i).T();
      DigitId = DigitIdArray->at(i);
		  RecoDigit recoDigit(region, pos_reco, calT, calQ, digitType, DigitId);
		  fDigitList->push_back(recoDigit); 
    }
  }
  //Now, we'll load in the necessary muon information
  Direction muondirection;
  Position muonstartpos;
  double muonstarttime;
  muondirection.SetX(MuonDirection->X());
  muondirection.SetY(MuonDirection->Y());
  muondirection.SetZ(MuonDirection->Z());
  muonstartpos.SetX(MuonStartVertex->X()); 
  muonstartpos.SetY(MuonStartVertex->Y() + 14.46469);
  muonstartpos.SetZ(MuonStartVertex->Z() - 168.1);
  std::cout << "MUON INFORMATION: X = " <<muonstartpos.X()<<", Y = "<<muonstartpos.Y()<<
      ", Z = "<<muonstartpos.Z()<<", T = "<<muonstarttime<<std::endl;
  muonstarttime = MuonStartVertex->T();
  fMuonStartVertex->SetVertex(muonstartpos, muonstarttime);
  fMuonStartVertex->SetDirection(muondirection);
  this->PushTrueVertex(true);

  //EventSelectorDoE will have a rudimentary check if track went into MRD
  m_data->Stores.at("RecoEvent")->Set("TrackLengthInMRD",TrackLengthInMrd);

  //by default, all events pass the event cut status
  m_data->Stores.at("RecoEvent")->Set("EventCutStatus", true);
  
  m_data->Stores.at("ANNIEEvent")->Set("MCEventNum",EventNum);
	m_data->Stores.at("ANNIEEvent")->Set("MCTriggernum",0);
	m_data->Stores.at("ANNIEEvent")->Set("EventNumber",EventNum);
  EventNum++;
  return true;
}


bool DigitBuilderDoE::Finalise(){
  //WE NEED TO DELETE STUFF AND CLEAN UP... 
	delete DigitVertices;
  delete DigitWhichDet; 
  delete DigitCharges;
	delete DigitIdArray;
	delete MuonStartVertex;
	delete MuonStopVertex;
	delete MuonDirection;	
  delete TrackLengthInMrd;
  delete doechain;
  return true;
}

void DigitBuilderDoE::PushTrueVertex(bool savetodisk) {
  Log("EventSelector Tool: Push true vertex to the RecoEvent store",v_message,verbosity);
  m_data->Stores.at("RecoEvent")->Set("TrueVertex", fMuonStartVertex, savetodisk); 
}


void DigitBuilderDoE::LinkChain(){
  //Do all the awesome linking here
 doechain->SetBranchAddress("MuonStartVertex", &MuonStartVertex);
 doechain->SetBranchAddress("MuonStopVertex", &MuonStopVertex);
 doechain->SetBranchAddress("TrackLengthInMrd", &TrackLengthInMrd);
 doechain->SetBranchAddress("MuonDirection", &MuonDirection);
 doechain->SetBranchAddress("DigitVertices", &DigitVertices);
 doechain->SetBranchAddress("DigitCharges", &DigitCharges);
 doechain->SetBranchAddress("DigitWhichDet", &DigitWhichDet);
 doechain->SetBranchAddress("DigitPmtId", &DigitIdArray);
}


