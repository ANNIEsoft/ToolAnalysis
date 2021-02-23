#ifndef MaxPEPlots_H
#define MaxPEPlots_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "ADCPulse.h"

#include "TH1F.h"
#include "TH2F.h"
#include "TFile.h"
#include "TTree.h"
#include "TF1.h"

/**
 * \class MaxPEPlots
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class MaxPEPlots: public Tool {


 public:

  MaxPEPlots(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.


 private:

  std::string maxpe_filename;
  int verbosity;

  int evnum;
  std::map<unsigned long, std::vector<Hit>>* Hits = nullptr;
  Geometry *geom = nullptr;
  std::map<string,bool> DataStreams;
  bool extended;
  int extended_trig;
  std::map<int,double>* ChannelNumToTankPMTSPEChargeMap = nullptr;
  int triggerword;
  std::vector<double> baseline;
  std::vector<double> amplitude;
  std::vector<double> chkey;
  int n_mpe;

  std::map<int,int> map_baseline;
  std::map<int,int> map_mpe;

  int n_tank_pmts;
  std::vector<unsigned long> pmt_detkeys;
  std::map<unsigned long, double> PMT_maxpe;

  TH1F *h_maxpe = nullptr;
  TH1F *h_maxpe_prompt = nullptr;
  TH1F *h_maxpe_delayed = nullptr;
  TH2F *h_maxpe_chankey = nullptr; 
  TH2F *h_maxpe_prompt_chankey = nullptr; 
  TH2F *h_maxpe_delayed_chankey = nullptr;
  TH1F *h_maxpe_trigword5 = nullptr;
  TH1F *h_maxpe_prompt_trigword5 = nullptr;
  TH1F *h_maxpe_delayed_trigword5 = nullptr;
  TH2F *h_maxpe_chankey_trigword5 = nullptr; 
  TH2F *h_maxpe_prompt_chankey_trigword5 = nullptr; 
  TH2F *h_maxpe_delayed_chankey_trigword5 = nullptr;
  TH1F *h_multiplicity_delayed_mpe = nullptr;
  TH1F *h_multiplicity_prompt_mpe = nullptr;
  TH1F *h_multiplicity_prompt_mpe_extended0 = nullptr;
  TH1F *h_multiplicity_delayed_mpe_extended0 = nullptr;
  TH1F *h_multiplicity_prompt_mpe_extended1 = nullptr;
  TH1F *h_multiplicity_delayed_mpe_extended1 = nullptr;
  TH1F *h_multiplicity_prompt_mpe_extended2 = nullptr;
  TH1F *h_multiplicity_delayed_mpe_extended2 = nullptr;
  TH1F *h_multiplicity_delayed_mpe_5pe = nullptr;
  TH1F *h_multiplicity_prompt_mpe_5pe = nullptr;
  TH1F *h_baseline_diff = nullptr;
  TH1F *h_chankey_prompt_mpe = nullptr;
  TH1F *h_chankey_delayed_mpe = nullptr;
  TH1F *h_chankey_prompt_mpe_5pe = nullptr;
  TH1F *h_chankey_delayed_mpe_5pe = nullptr;
  TH2F *h_maxpe_mpe_prompt = nullptr;
  TH2F *h_maxpe_mpe_extended = nullptr;
  TH1F *h_eventtypes_trig = nullptr;
  TH1F *h_window_opened = nullptr;
  TH1F *h_extended_trig = nullptr;
  TH1F *h_nowindow = nullptr;

  std::vector<TH1F*> hv_baseline_prompt, hv_baseline_delayed;
  std::vector<TH1F*> hv_amplitude_prompt, hv_amplitude_delayed;
  std::vector<TH1F*> hv_time_pulse;

  TFile *f_maxpe = nullptr;
  TTree *t_maxpe = nullptr;

  double max_pe_global;
  std::vector<double> max_pe;

  int v_error = 0;
  int v_warning = 1;
  int v_message = 2;
  int v_debug = 3;
  int vv_debug = 4;

};


#endif
