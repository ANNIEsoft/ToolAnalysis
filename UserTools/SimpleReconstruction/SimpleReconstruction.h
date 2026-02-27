#ifndef SimpleReconstruction_H
#define SimpleReconstruction_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "Position.h"

/**
 * \class SimpleReconstruction
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: M. Nieslony $
* $Date: 2023/01/25 10:44:00 $
* Contact: mnieslon@uni-mainz.de
*/
class SimpleReconstruction: public Tool {


 public:

  SimpleReconstruction(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  void SetDefaultValues(); ///< Set default values for SimpleReconstruction
  bool SimpleEnergyReconstruction(); ///< Simple energy reconstruction for muon energy
  bool SimpleVertexReconstruction(); ///< Simple vertex reconstruction for neutrino interaction vertex
  bool GetANNIEEventVariables(); ////< get relevant variables from ANNIEEvent store
  bool RecoTankExitPoint(std::vector<int> clusterid); ////< Reconstruct the tank exit point of the muon

 private:

  //configuration variables
  int verbosity;  

  //reconstruction variables
  int SimpleRecoFlag;
  double SimpleRecoEnergy;
  Position SimpleRecoVtx;
  Position SimpleRecoStopVtx;
  double SimpleRecoCosTheta;
  double SimpleRecoPt;
  bool SimpleRecoFV;
  double SimpleRecoMrdEnergyLoss;
  double SimpleRecoTrackLengthInMRD;
  double SimpleRecoTrackLengthInTank;
  Position SimpleRecoMRDStart;
  Position SimpleRecoMRDStop;

  //event variables
  std::vector<double> fMRDTrackAngle;
  std::vector<double> fMRDTrackAngleError;
  std::vector<double> fMRDTrackLength;
  std::vector<double> fMRDPenetrationDepth;
  std::vector<double> fMRDEntryPointRadius;
  std::vector<double> fMRDEnergyLoss;
  std::vector<double> fMRDEnergyLossError;
  std::vector<double> fMRDTrackStartX;
  std::vector<double> fMRDTrackStartY;
  std::vector<double> fMRDTrackStartZ;
  std::vector<double> fMRDTrackStopX;
  std::vector<double> fMRDTrackStopY;
  std::vector<double> fMRDTrackStopZ;
  std::vector<bool> fMRDStop;
  std::vector<bool> fMRDSide;
  std::vector<bool> fMRDThrough;
  std::vector<int> fMRDTrackEventID;

  double dist_pmtvol_tank;
  double mrd_eloss;
  double max_pe;
  double mrd_tracklength;
  double exitx;
  double exity;
  double exitz;
  double dirx;
  double diry;
  double dirz;

  //verbosity variables
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  int vv_debug=4;


};


#endif
