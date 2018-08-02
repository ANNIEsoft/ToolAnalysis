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
  fOutput_tfile = std::unique_ptr<TFile>(new TFile(output_filename.c_str(), "recreate"));
  fRecoTree = new TTree("phaseII", "ANNIE Phase II Reconstruction Tree");
  fRecoTree->Branch("nhits",&fNhits,"fNhits/I");
  fRecoTree->Branch("filter",&fIsFiltered);
  fRecoTree->Branch("digitX",&fDigitX);
  fRecoTree->Branch("digitY",&fDigitY);
  fRecoTree->Branch("digitZ",&fDigitZ);
  fRecoTree->Branch("digitT",&fDigitT);
  fRecoTree->Branch("digitQ",&fDigitQ);
  fRecoTree->Branch("digitType", &fDigitType, "fDigitType/I");
	

  return true;
}


bool PhaseIITreeMaker::Execute(){
	Log("===========================================================================================",v_debug,verbosity);
  Log("PhaseIITreeMaker Tool: Executing",v_debug,verbosity);
  
  // Get a pointer to the ANNIEEvent Store
  auto* annie_event = m_data->Stores["RecoEvent"];
  if (!annie_event) {
    Log("Error: The PhaseITreeMaker tool could not find the ANNIEEvent Store",
      0, verbosity);
    return false;
  }
  // Read digits
  std::vector<RecoDigit>* digitList = nullptr;
	auto get_digit = m_data->Stores.at("RecoEvent")->Get("RecoDigit",digitList);  ///> Get digits from "RecoEvent" 
  if(!get_digit){ 
  	Log("PhaseIITreeMaker  Tool: Error retrieving RecoDigits,no digit from the RecoEvent store!",v_error,verbosity); 
  	return true;
  }
  for( auto& digit : *digitList ){
    fDigitX.push_back(digit.GetPosition().X());
    fDigitY.push_back(digit.GetPosition().Y());
    fDigitZ.push_back(digit.GetPosition().Z());
    fDigitT.push_back(digit.GetCalTime());      
    fDigitQ.push_back(digit.GetCalCharge());
    fDigitType.push_back(digit.GetDigitType());
  }
  fRecoTree->Fill();
  //fOutput_tfile->cd();
	//fRecoTree->Write();
  return true;
}


bool PhaseIITreeMaker::Finalise(){
	//fOutput_tfile->cd();
	fRecoTree->Write();
	fOutput_tfile->Close();

  return true;
}
