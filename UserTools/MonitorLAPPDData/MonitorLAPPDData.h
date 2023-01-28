#ifndef MonitorLAPPDData_H
#define MonitorLAPPDData_H

#include <string>
#include <iostream>
#include <vector>
#include <bitset>

#include "Tool.h"
#include <Store.h>
#include <BoostStore.h>
#include <SlowControlMonitor.h>
#include <PsecData.h>
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
#include "TF1.h"
#include "TLine.h"

/**
 * \class MonitorLAPPDData
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: M. Stender, M. Nieslony $
* $Date: 2021/10/11 13:00:00 $
* Contact: malte.stender@desy.de, mnieslon@uni-mainz.de
*/

class MonitorLAPPDData: public Tool {


 public:

  MonitorLAPPDData(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  //Configuration functions
  void ReadInConfiguration();
  void InitializeHistsLAPPD();
  void LoadACDCBoardConfig(std::string acdc_config);

  //Read/Write functions
  void ProcessLAPPDData();
  void WriteToFile();
  void ReadFromFile(ULong64_t timestamp, double time_frame);

  //Draw functions
  void DrawLastFilePlots();
  void UpdateMonitorPlotsLAPPD(std::vector<double> timeFrames, std::vector<ULong64_t> endTimes, std::vector<std::string> fileLabels, std::vector<std::vector<std::string>> plotTypes);
  void DrawStatus_PsecData();
  void DrawLastFileHists();
  void DrawTimeEvolutionLAPPDData(ULong64_t timestamp_end, double time_frame, std::string file_ending);
  void DrawTimeAlignment();
  void DrawFileHistoryLAPPD(ULong64_t timestamp_end, double time_frame, std::string file_ending, int _linewidth);
  void PrintFileTimeStampLAPPD(ULong64_t timestamp_end, double time_frame, std::string file_ending);

  //Helper functions
  std::string convertTimeStamp_to_Date(ULong64_t timestamp);
  bool does_file_exist(std::string filename);
  void ModifyBeamgateData(size_t numberOfFiles, int boardNumber, std::map<int,std::vector<std::vector<uint64_t>>> &dataVector);
  void PedestalFits(int board_nr, int i_board);
  void GetRunSubPart(std::string filename);

 private:

  //Configuration variables
  std::string outpath_temp;
  std::string outpath;
  std::string StartTime;
  std::string referenceDate;
  std::string referenceTime;
  std::string referenceTimeDate;
  double update_frequency;
  std::string path_monitoring;
  std::string img_extension;
  bool force_update;
  bool draw_marker;
  int verbosity;
  std::string plot_configuration;
  double threshold_pulse;
  bool sync_reference_time;
  //Plot configuration variables
  std::vector<double> config_timeframes;
  std::vector<std::string> config_endtime;
  std::vector<std::string> config_label;
  std::vector<std::vector<std::string>> config_plottypes;
  std::vector<ULong64_t> config_endtime_long;
  int num_history_lappd;

  //Board configuration variables
  std::vector<int> board_configuration;
  std::vector<int> board_channel;

  //Time reference variables
  boost::posix_time::ptime *Epoch = nullptr;
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
  ULong64_t t_current;

  //Variables to convert times
  double MSEC_to_SEC = 1000.;
  double SEC_to_MIN = 60.;
  double MIN_to_HOUR = 60.;
  double HOUR_to_DAY = 24.;
  ULong64_t utc_to_fermi = 2.7e12;  //6h clock delay in ADC clocks (timestamps in UTC time compared to Fermilab time)
  ULong64_t utc_to_t=21600000;  //6h clock delay in millisecons
  double CLOCK_to_SEC = 3.125e-9;	//320MHz clock -> 1/320MHz = 3.125ns
  ULong64_t reference_time;
  ULong64_t last_pps_timestamp;

  //Geometry variables
  Geometry *geom = nullptr;
  double tank_center_x, tank_center_y, tank_center_z;

  //Data
  PsecData lappd_psec;
  BoostStore *LAPPDData = nullptr;

  //Processed psec data (last file)
  //Averaged values - one vector entry per board (last file)
  std::vector<double> current_pps_rate;
  std::vector<double> current_frame_rate;
  std::vector<double> current_beamgate_rate;
  std::vector<double> current_int_charge;
  std::vector<double> current_buffer_size;
  std::vector<int> current_board_index;
  std::vector<int> current_num_entries;
  std::vector<uint64_t> first_beamgate_timestamp;
  std::vector<uint64_t> last_beamgate_timestamp;
  std::vector<uint64_t> first_timestamp;
  std::vector<uint64_t> last_timestamp;
  std::vector<uint64_t> first_pps_timestamps;
  std::vector<uint64_t> last_pps_timestamps;
  std::vector<bool> first_entry;
  std::vector<bool> first_entry_pps;
  std::vector<int> n_buffer;
  std::vector<int> n_data;
  std::vector<int> n_pps;
  std::vector<uint64_t> t_file_end;
  ULong64_t t_file_start;
  ULong64_t t_file_end_global;
  int current_run;
  int current_subrun;
  int current_partrun;
  int current_pps_count;
  int current_frame_count; 
  int totalPPSCount = 0;
  int totalFrameCount = 0;
  int totalRun = 0; 

  //Averaged values - one vector entry per chkey (last file)
  std::vector<int> current_chkey;
  std::vector<double> current_rate;
  std::vector<double> current_ped;
  std::vector<double> current_sigma;

  //Single values - multiple vector entries per board (last file)
  std::vector<uint64_t> beamgate_timestamp;
  std::vector<uint64_t> data_timestamp;
  std::vector<uint64_t> pps_timestamp;
  std::vector<int> board_index;
  std::vector<int> num_channels;
  std::vector<double> average_buffer;
  std::map<int,std::vector<int>> buffer_size;
  std::map<int,std::vector<uint64_t>> data_beamgate_lastfile;
  std::map<int,std::vector<std::vector<uint64_t>>> data_beamgate_last5files;
  std::map<int,std::vector<std::vector<uint64_t>>> data_beamgate_last10files;
  std::map<int,std::vector<std::vector<uint64_t>>> data_beamgate_last20files;
  std::map<int,std::vector<std::vector<uint64_t>>> data_beamgate_last100files;
  std::map<int,std::vector<std::vector<uint64_t>>> data_beamgate_last1000files;

  //Plotting variables in vectors (multiple files)
  std::map<int,std::vector<ULong64_t>> data_times_plot;
  std::map<int,std::vector<ULong64_t>> data_times_end_plot;
  std::map<int,std::vector<double>> pps_rate_plot;
  std::map<int,std::vector<double>> frame_rate_plot;
  std::map<int,std::vector<double>> beamgate_rate_plot;
  std::map<int,std::vector<double>> int_charge_plot;
  std::map<int,std::vector<double>> buffer_size_plot;
  std::map<int,std::vector<int>> num_channels_plot;
  std::map<int,std::vector<TDatime>> labels_timeaxis;
  std::map<int,std::vector<int> > num_entries;
  std::map<int,std::vector<double> > mean_pedestal;
  std::map<int,std::vector<double> > sigma_pedestal;
  std::map<int,std::vector<double> > rate_pedestal;
  std::map<int,std::vector<double> > ped_plot;
  std::map<int,std::vector<double> > sigma_plot;
  std::map<int,std::vector<double> > rate_plot;
  std::vector<int> run_plot;
  std::vector<int> subrun_plot;
  std::vector<int> partrun_plot;
  std::vector<ULong64_t> lappdoffset_plot;
  std::vector<int> ppscount_plot;
  std::vector<int> framecount_plot;

  //canvas
  TCanvas *canvas_status_data = nullptr;
  TCanvas *canvas_pps_rate = nullptr;
  TCanvas *canvas_frame_rate = nullptr;
  TCanvas *canvas_buffer_size = nullptr;
  TCanvas *canvas_int_charge = nullptr;
  TCanvas *canvas_align_1file = nullptr;
  TCanvas *canvas_align_5files = nullptr;
  TCanvas *canvas_align_10files = nullptr;
  TCanvas *canvas_align_20files = nullptr;
  TCanvas *canvas_align_100files = nullptr;
  TCanvas *canvas_align_1000files = nullptr;
  TCanvas *canvas_adc_channel = nullptr;
  TCanvas *canvas_waveform = nullptr;
  TCanvas *canvas_buffer_channel = nullptr;
  TCanvas *canvas_buffer = nullptr;
  TCanvas *canvas_waveform_voltages = nullptr;
  TCanvas *canvas_waveform_onedim = nullptr;
  TCanvas *canvas_pedestal = nullptr;
  TCanvas *canvas_pedestal_all = nullptr;
  TCanvas *canvas_pedestal_difference = nullptr;
  TCanvas *canvas_buffer_size_all = nullptr;
  TCanvas *canvas_rate_threshold_all = nullptr;
  TCanvas *canvas_logfile_lappd = nullptr;
  TCanvas *canvas_file_timestamp_lappd = nullptr;
  TCanvas *canvas_events_per_channel = nullptr;
  TCanvas *canvas_ped_lappd = nullptr;
  TCanvas *canvas_sigma_lappd = nullptr;
  TCanvas *canvas_rate_lappd = nullptr;
  TCanvas *canvas_frame_count = nullptr;
  TCanvas *canvas_pps_count = nullptr;

  //graphs
  std::map<int, TGraph*> graph_pps_rate;
  std::map<int, TGraph*> graph_frame_rate;
  std::map<int, TGraph*> graph_buffer_size;
  std::map<int, TGraph*> graph_int_charge;
  std::map<int, TGraph*> graph_rate;
  std::map<int, TGraph*> graph_ped;
  std::map<int, TGraph*> graph_sigma;
  TGraph *graph_pps_count = nullptr;
  TGraph *graph_frame_count = nullptr;

  //multi-graphs
  TMultiGraph *multi_ped_lappd = nullptr;
  TMultiGraph *multi_sigma_lappd = nullptr;
  TMultiGraph *multi_rate_lappd = nullptr;

  //legends
  TLegend *leg_ped_lappd = nullptr;
  TLegend *leg_sigma_lappd = nullptr;
  TLegend *leg_rate_lappd = nullptr;

  //histograms
  std::map<int,TH1F*> hist_align_1file;
  std::map<int,TH1F*> hist_align_5files;
  std::map<int,TH1F*> hist_align_10files;
  std::map<int,TH1F*> hist_align_20files;
  std::map<int,TH1F*> hist_align_100files;
  std::map<int,TH1F*> hist_align_1000files;
  std::map<int,TH2F*> hist_adc_channel;
  std::map<int,TH2F*> hist_waveform_channel;
  std::map<int,TH2F*> hist_buffer_channel;
  std::map<int,TH1F*> hist_buffer;
  std::map<int,TH2F*> hist_waveform_voltages;
  std::vector<std::map<int, std::vector<TH1F*> > > hist_waveforms_onedim;
  std::map<int, std::vector<TH1F*> > hist_pedestal;
  TH2F* hist_pedestal_all = nullptr;
  TH2F* hist_pedestal_difference_all = nullptr;
  TH2F* hist_buffer_size_all = nullptr;
  TH2F* hist_rate_threshold_all = nullptr;
  TH1F *log_files_lappd;
  TH2F* hist_events_per_channel = nullptr;

  //text
  TText *text_data_title = nullptr;
  TText *text_pps_rate = nullptr;
  TText *text_frame_rate = nullptr;
  TText *text_buffer_size = nullptr;
  TText *text_int_charge = nullptr;
  TText *text_pps_count = nullptr;
  TText *text_frame_count = nullptr;

  //Verbosity variables
  int v_error = 0;
  int v_warning = 1;
  int v_message = 2;
  int v_debug = 3;
  int vv_debug = 4;

};


#endif
