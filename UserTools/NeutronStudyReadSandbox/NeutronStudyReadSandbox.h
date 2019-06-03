#ifndef NeutronStudyReadSandbox_H
#define NeutronStudyReadSandbox_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TFile.h"
#include "TTree.h"

class NeutronStudyReadSandbox: public Tool {


 public:

  NeutronStudyReadSandbox();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  int primneut,totneut,ispi;
  double nuE,muE,muAngle,mupx,mupy,mupz,piE,piAngle,q2,recoE;
  int iterationNum;
  int nentries;
  TFile* tf;
  TTree* neutT;




};


#endif
