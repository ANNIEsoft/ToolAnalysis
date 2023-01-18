#ifndef MonitorLAPPDSC_H
#define MonitorLAPPDSC_H

#include <string>
#include <iostream>
#include <vector>

#include "Tool.h"
#include <Store.h>
#include <BoostStore.h>
#include <SlowControlMonitor.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include "TMath.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TLegend.h"
#include "TDatime.h"
#include "TPaveText.h"
#include "TFile.h"
#include "TPad.h"
#include "TAxis.h"
#include "TROOT.h"
#include "TH2F.h"
#include "TH1I.h"
#include "TH1F.h"
#include "TObjectTable.h"
#include "TLatex.h"
#include "TText.h"
#include "TTree.h"
#include "TRandom3.h"

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

  //Configuration functions
  void ReadInConfiguration();
  void InitializeHists();

  //Read/Write functions
  void WriteToFile();
  void ReadFromFile(ULong64_t timestamp, double time_frame);

  //Draw functions
  void DrawLastFilePlots();
  void UpdateMonitorPlots(std::vector<double> timeFrames, std::vector<ULong64_t> endTimes, std::vector<std::string> fileLabels, std::vector<std::vector<std::string>> plotTypes);
  void DrawStatus_TempHumidity();
  void DrawStatus_LVHV();
  void DrawStatus_Trigger();
  void DrawStatus_Relay();
  void DrawStatus_Errors();
  void DrawStatus_Overview();
  void DrawTimeEvolutionLAPPDSC(ULong64_t timestamp_end, double time_frame, std::string file_ending);

  //Helper functions
  std::string convertTimeStamp_to_Date(ULong64_t timestamp);
  bool does_file_exist(std::string filename);

 private:

  //Configuration variables
  std::string outpath_temp;
  std::string outpath;
  std::string StartTime;
  double update_frequency;
  std::string path_monitoring;
  std::string img_extension;
  bool force_update;
  bool draw_marker;
  int verbosity;
  std::string plot_configuration;
  double v33_min;
  double v33_max;
  double v25_min;
  double v25_max;
  double v12_min;
  double v12_max;
  double limit_salt_low;
  double limit_salt_high;
  double limit_temperature_low;
  double limit_temperature_high;
  double limit_humidity_low;
  double limit_humidity_high;
  double limit_thermistor_temperature_low;
  double limit_thermistor_temperature_high;
  std::string lappd_id_file;
  std::vector<int> vector_lappd_id;

  //Plot configuration variables
  std::vector<double> config_timeframes;
  std::vector<std::string> config_endtime;
  std::vector<std::string> config_label;
  std::vector<std::vector<std::string>> config_plottypes;
  std::vector<ULong64_t> config_endtime_long;

  //Time reference variables
  boost::posix_time::ptime *Epoch = nullptr;
  boost::posix_time::ptime current;
  boost::posix_time::time_duration period_update;
  boost::posix_time::time_duration period_warning;
  boost::posix_time::time_duration duration;
  std::map<int,boost::posix_time::time_duration> map_duration;
  boost::posix_time::ptime last;
  std::map<int,boost::posix_time::ptime> map_last;
  boost::posix_time::ptime utc;
  boost::posix_time::time_duration current_stamp_duration;
  boost::posix_time::time_duration current_utc_duration;
  time_t t;
  std::stringstream title_time; 
  long current_stamp, current_utc;
  ULong64_t readfromfile_tend;
  double readfromfile_timeframe; 	//TODO: is this set in the code?
  ULong64_t t_current;
  ULong64_t t_last_update;

  //Variables to convert times
  double MSEC_to_SEC = 1000.;
  double SEC_to_MIN = 60.;
  double MIN_to_HOUR = 60.;
  double HOUR_to_DAY = 24.;
  ULong64_t utc_to_fermi = 2.7e12;  //6h clock delay in ADC clocks (timestamps in UTC time compared to Fermilab time)
  ULong64_t utc_to_t=21600000;  //6h clock delay in millisecons

  //Geometry variables
  Geometry *geom = nullptr;
  double tank_center_x, tank_center_y, tank_center_z;

  //Data
  SlowControlMonitor lappd_SC;

  //Plotting variables in vectors
  std::vector<ULong64_t> times_plot;
  std::vector<float> temp_plot;
  std::vector<float> humidity_plot;
  std::vector<float> thermistor_plot;
  std::vector<float> salt_plot;
  std::vector<int> hv_mon_plot;
  std::vector<float> hv_volt_plot;
  std::vector<bool> hv_stateset_plot;
  std::vector<float> hv_returnmon_plot;
  std::vector<int> lv_mon_plot;
  std::vector<bool> lv_stateset_plot;
  std::vector<float> lv_v33_plot;
  std::vector<float> lv_v25_plot;
  std::vector<float> lv_v12_plot;
  std::vector<float> hum_low_plot;
  std::vector<float> hum_high_plot;
  std::vector<float> temp_low_plot;
  std::vector<float> temp_high_plot;
  std::vector<float> thermistor_low_plot;
  std::vector<float> thermistor_high_plot;
  std::vector<int> flag_temp_plot;
  std::vector<int> flag_hum_plot;
  std::vector<int> flag_thermistor_plot;
  std::vector<int> flag_salt_plot;
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
  std::vector<TDatime> labels_timeaxis;
  std::vector<unsigned int> lappdid_plot;

  //canvas
  TCanvas *canvas_temp = nullptr;
  TCanvas *canvas_thermistor = nullptr;
  TCanvas *canvas_salt = nullptr;
  TCanvas *canvas_humidity = nullptr;
  TCanvas *canvas_light = nullptr;
  TCanvas *canvas_hv = nullptr;
  TCanvas *canvas_lv = nullptr;
  TCanvas *canvas_status_temphum = nullptr;
  TCanvas *canvas_status_lvhv = nullptr;
  TCanvas *canvas_status_relay = nullptr;
  TCanvas *canvas_status_trigger = nullptr;
  TCanvas *canvas_status_error = nullptr;  
  TCanvas *canvas_status_overview = nullptr;
  TCanvas *canvas_status_thermistor = nullptr;
  TCanvas *canvas_status_salt = nullptr;

  //graphs
  std::map<int,TGraph*> map_graph_temp;
  std::map<int,TGraph*> map_graph_humidity;
  std::map<int,TGraph*> map_graph_thermistor;
  std::map<int,TGraph*> map_graph_salt;
  std::map<int,TGraph*> map_graph_light;
  std::map<int,TGraph*> map_graph_hv_volt;
  std::map<int,TGraph*> map_graph_hv_volt_mon;
  std::map<int,TGraph*> map_graph_lv_volt1;
  std::map<int,TGraph*> map_graph_lv_volt2;
  std::map<int,TGraph*> map_graph_lv_volt3;
  
  //multi-graphs
  TMultiGraph *multi_lv = nullptr;
  TMultiGraph *multi_temp = nullptr;
  TMultiGraph *multi_humidity = nullptr;
  TMultiGraph *multi_thermistor = nullptr;
  TMultiGraph *multi_salt = nullptr;
  TMultiGraph *multi_light = nullptr;
  TMultiGraph *multi_hv_volt = nullptr;
  TMultiGraph *multi_hv_volt_mon = nullptr;
  TMultiGraph *multi_lv_volt1 = nullptr;
  TMultiGraph *multi_lv_volt2 = nullptr;
  TMultiGraph *multi_lv_volt3 = nullptr;
  
  //legends
  TLegend *leg_lv = nullptr;
  TLegend *leg_temp = nullptr;
  TLegend *leg_humidity = nullptr;
  TLegend *leg_thermistor = nullptr;
  TLegend *leg_salt = nullptr;
  TLegend *leg_light = nullptr;
  TLegend *leg_hv_volt = nullptr;
  TLegend *leg_hv_volt_mon = nullptr;
  TLegend *leg_lv_volt1 = nullptr;
  TLegend *leg_lv_volt2 = nullptr;
  TLegend *leg_lv_volt3 = nullptr;

  //text
  //TempHumidity texts
  TText *text_temphum_title = nullptr;
  TText *text_temp = nullptr;
  TText *text_hum = nullptr;
  TText *text_thermistor = nullptr;
  TText *text_salt = nullptr;
  TText *text_light = nullptr;
  TText *text_flag_temp = nullptr;
  TText *text_flag_hum = nullptr;
  TText *text_flag_thermistor = nullptr;
  TText *text_flag_salt = nullptr;

  //LVHV texts
  TText *text_lvhv_title = nullptr;
  TText *text_hv_state = nullptr;
  TText *text_hv_mon = nullptr;
  TText *text_hv_volt = nullptr;
  TText *text_hv_monvolt = nullptr;
  TText *text_lv_state = nullptr;
  TText *text_lv_mon = nullptr;
  TText *text_v33 = nullptr;
  TText *text_v25 = nullptr;
  TText *text_v12 = nullptr;

  //Trigger texts
  TText *text_trigger_title = nullptr;
  TText *text_trigger_vref = nullptr;
  TText *text_trigger_trig0_thr = nullptr;
  TText *text_trigger_trig0_mon = nullptr;
  TText *text_trigger_trig1_thr = nullptr;
  TText *text_trigger_trig1_mon = nullptr;

  //Relay texts
  TText *text_relay_title = nullptr;
  TText *text_relay_set1 = nullptr;
  TText *text_relay_set2 = nullptr;
  TText *text_relay_set3 = nullptr;
  TText *text_relay_mon1 = nullptr;
  TText *text_relay_mon2 = nullptr;
  TText *text_relay_mon3 = nullptr;

  //Error texts
  TText *text_error_title = nullptr;
  TText *text_error_number1 = nullptr;
  TText *text_error_number2 = nullptr;
  TText *text_error_number3 = nullptr;
  TText *text_error_number4 = nullptr;
  TText *text_error_number5 = nullptr;
  TText *text_error_number6 = nullptr;
  TText *text_error_number7 = nullptr;
  TText *text_error_number8 = nullptr;
  TText *text_error_number9 = nullptr;
  std::vector<TText*> text_error_vector;

  //booleans for summary status
  bool temp_humid_check = true;
  bool lvhv_check = true;
  bool trigger_check = true;
  bool relay_check = true;
  bool error_check = true;
  bool error_warning = false;
  bool relay_warning = false;
  bool trigger_warning = false;
  bool lvhv_warning = false;
  bool temp_humid_warning = false;

  //Overview texts
  TText *text_overview_title = nullptr;
  TText *text_overview_temp = nullptr;
  TText *text_overview_lvhv = nullptr;
  TText *text_overview_trigger = nullptr;
  TText *text_overview_relay = nullptr;
  TText *text_overview_error = nullptr;
  TText *text_current_time = nullptr;
  TText *text_sc_time = nullptr;

  //Verbosity variables
  int v_error = 0;
  int v_warning = 1;
  int v_message = 2;
  int v_debug = 3;
  int vv_debug = 4;

};


#endif
