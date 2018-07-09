#ifndef NeutronStudyPMCS_H
#define NeutronStudyPMCS_H

#include <string>
#include <iostream>
#include "TRandom3.h"
#include "TVector3.h"

#include "Tool.h"

class NeutronStudyPMCS: public Tool {


 public:

  NeutronStudyPMCS();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  double nuEEfficiency(double nuE);
  double MuonEfficiency(double mu_E, double mu_angle);
  double PionInefficiency(double pi_E, double pi_angle);
  double MuEsmear(double mu_E, double Eres);
  double MuAnglesmear(double mu_px, double mu_py, double mu_pz, double angsmear);
  double RecoE(double mu_E,double mu_angle);
  int DetectedNeutrons(int totneut);
  int BkgNeutrons(double prob);

 private:

  TRandom3* ttr;
  TRandom3* trr;
  // how much to smear the muon energy
  double muEsmear;
  // how much to smear muon angle
  double muAngsmear;

};


#endif
