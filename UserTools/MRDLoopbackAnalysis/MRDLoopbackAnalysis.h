#ifndef MRDLoopbackAnalysis_H
#define MRDLoopbackAnalysis_H

#include <string>
#include <iostream>

#include "Tool.h"

#include "TH1D.h"
#include "TH2D.h"
#include "TFile.h"

/**
 * \class MRDLoopbackAnalysis
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: M.Nieslony $
* $Date: 2020/04/06 10:54:00 $
* Contact: mnieslon@uni-mainz.de
*/

class MRDLoopbackAnalysis: public Tool {


 public:

  MRDLoopbackAnalysis(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.


 private:

  //Configuration variables
  int verbosity;
  std::string output_rootfile;

  //Input objects
  std::vector<std::vector<int>> MrdTimeClusters;
  std::vector<double> MrdDigitTimes;

  //Variable to keep track of time differences
  uint64_t PreviousTimeStamp;

  //ROOT File for storing output files
  TFile *mrddigitts_file = nullptr;

  //Histograms storing information about MRD Loopback channels
  TH1D *time_diff_beam = nullptr;
  TH1D *time_diff_cosmic = nullptr;
  TH2D *mrddigitts_beamloopback = nullptr;
  TH2D *mrddigitts_cosmicloopback = nullptr;
  TH2D *time_diff_loopback_beam = nullptr;
  TH2D *time_diff_loopback_cosmic = nullptr;
  TH2D *time_diff_hittimes_beam = nullptr;
  TH2D *time_diff_hittimes_cosmic = nullptr;

  //verbosity variables
  int v_error = 0;
  int v_warning = 1;
  int v_message = 2;
  int v_debug = 3;

};


#endif
