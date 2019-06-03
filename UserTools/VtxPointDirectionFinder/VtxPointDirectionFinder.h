#ifndef VtxPointDirectionFinder_H
#define VtxPointDirectionFinder_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "VertexGeometry.h"
#include "MinuitOptimizer.h"

class VtxPointDirectionFinder: public Tool {


 public:

  VtxPointDirectionFinder();
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
  
  /// \brief
  RecoVertex* FindSimpleDirection(RecoVertex* myvertex);
 	
 	/// \brief 
 	RecoVertex* FitPointDirection(RecoVertex* myvertex);
 	
 	/// \brief Reset everything
 	void Reset();
 	
 	/// \brief Push found simple direction to store
 	void PushSimpleDirection(RecoVertex* vtx, bool savetodisk);
 	
 	/// \brief Push fitted point direction to store
 	void PushPointDirection(RecoVertex* vtx, bool savetodisk);
 	
 	bool fUseTrueVertexAsSeed;
 	RecoVertex* fTrueVertex = 0;
 	std::vector<RecoDigit>* fDigitList = 0;
 	
 	/// \brief simple direction
 	RecoVertex* fSimpleDirection = 0;
 	/// \brief point direction
 	RecoVertex* fPointDirection = 0;
 	
 	/// verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity=-1;
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;


};


#endif
