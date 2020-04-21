#ifndef StoreClassificationVars_H
#define StoreClassificationVars_H

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <fstream>

#include "Tool.h"
#include "TH1F.h"
#include "TFile.h"
#include "TTree.h"
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

  bool LoadVariableConfig(std::string config_name);
  void InitClassHistograms();
  void InitClassTree();
  void InitCSV();
  void PopulateConfigMap();
  void UpdateConfigMap();
  void FillClassHistograms();
  void FillClassTree();
  void FillCSV();
  void WriteClassHistograms();

 private:

  // Configuration variables
  
  int verbosity=0;
  std::string filename;
  bool save_root;
  bool save_csv;
  std::string variable_config;
  std::string variable_config_path;
  bool EventCutStatus;
  bool selection_passed;
  bool mldata_present;
  bool isData;

  std::vector<std::string> variable_names;
  std::map<std::string,double> variable_map;

  // Files to save data in
  
  TFile *file = nullptr;
  ofstream csv_file, csv_statusfile;

  // Classification variables - TTree
  TTree *tree = nullptr;

  // Classification variables - histograms (1D)
  
  TH1F *hist_pmtPE_single = nullptr;
  TH1F *hist_pmtTime_single = nullptr;
  TH1F *hist_pmtTheta_single = nullptr;
  TH1F *hist_pmtPhi_single = nullptr;
  TH1F *hist_pmtY_single = nullptr;
  TH1F *hist_pmtDist_single = nullptr;
  TH1F *hist_pmtThetaBary_single = nullptr;
  TH1F *hist_pmtPhiBary_single = nullptr;
  TH1F *hist_pmtHits = nullptr;
  TH1F *hist_pmtPEtotal = nullptr;
  TH1F *hist_pmtPEtotalClustered = nullptr;
  TH1F *hist_pmtAvgTime = nullptr;
  TH1F *hist_pmtAvgDist = nullptr;
  TH1F *hist_pmtThetaBary = nullptr;
  TH1F *hist_pmtThetaRMS = nullptr;
  TH1F *hist_pmtThetaVar = nullptr;
  TH1F *hist_pmtThetaBaryRMS = nullptr;
  TH1F *hist_pmtThetaBaryVar = nullptr;
  TH1F *hist_pmtPhiBaryRMS = nullptr;
  TH1F *hist_pmtPhiBaryVar = nullptr;
  TH1F *hist_pmtPhiRMS = nullptr;
  TH1F *hist_pmtPhiVar = nullptr;
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

  TH1F *hist_lappdPE_single = nullptr;
  TH1F *hist_lappdTime_single = nullptr;
  TH1F *hist_lappdTheta_single = nullptr;
  TH1F *hist_lappdThetaBary_single = nullptr;
  TH1F *hist_lappdDist_single = nullptr;

  TH1F *hist_lappdHits = nullptr;
  TH1F *hist_lappdPEtotal = nullptr;
  TH1F *hist_lappdAvgTime = nullptr;
  TH1F *hist_lappdAvgDist = nullptr;
  TH1F *hist_lappdThetaBary = nullptr;
  TH1F *hist_lappdThetaRMS = nullptr;
  TH1F *hist_lappdThetaVar = nullptr;
  TH1F *hist_lappdThetaBaryRMS = nullptr;
  TH1F *hist_lappdThetaBaryVar = nullptr;
  TH1F *hist_lappdFracRing = nullptr;
  
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
  TH1F *hist_truetime = nullptr;


  //classification variables - csv

  //mctruth variables
  bool use_mctruth, multiplerings;
  int nrings, evnum, pdg;
  double energy, distWallHor, distWallVert, distInnerStrHor, distInnerStrVert, true_time, pmt_fracRing, pmt_fracRingNoWeight, lappd_fracRing, lappd_fracRingNoWeight; 

  //general variables
  int pmt_hits, lappd_hits,pmt_hitsLargeAnglePhi, pmt_hitsLargeAngleTheta;
  double pmtBaryTheta, pmt_avgDist, pmt_totalQ, pmt_totalQ_Clustered, pmt_avgT, pmt_baryTheta, pmt_rmsTheta, pmt_varTheta, pmt_rmsThetaBary, pmt_varThetaBary, pmt_fracDownstream, pmt_fracHighestQ, pmt_fracClustered, pmt_fracLowQ, pmt_fracLateT, pmt_fracEarlyT, pmt_rmsPhiBary, pmt_rmsPhi, pmt_varPhi, pmt_varPhiBary, pmt_fracLargeAnglePhi, pmt_fracLargeAngleTheta;
  double lappdBaryTheta, lappd_avgDist, lappd_totalQ, lappd_avgT, lappd_baryTheta, lappd_rmsTheta, lappd_varTheta, lappd_rmsThetaBary, lappd_varThetaBary; 
  int num_mrd_paddles, num_mrd_layers, num_mrd_conslayers, num_mrd_adjacent, num_mrd_clusters;
  double mrd_mean_xspread, mrd_mean_yspread, mrd_padperlayer;
  std::vector<double> pmtQ, pmtT, pmtDist, pmtTheta, pmtThetaBary, pmtPhi, pmtPhiBary, pmtY, lappdQ, lappdT, lappdDist, lappdTheta, lappdThetaBary;
  std::vector<double> *pmtQ_vec=0, *pmtT_vec=0, *pmtDist_vec=0, *pmtTheta_vec=0, *pmtThetaBary_vec=0, *pmtPhi_vec=0, *pmtPhiBary_vec=0, *pmtY_vec=0, *lappdQ_vec=0, *lappdT_vec=0, *lappdDist_vec=0, *lappdTheta_vec=0, *lappdThetaBary_vec=0;


  double cherenkov_angle = 0.719889;	//arccos(1/1.33), if assuming relevant primaries move with velocity c

  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;
  int get_ok;

};


#endif
