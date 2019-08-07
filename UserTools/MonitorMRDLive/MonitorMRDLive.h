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

class MonitorMRDLive: public Tool {

 public:

  MonitorMRDLive();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

  void MRDTDCPlots();

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

  time_t t;
  std::stringstream title_time;         //for plotting the date & time on each monitoring plot

  int active_channel[2][24]={{0}};
  int n_active_slots_cr1, n_active_slots_cr2;
  int num_active_slots;
  std::vector<int> active_slots_cr1;
  std::vector<int> active_slots_cr2;
  std::vector<int> mapping_vector_ch;

  //MRD store includes the following variables
  unsigned int OutN, Trigger;
  std::vector<unsigned int> Value, Slot, Channel, Crate;
  std::vector<std::string> Type;
  ULong64_t TimeStamp;
  

};


#endif
