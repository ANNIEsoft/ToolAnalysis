#ifndef FilterEvents_H
#define FilterEvents_H

#include <string>
#include <iostream>

#include "Tool.h"

#include "ADCPulse.h"
#include "PsecData.h"
#include "Hit.h"

/**
 * \class FilterEvents
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class FilterEvents: public Tool {


 public:

  FilterEvents(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  void SetAndSaveEvent();

 private:

  std::string FilterName;
  std::string FilteredFilesBasename;
  std::string SavePath;
  int verbosity;
  BoostStore* FilteredEvents = nullptr;
  int matched;

  int v_error = 0;
  int v_warning = 1;
  int v_message = 2;
  int v_debug = 3;
  int vv_debug = 4;

};


#endif
