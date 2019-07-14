#ifndef FTBFAnalysis_H
#define FTBFAnalysis_H

#include <string>
#include <iostream>

#include "Tool.h"
#include <TH2.h>
#include <TH1.h>
#include <TFile.h>
#include <TCanvas.h>
#include <TString.h>
#include "LAPPDDisplay.h"
#include "NnlsSolution.h"

class FTBFAnalysis: public Tool {


 public:

  FTBFAnalysis();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  void HeatmapEvent(int event, int board);
  void PlotSeparateChannels(int event, int board);
  void PlotRawHists();
  void PlotNNLSandRaw();



 private:
 	int _display_config;
 	int _event_counter;
    int _file_number;
    LAPPDDisplay* _display;
    map<int, vector<Waveform<double>>> LAPPDWaveforms;
    map<int, NnlsSolution> NNLSsoln;


};


#endif
