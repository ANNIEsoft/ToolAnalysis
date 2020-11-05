#ifndef MonitorTankTime_H
#define MonitorTankTime_H

#include <string>
#include <iostream>
#include <vector>

#include "Tool.h"
#include <Store.h>
#include <BoostStore.h>
#include <CardData.h>
#include <TriggerData.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include "TCanvas.h"
#include "TLegend.h"
#include "TF1.h"
#include "TThread.h"
#include "MRDOut.h"
#include "TApplication.h"
#include "TLegend.h"
#include "TPaletteAxis.h"
#include "TGraph.h"
#include "TMultiGraph.h"
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

  //configuration and initialization functions
  void ReadInConfiguration();
  void InitializeHists(); ///< Function to initialize all histograms and canvases
  void LoopThroughDecodedEvents(std::map<uint64_t, std::map<std::vector<int>, std::vector<uint16_t>>> finishedPMTWaves);
  void WriteToFile();
  void ReadFromFile(ULong64_t timestamp_end, double time_frame);

  //Draw functions
  void DrawLastFilePlots();
  void DrawBufferPlots();         //show the actual traces of PMTs for the data of the last file
  void DrawADCFreqPlots();        //show the ADC frequency histograms for the last file
  void DrawFIFOPlots();		  //show the number of FIFO errors for the last file
  void UpdateMonitorPlots(std::vector<double> timeFrames, std::vector<ULong64_t> endTimes, std::vector<std::string> fileLabels, std::vector<std::vector<std::string>> plotTypes);
  void DrawRatePlotElectronics(ULong64_t timestamp_end, double time_frame, std::string file_ending);
  void DrawRatePlotPhysical(ULong64_t timestamp_end, double time_frame, std::string file_ending);
  void DrawPedPlotElectronics(ULong64_t timestamp_end, double time_frame, std::string file_ending);
  void DrawPedPlotPhysical(ULong64_t timestamp_end, double time_frame, std::string file_ending);
  void DrawSigmaPlotElectronics(ULong64_t timestamp_end, double time_frame, std::string file_ending);
  void DrawSigmaPlotPhysical(ULong64_t timestamp_end, double time_frame, std::string file_ending);
  void DrawTimeEvolution(ULong64_t timestamp_end, double time_frame, std::string file_ending);
  void DrawTimeDifference(ULong64_t timestamp_end, double time_frame, std::string file_ending);
  void DrawHitMap(ULong64_t timestamp_end, double time_frame, std::string file_ending);
  void DrawVMEHistogram();

  void DrawFileHistory(ULong64_t timestamp_end, double time_frame, std::string file_ending, int _linewidth);
  void PrintFileTimeStamp(ULong64_t timestamp_end, double time_frame, std::string file_ending);

  //helper functions
  std::string convertTimeStamp_to_Date(ULong64_t timestamp);
  bool does_file_exist(std::string filename);
  void CardIDToElectronicsSpace(int CardID, int &CrateNum, int &SlotNum);


 private:

  //configfile variables
  std::string outpath_temp;
  std::string outpath;
  std::string active_slots;
  std::string StartTime;
  std::string path_monitoring;
  std::string img_extension;
  double update_frequency;
  bool force_update;
  bool draw_marker;
  bool draw_single;
  std::string plot_configuration;
  int verbosity;
  std::string signal_channels;
  std::string disabled_channels;

  //define variables that contain the configuration option for the plots
  std::vector<double> config_timeframes;
  std::vector<std::string> config_endtime;
  std::vector<std::string> config_label;
  std::vector<std::vector<std::string>> config_plottypes;
  std::vector<ULong64_t> config_endtime_long;

  
  //CStore variables
  std::map<uint64_t, std::map<std::vector<int>, std::vector<uint16_t> > > FinishedPMTWaves;  //MCT, vector<int,int>{crate,slot}, vector<int>{waveform]
  std::map<std::vector<int>,int>* PMTCrateSpaceToChannelNumMap = nullptr;


  //variables for time calculations
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


  //variables to convert times
  double MSEC_to_SEC = 1000.;
  double SEC_to_MIN = 60.;
  double MIN_to_HOUR = 60.;
  double HOUR_to_DAY = 24.;
  ULong64_t utc_to_fermi = 2.7e12;  //6h clock delay in ADC clocks (timestamps in UTC time compared to Fermilab time)
  ULong64_t utc_to_t=21600000;  //6h clock delay in millisecons


  //hardware variables
  const int num_crates_tank = 3;
  const int num_slots_tank = 21;
  const int num_channels_tank = 4;
  unsigned int u_num_channels_tank = 4;
  std::vector<int> crate_numbers;   //vector container to store the numbers of the employed VME crates
  unsigned int Crate_BRF, Crate_RWM, Slot_BRF, Slot_RWM, Channel_BRF, Channel_RWM;
  int ADC_TO_NS = 2;	//each ADC tick corresponds to 2 ns

  
  //several containers storing the position of active cards within the VME crates follow
  int num_active_slots, num_active_slots_cr1, num_active_slots_cr2, num_active_slots_cr3;
  int active_channel_cr1[21]={0};   
  int active_channel_cr2[21]={0};
  int active_channel_cr3[21]={0};
  std::vector<unsigned int> active_slots_cr1;
  std::vector<unsigned int> active_slots_cr2;
  std::vector<unsigned int> active_slots_cr3;
  std::map<std::vector<unsigned int>,int> map_crateslotch_to_ch;
  std::map<std::vector<unsigned int>,int> map_crateslot_to_slot;  //map crate & slot id to the position in the data storing vectors
  std::map<int,std::vector<unsigned int>> map_ch_to_crateslotch;
  std::map<int,std::vector<unsigned int>> map_slot_to_crateslot;
  std::vector<std::vector<unsigned int>> vec_disabled_channels;
  std::vector<int> vec_disabled_global;
  std::vector<std::vector<int>> inactive_xy;

  //geometry variables
  Geometry *geom = nullptr;
  double tank_center_x, tank_center_y, tank_center_z;


  //define live storing variables (for data of current file)
  std::vector<std::vector<double>> ped_file, sigma_file, rate_file;
  std::vector<std::vector<int>> samples_file;
  std::vector<uint64_t> timestamp_file;
  long t_file_start, t_file_end;
  int minimum_adc = 200;          //define x-borders of ADC histogram plots, does 200 and 400 make sense?
  int maximum_adc = 400;          //define x-borders of ADC histogram plots, does 200 and 400 make sense?
  double conversion_ADC_Volt = 2.415/pow(2.0, 12.0);          //conversion ADC counts --> Volts


  //define storing variables for reading in data in a given time slot from monitoring file / database
  std::vector<std::vector<double>> ped_plot;
  std::vector<std::vector<double>> sigma_plot;
  std::vector<std::vector<double>> rate_plot;
  std::vector<std::vector<int>> channelcount_plot;
  std::vector<ULong64_t> tstart_plot;
  std::vector<ULong64_t> tend_plot;


  //histograms to display the current VME properties
  TH2F *h2D_ped = nullptr;        //define 2D histograms to show the current rates, pedestal values, sigma values
  TH2F *h2D_sigma = nullptr;
  TH2F *h2D_rate = nullptr; 
  TH2F *h2D_peddiff = nullptr;
  TH2F *h2D_sigmadiff = nullptr;
  TH2F *h2D_ratediff = nullptr;
  TH2F *h2D_fifo1 = nullptr;
  TH2F *h2D_fifo2 = nullptr;
  std::vector<TH1F*> hChannels_temp;  //temp plots for each channel
  std::vector<TH1I*> hChannels_freq;  //frequency plots for each channel
  TH1I* hChannels_BRF = nullptr;
  TH1I* hChannels_RWM = nullptr;
  TH1F* hChannels_temp_BRF = nullptr;
  TH1F* hChannels_temp_RWM = nullptr;
  std::vector<TH1F*> hist_hitmap;
  std::vector<TH1F*> hist_hitmap_slot;
  std::map<unsigned int,std::vector<TBox*>> vector_box_inactive_hitmap;
  TCanvas *canvas_ped = nullptr;
  TCanvas *canvas_sigma = nullptr;
  TCanvas *canvas_rate = nullptr;
  TCanvas *canvas_peddiff = nullptr;
  TCanvas *canvas_sigmadiff = nullptr;
  TCanvas *canvas_ratediff = nullptr;
  TCanvas *canvas_ch_single_tank = nullptr;
  TCanvas *canvas_fifo = nullptr;
  TCanvas *canvas_vme = nullptr;
  TCanvas *canvas_hitmap_tank = nullptr;
  TCanvas *canvas_hitmap_tank_slot = nullptr;
  std::vector<TCanvas*> canvas_Channels_temp;
  std::vector<TCanvas*> canvas_Channels_freq;


  //graphs to display time evolution plots
  std::vector<TGraph*> gr_ped, gr_sigma, gr_rate;
  TMultiGraph *multi_ch_ped = nullptr;
  TMultiGraph *multi_ch_sigma = nullptr;
  TMultiGraph *multi_ch_rate = nullptr;
  TLegend *leg_ped = nullptr;
  TLegend *leg_sigma = nullptr;
  TLegend *leg_rate = nullptr;
  TCanvas *canvas_ch_ped = nullptr;
  TCanvas *canvas_ch_sigma = nullptr;
  TCanvas *canvas_ch_rate_tank = nullptr;


  //helper root objects / visualization
  std::vector<TBox*> vector_box_inactive;
  TLine *line1 = nullptr;
  TLine *line2 = nullptr;
  TLine *separate_crates = nullptr;
  TLine *separate_crates2 = nullptr;
  TText *label_rate = nullptr;
  TLatex *label_ped = nullptr;
  TLatex *label_sigma = nullptr;
  TLatex *label_peddiff = nullptr;
  TLatex *label_sigmadiff = nullptr;
  TLatex *label_ratediff = nullptr;
  TLatex *label_fifo = nullptr;
  std::vector<TDatime> labels_timeaxis;
  TText *text_crate1 = nullptr;
  TText *text_crate2 = nullptr;
  TText *text_crate3 = nullptr;
  TLegend *leg_freq = nullptr;
  TLegend *leg_temp = nullptr;
  TPaveText *label_cr1 = nullptr;
  TPaveText *label_cr2 = nullptr;
  TPaveText *label_cr3 = nullptr;
  TF1 *f1 = nullptr;
  std::string str_ped, str_sigma, str_rate, str_peddiff, str_sigmadiff, str_ratediff, str_fifo1, str_fifo2;
  std::string crate_str, slot_str, ch_str;
  std::stringstream ss_title_ped, ss_title_sigma, ss_title_rate, ss_title_peddiff, ss_title_sigmadiff, ss_title_ratediff, ss_title_fifo1, ss_title_fifo2;
  std::stringstream ss_title_hist, ss_title_hist_temp;

  //Histograms of clustered hits
  TH1F *hist_vme = nullptr;
  TH1F *hist_vme_cluster = nullptr;
  TH1F *hist_vme_cluster_20 = nullptr;
  std::vector<std::vector<int>> overall_coinc_times;

  //define histogram showing the history (log) of files 
  TH1F *log_files=nullptr;
  TCanvas *canvas_logfile_tank = nullptr;
  TCanvas *canvas_file_timestamp_tank = nullptr;
  int num_files_history;


  //vectors to store relevant quantities to be plotted
  std::vector<double> channels_rate;  //map of PMT VME channel # to the rate of the respective PMT
  std::vector<unsigned long> channels_timestamps;
  std::vector<double> channels_mean;
  std::vector<double> channels_sigma;
  std::vector<TF1*> vector_gaus;
  std::vector<int> fifo1, fifo2;
  std::vector<std::vector<std::vector<int>>> channels_times;	//hittimes of all PMTs to create clustered plots

  //verbosity variables
  int v_error = 0;
  int v_warning = 1;
  int v_message = 2;
  int v_debug = 3;

};

#endif
