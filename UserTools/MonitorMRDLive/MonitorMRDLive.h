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
#include "TText.h"
#include "TLatex.h"
#include "TDatime.h"

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
  std::string outpath_temp;
  std::string outpath;
  MRDOut MRDout;      //the class that has all the information about the mrd data format
  std::string active_slots;
  std::string inactive_channels;
  std::string loopback_channels;
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
  std::vector<unsigned int> loopback_crate, loopback_slot, loopback_channel;
  std::vector<std::string> loopback_name;

  //MRD store includes the following variables
  unsigned int OutN, Trigger;
  std::vector<unsigned int> Value, Slot, Channel, Crate;
  std::vector<std::string> Type;
  ULong64_t TimeStamp;
  long current_stamp;

  //time variables
  boost::posix_time::ptime *Epoch;
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
  std::vector<std::string> vector_triggertype;
  std::map<std::string,int> map_triggertype_color;
  std::vector<TText*> trigger_labels;
  std::vector<TLine*> vector_lines;

  //current event histograms
  TH1I *hChannel_cr1 = nullptr;
  TH1I *hChannel_cr2 = nullptr;   //two separate histograms for the channels of the two respective crates
  TH1D *hSlot_cr1 = nullptr;
  TH1D *hSlot_cr2 = nullptr;
  TH1D *hCrate = nullptr;         //average crate/slot information
  TH2I *h2D_cr1 = nullptr;
  TH2I *h2D_cr2 = nullptr;        //2D representation of channels, slots
  TH1F *hTimes = nullptr;

  //integrated rate histograms of live event information
  TH2F *rate_crate1=nullptr;
  TH2F *rate_crate1_hour=nullptr;;
  TH2F *rate_crate2=nullptr;
  TH2F *rate_crate2_hour=nullptr;
  TH1F *TDC_hist=nullptr;
  TH1F *TDC_hist_hour=nullptr;
  TH1F *n_paddles_hit=nullptr;
  TH1F *n_paddles_hit_hour=nullptr;
  TH1F *log_live_events=nullptr;
  int n_bins_loglive;

  //canvas (current event histograms)
  TCanvas *c_FreqChannels = nullptr;
  TCanvas *c_FreqSlots = nullptr;
  TCanvas *c_FreqCrates = nullptr;
  TCanvas *c_Freq2D = nullptr;
  TCanvas *c_Times = nullptr;
  TCanvas *c_loglive = nullptr;

  //canvas (integrated rate histograms)
  TCanvas *canvas_rates = nullptr;
  TCanvas *canvas_rates_hour = nullptr;
  TCanvas *canvas_tdc_live = nullptr;
  TCanvas *canvas_tdc_hour = nullptr;
  TCanvas *canvas_npaddles = nullptr;
  TCanvas *canvas_npaddles_hour = nullptr;

  //helper objects for visualization purposes
  const char *labels_crate[2]={"Rack 7","Rack 8"};
  std::vector<TBox*> vector_box_inactive;
  int inactive_crate1, inactive_crate2;
  TPaveText *label_cr1 = nullptr;
  TPaveText *label_cr2 = nullptr;
  TLatex *label_rate_cr1 = nullptr;
  TLatex *label_rate_cr2 = nullptr; 
  TText *label_tdc = nullptr;

};


#endif
