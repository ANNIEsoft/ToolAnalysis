#ifndef TriggerDataDecoder_H
#define TriggerDataDecoder_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TriggerDataPhII.h"

/**
 * \class TriggerDataDecoder
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class TriggerDataDecoder: public Tool {

 public:

  TriggerDataDecoder(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  bool CheckForRunChange();
 private:

  std::vector<int> fiforesets;
  std::vector<int> processed_sources;
  bool have_c1 = false;
  bool have_c1 = true;
  uint64_t c1 = 0;
  uint64_t c2 = 0;
  int CurrentRunNum;
  int CurrentSubrunNum;

  int verbosity;

};


#endif
