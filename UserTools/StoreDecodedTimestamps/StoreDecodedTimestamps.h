#ifndef StoreDecodedTimestamps_H
#define StoreDecodedTimestamps_H

#include <string>
#include <iostream>

#include "Tool.h"

#include "TFile.h"
#include "TTree.h"

/**
 * \class StoreDecodedTimestamps
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: M.Nieslony $
* $Date: 2021/01/20 10:44:00 $
* Contact: mnieslon@uni-mainz.de
*/
class StoreDecodedTimestamps: public Tool {


 public:

  StoreDecodedTimestamps(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.


 private:

  int verbosity;
  std::string output_timestamps;
  bool save_mrd = false;
  bool save_pmt = false;
  bool save_ctc = false;
  bool delete_timestamps;

  bool new_mrd_data;
  bool new_pmt_data;
  bool new_ctc_data;
  std::map<uint64_t, std::vector<std::pair<unsigned long, int> > > MRDEvents;
  std::map<uint64_t, std::map<std::vector<int>,std::vector<uint16_t> > > *InProgressTankEvents;
  std::map<uint64_t, std::vector<uint32_t>>* TimeToTriggerWordMap;
  std::map<uint64_t, int> AlmostCompleteWaveforms;

  TFile *f_timestamps = nullptr;
  TTree *t_timestamps_mrd = nullptr;
  TTree *t_timestamps_pmt = nullptr;
  TTree *t_timestamps_ctc = nullptr;
  ULong64_t t_pmt;
  ULong64_t t_mrd;
  ULong64_t t_ctc;
  int triggerword_ctc;
  double t_pmt_sec;
  double t_mrd_sec;
  double t_ctc_sec;

  int ExecuteNr;

};


#endif
