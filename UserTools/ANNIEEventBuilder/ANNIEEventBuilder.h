#ifndef ANNIEEventBuilder_H
#define ANNIEEventBuilder_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TimeClass.h"
#include "TriggerClass.h"

/**
 * \class ANNIEEventBuilder
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class ANNIEEventBuilder: public Tool {


 public:

  ANNIEEventBuilder(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.


 private:

  std::map<uint64_t, std::map<std::vector<int>, std::vector<uint16_t> > >* FinishedWaves;  //Key: {MTCTime}, value: map of fully-built waveforms from WaveBank
  BoostStore TrigData(false,2);

  std::string InputFile;

  // Number of PMTs that must be found in a WaveSet to build the event
  //
  int NumWavesInSet = 131;  
  int RunNum;
  int SubRunNum;
};


#endif
