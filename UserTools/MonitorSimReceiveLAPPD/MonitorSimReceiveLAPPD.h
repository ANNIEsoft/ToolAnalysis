#ifndef MonitorSimReceiveLAPPD_H
#define MonitorSimReceiveLAPPD_H

#include <string>
#include <iostream>

#include "Tool.h"

#include "SlowControlMonitor.h"
#include "TRandom3.h"

/**
 * \class MonitorSimReceiveLAPPD
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class MonitorSimReceiveLAPPD: public Tool {


 public:

  MonitorSimReceiveLAPPD(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.


 private:

  int verbosity;
  std::string mode;
  std::string outpath;


};


#endif
