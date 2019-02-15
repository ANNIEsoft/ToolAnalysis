#ifndef MonitorMRDTime_H
#define MonitorMRDTime_H


#include <string>
#include <iostream>

#include "Tool.h"
#include "zmq.h"

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
#include "TCanvas.h"
#include "TDatime.h"  //for labeling the x-axis with time labels

#include <boost/date_time/posix_time/posix_time.hpp>


class MonitorMRDTime: public Tool {


 public:

  MonitorMRDTime();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

  void MRDTimePlots();
  void UpdateMonitorSources();
  void FillEvents();
  void InitializeVectors();


 private:

  BoostStore *CCData;
  std::string outpath_temp;
  std::string outpath;
  std::string active_slots;
  BoostStore* MRDdata;
  MRDOut MRDout;
  ULong64_t offset_date;        //in milliseconds
  int verbosity;
  bool draw_marker;
  int max_files;

  boost::posix_time::ptime *Epoch;
  std::string StartTime;

  static const int num_crates = 2;
  static const int num_slots = 24;
  static const int num_channels = 32;
  static const int min_crate = 7;
  int num_active_slots;
  int num_active_slots_cr1, num_active_slots_cr2;
  int active_channel[2][24]={{0}};
  std::vector<int> nr_slot;

  double max_canvas, min_canvas, max_canvas_rms, min_canvas_rms, max_canvas_freq, min_canvas_freq;

  //MRD store includes the following variables
  unsigned int OutN, Trigger;
  std::vector<unsigned int> Value, Slot, Channel, Crate;
  std::vector<std::string> Type;
  ULong64_t TimeStamp;
  int nentries_MRDData;
  bool data_available;
  int enum_slots;
  bool initial;

  //time variables to check for regular time intervals
  boost::posix_time::time_duration period_update;
  boost::posix_time::time_duration duration;
  boost::posix_time::ptime last;

  //define live storing variables
  std::vector<std::vector<double> > live_mins, timestamp_mins, live_halfhour, timestamp_halfhour, live_hour, timestamp_hour, live_file, timestamp_file, timediff_file, n_live_file;
  static const int num_fivemin = 12;				//const ints for the use in the Initialise functions
  static const int num_halfhour = 12;
  static const int num_hour = 24;
  std::vector<int> mapping_vector_ch;
  int j_fivemin;
  double duration_fivemin = 5.;
  double duration_halfhour = 30.;
  double duration_hour = 60.;

  TDatime label_fivemin[12];					//the labels to be displayed on the x-axis
  TDatime label_halfhour[12];
  TDatime label_hour[24];

  bool update_mins, update_halfhour, update_hour;

  //define all containers for saving variables
  std::vector<std::array<double, 12> > times_channel_hour, times_slot_hour, times_crate_hour;	//one data point every 5 mins -> 12 point TGraph
  std::vector<std::array<double, 12> > times_channel_sixhour, times_slot_sixhour, times_crate_sixhour;	//one data point every 30 mins -> shows last 6 hours
  std::vector<std::array<double, 24> > times_channel_day, times_slot_day, times_crate_day;	//one data point per hour --> 24 point TGraph
  std::vector<std::array<double, 12> > rms_channel_hour, rms_slot_hour, rms_crate_hour;		//rms values of the timing distributions
  std::vector<std::array<double, 12> > rms_channel_sixhour, rms_slot_sixhour, rms_crate_sixhour;
  std::vector<std::array<double, 24> > rms_channel_day, rms_slot_day, rms_crate_day;		
  std::vector<std::array<double, 12> > frequency_channel_hour, frequency_slot_hour, frequency_crate_hour;	  //frequency hitmaps for the different channels
  std::vector<std::array<double, 12> > frequency_channel_sixhour, frequency_slot_sixhour, frequency_crate_sixhour;
  std::vector<std::array<double, 24> > frequency_channel_day, frequency_slot_day, frequency_crate_day;

