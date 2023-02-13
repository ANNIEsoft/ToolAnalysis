#ifndef LAPPDTraceMax_H
#define LAPPDTraceMax_H

#include <string>
#include <iostream>

#include "Tool.h"

class LAPPDTraceMax: public Tool {


 public:

  LAPPDTraceMax();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

   double CalcIntegral(Waveform<double> hwav, double lowR, double hiR);
   double CalcIntegralSmoothed(Waveform<double> hwav, double lowR, double hiR);
   std::vector<double> CalcAmp(Waveform<double> hwav, double lowR, double hiR);
   std::vector<double> CalcAmpSmoothed(Waveform<double> hwav, double lowR, double hiR);
   int DimSize;
   double Deltat;
   double lowR;
   double hiR;
   int Nsmooth;

   //int IS1, IS2, IS3, IS4;
   Geometry* _geom;
   int LAPPDTMVerbosity;

};


#endif
