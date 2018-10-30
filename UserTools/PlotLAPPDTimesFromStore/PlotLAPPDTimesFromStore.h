#ifndef PlotLAPPDTimesFromStore_H
#define PlotLAPPDTimesFromStore_H

#include <string>
#include <iostream>

#include "Tool.h"

// for drawing
class TApplication;
class TCanvas;
class TH1D;

class PlotLAPPDTimesFromStore: public Tool {


 public:

  PlotLAPPDTimesFromStore();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

 private:

  int verbose;
  int get_ok;
  TimeClass* EventTime=nullptr;
  uint64_t MCEventNum;
  uint16_t MCTriggernum;
  std::map<ChannelKey,std::vector<LAPPDHit>>* MCLAPPDHits=nullptr;
  TApplication* lappdRootDrawApp;
  TCanvas* lappdRootCanvas;
  TH1D* digitime;

};


#endif
