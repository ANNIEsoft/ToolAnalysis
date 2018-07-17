#ifndef VtxPointPositionFinder_H
#define VtxPointPositionFinder_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "VertexGeometry.h"
#include "MinuitOptimizer.h"

class VtxPointPositionFinder: public Tool {


 public:

  VtxPointPositionFinder();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:
 	/// Data variables
 	uint64_t fMCEventNum;      ///< event number in MC file
 	uint16_t fMCTriggernum;    ///< trigger number in MC file
 	
 	RecoVertex* FitPointVertex(RecoVertex* myvertex);
 	
 	bool fUseTrueVertexAsSeed;
 	RecoVertex* fTrueVertex = 0;
 	std::vector<RecoDigit>* fDigitList = 0;
 	
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
