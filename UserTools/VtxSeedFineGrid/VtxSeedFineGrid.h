#ifndef VtxSeedFineGrid_H
#define VtxSeedFineGrid_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "ANNIEGeometry.h"
#include "Parameters.h"
#include "TMath.h"
#include "TRandom.h"
#include "TRandom3.h"
#include "FoMCalculator.h"
#include "VertexGeometry.h"


/**
 * \class VtxSeedFineGrid
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: F. Lemmons $
* $Date: 2021/11/16 10:41:00 $
* Contact: flemmons@fnal.gov
*/
class VtxSeedFineGrid: public Tool {


 public:

  VtxSeedFineGrid(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.


 private:
	 Position FindCenter();

	 void GenerateFineGrid();

	 Direction findDirectionMRD();
	 RecoVertex* FindSimpleDirection(RecoVertex* myVertex);

//	 double GetMedianSeedTime(Position pos);


	 std::vector<RecoVertex>* vSeedVtxList = nullptr;
	 std::vector<int> vSeedDigitList;
	 std::vector<RecoDigit>* fDigitList = nullptr;

	 std::vector<RecoVertex>* SeedGridList = nullptr;
	 RecoVertex* fTrueVertex = 0;
	 std::vector<Position> Center;
	 Direction SeedDir;
	 int fThisDigit = 0;
	 int fSeedType = 2;
	 int centerIndex[3];
	 int verbosity = -1;
	 int v_error = 0;
	 int v_warning = 1;
	 int v_message = 2;
	 int v_debug = 3;
	 bool useTrueDir = 1;
	 bool useSimpleDir = 0;
	 bool useMRDTrack = 0;
	 bool usePastResolution = 0;
	 bool useDirectionGrid = 0;
	 bool multiGrid = 0;

	 // \brief Event Status flag masks
	  int fEventStatusApplied;
	  int fEventStatusFlagged;
	
	   std::string InputFile =" ";

	 std::vector<BoostStore>* theMrdTracks;   // the MRD tracks
	 int numtracksinev;

	 std::string logmessage;

};


#endif
