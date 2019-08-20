#ifndef MonitorMRDLive_H
#define MonitorMRDLive_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "zmq.h"

#include "TCanvas.h"
#include "TH1I.h"
#include "TH1F.h"
#include "TH2I.h"
#include "TGraph2D.h"
#include "TLegend.h"
#include "TF1.h"
#include "TThread.h"
#include "MRDOut.h"
#include "TPaletteAxis.h"
#include "TPaveText.h"

#include "TObjectTable.h"

#include "TROOT.h"
#include "TList.h"

#include <boost/date_time/posix_time/posix_time.hpp>


class MonitorMRDLive: public Tool {

 public:

  MonitorMRDLive();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

  void MRDTDCPlots();
  void InitializeVectors();
  void EraseOldData();
  void UpdateRatePlots();

 private:

  //config input variables
  BoostStore *CCData;
  std::string outpath;
  MRDOut MRDout;      //the class that has all the information about the mrd data format
  std::string active_slots;
  int verbosity;
  bool draw_average;

  static const int num_crates = 2;      //crate numbers are 7 and 8
  static const int num_slots = 24;      //CAMAC crate has 24 slots
  static const int num_channels = 32;   //each TDC has 32 channels
  static const int min_crate = 7;       //crates 7 and 8 are used, use variable to start numbering from 0

  int active_channel[2][24]={{0}};
  int n_active_slots_cr1, n_active_slots_cr2;
  int num_active_slots;
  std::vector<int> active_slots_cr1;
  std::vector<int> active_slots_cr2;
  std::vector<int> mapping_vector_ch;
  std::vector<int> nr_slot;
  std::vector<int> inactive_ch_crate1, inactive_slot_crate1;
  std::vector<int> inactive_ch_crate2, inactive_slot_crate2;

  //MRD store includes the following variables
  unsigned int OutN, Trigger;
  std::vector<unsigned int> Value, Slot, Channel, Crate;
  std::vector<std::string> Type;
  ULong64_t TimeStamp;
  long current_stamp;

  //time variables
  boost::posix_time::time_duration period_update;
  boost::posix_time::time_duration duration;
  boost::posix_time::ptime last;
  time_t t;
  std::stringstream title_time; 
  double integration_period = 5.;   //5 minutes integration for rate calculation?
  double integration_period_hour = 60.;
  boost::posix_time::ptime current;

  //storing variables
  std::vector<std::vector<unsigned int> > live_tdc, vector_tdc, live_tdc_hour, vector_tdc_hour;
  std::vector<unsigned int> vector_nchannels, vector_nslots, vector_nchannels_hour;
  std::vector<unsigned int> tdc_paddle, tdc_paddle_hour;
  std::vector<std::vector<ULong64_t> > live_timestamp, live_timestamp_hour;
  std::vector<ULong64_t> vector_timestamp, vector_timestamp_hour;

  //rate histograms
  TH2F *rate_crate1=nullptr;
  TH2F *rate_crate1_hour=nullptr;;
  TH2F *rate_crate2=nullptr;
  TH2F *rate_crate2_hour=nullptr;
  TH1F *TDC_hist=nullptr;
  TH1F *TDC_hist_hour=nullptr;
  TH1F *TDC_hist_coincidence=nullptr;
  TH1F *n_paddles_hit=nullptr;
  TH1F *n_paddles_hit_hour=nullptr;
  TH1F *n_paddles_hit_coincidence=nullptr;
  double min_ch, max_ch;
  bool update_plots;
  

};


#endif
