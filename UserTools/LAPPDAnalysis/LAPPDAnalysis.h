#ifndef LAPPDAnalysis_H
#define LAPPDAnalysis_H

#include <string>
#include <iostream>

#include "TH1.h"
#include "TF1.h"
#include "Tool.h"

class LAPPDAnalysis: public Tool {


 public:

  LAPPDAnalysis();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

      Waveform<double> SubtractSine(Waveform<double> iwav);
      bool isSim;
      int DimSize;
      double Deltat;
      double LowBLfitrange;
      double HiBLfitrange;




};


#endif
