#ifndef LAPPDcfd_H
#define LAPPDcfd_H

#include <string>
#include <iostream>
#include "TSplineFit.h"
#include "TPoly3.h"
#include "LAPPDPulse.h"
#include "LAPPDHit.h"
#include "Waveform.h"
#include "TH1D.h"

#include "Tool.h"

class LAPPDcfd: public Tool {


 public:

  LAPPDcfd();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  double CFD_Discriminator1(std::vector<double>* trace, LAPPDPulse pulse);
  double CFD_Discriminator2(std::vector<double>* trace, LAPPDPulse pulse);


 private:
   bool isSim;
   double Fraction_CFD;
   string CFDInputWavLabel;


};


#endif
