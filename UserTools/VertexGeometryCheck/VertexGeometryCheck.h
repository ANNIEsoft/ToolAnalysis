#ifndef VertexGeometryCheck_H
#define VertexGeometryCheck_H

#include <string>
#include <iostream>

#include "FoMCalculator.h"
#include "VertexGeometry.h"
#include "Parameters.h"
#include "Tool.h"
#include "TTree.h"
#include "TH2D.h"

class VertexGeometryCheck: public Tool {


 public:

  VertexGeometryCheck();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


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
  
  /// \brief recodigit vector
 	std::vector<RecoDigit>* fDigitList = 0;
 		
 	/// \brief true vertex pointer
 	RecoVertex* fTrueVertex = 0;
 	
 	/// \brief select a particle event to show
 	int fShowEvent = 0;
  
  /// \brief Histogram
  double recoVtxX, recoVtxY, recoVtxZ, recoVtxT, recoDirX, recoDirY, recoDirZ;
  double trueVtxX, trueVtxY, trueVtxZ, trueVtxT, trueDirX, trueDirY, trueDirZ;
  TH1D *flappdextendedtres; ///< lappd extended time residual
  TH1D *fpmtextendedtres; ///< pmt extended time residual
  TH1D *fpointtres; ///< point time residual
  TH1D *fdelta; ///< extended time residual
  TH1D *fmeanres; ///< mean value of the time residual distribution
  TH1D *fltrack; ///< muon track path length before producing a particular Cherenkov photon
  TH1D *flphoton; ///< Cherenkov photon track path length
  TH1D *fzenith; 
  TH1D *fazimuth;
  TH1D *fconeangle;
  TH1D *fdigitcharge;
  TH1D *fdigittime;
  TH1D *flappdtimesmear; 
  TH1D *fpmttimesmear;
  TH2D *fYvsDigitTheta_all;
  double vertheta = -999, verphi = -999;


/// verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity=-1;
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	int get_ok;	
};


#endif