  //keep track how many entrys are in the different time bins
  std::vector<std::array<long, 12> > n_channel_hour, n_slot_hour, n_crate_hour; //one data point every 5 mins -> 12 point TGraph
  std::vector<std::array<long, 12> > n_channel_sixhour, n_slot_sixhour, n_crate_sixhour;  //one data point every 30 mins -> shows last 6 hours
  std::vector<std::array<long, 24> > n_channel_day, n_slot_day, n_crate_day;  //one data point per hour --> 24 point TGraph

  // graphs
  std::vector<TGraph*> gr_times_hour, gr_times_sixhour, gr_times_day;	//one TGraph for every channel
  std::vector<TGraph*> gr_rms_hour, gr_rms_sixhour, gr_rms_day;	
  std::vector<TGraph*> gr_frequency_hour, gr_frequency_sixhour, gr_frequency_day;	
  std::vector<TGraph*> gr_slot_times_hour, gr_slot_times_sixhour, gr_slot_times_day;   //slot graphs
  std::vector<TGraph*> gr_slot_rms_hour, gr_slot_rms_sixhour, gr_slot_rms_day;
  std::vector<TGraph*> gr_slot_frequency_hour, gr_slot_frequency_sixhour, gr_slot_frequency_day;
  std::vector<TGraph*> gr_crate_times_hour, gr_crate_times_sixhour, gr_crate_times_day;   //crate graphs
  std::vector<TGraph*> gr_crate_rms_hour, gr_crate_rms_sixhour, gr_crate_rms_day;
  std::vector<TGraph*> gr_crate_frequency_hour, gr_crate_frequency_sixhour, gr_crate_frequency_day;


};

double compute_variance(const double mean, const std::vector<double>& numbers){

  if (numbers.size() <= 1u)
    return std::numeric_limits<double>::quiet_NaN();

  auto add_square = [mean](double sum, int i)
    {
      auto d = i - mean;
      return sum + d*d;
    };
  double total = std::accumulate(numbers.begin(), numbers.end(), 0.0, add_square);
  if (numbers.size()==1) return 0.;
  return total / (numbers.size() - 1);
}

double accumulate_array12(const std::vector<std::array<double,12> >& numbers, int start, int stop, int entry){

  if (numbers.size() <= 1u)
    return std::numeric_limits<double>::quiet_NaN();
  int entries=0;
  double return_value=0.;
  for (int i_vector=start; i_vector<stop; i_vector++){
    entries++;
    return_value+=numbers.at(i_vector).at(entry);
  }
  if (entries>0) return_value/=double(entries);
  return return_value;
}

double accumulate_array24(const std::vector<std::array<double,24> >& numbers, int start, int stop, int entry){

  if (numbers.size() <= 1u)
    return std::numeric_limits<double>::quiet_NaN();
  int entries=0;
  double return_value=0.;
  for (int i_vector=start; i_vector<stop; i_vector++){
    entries++;
    return_value+=numbers.at(i_vector).at(entry);
  }
  if (entries>0) return_value/=double(entries);

  return return_value;
}

long accumulate_longarray12(const std::vector<std::array<long,12> >& numbers, int start, int stop, int entry){

  if (numbers.size() <= 1u)
    return std::numeric_limits<double>::quiet_NaN();
  int entries=0;
  long return_value=0.;
  for (int i_vector=start; i_vector<stop; i_vector++){
    entries++;
    return_value+=numbers.at(i_vector).at(entry);
  }
  if (entries>0) return_value/=double(entries);

  return return_value;
}

long accumulate_longarray24(const std::vector<std::array<long,24> >& numbers, int start, int stop, int entry){

  if (numbers.size() <= 1u)
    return std::numeric_limits<double>::quiet_NaN();
  int entries=0;
  long return_value=0.;
  for (int i_vector=start; i_vector<stop; i_vector++){
    entries++;
    return_value+=numbers.at(i_vector).at(entry);
  }
  if (entries>0) return_value/=double(entries);

  return return_value;
}


#endif