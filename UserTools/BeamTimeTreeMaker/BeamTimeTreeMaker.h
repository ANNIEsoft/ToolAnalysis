#ifndef BeamTimeTreeMaker_H
#define BeamTimeTreeMaker_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TRandom3.h"
#include "TTree.h"
#include "TFile.h"

class BeamTimeTreeMaker: public Tool {


 public:

  BeamTimeTreeMaker();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

   //TFile* tf;
   TTree* outtree;
   TString OutFile;

   int nutype,nuparent,nparticles;
   double parentE,parentpx,parentpy,parentpz,parentpperp,parentang;
   double nuE,nupx,nupy,nupz;
   double nupperp,nuang;
   double nustartx,nustarty,nustartz,nustartT;
   double nuendx,nuendy,nuendz,nuendT,nuendTs1,nuendTs2;
   double beamwgt;
   bool passescut;

   TRandom3* mrand1;
   TRandom3* mrand2;
};


#endif
