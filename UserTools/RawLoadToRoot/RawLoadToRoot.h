#ifndef RawLoadToRoot_H
#define RawLoadToRoot_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "Waveform.h"
#include "TH1D.h"
#include "TFile.h"
#include "TString.h"
#include "ADCPulse.h"
#include "CalibratedADCWaveform.h"
#include "TTree.h"

class RawLoadToRoot: public Tool {


 public:

  RawLoadToRoot();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

   TFile* treeoutput;
   TTree* rawtotree;
   TString treeoutfile;
   TH1D* AllStartTimesHist;
   TH1D* theWaveformHist;
   TH1D* BSubWaveformHist;
   TH1D* thePulseformHist;
   TH1D* ChanPulseHist;

   int lbound;
   int ubound;
   int onoffswitch;
   int onoffswitchII;
   int lboundII;
   int uboundII;
   int ttot;
   int RelEventNumber;
   int EventNumber;
   int RunNumber;
   int SubRunNumber;
   std::string key;
   std::string value;

   std::map<unsigned long, std::vector<Waveform<unsigned short>>>
    RawADCData;

    map<int,map<int,std::vector<ADCPulse>>> trigev;
    std::map<unsigned long, std::vector<CalibratedADCWaveform<double>>> caladcdata;




};


#endif
