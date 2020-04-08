#ifndef BeamClusterPlots_H
#define BeamClusterPlots_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "ADCPulse.h"
#include "Waveform.h"
#include "CalibratedADCWaveform.h"
#include "Hit.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TH2.h"
#include "TH2F.h"
#include "TFile.h"
#include "TApplication.h"
#include "TMath.h"
#include "TTree.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TGraph.h"

/**
 * \class BeamClusterPlots
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: Teal Pershing $
* $Date: 2020/03/10 10:44:00 $
* Contact: tjpershing@ucdavis.edu 
*/
class BeamClusterPlots: public Tool {


 public:

  BeamClusterPlots(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
  
  void WriteHistograms();
  void InitializeHistograms();
  void SetHistogramLabels();

  double CalculateChargePoint(std::vector<Hit> cluster_hits);
  double CalculateChargeBalance(std::vector<Hit> cluster_hits);
  double CalculateMaxPE(std::vector<Hit> cluster_hits);
 private:

  TFile* bca_file_out = nullptr;
  std::map<int,double> ChannelKeyToSPEMap;
  Geometry *geom = nullptr;

  std::map<double,std::vector<Hit>>* m_all_clusters = nullptr;  

  TH2F* hist_prompt_PEVNHit;
  TH2F* hist_prompt_ChargePoint;
  TH2F* hist_prompt_ChargeBalance;
  TH2F* hist_prompt_TotPEVsMaxPE;
  TH2F* hist_delayed_PEVNHit;
  TH2F* hist_delayed_ChargePoint;
  TH2F* hist_delayed_ChargeBalance;
  TH2F* hist_delayed_TotPEVsMaxPE;

  TH1F* hist_prompt_Time;
  TH1F* hist_prompt_PE;
  TH2F* hist_prompt_TimeVPE;
  TH1F* hist_delayed_Time;
  TH1F* hist_delayed_PE;
  TH2F* hist_delayed_TimeVPE;

  TH1F* hist_prompt_delayed_multiplicity;
  TH1F* hist_prompt_neutron_multiplicity; 
  TH1F* hist_prompt_delayed_deltat;
  TH1F* hist_prompt_neutron_deltat;
  TH2F* hist_prompt_neutron_multiplicityvstankE;

  // Load the default threshold settings for finding pulses
  std::string outputfile;
  int PromptPEMin;
  int PromptWindowMin;  //ns
  int PromptWindowMax; //ns
  int DelayedWindowMin;  //ns
  int DelayedWindowMax; //ns
  double NeutronPEMin;
  double NeutronPEMax;
  double PEPerMeV;

  std::map<double,double> ClusterMaxPEs;
  std::map<double,Direction> ClusterChargePoints;
  std::map<double,double> ClusterChargeBalances;


  /// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;

};


#endif
