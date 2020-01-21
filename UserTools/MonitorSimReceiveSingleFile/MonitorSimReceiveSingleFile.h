#ifndef MonitorSimReceiveSingleFile_H
#define MonitorSimReceiveSingleFile_H

#include <string>
#include <iostream>
#include <stdlib.h>
#include <time.h>

#include "Tool.h"


/**
 * \class MonitorSimReceiveSingleFile
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class MonitorSimReceiveSingleFile: public Tool {


 public:

  MonitorSimReceiveSingleFile(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.


 private:

  std::string MRDDataPath;
  std::string mode;
  int verbosity;
  
  BoostStore* MRDData=nullptr;
  BoostStore* MRDData2=nullptr;
  BoostStore* PMTData=nullptr;


};


#endif
