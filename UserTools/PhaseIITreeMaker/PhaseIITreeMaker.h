#ifndef PhaseIITreeMaker_H
#define PhaseIITreeMaker_H

#include <string>
#include <iostream>

#include "Tool.h"
// ROOT includes
#include "TFile.h"
#include "TTree.h"

class PhaseIITreeMaker: public Tool {


 public:

  PhaseIITreeMaker();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:
 	/// \brief ROOT TFile that will be used to store the output from this tool
  TFile* fOutput_tfile = nullptr;

  /// \brief TTree that will be used to store output
  TTree* fRecoTree = nullptr;
  
  // Branch variables
  // Digits
  int fNhits = 0;
  std::vector<int> fIsFiltered;
  std::vector<double> fDigitX;
  std::vector<double> fDigitY;
  std::vector<double> fDigitZ;
  std::vector<double> fDigitT;
  std::vector<double> fDigitQ;    
  std::vector<int> fDigitType;
  	
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
