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

  int verbosity;
  int get_ok;
  TimeClass* EventTime=nullptr;
  uint64_t MCEventNum;
  uint16_t MCTriggernum;
  std::map<unsigned long,std::vector<LAPPDHit>>* MCLAPPDHits=nullptr;
  std::vector<MCParticle>* MCParticles=nullptr;
  TApplication* lappdRootDrawApp;
  TCanvas* lappdRootCanvas;
  TH1D* digitime, *digitimewithmuon, *mutime;
  Geometry* anniegeom=nullptr;
  
  // verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;

};


#endif
