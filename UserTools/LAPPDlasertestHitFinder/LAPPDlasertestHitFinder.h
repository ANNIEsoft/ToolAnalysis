#ifndef LAPPDlasertestHitFinder_H
#define LAPPDlasertestHitFinder_H

#include "LAPPDPulse.h"
#include <string>
#include <iostream>
#include "Tool.h"
#include "LAPPDresponse.h"
#include "LAPPDHit.h"
#include "LAPPDFindPeak.h"

using namespace std;

class LAPPDlasertestHitFinder: public Tool {


 public:

  LAPPDlasertestHitFinder();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

   double Deltatime;
   std::vector<double> stripID;
   double stripID1;
   double stripID2;
   double stripID3;
   bool TwoSided;
   int CenterChannel;
   double PTRange;

   uint64_t MaxTimeWindow;
   uint64_t MinTimeWindow;




};


#endif
