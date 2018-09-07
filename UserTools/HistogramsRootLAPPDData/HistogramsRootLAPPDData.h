#ifndef HistogramsRootLAPPDData_H
#define LAPPDSaveROOT_H

#include <string>
#include <iostream>
#include "TFile.h"
#include "TH1D.h"
#include "TString.h"
#include "ChannelKey.h"
#include "Tool.h"
#include "LAPPDHit.h"
#include "LAPPDresponse.hh"
#include "TimeClass.h"
#include "LAPPDPulse.h"
class HistogramsRootLAPPDData: public Tool {


 public:

  HistogramsRootLAPPDData();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

   TFile* tff;
   int miter;
   TH1D* HitMultiplicity;
   TH2D* TimeToStrip;



   TTree* DataTree;
   double xpos;
   double ypos;
   double zpos;
   double thehittime;
   double LAPPDnumber;
   TTree* AmpTree;
   double amp;
};


#endif
