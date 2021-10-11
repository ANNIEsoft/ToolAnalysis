#ifndef MonitorLAPPDDataSingle_H
#define MonitorLAPPDDataSingle_H

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <bitset>

#include "Tool.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TText.h"
#include "TROOT.h"
#include "TObjectTable.h"


/**
 * \class MonitorLAPPDDataSingle
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: M. Nieslony $
* $Date: 2021/07/21 10:44:00 $
* Contact: mnieslon@uni-mainz.de
*/
class MonitorLAPPDDataSingle: public Tool {


 public:

  MonitorLAPPDDataSingle(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  void InitializeHistsLAPPDLive();
  void LoadACDCBoardConfig(std::string acdc_config);
  void ProcessLAPPDDataLive();
  
  void DrawLivePlots();
  void DrawLiveHistograms();
  void DrawLiveStatus();
  void DrawLiveTimestampEvolution();

 private:

   int verbosity;
   std::string path_monitoring;
   std::string acdc_configuration;
   std::string img_extension;
   std::string outpath_temp;
   std::string outpath;
   std::string StartTime;

   //Board configuration variables
   std::vector<int> board_configuration;

   //Time reference variables
   boost::posix_time::ptime *Epoch = nullptr;
   boost::posix_time::ptime current;
   boost::posix_time::time_duration period_update;
   boost::posix_time::time_duration duration;
   boost::posix_time::ptime last;
   boost::posix_time::ptime utc;
   boost::posix_time::time_duration current_stamp_duration;
   boost::posix_time::time_duration current_utc_duration;

   //Variables to convert times
   double MSEC_to_SEC = 1000.;
   double SEC_to_MIN = 60.;
   double MIN_to_HOUR = 60.;
   double HOUR_to_DAY = 24.;
   ULong64_t utc_to_fermi = 2.7e12;  //6h clock delay in ADC clocks (timestamps in UTC time compared to Fermilab time)
   ULong64_t utc_to_t=21600000;  //6h clock delay in millisecons
   double CLOCK_to_SEC = 3.125e-9;	//320MHz clock -> 1/320MHz = 3.125ns

  //Data
  BoostStore *LAPPDData = nullptr;


   TCanvas *canvas_live_status = nullptr;
   TCanvas *canvas_live_adc_channel = nullptr;
   TCanvas *canvas_live_buffer_channel = nullptr;
   TCanvas *canvas_live_waveform_channel = nullptr;
   TCanvas *canvas_live_timeevolution = nullptr;
   TCanvas *canvas_live_occupancy = nullptr;
   TCanvas *canvas_live_timealign_10 = nullptr;
   TCanvas *canvas_live_timealign_100 = nullptr;
   TCanvas *canvas_live_timealign_1000 = nullptr;
   TCanvas *canvas_live_timealign_10000 = nullptr;

   TText *text_live_status = nullptr;
   std::map<int,TText*> text_live_timestamp;

   std::map<int,TH2F*> hist_live_adc;
   std::map<int,TH2F*> hist_live_buffer;
   std::map<int,TH1F*> hist_live_time;
   std::map<int,TH1F*> hist_live_occupancy;
   std::map<int,TH2F*> hist_live_waveform;
   std::map<int,TH1F*> hist_live_timealign_10;
   std::map<int,TH1F*> hist_live_timealign_100;
   std::map<int,TH1F*> hist_live_timealign_1000;
   std::map<int,TH1F*> hist_live_timealign_10000;

   std::map<int, double> current_buffer_size;
   std::map<int, uint64_t> current_beam_timestamp;
   std::map<int, uint64_t> current_timestamp;
   std::map<int, int> current_numchannels;
   std::map<int, bool> current_hasdata;
   int current_boardidx;

   std::map<int,std::vector<ULong64_t>> vec_timestamps_10;
   std::map<int,std::vector<ULong64_t>> vec_timestamps_100;
   std::map<int,std::vector<ULong64_t>> vec_timestamps_1000;
   std::map<int,std::vector<ULong64_t>> vec_timestamps_10000;
   std::map<int,std::vector<ULong64_t>> vec_beamgates_10;
   std::map<int,std::vector<ULong64_t>> vec_beamgates_100;
   std::map<int,std::vector<ULong64_t>> vec_beamgates_1000;
   std::map<int,std::vector<ULong64_t>> vec_beamgates_10000;

   int v_error = 0;
   int v_warning = 1;
   int v_message = 2;
   int v_debug = 3;
   int vv_debug = 4;

};


#endif
