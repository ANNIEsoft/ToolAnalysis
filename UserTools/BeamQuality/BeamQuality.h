#ifndef BeamQuality_H
#define BeamQuality_H

#include <string>
#include <iostream>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <sstream>

#include "Tool.h"
#include "BeamStatus.h"
#include "MinibufferLabel.h"
#include "ANNIEconstants.h"
#include "BeamDataPoint.h"
#include "TimeClass.h"

#include "Tool.h"


/**
* \class BeamQuality
*
* Tool to apply quality cuts to the results of BeamFetcherV2
*
* $Author: A.Sutton $
* $Date: 2024/04/1 $
*/
class BeamQuality: public Tool {


 public:

  BeamQuality(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

 protected:
  BeamStatus assess_beam_quality(uint64_t timestamp);


 private:

  // Config variables
  int verbosity;

  // Cut values from config
    // Extend this for additional cuts
  double fPOTMin;
  double fPOTMax;
  double fHornCurrMin;
  double fHornCurrMax;
  double fBeamLoss;


  // CStore things
  bool fNewCTCData;
  bool fNewBeamData;
  std::map<uint64_t, std::vector<uint32_t>> *TimeToTriggerWordMap;      ///< Trigger information
  std::map<uint64_t, std::map<std::string, BeamDataPoint>> *BeamDataMap; ///< BeamDB information
  std::map<uint64_t, BeamStatus> *BeamStatusMap;                        ///< Map containing the beam status information that will be saved to the CStore

  // Keep the last timestamp around to make sure we don't double process things
  uint64_t fLastTimestamp;

  
  // Verbosity things
  int v_error   = 0;
  int v_warning = 1;
  int v_message = 2;
  int v_debug   = 3;
  int vv_debug  = 4;
  std::string logmessage;
};


#endif
