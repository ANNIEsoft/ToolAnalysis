#ifndef LAPPDnnlsPeak_H
#define LAPPDnnlsPeak_H

#include <string>
#include <iostream>
#include "nnls.h"

#include <TFile.h>
#include <TString.h>
#include <TH1.h>
#include "Tool.h"
#include "NnlsSolution.h"


/*
class LAPPDnnlsPeak
*/
class LAPPDnnlsPeak: public Tool {


 public:

  LAPPDnnlsPeak(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
  void BuildTemplateMatrix(nnlsmatrix* A, Waveform<double> tempwave, size_t nrows); //makes the nnls matrix A given a root template file
  void BuildWaveformVector(nnlsvector* b, Waveform<double> wave, vector<float> times, double template_timestep); //formats the waveform into the vector format expected by nnls algo
  void SaveNNLSOutput(NnlsSolution* soln, nnlsmatrix* A, nnlsvector* x, vector<double> signaltimes);
  bool CompactPeakExe(std::map<unsigned long, vector<Waveform<double>>> &lappddata,
                      std::map<int,NnlsSolution> &soln,
                      std::map<unsigned long, vector<Waveform<double>>> &FittedPulseNumber,
                      size_t nrows,
                      Waveform<double> tempwave,
                      vector<double> temptimes,
                      vector<float> sampletimes,
                      double newtimestep,
                      int maxiter,
                      vector<double> newsignaltimes,
                      int nnlsVerbosityLevel,
                      vector<int> keymap,
                      int start,
                      int end,
                      int threadCount);

 private:

int timeCount;
int nnlsPrintOption;
   int multiThread;
  int threadNumber;
  string InputWavLabel;
  string OutputWavLabel;
  double samplingFactor;
  TString tempfilename ;
 	TString temphistname ;
 	TFile* tempfile ;
 	TH1D* temphist ;
 	Waveform<double> tempwave; //signal of template
 	vector<double> temptimes;  //times of template
 	int nbins ;
 	double binwidth ; //should be in ns
 	double starttime ; //first time in template histogram, used to set template to start at 0ps
 	double endtime ;
 	double newtimestep ;
  int maxiter;
  int nnlsVerbosityLevel;
  Geometry* _geom;

};


#endif
