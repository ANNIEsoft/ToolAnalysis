#include "PhaseIITreeMaker.h"

PhaseIITreeMaker::PhaseIITreeMaker():Tool(){}


bool PhaseIITreeMaker::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  
  m_variables.Get("verbose", verbosity);
  std::string output_filename;
  m_variables.Get("OutputFile", output_filename);
  fOutput_tfile = new TFile(output_filename.c_str(), "recreate");
  fRecoTree = new TTree("phaseII", "ANNIE Phase II Reconstruction Tree");
  fRecoTree->Branch("McEntryNumber",&fMCEventNum,"McEntryNumber/I");
  fRecoTree->Branch("triggerNumber",&fMCTriggerNum,"triggerNumber/I");
  fRecoTree->Branch("eventNumber",&fEventNumber,"eventNumber/I");
  fRecoTree->Branch("nhits",&fNhits,"fNhits/I");
  fRecoTree->Branch("filter",&fIsFiltered);
  fRecoTree->Branch("digitX",&fDigitX);
  fRecoTree->Branch("digitY",&fDigitY);
  fRecoTree->Branch("digitZ",&fDigitZ);
  fRecoTree->Branch("digitT",&fDigitT);
  fRecoTree->Branch("digitQ",&fDigitQ);
  fRecoTree->Branch("digitType", &fDigitType);
  fRecoTree->Branch("trueVtxX",&fTrueVtxX,"trueVtxX/D");
  fRecoTree->Branch("trueVtxY",&fTrueVtxY,"trueVtxY/D");
  fRecoTree->Branch("trueVtxZ",&fTrueVtxZ,"trueVtxZ/D");
  fRecoTree->Branch("trueVtxTime",&fTrueVtxTime,"trueVtxTime/D");
  fRecoTree->Branch("trueDirX",&fTrueDirX,"trueDirX/D");
  fRecoTree->Branch("trueDirY",&fTrueDirY,"trueDirY/D");
  fRecoTree->Branch("trueDirZ",&fTrueDirZ,"trueDirZ/D");
  fRecoTree->Branch("recoVtxX",&fRecoVtxX,"recoVtxX/D");
  fRecoTree->Branch("recoVtxY",&fRecoVtxY,"recoVtxY/D");
  fRecoTree->Branch("recoVtxZ",&fRecoVtxZ,"recoVtxZ/D");
  fRecoTree->Branch("recoVtxTime",&fRecoVtxTime,"recoVtxTime/D");
  fRecoTree->Branch("recoDirX",&fRecoDirX,"recoDirX/D");
  fRecoTree->Branch("recoDirY",&fRecoDirY,"recoDirY/D");
  fRecoTree->Branch("recoDirZ",&fRecoDirZ,"recoDirZ/D");
  fRecoTree->Branch("recoStatus",&fRecoStatus,"recoStatus/I");
  
  //histograms
  hDeltaX          = new TH1D("DeltaX", "x difference", 200, -200, 200);  
  hDeltaY          = new TH1D("DeltaY", "y difference", 200, -200, 200);
  hDeltaZ          = new TH1D("DeltaZ", "z difference", 200, -200, 200);
  hDeltaT          = new TH1D("DeltaTime", "time difference", 400, 6600, 6800);
  hDeltaR          = new TH1D("DeltaR", "Distance between reconstruction and truth", 200, 0, 400);
  hDeltaParralel   = new TH1D("DeltaParallel", "Parallel component [||]", 200, -200, 200);
  hDeltaPerpendicular = new TH1D("DeltaPerpendicular", "Perpendicular component [T]", 200, -100, 300);
  hDeltaAzimuth    = new TH1D("DeltaAzimuth", "Azimuth angle", 200, -100, 100);
  hDeltaZenith     = new TH1D("DeltaZenith", "Zenith angle", 200, -100, 100);  
  hDeltaAngle      = new TH1D("DeltaAngle", "Delta angle", 100, 0, 100); 
  hFitStatus   = new TH1I("FitStatus", "Fit status", 85, -5, 80);
	
  return true;
}

