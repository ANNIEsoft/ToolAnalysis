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
#include "TFile.h"
#include "TApplication.h"

#include "Hit.h"
#include "LAPPDHit.h"
#include "Position.h"
#include "Direction.h"
#include "RecoVertex.h"
#include "RecoDigit.h"
#include "RecoCluster.h"

class CalcClassificationVars: public Tool {

 public:

  CalcClassificationVars();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

  double CalcArcTan(double x, double z);
  void InitialisePDFs();
  void InitialiseClassificationMaps();
  void StorePionEnergies();
  bool GetBoostStoreVariables();
  void ClassificationVarsMCTruth();
  void ClassificationVarsPMTLAPPD();
  void ClassificationVarsMRD();
  double ComputeChi2(TH1F *h1, TH1F *h2);


 private:

  // Configuration variables
  int verbosity=0;
  bool use_mctruth;
  bool isData;
  std::string pdf_emu;
  std::string pdf_rings;
  std::string singlePEgains;
  double charge_conversion;

  // ANNIEEvent / RecoStore variables
  int evnum, mcevnum;
  std::map<unsigned long,std::vector<Hit>>* MCHits=nullptr;
  std::map<unsigned long,std::vector<LAPPDHit>>* MCLAPPDHits=nullptr;
  std::map<unsigned long,std::vector<Hit>>* TDCData=nullptr;
  std::map<unsigned long,std::vector<MCHit>>* TDCData_MC=nullptr;
  std::vector<MCParticle>* mcparticles = nullptr;
  RecoVertex *TrueVertex = nullptr;
  RecoVertex *TrueStopVertex = nullptr;
  bool EventCutStatus;
  std::vector<RecoDigit>* RecoDigits;
  std::vector<RecoCluster*>* fHitCleaningClusters = nullptr;
  double TrueMuonEnergy;
  double TrueNeutrinoEnergy;
  MCParticle neutrino;
  int NumMrdTimeClusters;
  std::vector<unsigned long> mrddigitchankeysthisevent;
  std::vector<std::vector<int>> MrdTimeClusters;
  int nrings;
  bool no_pik; 
  int pdg;
  bool neutrino_sample;
  std::map<unsigned long, double> pmt_gains;

  // Particle maps
  std::map<int,std::vector<double>> map_pion_energies;
  std::map<int,double> pdgcodetocherenkov;

  //Variable maps
  std::map<std::string,int> classification_map_int;  //map for ints
  std::map<std::string,bool> classification_map_bool;  //map for bools
  std::map<std::string,double> classification_map_double;  //map for doubles
  std::map<std::string,std::vector<double>> classification_map_vector;	//map for vectors
  std::map<std::string,int> classification_map_map;	//maps variable name to correct map

  //PDFs
  TFile *f_emu = nullptr;
  TH1F *pdf_mu_charge = nullptr;
  TH1F *pdf_mu_time = nullptr;
  TH1F *pdf_mu_theta = nullptr;
  TH1F *pdf_mu_phi = nullptr;
  TH1F *pdf_e_charge = nullptr;
  TH1F *pdf_e_time = nullptr;
  TH1F *pdf_e_theta = nullptr;
  TH1F *pdf_e_phi = nullptr;
  TH1F *event_charge = nullptr;
  TH1F *event_time = nullptr;
  TH1F *event_theta = nullptr;
  TH1F *event_phi = nullptr;
 
  TFile *f_rings = nullptr;
  TH1F *pdf_single_charge = nullptr;
  TH1F *pdf_single_time = nullptr;
  TH1F *pdf_single_theta = nullptr;
  TH1F *pdf_single_phi = nullptr;
  TH1F *pdf_multi_charge = nullptr;
  TH1F *pdf_multi_time = nullptr;
  TH1F *pdf_multi_theta = nullptr;
  TH1F *pdf_multi_phi = nullptr;


  //General variables
  double pos_x, pos_y, pos_z, dir_x, dir_y, dir_z;
  Position pos;
  bool multi_ring;
  std::string filename;

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
