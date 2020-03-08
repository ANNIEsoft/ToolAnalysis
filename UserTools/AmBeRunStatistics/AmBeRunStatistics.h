#ifndef AmBeRunStatistics_H
#define AmBeRunStatistics_H

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
 * \class AmBeRunStatistics
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class AmBeRunStatistics: public Tool {


 public:

  AmBeRunStatistics(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  // Returns the SiPM pulses with the maximum amplitude and the number of pulses that cross the threshold values
  // S1Threshold and S2Threshold defined at config
  void GetSiPMPulseInfo(ADCPulse& SiPM1_Pulse, ADCPulse& SiPM2_Pulse, int& SiPM1_NumPulses, int& SiPM2_NumPulses);
  void WriteHistograms();
  void InitializeHistograms();
  void SetHistogramLabels();

 private:
  int verbosity;
  std::string outputfile;
  TFile* ambe_file_out = nullptr;
  // Build the calibrated waveforms
  std::map<unsigned long, std::vector<CalibratedADCWaveform<double> > >
    calibrated_auxwaveform_map;
  std::map<unsigned long, std::vector< std::vector<ADCPulse>> > aux_pulse_map;
  std::map<unsigned long, std::vector<Hit>>* AuxHits = nullptr;

  std::map<int,std::string>* AuxChannelNumToTypeMap;
  std::map<int,double> ChannelKeyToSPEMap;

  double SWindowMin;
  double SWindowMax;
  double S1Threshold; 
  double S2Threshold;
  double ClusterPEMin;
  double ClusterPEMax;
  double DeltaTimeThreshold; 

  float NumberOfEvents = 0;
  float NumberOfCleanTriggers = 0;
  float NumberGoldenNeutronCandidates = 0;

  std::map<double,std::vector<Hit>>* m_all_clusters = nullptr;  

  TH1F* h_SiPM1_Amplitude = nullptr;
  TH1F* h_SiPM2_Amplitude = nullptr;
  TH1F* h_SiPM1_Charge = nullptr;
  TH1F* h_SiPM2_Charge = nullptr;
  TH2F* h_S1S2_Amplitudes = nullptr;
  TH1F* h_S1S2_Deltat = nullptr;
  TH1F* h_S1S2_Chargeratio = nullptr;
  TH1F* h_SiPM1_PeakTime = nullptr;
  TH1F* h_SiPM2_PeakTime = nullptr;
  //Valid AmBe Trigger Histograms
  TH1F* h_Cluster_ChargeCleanPromptTrig = nullptr;
  TH1F* h_Cluster_TimeMeanCleanPromptTrig = nullptr;
  TH1F* h_Cluster_MultiplicityCleanPromptTrig = nullptr; 
  TH1F* h_SiPM1_AmplitudeCleanPromptTrig = nullptr;
  TH1F* h_SiPM2_AmplitudeCleanPromptTrig = nullptr;
  TH1F* h_SiPM1_ChargeCleanPromptTrig = nullptr;
  TH1F* h_SiPM2_ChargeCleanPromptTrig = nullptr;
  TH1F* h_SiPM1_PeakTimeCleanPromptTrig = nullptr;
  TH1F* h_SiPM2_PeakTimeCleanPromptTrig = nullptr;
  TH2F* h_S1S2_AmplitudesCleanPromptTrig = nullptr;
  TH1F* h_S1S2_ChargeratioCleanPromptTrig = nullptr;
  TH1F* h_S1S2_DeltatCleanPromptTrig = nullptr;
  TH1F* h_Cluster_ChargeNeutronCandidate = nullptr;
  TH1F* h_Cluster_TimeMeanNeutronCandidate = nullptr;
  TH1F* h_Cluster_MultiplicityNeutronCandidate = nullptr;
  TH1F* h_Cluster_PENeutronCandidate = nullptr;
  //Golden Neutron Candidate Histograms
  TH1F* h_Cluster_ChargeGoldenCandidate = nullptr;
  TH1F* h_Cluster_TimeMeanGoldenCandidate = nullptr;
  TH1F* h_Cluster_MultiplicityGoldenCandidate = nullptr;
  TH1F* h_Cluster_PEGoldenCandidate = nullptr;
  TH1F* h_SiPM1_AmplitudeGoldenCandidate = nullptr;
  TH1F* h_SiPM2_AmplitudeGoldenCandidate = nullptr;
  TH1F* h_SiPM1_ChargeGoldenCandidate = nullptr;
  TH1F* h_SiPM2_ChargeGoldenCandidate = nullptr;
  TH1F* h_SiPM1_PeakTimeGoldenCandidate = nullptr;
  TH1F* h_SiPM2_PeakTimeGoldenCandidate = nullptr;
  TH2F* h_S1S2_AmplitudesGoldenCandidate = nullptr;
  TH1F* h_S1S2_ChargeratioGoldenCandidate = nullptr;
  TH1F* h_S1S2_DeltatGoldenCandidate = nullptr;

  // ROOT TApplication variables
  // ---------------------------
  TApplication* rootTApp=nullptr;
  double canvwidth, canvheight;
  TCanvas* mb_canv=nullptr;
  TGraph* mb_graph=nullptr;

};


#endif
