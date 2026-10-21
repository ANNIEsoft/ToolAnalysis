#ifndef ClassicalEnergyReco_H
#define ClassicalEnergyReco_H

#include <string>
#include <iostream>
#include <cmath>
#include <vector>

#include "Tool.h"
#include "Position.h"
#include "RecoVertex.h"
#include "RecoCluster.h"

/**
 * \class ClassicalEnergyReco
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: F. A. Lemmons $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class ClassicalEnergyReco: public Tool {


 public:

  ClassicalEnergyReco(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  bool ClassicEnergyReconstruction();
  bool GetANNIEEventVariables();


 private:

	 int verbosity;

	 vector<RecoCluster>* fClusterList;

	 RecoVertex* recoVertex;
	 double recoVtxX,recoVtxY,recoVtxZ;
	 double recoDirX,recoDirY,recoDirZ;
	 double recoVtxT;

	 double exitX,exitY,exitZ;

	 double recoTankTrackLength;

	 int longesttrackEntryNumber;
	 double MaximumMRDTrackLength;
	 double longestMRDEloss;

	 int muonClusterIndex=0;

	 double max_pe;

	 int eventFlaggedStatus;

	 double ClassicRecoEnergy;
	 double crTransverseP;


	 //verbosity variables
	 int v_error = 0;
	 int v_warning = 1;
	 int v_message = 2;
	 int v_debug = 3;
	 int vv_debug = 4;

};


#endif