bool PhaseIITreeMaker::Execute(){
	Log("===========================================================================================",v_debug,verbosity);
  Log("PhaseIITreeMaker Tool: Executing",v_debug,verbosity);

  // Reset variables
  this->ResetVariables();
  // Get a pointer to the ANNIEEvent Store
  auto* annie_event = m_data->Stores["RecoEvent"];
  if (!annie_event) {
    Log("Error: The PhaseITreeMaker tool could not find the RecoEvent Store", v_error, verbosity);
    return false;
  }

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
  
  // Read True Vertex   
  RecoVertex* truevtx = 0;
  m_data->Stores.at("RecoEvent")->Get("TrueVertex",truevtx); 
  
  fTrueVtxX = truevtx->GetPosition().X();
  fTrueVtxY = truevtx->GetPosition().Y();
  fTrueVtxZ = truevtx->GetPosition().Z();
  fTrueVtxTime = truevtx->GetTime();
  fTrueDirX = truevtx->GetDirection().X();
  fTrueDirY = truevtx->GetDirection().Y();
  fTrueDirZ = truevtx->GetDirection().Z();
  
  
  // Read digits
  std::vector<RecoDigit>* digitList = nullptr;
	m_data->Stores.at("RecoEvent")->Get("RecoDigit",digitList);  ///> Get digits from "RecoEvent" 
  fNhits = digitList->size();
  for( auto& digit : *digitList ){
    fDigitX.push_back(digit.GetPosition().X());
    fDigitY.push_back(digit.GetPosition().Y());
    fDigitZ.push_back(digit.GetPosition().Z());
    fDigitT.push_back(digit.GetCalTime());      
    fDigitQ.push_back(digit.GetCalCharge());
    fDigitType.push_back(digit.GetDigitType());
  }
  
  fRecoTree->Fill();
  return true;
}

bool PhaseIITreeMaker::Finalise(){
	fOutput_tfile->cd();
	fRecoTree->Write();
	fOutput_tfile->Close();
	
//	//histograms
//  delete hDeltaX;           hDeltaX = 0;         
//  delete hDeltaY;           hDeltaY = 0;         
//  delete hDeltaZ;           hDeltaZ = 0;         
//  delete hDeltaR;           hDeltaR = 0;         
//  delete hDeltaParralel;    hDeltaParralel = 0;  
//  delete hDeltaPerpendicular;  hDeltaPerpendicular = 0;
//  delete hDeltaAzimuth;     hDeltaAzimuth= 0;    
//  delete hDeltaZenith;      hDeltaZenith = 0;
//  delete hDeltaAngle;       hDeltaAngle = 0;
//  delete hFitStatus;    		hFitStatus = 0;

	
	if(verbosity>0) cout<<"PhaseIITreeMaker exitting"<<endl;

  return true;
}

void PhaseIITreeMaker::ResetVariables() {
  // tree variables
  fMCEventNum = 0;
  fMCTriggerNum = 0;
  fEventNumber = 0;
  fNhits = 0;
  fTrueVtxX = 0;
  fTrueVtxY = 0;
  fTrueVtxZ = 0;
  fTrueVtxTime = 0;
  fTrueDirX = 0;
  fTrueDirY = 0;
  fTrueDirZ = 0;
  fRecoVtxX = 0;
  fRecoVtxY = 0;
  fRecoVtxZ = 0;
  fRecoVtxTime = 0;
  fRecoDirX = 0;
  fRecoDirY = 0;
  fRecoDirZ = 0;
  fIsFiltered.clear();
  fDigitX.clear();
  fDigitY.clear();
  fDigitZ.clear();
  fDigitT.clear();
  fDigitQ.clear();
  fDigitType.clear();	
}

