#ifndef BeamTimeAna_H
#define BeamTimeAna_H

#include <string>
#include <iostream>

#include "Tool.h"

#include "TVector3.h"
#include "TH1D.h"
#include "TH2D.h"

class BeamTimeAna: public Tool {


 public:

  BeamTimeAna();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  vector<double> Transit(double x0, double y0, double z0, double xslope, double yslope, double baseline, double radius);

  TH1D* hntp;
  TH1D* hbt;
  TH1D* hbE0;
  TH1D* hbE_early;
  TH1D* hbE_med;
  TH1D* hbE_late;
  TH1D* hbz0;
  TH1D* hbbaseline;
  TH2D* hbdvstimecorr;

  TString InFile;
  TString OutFile;
  double targetR;
  double baseline;
  double tc1;
  double tc2;
  double tc3;
  int ientry;

 private:





};


#endif
