#ifndef MCPropertiesToTree_H
#define MCPropertiesToTree_H

#include <string>
#include <iostream>

#include "TFile.h"
#include "TH1F.h"
#include "TTree.h"
#include "TROOT.h"

#include "Tool.h"
#include "Geometry.h"
#include "Particle.h"
#include "TriggerClass.h"
#include "Geometry.h"
#include "TimeClass.h"

/**
 * \class MCPropertiesToTree
 *
 * 
* $Author: M.Nieslony $
* $Date: 2019/09/16 15:28:00 $
* Contact: mnieslon@uni-mainz.de
*/

class MCPropertiesToTree: public Tool {


 public:

  MCPropertiesToTree(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  void DefineHistograms();
  void DefineTree();
  void ClearVectors();
  void WriteHistograms();
  void EvalTriggerData();
  void EvalMCParticles();
  void EvalMCHits();
  void EvalMCLAPPDHits();
  void EvalTDCData();
  void EvalGENIE();

 private:

  //Configuration variables
  int verbosity;
  std::string outfile_name;
  bool save_histograms;
  bool save_tree;
  bool has_genie;
	
  //BoostStore objects
  Geometry *geom = nullptr;
  std::vector<MCParticle>* mcparticles = nullptr;
  std::vector<TriggerClass>* TriggerData = nullptr;
  uint16_t MCTriggernum=0;
  std::map<unsigned long, std::vector<MCHit>>* MCHits=nullptr;
  std::map<unsigned long, std::vector<MCLAPPDHit>>* MCLAPPDHits=nullptr;
  std::map<unsigned long,vector<MCHit>>* TDCData=nullptr;
  int evnum=-1;
  int mcevnum=-1;
  int nrings=-1;
  TimeClass TriggerTime;	

  //General-purpose variables
  double tank_center_x=-1., tank_center_y=-1., tank_center_z=-1.;
  bool fill_tree=false;

  //Define ROOT TFile pointer
  TFile *f = nullptr;

  //Define ROOT histograms pointer
  TH1F *hE = nullptr;
  TH1F *hPosX = nullptr;
  TH1F *hPosY = nullptr;
  TH1F *hPosZ = nullptr;
  TH1F *hPosStopX = nullptr;
  TH1F *hPosStopY = nullptr;
  TH1F *hPosStopZ = nullptr;
  TH1F *hDirX = nullptr;
  TH1F *hDirY = nullptr;
  TH1F *hDirZ = nullptr;
  TH1F *hQ = nullptr;
  TH1F *hT = nullptr;
  TH1F *hQtotal = nullptr;
  TH1F *hQ_LAPPD = nullptr;
  TH1F *hT_LAPPD = nullptr;
  TH1F *hQtotal_LAPPD = nullptr;
  TH1F *hNumPrimaries = nullptr;
  TH1F *hNumSecondaries = nullptr;
  TH1F *hPDGPrimaries = nullptr;
  TH1F *hPDGSecondaries = nullptr;
  TH1F *hMRDPaddles = nullptr;
  TH1F *hMRDLayers = nullptr;
  TH1F *hMRDClusters = nullptr;
  TH1F *hVetoHits = nullptr;
  TH1F *hPMTHits = nullptr;
  TH1F *hLAPPDHits = nullptr;
  TH1F *hRings = nullptr;
  TH1F *hNoPiK = nullptr;
  TH1F *hMRDStop = nullptr;
  TH1F *hFV = nullptr;
  TH1F *hPMTVol = nullptr;

  //Define ROOT TTrees
  TTree *t = nullptr;
  TTree *t_genie = nullptr;

  //Define WCSim TTree variables
  std::vector<double> *particleE = nullptr;
  std::vector<int> *particlePDG = nullptr;
  std::vector<int> *particleParentPDG = nullptr;
  std::vector<int> *particleFlag = nullptr;
  std::vector<double> *sec_particleE = nullptr;
  std::vector<int> *sec_particlePDG = nullptr;
  std::vector<int> *sec_particleParentPDG = nullptr;
  std::vector<int> *sec_particleFlag = nullptr;
  bool is_prompt=false;
  int mctriggernum=-1;
  ULong64_t trigger_time=0;
  int particleTriggers=-1;
  int num_primaries=-1, num_secondaries=-1;
  int pmtHits=-1, lappdHits=-1, mrdPaddles=-1, mrdLayers=-1, mrdClusters=-1, num_veto_hits=-1, event_pmtclusters=-1;
  bool mrd_stop=false, event_fv=false, event_pmtvol=false, no_pik=false, event_singlering=false, event_multiring=false, event_pmtmrdcoinc=false;
  std::vector<double> *pmtQ = nullptr;
  std::vector<double> *pmtT = nullptr;
  std::vector<int> *pmtID = nullptr;
  std::vector<double> *lappdQ = nullptr;
  std::vector<double> *lappdT = nullptr;
  std::vector<int> *lappdID = nullptr;
  std::vector<double> *mrdT = nullptr;
  std::vector<int> *mrdID = nullptr;
  std::vector<double> *fmvT = nullptr;
  std::vector<int>* fmvID = nullptr;
  double pmtQtotal, lappdQtotal;
  std::vector<double> *particle_posX = nullptr;
  std::vector<double> *particle_posY = nullptr;
  std::vector<double> *particle_posZ = nullptr;
  std::vector<double> *particle_stopposX = nullptr;
  std::vector<double> *particle_stopposY = nullptr;
  std::vector<double> *particle_stopposZ = nullptr;
  std::vector<double> *particle_dirX = nullptr;
  std::vector<double> *particle_dirY = nullptr;
  std::vector<double> *particle_dirZ = nullptr;
  std::vector<double> *sec_particle_posX = nullptr;
  std::vector<double> *sec_particle_posY = nullptr;
  std::vector<double> *sec_particle_posZ = nullptr;
  std::vector<double> *sec_particle_stopposX = nullptr;
  std::vector<double> *sec_particle_stopposY = nullptr;
  std::vector<double> *sec_particle_stopposZ = nullptr;
  std::vector<double> *sec_particle_dirX = nullptr;
  std::vector<double> *sec_particle_dirY = nullptr;
  std::vector<double> *sec_particle_dirZ = nullptr;
  std::vector<double> *event_pmtclusters_Q = nullptr;
  std::vector<double> *event_pmtclusters_T = nullptr;
  std::vector<double> *event_mrdclusters_T = nullptr;

  //GENIE tree variables
  std::string genie_file="dummy";
  int genie_fluxver=-1;
  unsigned int genie_evtnum=0;
  int genie_parentpdg=-1;
  int genie_parentdecaymode=-1;
  std::string genie_parentdecaystring="dummy";
  Position genie_parentdecayvtx;
  float genie_parentdecayvtxx=-1., genie_parentdecayvtxy=-1., genie_parentdecayvtxz=-1.;
  Position genie_parentdecaymom;
  float genie_parentdecaymomx=-1., genie_parentdecaymomy=-1., genie_parentdecaymomz=-1.;
  Position genie_parentprodmom;
  float genie_prodmomx=-1., genie_prodmomy=-1., genie_prodmomz=-1.;
  int genie_parentprodmedium=-1;
  std::string genie_parentprodmediumstring = "dummy";
  int genie_parentpdgattgtexit=-1.;
  std::string genie_parenttypeattgtexitstring="dummy";
  Position genie_parenttgtexitmom;
  float genie_parenttgtexitmomx=-1., genie_parenttgtexitmomy=-1., genie_parenttgtexitmomz=-1.;

  bool genie_isquasielastic=false, genie_isresonant=false, genie_isdeepinelastic=false, genie_iscoherent=false, genie_isdiffractive=false, genie_isinversemudecay=false, genie_isimdannihilation=false, genie_issinglekaon=false, genie_isnuelectronelastic=false, genie_isem=false, genie_isweakcc=false, genie_isweaknc=false, genie_ismec=false;
  std::string genie_interactiontypestring="dummy";
  int genie_neutcode=-1;
  double genie_nuintvtxx=-1., genie_nuintvtxy=-1., genie_nuintvtxz=-1., genie_nuintvtxt=-1.;
  bool genie_nuvtxintank=false, genie_nuvtxinfidvol=false;
  double genie_eventQ2=-1.;
  double genie_neutrinoenergy=-1.;
  int genie_neutrinopdg=-1;
  double genie_muonenergy=-1., genie_muonangle=-1.;
  std::string genie_fsleptonname="dummy";
  int genie_numfsp=-1, genie_numfsn=-1, genie_numfspi0=-1, genie_numfspiplus=-1, genie_numfspiminus=-1, genie_numfskplus=-1, genie_numfskminus=-1;

  //std::string *genie_file_pointer = nullptr;
  //std::string *genie_parentdecaystring_pointer = nullptr;
  //std::string *genie_parentpdgattgtexitstring_pointer = nullptr;
  //std::string *genie_interactiontypestring_pointer = nullptr;
  //std::string *genie_fsleptonname_pointer = nullptr;

  //Verbosity variables
  int v_error = 0;
  int v_warning = 1;
  int v_message = 2;
  int v_debug = 3;

};


#endif
