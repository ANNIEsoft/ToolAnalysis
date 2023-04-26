#ifndef NeutronMultiplicity_H
#define NeutronMultiplicity_H

#include <string>
#include <iostream>

#include "Tool.h"

#include "Particle.h"
#include "Position.h"

#include "TTree.h"
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TGraph.h"
#include "TGraphErrors.h"

/**
 * \class NeutronMultiplicity
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: M.Nieslony $
* $Date: 2023/02/18 10:44:00 $
* Contact: mnieslon@uni-mainz.de
*/

class NeutronMultiplicity: public Tool {


 public:

  NeutronMultiplicity(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  bool InitialiseHistograms(); ///< Initialise root histograms & file
  bool SaveBoostStore(); ///< Save variables to BoostStore
  bool FillHistograms(); ///< Fill histograms & save to root file
  bool GetParticleInformation(); ///< Get reconstructed information about muon vertex
  bool ResetVariables(); ///< Reset variables every Execute step

 private:

  //configuration variables
  int verbosity;
  bool save_root;
  bool save_bs;
  std::string filename;
  bool mrdtrack_restriction = false;

  //root file variables
  TFile *file_neutronmult = nullptr;

  //BoostStore file variable
  BoostStore store_neutronmult;

  //Reconstruction variables
  int SimpleRecoFlag;
  Position SimpleRecoVtx;
  Position SimpleRecoStopVtx;
  double SimpleRecoEnergy;
  double SimpleRecoCosTheta;
  bool SimpleRecoFV;
  double SimpleRecoMrdEnergyLoss;
  int NumberNeutrons;
  std::vector<Particle> Particles;

  //histogram variables
  TH1F *h_time_neutrons = nullptr;
  TH1F *h_time_neutrons_mrdstop = nullptr;
  TH1F *h_neutrons = nullptr;
  TH1F *h_neutrons_mrdstop = nullptr;
  TH1F *h_neutrons_mrdstop_fv = nullptr;
  TH2F *h_neutrons_energy = nullptr;
  TH2F *h_neutrons_energy_fv = nullptr;
  TH2F *h_neutrons_energy_zoom = nullptr;
  TH2F *h_neutrons_energy_fv_zoom = nullptr;
  TH2F *h_primneutrons_energy = nullptr;
  TH2F *h_primneutrons_energy_fv = nullptr;
  TH2F *h_primneutrons_energy_zoom = nullptr;
  TH2F *h_primneutrons_energy_fv_zoom = nullptr;
  TH2F *h_totalneutrons_energy = nullptr;
  TH2F *h_totalneutrons_energy_fv = nullptr;
  TH2F *h_totalneutrons_energy_zoom = nullptr;
  TH2F *h_totalneutrons_energy_fv_zoom = nullptr;
  TH2F *h_pmtvolneutrons_energy = nullptr;
  TH2F *h_pmtvolneutrons_energy_fv = nullptr;
  TH2F *h_pmtvolneutrons_energy_zoom = nullptr;
  TH2F *h_pmtvolneutrons_energy_fv_zoom = nullptr;
  TH2F *h_neutrons_costheta = nullptr;
  TH2F *h_neutrons_costheta_fv = nullptr;
  TH2F *h_primneutrons_costheta = nullptr;
  TH2F *h_primneutrons_costheta_fv = nullptr;
  TH2F *h_totalneutrons_costheta = nullptr;
  TH2F *h_totalneutrons_costheta_fv = nullptr;
  TH2F *h_pmtvolneutrons_costheta = nullptr;
  TH2F *h_pmtvolneutrons_costheta_fv = nullptr;

  TH1F *h_muon_energy = nullptr;
  TH1F *h_muon_energy_fv = nullptr;
  TH2F *h_muon_vtx_yz = nullptr;
  TH2F *h_muon_vtx_xz = nullptr;
  TH1F *h_muon_costheta = nullptr;
  TH1F *h_muon_costheta_fv = nullptr;
  TH1F *h_muon_vtx_x = nullptr;
  TH1F *h_muon_vtx_y = nullptr;
  TH1F *h_muon_vtx_z = nullptr; 

  //tree variables 
  //
  TTree *neutron_tree = nullptr;
  //true tree variables
  int true_PrimNeut;
  int true_PrimProt;
  int true_NCaptures;
  int true_NCapturesPMTVol;
  double true_VtxX;
  double true_VtxY;
  double true_VtxZ;
  double true_Emu;
  double true_Enu;
  double true_Q2;
  int true_FV;
  double true_CosTheta;
  std::vector<double> *true_NeutVtxX = nullptr;
  std::vector<double> *true_NeutVtxY = nullptr;
  std::vector<double> *true_NeutVtxZ = nullptr;
  std::vector<double> *true_NeutCapNucl = nullptr;
  std::vector<double> *true_NeutCapTime = nullptr;
  std::vector<double> *true_NeutCapETotal = nullptr;
  std::vector<double> *true_NeutCapNGamma = nullptr;
  std::vector<int> *true_NeutCapPrimary = nullptr;
  int true_CC;
  int true_QEL;
  int true_DIS;
  int true_RES;
  int true_COH;
  int true_MEC;
  int true_MultiRing;
  std::vector<int> *true_PrimaryPdgs = nullptr;

  //reco tree variables
  double reco_Emu;
  double reco_Enu;
  double reco_Q2;
  std::vector<double> *reco_ClusterCB = nullptr;
  std::vector<double> *reco_ClusterTime = nullptr;
  std::vector<double> *reco_ClusterPE = nullptr;
  std::vector<double> *reco_NCandCB = nullptr;
  std::vector<double> *reco_NCandTime = nullptr;
  std::vector<double> *reco_NCandPE = nullptr;
  double reco_MrdEnergyLoss;
  int reco_TankMRDCoinc;
  int reco_Clusters;
  int reco_NCandidates;
  double reco_VtxX;
  double reco_VtxY;
  double reco_VtxZ;
  int reco_FV;
  double reco_CosTheta;

  //verbosity variables
  int v_error = 0;
  int v_warning = 1;
  int v_message = 2;
  int v_debug = 3;
  int vv_debug = 4;

};


#endif
