#ifndef LAPPDIntegratePulse_H
#define LAPPDIntegratePulse_H

#include <string>
#include <iostream>

#include "Tool.h"

class LAPPDIntegratePulse: public Tool {


 public:

  LAPPDIntegratePulse();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

   double CalcIntegral(Waveform<double> hwav, double lowR, double hiR);
   double CalcAmp(Waveform<double> hwav, double lowR, double hiR);
   int DimSize;
   double Deltat;
   double lowR;
   double hiR;
   int IS1, IS2, IS3, IS4;
   Geometry* _geom;
   int LAPPDIntegVerbosity;

};


#endif
