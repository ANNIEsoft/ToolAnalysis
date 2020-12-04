#ifndef MonitorDAQ_H
#define MonitorDAQ_H

#include <string>
#include <iostream>
#include <curl/curl.h>

#include <boost/algorithm/string.hpp>

#include "Tool.h"

#include "TH1F.h"
#include "TCanvas.h"
#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TLatex.h"
#include "TText.h"
#include "TLine.h"
#include "TGraph.h"
#include "TLegend.h"
#include "TObjectTable.h"
#include "TPie.h"
#include "TPieSlice.h"
#include "TMath.h"

#include "ServiceDiscovery.h"
#include "zmq.hpp"


/**
 * \class MonitorDAQ
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: M.Nieslony $
* $Date: 2020/11/26 16:51:00 $
* Contact: mnieslon@uni-mainz.de
*/
class MonitorDAQ: public Tool {


 public:

  MonitorDAQ(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  //Configuration and initialization functions
  void ReadInConfiguration();
  void InitializeHists();
  void GetFileInformation();
  void WriteToFile();
  void ReadFromFile(ULong64_t timestamp_end, double time_frame);
  void UpdateMonitorPlots(std::vector<double> timeFrames, std::vector<ULong64_t> endTimes, std::vector<std::string> fileLabels, std::vector<std::vector<std::string>> plotTypes);

  void GetVMEServices(bool is_online);

  void DrawVMEService(ULong64_t timestamp_end, double time_frame, std::string file_ending, bool current);
  void PrintInfoBox();
  void DrawDAQTimeEvolution(ULong64_t timestamp_end, double time_frame, std::string file_ending);

  //helper functions
  std::string convertTimeStamp_to_Date(ULong64_t timestamp);
  bool does_file_exist(std::string filename);

 private:
 
  //Configuration variables
  std::string outpath_temp;
  std::string outpath;
  std::string StartTime;
  double update_frequency;
  std::string plot_configuration;
  std::string path_monitoring;
  std::string img_extension;
  bool force_update;
  bool draw_marker;
  int verbosity;
  bool online;
  std::string hook;
  bool send_slack;

  //Configuration option for plots
  std::vector<double> config_timeframes;
  std::vector<std::string> config_endtime;
  std::vector<std::string> config_label;
  std::vector<std::vector<std::string>> config_plottypes;
  std::vector<ULong64_t> config_endtime_long;

  //Variables for time calculations
  boost::posix_time::ptime *Epoch;
  boost::posix_time::ptime current;
  boost::posix_time::time_duration period_update;
  boost::posix_time::time_duration duration;
  boost::posix_time::ptime last;
  boost::posix_time::ptime utc;
  boost::posix_time::time_duration current_stamp_duration;
  boost::posix_time::time_duration current_utc_duration;
  time_t t;
  std::stringstream title_time;
  long current_stamp, current_utc;
  ULong64_t readfromfile_tend;
  double readfromfile_timeframe;

  //Variables to convert times
  double MSEC_to_SEC = 1000.;
  double SEC_to_MIN = 60.;
  double MIN_to_HOUR = 60.;
  double HOUR_to_DAY = 24.;
  ULong64_t utc_to_fermi = 2.7e12;  //6h clock delay in ADC clocks (timestamps in UTC time compared to Fermilab time)
  ULong64_t utc_to_t=21600000;  //6h clock delay in millisecons

  //Live storing variables (for data in current file)
  bool file_has_trig;
  bool file_has_cc;
  bool file_has_pmt;
  std::string file_name;
  uintmax_t file_size_uint;
  double file_size;
  std::time_t file_time;
  ULong64_t file_timestamp;
  int num_vme_service;
  bool file_produced=false;
  long t_file_start, t_file_end;

  //Storing variables for reading in data in given time slot from monitoring file / database
  std::vector<bool> has_trig_plot;
  std::vector<bool> has_cc_plot;
  std::vector<bool> has_pmt_plot;
  std::vector<std::string> filename_plot;
  std::vector<double> filesize_plot;
  std::vector<ULong64_t> filetimestamp_plot;
  std::vector<int> num_vme_plot;
  std::vector<ULong64_t> tstart_plot;
  std::vector<ULong64_t> tend_plot;

  //Online stuff
  zmq::context_t *context = nullptr;
  ServiceDiscovery *SD = nullptr;
  std::string address;
  int port=5000;

  //Labels
  std::vector<TDatime> labels_timeaxis;

  //Text blocks for info box
  TText *text_summary = nullptr;
  TText *text_vmeservice = nullptr;
  TText *text_filesize = nullptr;
  TText *text_filedate = nullptr;
  TText *text_haspmt = nullptr;
  TText *text_hascc = nullptr;
  TText *text_hastrigger = nullptr;
  TText *text_currentdate = nullptr;
  TText *text_filename = nullptr;

  //Pie chart
  TPie *pie_vme = nullptr;
  TLegend *leg_vme = nullptr;  

  //TGraphs & related objects
  TGraph *gr_filesize = nullptr;
  TGraph *gr_vmeservice = nullptr;

  //Canvas
  TCanvas *canvas_infobox = nullptr;
  TCanvas *canvas_vmeservice = nullptr;
  TCanvas *canvas_timeevolution_size = nullptr;
  TCanvas *canvas_timeevolution_vme = nullptr;

  int v_error = 0;
  int v_warning = 1;
  int v_message = 2;
  int v_debug = 3;
  int vv_debug = 4;



};


#endif
