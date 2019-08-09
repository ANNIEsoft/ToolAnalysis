#ifndef WaveformNNLS_H
#define WaveformNNLS_H

#include <string>
#include <iostream>
#include "nnls.h"

#include <TFile.h>
#include <TString.h>
#include <TH1.h>
#include "Tool.h"
#include "NnlsSolution.h"


class WaveformNNLS: public Tool {


 public:

  WaveformNNLS();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  void BuildTemplateMatrix(nnlsmatrix* A, Waveform<double> tempwave, size_t nrows); //makes the nnls matrix A given a root template file
  void BuildWaveformVector(nnlsvector* b, Waveform<double> wave, vector<float> times, double template_timestep); //formats the waveform into the vector format expected by nnls algo
  void SaveNNLSOutput(NnlsSolution* soln, nnlsmatrix* A, nnlsvector* x, vector<double> signaltimes);



 private:





};


#endif
