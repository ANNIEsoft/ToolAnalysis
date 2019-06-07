#ifndef FTBFAnalysis_H
#define FTBFAnalysis_H

#include <string>
#include <iostream>

#include "Tool.h"

class FTBFAnalysis: public Tool {


 public:

  FTBFAnalysis();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  void HeatmapEvent(int event, int board);
  void PlotSeparateChannels(int event, int board);


 private:
 	TFile* tff;





};


#endif
