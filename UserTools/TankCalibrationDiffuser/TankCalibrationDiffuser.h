#ifndef TankCalibrationDiffuser_H
#define TankCalibrationDiffuser_H

#include <string>
#include <iostream>
#include <cmath>
#include <fstream>

#include "TObjectTable.h"

#include "Tool.h"
#include "Hit.h"
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

class TankCalibrationDiffuser: public Tool {

 public:

   TankCalibrationDiffuser();
   bool Initialise(std::string configfile,DataModel &data);
   bool Execute();
   bool Finalise();

   bool does_file_exist(const char *fileName);
   void draw_red_box(TH2 *hist, int bin_x, int bin_y, int verbosity);

 private:

   //define input variables
   std::string outputfile;
   std::string stabilityfile;
   std::string geometryfile;
   double diffuser_x;
   double diffuser_y;
   double diffuser_z;
   double tolerance_charge;
   double tolerance_time;
   bool use_tapplication;
   int verbose;

   //define ANNIEEvent variables
   int evnum;
   int runnumber;
   TimeClass* EventTime=nullptr;
   std::vector<TriggerClass>* TriggerData;
   BeamStatusClass* BeamStatus=nullptr;
   std::map<unsigned long, std::vector<Hit>>* MCHits = nullptr;
   Geometry *geom = nullptr;
   Detector det;
   std::map<unsigned long,int> channelkey_to_pmtid;
   std::map<int,unsigned long> pmt_tubeid_to_channelkey;


   //define useful variables
   const double n_water = 1.33;
   const double c_vacuum= 2.99792E8; //in m/s
   int n_tank_pmts, n_mrd_pmts, n_veto_pmts, n_lappds;
   double tank_center_x, tank_center_y, tank_center_z;
   static const unsigned long n_channels_per_lappd = 60;

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
   std::map<unsigned long, double> expected_time;

   //define histograms for stability plots
   TH1F *hist_tubeid = nullptr;
   TH1F *hist_charge = nullptr;
   TH1F *hist_time = nullptr;
   std::map<unsigned long, TH1F*> hist_charge_singletube;
   std::map<unsigned long, TH1F*> hist_time_singletube;
   TH1F *hist_charge_mean = nullptr;
   TH1F *hist_time_mean = nullptr;
   TH2F *hist_charge_2D_y_phi = nullptr;
   TH2F *hist_time_2D_y_phi = nullptr;

   //container for red boxes surrounding badly calibrated PMTs
   std::vector<TBox*> vector_tbox;

   //container for TF1s used to fit the distributions
   std::vector<TF1*> vector_tf1;

   //define graphs for stability plots
   TGraphErrors *gr_stability = nullptr;
   TGraphErrors *gr_stability_time = nullptr;

   //define file to save data
   TFile *file_out = nullptr;
   TFile *help_file = nullptr;

   //define overview canvas
   TCanvas *canvas_overview = nullptr;

   //define TApplication for possibility of interactive plotting
   TApplication *app_stability = nullptr;


};


#endif