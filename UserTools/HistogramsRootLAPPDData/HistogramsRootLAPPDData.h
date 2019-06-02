#ifndef HistogramsRootLAPPDData_H
#define LAPPDSaveROOT_H

#include <string>
#include <iostream>
#include "TFile.h"
#include "TH1D.h"
#include "TString.h"
#include "Tool.h"
#include "LAPPDHit.h"
#include "LAPPDresponse.h"
#include "LAPPDPulse.h"
#include "TTree.h"
#include "TH2D.h"

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
   
   // converting between LAPPDID and channelkey
   Geometry* anniegeom=nullptr;
   std::map<unsigned long,int> detectorkey_to_lappdid;
   int get_ok;



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
