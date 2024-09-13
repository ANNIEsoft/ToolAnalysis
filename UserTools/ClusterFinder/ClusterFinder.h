#ifndef ClusterFinder_H
#define ClusterFinder_H

#include <string>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <fstream>

#include "TObjectTable.h"

#include "Tool.h"
#include "Hit.h"
#include "ADCPulse.h"
#include "BeamStatus.h"
#include "TriggerClass.h"
#include "Detector.h"
#include "Position.h"
#include "Direction.h"
#include "Geometry.h"
#include "TProfile.h"
#include "TApplication.h"
#include "TBox.h"
#include "TGraphErrors.h"
#include "TMath.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TH2.h"
#include "TH2F.h"
#include "TFile.h"

/**
 * \class ClusterFinder
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class ClusterFinder: public Tool {


 public:

  ClusterFinder(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.


 private:

  //define input variables
  std::string HitStoreName = "MCHits";
  std::string outputfile;
  int verbose;
  double diffuser_x;
  double diffuser_y;
  double diffuser_z;
  double tolerance_charge;
  double tolerance_time;
  int ClusterFindingWindow;
  int AcqTimeWindow;
  int ClusterIntegrationWindow;
  int MinHitsPerCluster;
  bool draw_2D = false;
  double end_of_window_time_cut;
  double mc_pulse_width;

  // define ANNIEEvent variables
  int evnum;
  int runnumber;
  TimeClass* EventTime=nullptr;
  std::vector<TriggerClass>* TriggerData;
  BeamStatusClass* BeamStatus=nullptr;
  std::map<unsigned long, std::vector<MCHit>>* MCHits = nullptr;
  std::map<unsigned long, std::vector<Hit>>* Hits = nullptr;
  Geometry *geom = nullptr;
  std::vector<unsigned long> pmt_detkeys;

  //define useful variables
  const double n_water = 1.33;
  const double c_vacuum= 2.99792E8; //in m/s
  int n_tank_pmts, n_mrd_pmts, n_veto_pmts, n_lappds;
  double tank_center_x, tank_center_y, tank_center_z;
  std::map<std::string,double> map_type_radius;  //stores the radius information of the respective PMT as a function of the PMT type

  //define monitoring variables
  std::map<int,double> bad_time, bad_charge;

  //define arrays for storing PMT calibration values
  std::map<unsigned long, double> x_PMT;
  std::map<unsigned long, double> y_PMT;
  std::map<unsigned long, double> z_PMT;
  std::map<unsigned long, double> rho_PMT;
  std::map<unsigned long, double> phi_PMT;
  std::map<unsigned long, double> radius_PMT;
  std::map<unsigned long, int> PMT_ishit;
  std::map<unsigned long, double> mean_charge_fit;
  std::map<unsigned long, double> mean_time_fit;
  std::map<unsigned long, double> rms_charge_fit;
  std::map<unsigned long, double> rms_time_fit;
  std::map<unsigned long, double> expected_time;
  std::map<unsigned long, double> starttime_mean;
  std::map<unsigned long, double> peaktime_mean;
  std::map<unsigned long, double> baseline_mean;
  std::map<unsigned long, double> sigmabaseline_mean;
  std::map<unsigned long, double> rawamplitude_mean;
  std::map<unsigned long, double> amplitude_mean;
  std::map<unsigned long, double> rawarea_mean;

  // Arrays and vectors
  std::vector<double> v_hittimes; // array used to sort hits times
  std::vector<double> v_hittimes_sorted;
  std::vector<double> v_mini_hits;
  std::map<double, std::vector<double>> m_time_Nhits;
  std::vector<double> v_clusters;
  std::vector<double> v_local_cluster_times;
  std::map<double,std::vector<Hit>>* m_all_clusters;  
  std::map<double,std::vector<MCHit>>* m_all_clusters_MC;  
  std::map<double,std::vector<unsigned long>>* m_all_clusters_detkey; 
 
  // Other variables
  int max_Nhits = 0;
  double local_cluster = 0;
  int thiswindow_Nhits =0;
  int dummy_hittime_value = -9999; 
  
  //define file to save data
  TFile *file_out = nullptr;

  // Histograms and files
  TH1D* h_Cluster_times=nullptr;
  TH1D* h_Cluster_charges=nullptr;
  TH1D* h_Cluster_deltaT=nullptr;
  TH2D* h_Cluster_charge_time=nullptr;
  TH2D* h_Cluster_charge_deltaT=nullptr;
  TCanvas* canvas_Cluster=nullptr;
  TFile* f_output=nullptr;

  int v_error = 0;
  int v_warning = 1;
  int v_message = 2;
  int v_debug = 3;
  int vv_debug = 4;
};


#endif
