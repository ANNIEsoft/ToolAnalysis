#ifndef MonitorLAPPDSC_H
#define MonitorLAPPDSC_H

#include <string>
#include <iostream>

#include "Tool.h"

#include "TCanvas.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TLegend.h"


/**
 * \class MonitorLAPPDSC
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class MonitorLAPPDSC: public Tool {


 public:

  MonitorLAPPDSC(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  //TODO: Add functions

 private:

  std::string outpath_temp;
  std::string StartTime;
  double update_frequency;
  std::string path_monitoring;
  std::string img_extension;
  bool force_update:
  bool draw_marker;
  int verbosity;

  //Time reference
  boost::posix_time::ptime *Epoch = nullptr;

  //Plotting variables in vectors
  std::vector<ULong64_t> times_plot;
  std::vector<float> temp_plot;
  std::vector<float> humidity_plot;
  std::vector<int> hv_mon_plot;
  std::vector<float> hv_volt_plot;
  std::vector<bool> hv_stateset_plot;
  std::vector<int> lv_mon_plot;
  std::vector<bool> lv_stateset_plot;
  std::vector<float> lv_v33_plot;
  std::vector<float> lv_v21_plot;
  std::vector<float> lv_v12_plot;
  std::vector<float> hum_low_plot;
  std::vector<float> hum_high_plot;
  std::vector<float> temp_low_plot;
  std::vector<float> temp_high_plot;
  std::vector<int> flag_temp_plot;
  std::vector<int> flag_hum_plot;
  std::vector<bool> relCh1_plot;
  std::vector<bool> relCh2_plot;
  std::vector<bool> relCh3_plot;
  std::vector<bool> relCh1mon_plot;
  std::vector<bool> relCh2mon_plot;
  std::vector<bool> relCh3mon_plot;
  std::vector<float> trig1_plot;
  std::vector<float> trig0_plot;
  std::vector<float> trig1thr_plot;
  std::vector<float> trig0thr_plot;
  std::vector<float> trig_vref_plot;
  std::vector<float> light_plot;
  std::vector<int> num_errors_plot;
  //TODO: Add timelabel vector

  //canvas
  TCanvas *canvas_temp = nullptr;
  TCanvas *canvas_humidity = nullptr;
  TCanvas *canvas_light = nullptr;
  TCanvas *canvas_hv = nullptr;
  TCanvas *canvas_lv = nullptr;
  
  //graphs
  TGraph *graph_temp = nullptr;
  TGraph *graph_humidity = nullptr;
  TGraph *graph_light = nullptr;
  TGraph *graph_hv_volt = nullptr;
  TGraph *graph_lv_volt1 = nullptr;
  TGraph *graph_lv_volt2 = nullptr;
  TGraph *graph_lv_volt3 = nullptr;
  
  //multi-graphs
  TMultiGraph *multi_lv = nullptr;
  
  //legends
  TLegend *leg_lv = nullptr;

  //Verbosity variables
  int v_error = 0;
  int v_warning = 1;
  int v_message = 2;
  int v_debug = 3;
  int vv_debug = 4;

};


#endif
