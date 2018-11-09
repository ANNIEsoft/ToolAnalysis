#ifndef VtxPointPositionFinder_H
#define VtxPointPositionFinder_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "VertexGeometry.h"
#include "MinuitOptimizer.h"
#include <TRandom3.h>

class VtxPointPositionFinder: public Tool {


 public:

  VtxPointPositionFinder();
  ~VtxPointPositionFinder();
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
  
  /// \brief Random number generator
 	TRandom3 r;
 	/// \brief 
 	RecoVertex* FitPointPosition(RecoVertex* myvertex);
 	
 	/// \brief Find simple position
 	///
 	/// Find the best seed (x, y, z, t) from all the generated seed candidates
 	/// \param[] std::vector<RecoVertex>* vSeedVtxList: a vector of seed vertices
  RecoVertex* FindSimplePosition(std::vector<RecoVertex>* vSeedVtxList);
 	
 	/// \brief Reset everything
 	void Reset();
 	
 	/// \brief Push selected simple position to store
 	void PushSimplePosition(RecoVertex* vtx, bool savetodisk);
 		
 	/// \brief Push fitted point position to store
 	void PushPointPosition(RecoVertex* vtx, bool savetodisk);
 	
 	bool fUseTrueVertexAsSeed;
 	RecoVertex* fTrueVertex = 0;
 	std::vector<RecoDigit>* fDigitList = 0;
 		
 	/// \brief Simple position
 	RecoVertex* fSimplePosition = 0;
 	
 	/// \brief Point position
 	RecoVertex* fPointPosition = 0;
 	
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
