#ifndef CalcClassificationVars_H
#define CalcClassificationVars_H

#include <string>
#include <iostream>
#include <vector>
#include <map>

#include "Tool.h"
#include "TH1F.h"
#include "TFile.h"
#include "TMath.h"

#include "Hit.h"
#include "LAPPDHit.h"
#include "Position.h"
#include "Direction.h"
#include "RecoVertex.h"
#include "RecoDigit.h"


class CalcClassificationVars: public Tool {

 public:

  CalcClassificationVars();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

  double CalcArcTan(double x, double z);

 private:

  // Configuration variables
  int verbosity=0;
  bool use_mctruth;
  bool isData;
  double lateT;
  double lowQ;

  // ANNIEEvent / RecoStore variables
  int evnum, mcevnum;
  std::map<unsigned long,std::vector<Hit>>* MCHits=nullptr;
  std::map<unsigned long,std::vector<LAPPDHit>>* MCLAPPDHits=nullptr;
  std::map<unsigned long,std::vector<Hit>>* TDCData=nullptr;
  RecoVertex *TrueVertex = nullptr;
  RecoVertex *TrueStopVertex = nullptr;
  bool EventCutStatus;
  std::vector<RecoDigit>* RecoDigits; 
  double TrueMuonEnergy;
  int NumMrdTimeClusters;
  int nrings;
  bool no_pik; 
  int pdg;

  // Geometry variables
  Geometry *geom = nullptr;
  int n_tank_pmts, n_veto_pmts, n_mrd_pmts, n_lappds;
  double tank_center_x, tank_center_y, tank_center_z;
  std::map<unsigned long, double> pmts_x, pmts_y, pmts_z;
  double tank_R, tank_H;
  double tank_innerR = 1.275;		//values for inner structure, outer PMT mountings
  double tank_ymin = 0.;
  double tank_ymax = 0.;

  double cherenkov_angle = 0.719889;	//arccos(1/1.33), if assuming relevant primaries move with velocity c

  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;
  int get_ok;

};


#endif
