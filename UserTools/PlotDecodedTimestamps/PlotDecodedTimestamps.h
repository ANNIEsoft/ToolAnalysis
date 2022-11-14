#ifndef PlotDecodedTimestamps_H
#define PlotDecodedTimestamps_H

#include <string>
#include <iostream>

#include "Tool.h"

#include "TFile.h"
#include "TTree.h"
#include "TH2F.h"
#include "TLine.h"
#include "TCanvas.h"
#include "TBox.h"

/**
 * \class PlotDecodedTimestamps
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: M.Nieslony $
* $Date: 2021/01/20 10:44:00 $
* Contact: mnieslon@uni-mainz.de
*/
class PlotDecodedTimestamps: public Tool {


 public:

  PlotDecodedTimestamps(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  void SetupFilesAndTrees();
  void SetupTriggerColorScheme();

 private:

  int verbosity;
  std::string output_timestamps;
  std::string input_datasummary;
  std::string input_timestamps;
  std::string triggerword_config;
  double seconds_per_plot=10.;

  TFile *f_datasummary = nullptr;
  TTree *t_datasummary = nullptr;
  TTree *t_datasummary_orphan = nullptr;
  TFile *f_timestamps = nullptr;
  TTree *t_timestamps = nullptr;
  TTree *t_timestamps_pmt = nullptr;
  TTree *t_timestamps_mrd = nullptr;
  TFile *f_out = nullptr;
  std::vector<TCanvas*> canvas_snapshot;
  std::vector<TLine*> timestamp_snapshot;
  TCanvas *canvas_timestreams = nullptr;
  std::vector<TBox*> timestreams_boxes;

  ULong64_t t_ctc;
  ULong64_t t_mrd;
  ULong64_t t_pmt;
  int triggerword_ctc;
  ULong64_t ctctimestamp;
  ULong64_t mrdtimestamp;
  ULong64_t pmttimestamp;
  uint32_t triggerword;
  std::string *type_orphan = nullptr;
  ULong64_t orphantimestamp;
  std::string *cause_orphan = nullptr;
  int adc_samples;
  bool extended_window;
  bool data_tank;
  bool trig_extended;
  bool trig_extended_cc;
  bool trig_extended_nc;

  int entries_datasummary;
  int entries_orphan;
  int entries_timestamps;
  int entries_timestamps_pmt;
  int entries_timestamps_mrd;

  std::vector<int> vector_triggerwords;
  std::map<int,int> map_triggerword_color;


};


#endif
