#ifndef VtxExtendedVertexFinder_H
#define VtxExtendedVertexFinder_H

#include <string>
#include <iostream>

#include "Tool.h"

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
 	
 	/// \brief 
 	RecoVertex* FitExtendedVertex(RecoVertex* myvertex);
 	
 	/// \brief Reset everything
 	void Reset();
 	
 	/// \brief Push fitted extended vertex to store
 	void PushExtendedVertex(RecoVertex* vtx, bool savetodisk);
 	
 	bool fUseTrueVertexAsSeed;
 	RecoVertex* fTrueVertex = 0;
 	std::vector<RecoDigit>* fDigitList = 0;
 	
 	/// \brief extended vertex
 	RecoVertex* fExtendedVertex = 0;
 	
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
