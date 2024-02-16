#ifndef LikelihoodFitterCheck_H
#define LikelihoodFitterCheck_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TH1.h"
#include "TH2D.h"
#include "TFile.h"

#include "FoMCalculator.h"
#include "VertexGeometry.h"
#include "Parameters.h"
#include "TTree.h"

class LikelihoodFitterCheck: public Tool {


 public:

  LikelihoodFitterCheck();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  bool GetPDF(TH1D & pdf);


 private:
  /// \brief ROOT TFile that will be used to store the output from this tool
  TFile* fOutput_tfile = nullptr;

  /// \brief TTree that will be used to store output
  TTree* fVertexGeometry = nullptr;
  
 	/// \brief MC entry number
  uint64_t fMCEventNum;
  
  /// \brief trigger number
  uint16_t fMCTriggerNum;
  
  /// \brief ANNIE event number
  uint32_t fEventNumber;
  
  /// \brief Selecte a particular event to show
  int fShowEvent = 0;
  
 	std::vector<RecoDigit>* fDigitList = 0;
 	RecoVertex* fTrueVertex = 0;
 	
 	/// \brief histograms
 	TH2D* Likelihood2D = 0;
 	TGraph *gr_parallel = 0;
 	TGraph *gr_transverse = 0;

    /// \comparison histograms
    TH2D* Likelihood2D_pdf = 0;
    TGraph* pdf_parallel = 0;
    TGraph* pdf_transverse = 0;
 	
 	/// verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity=-1;
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	int get_ok;	
	bool ifPlot2DFOM = false;
    std::string pdffile;
    bool fUsePDFFile = 0;
    TH1D pdf;
	


};


#endif
