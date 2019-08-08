#ifndef MonitorTankTime_H
#define MonitorTankTime_H

#include <string>
#include <iostream>
#include <vector>

#include "Tool.h"

#include "TObjectTable.h"

#include "TCanvas.h"
#include "TLegend.h"
#include "TF1.h"
#include "TThread.h"
#include "MRDOut.h"
//#include "PMTOut.h"						//include later
#include "TApplication.h"
#include "TLegend.h"
#include "TPaletteAxis.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TDatime.h"            //for labeling the x-axis with time labels
#include "TPaveText.h"
#include "TFile.h"
#include "TPad.h"
#include "TAxis.h"
#include "TROOT.h"
#include "TH2F.h"
#include "TH1I.h"

#include <boost/date_time/posix_time/posix_time.hpp>

#include "MonitorMRDTime.h"


/**
 * \class MonitorTankTime
*
* $Author: M. Nieslony $
* $Date: 2019/08/09 01:01:00 $
* Contact: mnieslon@uni-mainz.de
*/

class MonitorTankTime: public Tool {


 public:

  MonitorTankTime(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.

  //define functions for time evolution plots here


 private:

  BoostStore *CCData;
  std::string outpath_temp;
  std::string outpath;
  std::string active_slots;
  BoostStore* PMTdata;
  //PMTOut PMTout;                  //include as soon as PMTOut class is available

  int verbosity;

  //variables for time calculations
  boost::posix_time::ptime *Epoch;
  boost::posix_time::ptime current;
  std::string StartTime;
  boost::posix_time::time_duration period_update;
  boost::posix_time::time_duration duration;
  boost::posix_time::ptime last;
  time_t t;
  std::stringstream title_time; 
  long current_stamp;

  //hardware variables
  const int num_crates_tank = 3;
  const int num_slots_tank = 21;
  const int num_channels_tank = 4;

  int num_active_slots, num_active_slots_cr1, num_active_slots_cr2, num_active_slots_cr3;

  //PMT store PMTOut will (probably) include the following variables
  int SequenceID, StartTimeSec, StartTimeNSec, BufferSize, FullBufferSize, EventSize, TriggerNumber;
  long StartCount;
  std::vector<int> CrateID, CardID, Channels, TriggerCounts, Rates;
  std::vector<unsigned short> Data;

};

#endif
