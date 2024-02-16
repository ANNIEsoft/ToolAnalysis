#ifndef VtxSeedGenerator_H
#define VtxSeedGenerator_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "ANNIEGeometry.h"
#include "Parameters.h"
#include "TMath.h"
#include "TRandom.h"
#include "FoMCalculator.h"
#include "VertexGeometry.h"

class VtxSeedGenerator: public Tool {


 public:

  VtxSeedGenerator();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

 private:
 	
 	/// \brief Reset true vertex and vertex seeds
 	///
 	/// Clear true vertex and vertex seed list
 	void Reset();
 
        /// \brief Grid Seed calculator
        ///
	bool GenerateSeedGrid(int NSeeds);	
	/// from the position to a digit
	double GetMedianSeedTime(Position pos);	
 	
 	/// \brief Calculate seed candidate
 	///
 	/// Use VertexGeometry to find the seeds
 	bool GenerateVertexSeeds(int NSeeds);
 	
 	/// \brief Calculate simple vertex
 	///
 	/// \param[in] double& vtxX: vertex x position
 	/// \param[in] double& vtxY: vertex y position
 	/// \param[in] double& vtxZ: vertex z position
 	/// \param[in] double& vtxTime: vertex time
 	void CalcSimpleVertex(double& vtxX, double& vtxY, double& vtxZ, double& vtxTime);
 	
 	/// \brief Choose a four-hit combination
 	///
 	/// Choose four different digits from the digit list
 	void ChooseNextQuadruple(double& x0, double& y0, double& z0, double& t0, 
 																				double& x1, double& y1, double& z1, double& t1, 
 																				double& x2, double& y2, double& z2, double& t2, 
 																				double& x3, double& y3, double& z3, double& t3);
 	
 	/// \brief Choose next digit
 	///
 	/// Choose a digit to form a quadruple. 
 	void ChooseNextDigit(double& xpos, double& ypos, double& zpos, double& time);

 	
 	/// \brief Calculate seed candidate
 	///
 	/// Use VertexGeometry to find the seeds
 	/// \param[in] bool savetodisk: save object to disk if savetodisk=true
 	void PushVertexSeeds(bool savetodisk);

	Position FindCenter();

	void GenerateFineGrid(int NSeeds, Position Center);

	RecoVertex* FindSimpleDirection(RecoVertex* myVertex);
 	
 		
 	/// Data variables
 	uint64_t fMCEventNum;      ///< event number in MC file
 	uint16_t fMCTriggernum;    ///< trigger number in MC file
 	std::vector<MCParticle>* fMCParticles=nullptr;  ///< truth tracks
	int fNumSeeds;    ///< Number of seeds to be generated
	
	
	/// Seed information
  int fThisDigit;
  int fLastEntry;
  int fCounter;
  int fSeedType;
  int fFineGrid;
  std::vector<RecoVertex>* vSeedVtxList = nullptr;
  std::vector<int> vSeedDigitList;	///< a vector thats stores the index of the digits used to calculate the seeds
  std::vector<RecoDigit>* fDigitList=nullptr;


  // Initialize the list that grid vertices will go to
  int UseSeedGrid=0;
  std::vector<RecoVertex>* SeedGridList = nullptr;
  
  /// verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity=-1;
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
 
	

};


#endif
