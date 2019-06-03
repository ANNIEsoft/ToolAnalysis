#ifndef LAPPDFilter_H
#define LAPPDFilter_H

#include <string>
#include <iostream>
#include "TVirtualFFT.h"

#include "Tool.h"
#include "TH1D.h"
#include "TMath.h"

class LAPPDFilter: public Tool {


 public:

  LAPPDFilter();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  float Waveform_Filter2(float CutoffFrequency, int T, float finput);
  Waveform<double> Waveform_FFT(Waveform<double> iwav);

  bool isSim;
  int DimSize;
  double CutoffFrequency;
  double Deltat;
  string FilterInputWavLabel;


};


#endif
