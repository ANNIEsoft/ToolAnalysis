#ifndef LAPPDSim_H
#define LAPPDSim_H

#include <string>
#include <iostream>

#include "Geometry.h"
#include "Detector.h"
#include "Tool.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "wcsimT.h"
#include "LAPPDresponse.h"
#include "TBox.h"
#include "TApplication.h"
#include "LAPPDDisplay.h"
#include "TRint.h"
// #include "Hit.h"
// #include "LAPPDHit.h"

class LAPPDSim: public Tool {


 public:

  LAPPDSim();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  Waveform<double> SimpleGenPulse(vector<double> pulsetimes);

 private:
   TRandom3* myTR;
   TFile* _tf;
   int _event_counter;
   int _file_number;
   int _display_config;
   bool _is_artificial;
   LAPPDDisplay* _display;
   Geometry* _geom;
   std::map<unsigned long, Waveform<double> >* LAPPDWaveforms;

};


#endif
