#ifndef NeutronStudyWriteTree_H
#define NeutronStudyWriteTree_H

#include <string>
#include <iostream>

#include "Tool.h"

class NeutronStudyWriteTree: public Tool {


 public:

  NeutronStudyWriteTree();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

 TFile* tf;
 TTree* outtree;

 int primneut,totneut,ispi;
 double nuE,muE,muAngle,mupx,mupy,mupz,piE,piAngle,q2,recoE;
 int isgoodmuon,isPismeared,Ntotneutsmeared,Nprimneutsmeared,Nbkgdneutrons;
 int passedselection;
 double smearedMuE,smearedMuangle,myRecoE,unsmearedMuangle,muonefficiency;

};


#endif
