#ifndef VtxExtendedVertexFinder_H
#define VtxExtendedVertexFinder_H

#include <string>
#include <iostream>

#include "Tool.h"
#include <VertexGeometry.h>
#include <TMinuit.h>
#include <MinuitOptimizer.h>

class VtxExtendedVertexFinder: public Tool {


 public:

  VtxExtendedVertexFinder();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:
 	/// \brief MC entry number
  uint64_t fMCEventNum;
  
  /// \brief trigger number
  uint16_t fMCTriggerNum;
  
  /// \brief ANNIE event number
  uint32_t fEventNumber;

  // \brief minimum of fit time window
  double fTmin;
  
  // \brief maximum of fit time window
  double fTmax;

  // \use external file for PDF?  If 0, will use equation fit
  bool fUsePDFFile = 0;

  // \file containing histogram of PDF of charge-angle distribution
  std::string pdffile;

  /// \brief 
  RecoVertex* FitExtendedVertex(RecoVertex* myvertex);
  
  /// \brief Run ExtendedVertex with every grid seed
  RecoVertex* FitGridSeeds(std::vector<RecoVertex>* vSeedVtxList);
  
  /// \brief Find a simple direction using weighted sum of digit charges 
  RecoVertex* FindSimpleDirection(RecoVertex* myvertex);
  bool GetPDF(TH1D &pdf);
  
  /// \brief Reset everything
  void Reset();
  
  /// \brief Push fitted extended vertex to store
  void PushExtendedVertex(RecoVertex* vtx, bool savetodisk);
  
  bool fUseTrueVertexAsSeed;
  bool fSeedGridFits;
  
  RecoVertex* fTrueVertex = 0;
  std::vector<RecoDigit>* fDigitList = 0;
  
  /// \brief extended vertex
  RecoVertex* fExtendedVertex = 0;
  
  /// Vertex Geometry shared by Fitter tools
  VertexGeometry* myvtxgeo;
  
  /// verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity=-1;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;
  int get_ok;
  TH1D pdf;
  



};


#endif
