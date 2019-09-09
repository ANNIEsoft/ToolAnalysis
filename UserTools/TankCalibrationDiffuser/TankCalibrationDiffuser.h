#ifndef TankCalibrationDiffuser_H
#define TankCalibrationDiffuser_H

#include <string>
#include <iostream>
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
#include "TH2.h"
#include "TH2F.h"
#include "TFile.h"

/**
 * \class MonitorTankLive
*
* $Author: M. Nieslony $
* $Date: 2019/08/09 12:55:00 $
* Contact: mnieslon@uni-mainz.de
*/

class TankCalibrationDiffuser: public Tool {

   public:

      TankCalibrationDiffuser(); ///< Simple constructor
      bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
      bool Execute(); ///< Execute function used to perform Tool purpose.
      bool Finalise(); ///< Finalise funciton used to clean up resources.

      bool does_file_exist(const char *fileName); ///< Function to check if a certain file exists
      void draw_red_box(TH2 *hist, int bin_x, int bin_y, int verbosity); ///< Function to draw rectangles around PMTs that seem to be misaligned

   private:

      //define input variables
      std::string HitStoreName = "MCHits";
      std::string outputfile;
      int nBinsTimeTotal;
      double timeTotalMin;
      double timeTotalMax;
      int nBinsChargeTotal;
      double chargeTotalMin;
      double chargeTotalMax;
      int nBinsTime;
      double timeMin;
      double timeMax;
      int nBinsCharge;
      double chargeMin;
      double chargeMax;
      int nBinsStartTime;
      int nBinsStartTimeTotal;
      double startTimeMin;
      double startTimeMax;
      int nBinsPeakTime;
      int nBinsPeakTimeTotal;
      double peakTimeMin;
      double peakTimeMax;
      int nBinsBaseline;
      int nBinsBaselineTotal;
      double baselineMin;
      double baselineMax;
      int nBinsSigmaBaseline;
      int nBinsSigmaBaselineTotal;
      double sigmaBaselineMin;
      double sigmaBaselineMax;
      int nBinsRawAmplitude;
      int nBinsRawAmplitudeTotal;
      double rawAmplitudeMin;
      double rawAmplitudeMax;
      int nBinsAmplitude;
      int nBinsAmplitudeTotal;
      double amplitudeMin;
      double amplitudeMax;
      int nBinsRawArea;
      int nBinsRawAreaTotal;
      double rawAreaMin;
      double rawAreaMax;
      int nBinsTimeFit;
      double timeFitMin;
      double timeFitMax;
      int nBinsTimeDev;
      double timeDevMin;
      double timeDevMax;
      int nBinsChargeFit;
      double chargeFitMin;
      double chargeFitMax;
      double diffuser_x;
      double diffuser_y;
      double diffuser_z;
      double tolerance_charge;
      double tolerance_time;
      std::string FitMethod;
      double gaus1Constant;
      double gaus1Mean;
      double gaus1Sigma;
      double gaus2Constant;
      double gaus2Mean;
      double gaus2Sigma;
      double expConstant;
      double expDecay;
      bool use_tapplication;
      int verbose;

      //define ANNIEEvent variables
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

      //define histograms for stability plots
      TH1F *hist_tubeid = nullptr;
      TH1F *hist_charge = nullptr;
      TH1F *hist_time = nullptr;
      TH1F *hist_starttime = nullptr;
      TH1F *hist_peaktime = nullptr;
      TH1F *hist_baseline = nullptr;
      TH1F *hist_sigmabaseline = nullptr;
      TH1F *hist_rawamplitude = nullptr;
      TH1F *hist_amplitude = nullptr;
      TH1F *hist_rawarea = nullptr;
      TH1F *hist_tubeid_adc = nullptr;

      std::map<unsigned long, TH1F*> hist_charge_singletube;
      std::map<unsigned long, TH1F*> hist_time_singletube;
      std::map<unsigned long, TH1F*> hist_starttime_singletube;
      std::map<unsigned long, TH1F*> hist_peaktime_singletube;
      std::map<unsigned long, TH1F*> hist_baseline_singletube;
      std::map<unsigned long, TH1F*> hist_sigmabaseline_singletube;
      std::map<unsigned long, TH1F*> hist_rawamplitude_singletube;
      std::map<unsigned long, TH1F*> hist_amplitude_singletube;
      std::map<unsigned long, TH1F*> hist_rawarea_singletube;


      TH1F *hist_charge_mean = nullptr;
      TH1F *hist_time_mean = nullptr;
      TH1F *hist_time_dev = nullptr;
      TH1F *hist_starttime_mean = nullptr;
      TH1F *hist_peaktime_mean = nullptr;
      TH1F *hist_baseline_mean = nullptr;
      TH1F *hist_sigmabaseline_mean = nullptr;
      TH1F *hist_rawamplitude_mean = nullptr;
      TH1F *hist_amplitude_mean = nullptr;
      TH1F *hist_rawarea_mean = nullptr;
      TH1F *hist_detkey_charge = nullptr;
      TH1F *hist_detkey_time_mean = nullptr;
      TH1F *hist_detkey_time_dev = nullptr;
      TH1F *hist_detkey_starttime = nullptr;
      TH1F *hist_detkey_peaktime = nullptr;
      TH1F *hist_detkey_baseline = nullptr;
      TH1F *hist_detkey_sigmabaseline = nullptr;
      TH1F *hist_detkey_rawamplitude = nullptr;
      TH1F *hist_detkey_amplitude = nullptr;
      TH1F *hist_detkey_rawarea = nullptr;
      TH2F *hist_charge_2D_y_phi = nullptr;
      TH2F *hist_time_2D_y_phi = nullptr;
      TH2F *hist_time_2D_y_phi_mean = nullptr;
      TH2F *hist_starttime_2D_y_phi = nullptr;
      TH2F *hist_peaktime_2D_y_phi = nullptr;
      TH2F *hist_baseline_2D_y_phi = nullptr;
      TH2F *hist_sigmabaseline_2D_y_phi = nullptr;
      TH2F *hist_rawamplitude_2D_y_phi = nullptr;
      TH2F *hist_amplitude_2D_y_phi = nullptr;
      TH2F *hist_rawarea_2D_y_phi = nullptr;
      TH2F *hist_detkey_2D_y_phi = nullptr;

      //container for red boxes surrounding badly calibrated PMTs
      std::vector<TBox*> vector_tbox;

      //container for TF1s used to fit the distributions
      std::vector<TF1*> vector_tf1;

      //define graphs for stability plots
      TGraphErrors *gr_stability = nullptr;
      TGraphErrors *gr_stability_time = nullptr;
      TGraphErrors *gr_stability_time_mean = nullptr;

      //define file to save data
      TFile *file_out = nullptr;
      TFile *help_file = nullptr;

      //define overview canvas
      TCanvas *canvas_overview = nullptr;
      TCanvas *canvas_overview2 = nullptr;
      TCanvas *canvas_overview3 = nullptr;
      TCanvas *canvas_overview4 = nullptr;

      //define TApplication for possibility of interactive plotting
      TApplication *app_stability = nullptr;


};


#endif
