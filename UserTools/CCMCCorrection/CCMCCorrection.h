#ifndef CCMCCorrection_H
#define CCMCCorrection_H

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "Tool.h"


/**
 * \class CCMCCorrection
 *
 * Tool is used to apply MC Corrections. Made with CC Analysis in mind. Pulls from a separate calibration file for MRD Calibration
*
* $Author: James Minock $
* $Date: 2026/03/17 $
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

 private:


  string mrd_cal_file;
  int verbosity;
  double mrd_eff; //weight for MRD Efficiency correction
  double dirt_mu;
  const double dirt_scale = 0.0697;

  double simpletracklength;

  std::vector<double> bins_front;
  std::vector<double> bins_TL;
  std::vector<double> factorX;
  std::vector<double> factorY;
  std::vector<double> factorTL;
  string bins_front_str = "";
  string bins_TL_str = "";
  string factorX_str = "";
  string factorY_str = "";
  string factorTL_str = "";

  std::vector<std::vector<int>> MrdTimeClusters;

  std::vector<BoostStore>* theMrdTracks;   // the reconstructed tracks
  int numtracksinev;

};


#endif
