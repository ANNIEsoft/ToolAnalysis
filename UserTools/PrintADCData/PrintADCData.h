#ifndef PrintADCData_H
#define PrintADCData_H

#include <string>
#include <iostream>
#include <fstream>
#include <thread>

#include "Tool.h"
#include "ADCPulse.h"
#include "Position.h"
#include "Detector.h"

#include "TApplication.h"
#include "TMath.h"
#include "TFile.h"
#include "TTree.h"
#include "TH2F.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TCanvas.h"
#include "TGraph.h"

/**
 * \class PrintADCData
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class PrintADCData: public Tool {


 public:

  PrintADCData(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
  void MakeYPhiHists();
  void PrintInfoInData(std::map<unsigned long, std::vector<Waveform<uint16_t>> > RawADCData,
        bool isAuxData); // Fill ROOT file with histograms from either PMT ADC data or auxiliary channel data.
                         // For now, all Aux Data will be output to the file regardless of any pulse activity
  void SaveOccupancyInfo(uint32_t Run, uint32_t Subrun);
  void ClearOccupancyInfo();
  ofstream result_file;

 private:

  //Variables needed to fill occupancy plots; code modified from Michael
  TH2F *hist_pulseocc_2D_y_phi = nullptr;
  TH2F *hist_frachit_2D_y_phi = nullptr;
  std::map<unsigned long,unsigned long> detkey_to_chankey;
  std::vector<unsigned long> pmt_detkeys;
  Geometry *geom = nullptr;
  std::map<unsigned long, double> x_PMT;
  std::map<unsigned long, double> y_PMT;
  std::map<unsigned long, double> z_PMT;
  std::map<unsigned long, double> rho_PMT;
  std::map<unsigned long, double> phi_PMT;
  int n_tank_pmts;
  double tank_center_x, tank_center_y, tank_center_z;

  bool use_led_waveforms;
  int pulse_threshold;
  std::string outputfile;
  TFile *file_out = nullptr;

  bool PulsesOnly;
  bool SaveWaves;
  int MaxWaveforms;
  std::string LEDsUsed;
  std::string LEDSetpoints;
  int WaveformNum;
  long totalentries=0;

  uint32_t RunNum;
  uint32_t SubrunNum;
  uint32_t CurrentRun;
  uint32_t CurrentSubrun;
  long EntryNum;

  std::map<unsigned long, std::vector<Waveform<uint16_t>> > RawADCData;
  std::map<unsigned long, std::vector<Waveform<uint16_t>> > RawADCAuxData;
  std::map<unsigned long, std::vector< std::vector<ADCPulse>> > RecoADCHits;
  int RunNumber;

  //Used to print information on how many pulses are found and % of events with a pulse
  std::map<unsigned long, int> NumPulses;
  std::map<unsigned long, int> NumWavesWithAPulse;
  std::map<unsigned long, int> NumWaves;


  /// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;

  // internal members:
  int SampleLength;
  std::vector<int> numberline;
  std::vector<int> upcastdata;
  int maxwfrmamp=0;
  int WaveformSource;
  std::map<std::string, TDirectory *> ChanKeyToDirectory;


  // ROOT TApplication variables
  // ---------------------------
  TApplication* rootTApp=nullptr;
  double canvwidth, canvheight;
  TCanvas* mb_canv=nullptr;
  TGraph* mb_graph=nullptr;
  

};


#endif
