#ifndef LAPPDSaveROOT_H
#define LAPPDSaveROOT_H

#include <string>
#include <iostream>
#include "TFile.h"
#include "TH1D.h"
#include "TString.h"

#include "Tool.h"

class LAPPDSaveROOT: public Tool {


 public:

  LAPPDSaveROOT();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

   TFile* tf;
   int miter;

   TTree* outtree;
   int NChannel;
   int TrigChannel;
   int NHistos;
   TH1D** hAmp;
   TH1D** hQ;
   TH1D** hTime;
   bool isFiltered;
   bool isIntegrated;
   bool isSim;
   bool isBLsub;
   int chno;
   double cfdtime;
   double amp;
   double twidth;
   double Deltat;
};


#endif
