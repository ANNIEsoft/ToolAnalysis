#ifndef LAPPDFindPeak_H
#define LAPPDFindPeak_H

#include <string>
#include <iostream>
#include "ANNIEalgorithms.h"
#include "LAPPDPulse.h"
#include "TVector3.h"
#include "TString.h"

#include "Tool.h"

class LAPPDFindPeak: public Tool {


 public:

  LAPPDFindPeak();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  std::vector<LAPPDPulse> FindPulses_TOT(std::vector<double> *theWav);
  std::vector<LAPPDPulse> FindPulses_Thresh(std::vector<double> *theWav);
  string FiltPeakInputWavLabel;
  string RawPeakInputWavLabel;
  string BLSPeakInputWavLabel;

 private:

  double TotThreshold;
  double MinimumTot;
  double Deltat;
  int FindPeakVerbosity;
  Geometry* _geom;


};


#endif
