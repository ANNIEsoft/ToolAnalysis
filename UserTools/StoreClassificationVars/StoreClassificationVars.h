#ifndef StoreClassificationVars_H
#define StoreClassificationVars_H

#include <string>
#include <iostream>
#include <vector>
#include <map>

#include "Tool.h"
#include "TH1F.h"
#include "TFile.h"
#include "TMath.h"

#include "Hit.h"
#include "LAPPDHit.h"
#include "Position.h"
#include "Direction.h"
#include "RecoVertex.h"
#include "RecoDigit.h"


class StoreClassificationVars: public Tool {

 public:

  StoreClassificationVars();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

 private:

  // Configuration variables
  
  int verbosity=0;
  std::string filename;
  bool save_root;
  bool save_csv;

  bool EventCutStatus;

  // Files to save data in
  
  TFile *file = nullptr;
  ofstream csv_file, csv_statusfile;


  // Classification variables - histograms (1D)
  
  TH1F *hist_pmtPE = nullptr;
  TH1F *hist_pmtTime = nullptr;
  TH1F *hist_pmtTheta = nullptr;
  TH1F *hist_pmtPhi = nullptr;
  TH1F *hist_pmtY = nullptr;
  TH1F *hist_pmtDist = nullptr;
  TH1F *hist_pmtHits = nullptr;
  TH1F *hist_pmtPEtotal = nullptr;
  TH1F *hist_pmtAvgTime = nullptr;
  TH1F *hist_pmtAvgDist = nullptr;
  TH1F *hist_pmtThetaBary = nullptr;
  TH1F *hist_pmtRMSTheta = nullptr;
  TH1F *hist_pmtVarTheta = nullptr;
  TH1F *hist_pmtSkewTheta = nullptr;
  TH1F *hist_pmtKurtTheta = nullptr;
  TH1F *hist_pmtRMSThetaBary = nullptr;
  TH1F *hist_pmtVarThetaBary = nullptr;
  TH1F *hist_pmtSkewThetaBary = nullptr;
  TH1F *hist_pmtKurtThetaBary = nullptr;
  TH1F *hist_pmtRMSPhiBary = nullptr;
  TH1F *hist_pmtVarPhiBary = nullptr;
  TH1F *hist_pmtFracLargeAnglePhiBary = nullptr;
  TH1F *hist_pmtFracLargeAngleThetaBary = nullptr;
  TH1F *hist_pmtFracRing = nullptr;
  TH1F *hist_pmtFracDownstream = nullptr;
  TH1F *hist_pmtFracRingNoWeight = nullptr;
  TH1F *hist_pmtFracHighestQ = nullptr;
  TH1F *hist_pmtFracClustered = nullptr;
  TH1F *hist_pmtFracLowCharge = nullptr;
  TH1F *hist_pmtFracLateTime = nullptr;
  TH1F *hist_pmtFracEarlyTime = nullptr;
  TH1F *hist_pmtThetaBary_all = nullptr;
  TH1F *hist_pmtThetaBaryQweighted_all = nullptr;
  TH1F *hist_pmtYBary_all = nullptr;
  TH1F *hist_pmtYBaryQweighted_all = nullptr;
  TH1F *hist_pmtPhiBary_all = nullptr;
  TH1F *hist_pmtPhiBaryQweighted_all = nullptr;

  TH1F *hist_lappdPE = nullptr;
  TH1F *hist_lappdTime = nullptr;
  TH1F *hist_lappdTheta = nullptr;
  TH1F *hist_lappdDist = nullptr;
  TH1F *hist_lappdHits = nullptr;
  TH1F *hist_lappdPEtotal = nullptr;
  TH1F *hist_lappdAvgTime = nullptr;
  TH1F *hist_lappdAvgDist = nullptr;
  TH1F *hist_lappdThetaBary = nullptr;
  TH1F *hist_lappdRMSTheta = nullptr;
  TH1F *hist_lappdVarTheta = nullptr;
  TH1F *hist_lappdSkewTheta = nullptr;
  TH1F *hist_lappdKurtTheta = nullptr;
  TH1F *hist_lappdRMSThetaBary = nullptr;
  TH1F *hist_lappdVarThetaBary = nullptr;
  TH1F *hist_lappdSkewThetaBary = nullptr;
  TH1F *hist_lappdKurtThetaBary = nullptr;
  TH1F *hist_lappdFracRing = nullptr;
  TH1F *hist_lappdFracRingNoWeight = nullptr;
  
  TH1F *hist_mrdPaddles = nullptr;
  TH1F *hist_mrdLayers = nullptr;
  TH1F *hist_mrdconsLayers = nullptr;  
  TH1F *hist_mrdClusters = nullptr;
  TH1F *hist_mrdXSpread = nullptr;
  TH1F *hist_mrdYSpread = nullptr;
  TH1F *hist_mrdAdjacentHits = nullptr;
  TH1F *hist_mrdPaddlesPerLayer = nullptr; 

  TH1F *hist_evnum = nullptr;
  TH1F *hist_distWallHor = nullptr;
  TH1F *hist_distWallVert = nullptr;
  TH1F *hist_distInnerStrHor = nullptr;
  TH1F *hist_distInnerStrVert = nullptr;
  TH1F *hist_energy = nullptr;
  TH1F *hist_nrings = nullptr;
  TH1F *hist_multiplerings = nullptr;
  TH1F *hist_pdg = nullptr;

  //classification variables - csv

  //mctruth variables
  bool use_mctruth;
  double energy, distWallHor, distWallVert, distInnerStrHor, distInnerStrVert, true_time, pmt_fracRing, pmt_fracRingNoWeight, lappd_fracRing, lappd_fracRingNoWeight; 

  //general variables
  double pmt_avgDist, pmt_hits, pmt_totalQ, pmt_avgT, pmt_baryTheta, pmt_rmsTheta, pmt_varTheta, pmt_skewTheta, pmt_kurtTheta, pmt_rmsThetaBary, pmt_varThetaBary, pmt_skewThetaBary, pmt_kurtThetaBary, pmt_fracDownstream, pmt_fracHighestQ, pmt_fracClustered, pmt_fracLowQ, pmt_fracLateT, pmt_fracEarlyT, pmt_rmsPhiBary, pmt_rmsPhi, pmt_varPhi, pmt_varPhiBary, pmt_fracLargeAnglePhi, pmt_fracLargeAngleTheta, pmt_hitsLargeAnglePhi, pmt_hitsLargeAngleTheta;
  double lappd_avgDist, lappd_hits, lappd_totalQ, lappd_avgT, lappd_baryTheta, lappd_rmsTheta, lappd_varTheta, lappd_skewTheta, lappd_kurtTheta, lappd_rmsThetaBary, lappd_varThetaBary, lappd_skewThetaBary, lappd_kurtThetaBary; 
  int num_mrd_paddles, num_mrd_layers, num_mrd_conslayers, num_mrd_clusters, num_mrd_adjacent;
  double mrd_mean_xspread, mrd_mean_yspread, mrd_padperlayer;
  int nrings, multiplerings, evnum, pdg;

  double cherenkov_angle = 0.719889;	//arccos(1/1.33), if assuming relevant primaries move with velocity c

  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;
  int get_ok;

};


#endif
