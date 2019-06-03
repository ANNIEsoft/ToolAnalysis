#ifndef LAPPDSaveROOT_H
#define LAPPDSaveROOT_H

#include <string>
#include <iostream>
#include "TFile.h"
#include "TH1D.h"
#include "TString.h"

#include "Tool.h"
#include "TTree.h"

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

   TTree* LAPPDTree;
   double chrgCH1;
   double chrgCH2;
   double chrgCH3;
   double ampCH1;
   double ampCH2;
   double ampCH3;
   double tpsecCH1;
   double tpsecCH2;
   double tpsecCH3;
   double ParaPos;
   double TransPos;
   int PulseNum;
};


#endif
