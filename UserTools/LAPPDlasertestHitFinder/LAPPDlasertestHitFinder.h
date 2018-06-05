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
   bool TwoSided;
   int CenterChannel;
   double MaxAmp;
   double Deltatime;
   double PTRange;
   double ParaPosition;
   double PerpPosition;
   double HitTime;
   std::vector<double> channelID = vector<double>(3);
   std::vector<double> AbsPosition;
   std::vector<double> LocalPosition;
   std::vector<LAPPDPulse> NeighboursPulses;
   LAPPDPulse MaxPulse;
   LAPPDPulse OpposPulse;
   uint64_t MaxTimeWindow;
   uint64_t MinTimeWindow;




};


#endif
