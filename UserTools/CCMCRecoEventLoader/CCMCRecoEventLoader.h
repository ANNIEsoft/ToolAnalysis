#ifndef CCMCRecoEventLoader_H
#define CCMCRecoEventLoader_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "ANNIEGeometry.h"
#include "Detector.h"
#include "TMath.h"

class CCMCRecoEventLoader: public Tool {


 public:

  CCMCRecoEventLoader();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

  // Config file inputs
  int verbosity=1;
  bool fGetPiKInfo;
  bool fGetNRings;
  int fParticleID;
  bool fFollowerCheck; 
  double xshift;
  double yshift;
  double zshift;

  RecoVertex* fMuonStartVertex = nullptr; 	 ///< true muon start vertex
  RecoVertex* fMuonStopVertex = nullptr; 	 ///< true muon stop vertex
  std::vector<MCParticle>* fMCParticles=nullptr;  ///< truth tracks
  double TrueMuonEnergy = -9999.;
  double WaterTrackLength = -9999.;
  double MRDTrackLength = -9999.;
  bool projectedmrdhit = false;
  std::map<int,double> pdgcodetomass;     ///< map that links pdg code to particle mass in MeV, provided by MCParticleProperties
  std::map<int,std::string> pdgcodetoname;
  std::map<int,double> pdgcodetocherenkov;

  double n = 1.333;   //refractive index of water, should probably rather be defined somewhere else


  /// \brief Find and save true neutrino vertex and muon stop vertex
  ///
  /// Loop over all MC particles and find the particle with highest energy. 
  /// This particle is the primary muon. The muon start position, time and 
  /// the muon direction are used to initise the true neutrino vertex and includes
  /// muon energy  
  void FindTrueVertexFromMC();
 
  /// \brief Find PionKaon Count 
  ///
  /// Loop over all MC particles and find any particles with PDG codes
  /// Consistent with Pions or Kaons of any charges. Racks up a count
  /// of the number of each type of particle
  /// In addition: Loop over MC particles and count the number of rings that should be produced
  /// by those particles. The particle needs to be above Cherenkov threshold to
  /// produce a ring. Neutrally charged particles like the Pi0 produce 2 rings
  /// Loop over all MC particles with parent ID = 0 (primaries) and
  /// store their pdg numbers in a vector
  /// Includes the following information for all Final State Particles
  /// Energy, tank track length, mrd track length, whether it is contained
  /// within the tank or mrd, angle within the mrd, energy difference
  /// between start and stop points

  void FindPionKaonCountFromMC();

  /// \brief Find and save followers from neutral pion decays
  ///
  /// Loop over the descendants of primary pions. Find and save
  /// any and all descendants that would show up in the detector such as:
  /// muons, electrons, or gammas

  void FindFollowersFromMC();

  /// \brief GetCherenkovThresholdE
  /// Get Cherenkov threshold energy for a given particle PDG number code

  double GetCherenkovThresholdE(int pdg_code);
  
  /// \brief Push IBD/IBD-like true information to RecoEvent store
  void PushIBDInfo();

  /// \brief Reset initialized classes
  ///
  /// Clear True Vertices 
  void Reset();

  /// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;

 private:





};


#endif
