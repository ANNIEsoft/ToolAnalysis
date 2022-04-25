#ifndef LAPPDGausBaselineSubtraction_H
#define LAPPDGausBaselineSubtraction_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TF1.h"
#include "TH1.h"

class LAPPDGausBaselineSubtraction: public Tool {


 public:

  LAPPDGausBaselineSubtraction();
  bool Initialise(std::string configfile,DataModel &data); 
  bool Execute();
  bool Finalise();


 private:
  int LAPPDchannelOffset;
  int BLSVerbosityLevel;
  bool isSim;
  int DimSize;
  int TrigChannel;
  double Deltat;
  string BLSInputWavLabel;
  string BLSOutputWavLabel;
  TH1D* BLHist;

};


#endif
