#ifndef CCMCCorrection_H
#define CCMCCorrection_H

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "TRandom3.h"

#include "Tool.h"


/**
 * \class CCMCCorrection
 *
 * Tool is used to apply MC Corrections and MC Correction uncertainties. Made with CC Analysis in mind. Pulls from a separate calibration file for MRD Calibration
*
* $Author: James Minock $
* $Date: 2026/03/27 $
* Contact: jmm1018@physics.rutgers.edu
*/
class CCMCCorrection: public Tool {


 public:

  CCMCCorrection(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
  void breakCSV(string line, vector<double> &tokens); ///< Break up comma separated string into vector of components
  int findBin(double Y, int iter, vector<double> const &bins); ///< Find appropriate bin for value given binning scheme from calibration file
  double MRDEfficiency(); ///< Function to perform MRD Efficiency Calibration
  double DirtScaling(); ///< Function to scale dirt events
  void Reset(); ///< Initializes/resets variables to default and clears vectors

 private:


  string mrd_cal_file;
  int verbosity;
  int seed;
  int n_univ;
  TRandom3 rnd;
  double mrd_eff; //weight for MRD Efficiency correction
  double dirt_mu;
  std::vector<double> MRDUnc; //collection of universes for uncertainty reweighting
  std::vector<double> DirtUnc;
  std::vector<std::vector<double>> mrd_reweight_vector; //Uncertainty per bin per universe
  std::vector<double> dirt_rew_vector;
  const double dirt_scale = 0.0697;
  const double dirt_unc = 0.0028;

  double TrueNuIntxVtx_Z;

  std::vector<double> bins_front;
  std::vector<double> factorY;
  std::vector<double> uncsY;
  string bins_front_str = "";
  string factorY_str = "";
  string uncsY_str = "";

  std::vector<std::vector<int>> MrdTimeClusters;

  std::vector<BoostStore>* theMrdTracks;   // the reconstructed tracks
  int numtracksinev;

};


#endif
