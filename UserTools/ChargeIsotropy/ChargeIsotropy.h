#ifndef ChargeIsotropy_H
#define ChargeIsotropy_H

#include <string>
#include <iostream>
#include <cmath>

#include "Tool.h"


/**
 * \class ChargeIsotropy
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: James Minock $
* $Date: 2026/03/03 $
* Contact: jmm1018@physics.rutgers.edu
*/
class ChargeIsotropy: public Tool {


 public:

  ChargeIsotropy(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  double LawOfCosines(double vtxx, double vtxy, double vtxz, double pmt1x, double pmt1y, double pmt1z,
    double pmt2x, double pmt2y, double pmt2z); //< Law of Cosines. Returns angle between 2 tank PMT hits with respect to the reconstructed vertex



 private:

  //configuration variables
  int verbosity;  
  double time_window;
  bool IsData;

  //reconstruction variables
  int SimpleRecoFlag;
  double Qij; //< Charge Isotropy value
  Position SimpleRecoVtx;
  Position SimpleRecoStopVtx;
  double SimpleRecoCosTheta;
  double SimpleRecoPt;
  bool SimpleRecoFV;
  double SimpleRecoMrdEnergyLoss;
  double SimpleRecoTrackLengthInMRD;
  double SimpleRecoTrackLengthInTank;

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
