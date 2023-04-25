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
  bool GetParticleInformation();

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


  //verbosity variables
  int v_error = 0;
  int v_warning = 1;
  int v_message = 2;
  int v_debug = 3;
  int vv_debug = 4;

};


#endif
