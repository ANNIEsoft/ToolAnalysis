#ifndef LAPPDlasertestHitFinder_H
#define LAPPDlasertestHitFinder_H

#include "LAPPDPulse.h"
#include <string>
#include <iostream>
#include "Tool.h"
#include "../LAPPDSim/LAPPDresponse.hh"
#include "LAPPDHit.h"
#include "LAPPDFindPeak.h"

class LAPPDlasertestHitFinder: public Tool {


 public:

  LAPPDlasertestHitFinder();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

   double Deltatime;
   std::vector<double> stripID = vector<double>(3);

   bool TwoSided;
   int CenterChannel;
   double PTRange;

   uint64_t MaxTimeWindow;
   uint64_t MinTimeWindow;




};


#endif
