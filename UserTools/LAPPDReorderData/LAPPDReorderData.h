#ifndef LAPPDReorderData_H
#define LAPPDReorderData_H

#include <string>
#include <iostream>

#include "TFile.h"
#include "TH1D.h"

#include "Tool.h"
#include <bitset>

/**
 * \class LAPPDReorderData
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class LAPPDReorderData: public Tool {


 public:

  LAPPDReorderData(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.


 private:

  int delayoffset;
  int globalshift;
  int VerbosityLevel;
  string InputWavLabel;
  string OutputWavLabel;

  TH1D* Trigdelay;





};


#endif
