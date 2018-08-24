#ifndef PhaseIITreeMaker_H
#define PhaseIITreeMaker_H

#include <string>
#include <iostream>

#include "Tool.h"
// ROOT includes
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"

class PhaseIITreeMaker: public Tool {


 public:

  PhaseIITreeMaker();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:
 	/// \brief Reset all variables. 
 	void ResetVariables();
 	
 	/// \brief ROOT TFile that will be used to store the output from this tool
  TFile* fOutput_tfile = nullptr;

  /// \brief TTree that will be used to store output
  TTree* fRecoTree = nullptr;
  
  
  /// \brief Branch variables
  
  /// \brief MC entry number
  uint64_t fMCEventNum;
  
  /// \brief trigger number
  uint16_t fMCTriggerNum;
  
  /// \brief ANNIE event number
  uint32_t fEventNumber;
  
  // Digits
  int fNhits = 0;
  std::vector<int> fIsFiltered;
  std::vector<double> fDigitX;
  std::vector<double> fDigitY;
  std::vector<double> fDigitZ;
  std::vector<double> fDigitT;
  std::vector<double> fDigitQ;    
  std::vector<int> fDigitType;
  	
  // True muon
  double fTrueVtxX;
  double fTrueVtxY;
  double fTrueVtxZ;
  double fTrueVtxTime;
  double fTrueDirX;
  double fTrueDirY;
  double fTrueDirZ;
  double fTrueTheta;
  double fTruePhi;
  double fTrueEnergy; 
  
  // Reco vertex
  // Vertex
  double fRecoVtxX;
  double fRecoVtxY;
  double fRecoVtxZ;
  double fRecoVtxTime;
  double fRecoDirX;
  double fRecoDirY;
  double fRecoDirZ;
  double fRecoTheta;
  double fRecoPhi;
  int fRecoStatus;
  
  //histograms
  TH1D *hDeltaX; 
  TH1D *hDeltaY;
  TH1D *hDeltaZ;
  TH1D *hDeltaR;
  TH1D *hDeltaT;
  TH1D *hDeltaParralel;
  TH1D *hDeltaPerpendicular;
  TH1D *hDeltaAzimuth;
  TH1D *hDeltaZenith;  
  TH1D *hDeltaAngle;
  TH1I *hFitStatus;
  
  	
  /// \brief Integer that determines the level of logging to perform
  int verbosity = 0;
  int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
		
	int get_ok;	

};


#endif
