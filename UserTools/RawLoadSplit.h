#ifndef RawLoadSplit_H
#define RawLoadSplit_H

#include <string>
#include <iostream>

#include <TFile.h>
#include <TTree.h>

#include "Tool.h"
#include "PMTData.h"
#include "RunInformation.h"
#include "MRDTree.h"
#include "PulseTree.h"
#include "RawReadout.hh"

class RawLoadSplit: public Tool {


 public:

  RawLoadSplit();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  int m_debug;
  long currententry;
  int currentsequenceid;
  long totalentries;

  TChain* PMTDataChain;
  TChain* RunInformationChain;
  TChain* MRDChain;
  PMTData* WaterPMTData;
  MRDTree* MRDData;
  RunInformation* RunInformationData;
  PulseTree* PulseData;



};


#endif
