#ifndef BackTracker_H
#define BackTracker_H

#include <string>
#include <iostream>
#include <set>

#include "Tool.h"
#include "ADCPulse.h"
#include "Hit.h"
#include "Particle.h"


/**
 * \class BackTracker
 *
 * A tool to link reco info to the paticle(s) that generated the light 
*
* $Author: A.Sutton $ -> D. Ajana
* $Date: 2024/06/16 $ -> 2026/01/26
* Contact: atcsutton@gmail.com -> dja23@fsu.edu
*/
class BackTracker: public Tool {


 public:

  BackTracker(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  bool LoadFromStores(); ///< Does all the loading so I can move it away from the Execute function
  void SumParticleTankCharge();
  void MatchMCParticle(std::vector<MCHit> const &mchits, int &prtId, int &prtPdg, double &eff, double &pur, double &totalCharge); ///< The meat and potatoes
  void DirectParentsFromClockTickWindows();
  void FindNeutronAncestors();

 private:

  // Classifies the PDG of an immediate-background-particle lookup (see
  // fMCHitToImmediateAncestorClass below for the class codes).
  int ClassifyBackgroundPDG(int pdg) const;

  // Things we need to pull out of the store
  std::map<unsigned long, std::vector<MCHit>> *fMCHitsMap = nullptr;          ///< All of the MCHits keyed by channel number
  std::map<double, std::vector<MCHit>>        *fClusterMapMC = nullptr;       ///< Clusters that we will be linking MCParticles to
  std::vector<MCParticle>                     *fMCParticles = nullptr;        ///< The true particles from the event
  std::map<int, int>                          *fMCParticleIndexMap = nullptr; ///< Map between the particle Id and it's position in MCParticles vector
  std::map<unsigned long, std::map<uint16_t, std::vector<int>>> fPMTToDirectParentMap; ///< PMT -> t0 tick -> direct parent IDs from PMTWaveformSim
  std::map<unsigned long, std::vector<std::vector<ADCPulse>>> fRecoADCHits; ///< Reconstructed ADCPulses from PhaseIIADCHitFinder

  // We'll calculate this map from MCHit parent particle to the total charge deposited throughout the tank
  // technically a MCHit could have multiple parents, but they don't appear to in practice
  // the key is particle Id and value is total tank charge
  std::map<int, double> fParticleToTankTotalCharge; 
    
  // We'll save out maps between the local cluster time and
  //   the ID and PDG of the particle that contributed the most energy
  //   the efficiency of capturing all light from the best matched particle in that cluster
  //   the the purity based on the best matched particle
  //   the total deposited charge in the cluster
  //   the ammount of cluster charge due to neutrons
  std::map<double, int>    *fClusterToBestParticleID  = nullptr;
  std::map<double, int>    *fClusterToBestParticlePDG = nullptr; 
  std::map<double, double> *fClusterEfficiency        = nullptr;
  std::map<double, double> *fClusterPurity            = nullptr;
  std::map<double, double> *fClusterTotalCharge       = nullptr;

  // Cluster Time -> MCHit DirectParentIDs
  std::map<double, std::vector<std::vector<int>>> *fClusterHitToDirectParentTrackIDs = nullptr;
  // PMT ID -> reco hit time -> direct parent track IDs
  std::map<unsigned long, std::map<double, std::vector<int>>> *fMCHitToDirectParents = nullptr;

  // PMT ID -> reco hit time -> (neutron trackID, neutron PDG)
  std::map<unsigned long, std::map<double, std::pair<int, int>>> *fMCHitToNeutronAncestor = nullptr;
  // PMT ID -> reco hit time -> neutron class
  //   0  dark noise (pure-noise pulse; all contributing MCHits have primary parent -1)
  //   1  primary neutron from initial interaction boundary
  //   2  secondary neutron from proton
  //   3  secondary neutron from neutron
  //   4  secondary neutron from other parent type
  //  -5  non-neutron physics background aka MCHits from other particles (no neutron in ancestry)
  std::map<unsigned long, std::map<double, int>> *fMCHitToNeutronAncestorClass = nullptr;
  // PMT ID -> reco hit time -> (neutron direct parent trackID, neutron direct parent PDG)
  std::map<unsigned long, std::map<double, std::pair<int, int>>> *fMCHitToNeutronParent = nullptr;
  // PMT ID -> reco hit time -> true if pulse is pure dark noise
  std::map<unsigned long, std::map<double, bool>> *fMCHitToIsDarknoise = nullptr;
  // PMT ID -> reco hit time -> WCSim Nuance interaction mode
  //   -999  = dark noise (no physics parent)
  //   -9999 = mode unavailable (LoadWCSim not run or BackTracker skipped)
  std::map<unsigned long, std::map<double, int>> *fMCHitToInteractionMode = nullptr;
  std::vector<int> fWCSimInteractionModes; // one entry per trigger, from LoadWCSim

  // PMT ID -> reco hit time -> (immediate background ancestor trackID, PDG)
  //   Generalization of the neutron-only walk above to any species: walks up the
  //   DirectParentID chain from the hit's direct parent. If that direct parent is an
  //   e-/e+ (PDG +-11) -- in a water Cherenkov detector essentially every hit is
  //   mediated by an electron/positron at the last step (Cherenkov radiation,
  //   Compton/pair-production, photoelectric effect), so naming it as "the
  //   background particle" carries no information -- look exactly one step further
  //   up to that electron's own direct parent and report that instead.
  //   -5 = dark noise, or ancestor track ID not present in MCParticles (untraced)
  std::map<unsigned long, std::map<double, std::pair<int, int>>> *fMCHitToImmediateAncestor = nullptr;
  // PMT ID -> reco hit time -> classification of fMCHitToImmediateAncestor's PDG
  //   0  dark noise
  //   1  neutron (2112)
  //   2  muon (+-13)
  //   3  charged pion (+-211)
  //   4  proton (2212)
  //   5  photon (22) -- e.g. non-capture gamma, such as from a pi0 decay or bremsstrahlung
  //   6  kaon (+-321, 311, -311)
  //   7  electron/positron (+-11) -- the one-step skip landed on another lepton
  //   8  other identified species (not covered above)
  //  -5  untraced -- ancestor track ID not found in MCParticles (not saved by WCSim)
  std::map<unsigned long, std::map<double, int>> *fMCHitToImmediateAncestorClass = nullptr;

  // ---------------------------------------------------------------------------------
  // DISABLED (kept for future retrieval, never committed anywhere): PrimaryParentID-based
  // ancestor. This read MCParticle::GetPrimaryParentID() instead of walking DirectParentID.
  // Turned off deliberately because PrimaryParentID is a separate WCSim mechanism (inherited
  // top-down through WCSimTrackInformation), so it answers a question derived differently
  // from the neutron scheme and the rest of these branches. fMCHitToRootAncestor below is
  // the walk-derived replacement. To re-enable, uncomment this plus the four blocks marked
  // "DISABLED: PrimaryAncestor" in BackTracker.cpp and ANNIEEventTreeMaker.{h,cpp}.
  //
  // std::map<unsigned long, std::map<double, std::pair<int, int>>> *fMCHitToPrimaryAncestor = nullptr;
  // ---------------------------------------------------------------------------------

  // PMT ID -> reco hit time -> (root ancestor trackID, PDG)
  //   The particle at the TOP of the DirectParentID chain below -- i.e. the generator-level
  //   particle out of the neutrino interaction that this charge deposit descends from.
  //   Deliberately derived by walking DirectParentID to its end, exactly like the neutron
  //   scheme, and NOT from MCParticle::GetPrimaryParentID(): PrimaryParentID is a separate
  //   WCSim mechanism (inherited top-down through WCSimTrackInformation) and would answer a
  //   question derived differently from the rest of these branches.
  //   Only meaningful when the corresponding LineageStatus == 1 (chain reached a primary).
  //   -5 = dark noise, or nothing on the chain resolved
  std::map<unsigned long, std::map<double, std::pair<int, int>>> *fMCHitToRootAncestor = nullptr;

  // PMT ID -> reco hit time -> FULL lineage of the optical photon, ordered nearest-first:
  //   [0] = the hit's direct parent (in a water Cherenkov detector nearly always an e-/e+),
  //   [1] = that particle's parent (e.g. the capture gamma), ... up to the generator primary.
  //   Each entry is (trackID, PDG). This is the species-general analogue of the neutron
  //   scheme's {NeutronAncestor, NeutronParent} pair, but as a whole chain rather than a
  //   single hop, so a class-(-5) hit can be read as "e- <- gamma <- neutron <- proton".
  std::map<unsigned long, std::map<double, std::vector<std::pair<int, int>>>> *fMCHitToLineage = nullptr;
  // PMT ID -> reco hit time -> how the walk terminated:
  //    1  reached a generator-level primary (DirectParentID == 0) -- lineage is complete
  //    0  truncated: the next ancestor is not in MCParticles (WCSim did not save it)
  //   -1  aborted on a circular DirectParentID reference or the depth cap
  //   -5  dark noise: no lineage
  std::map<unsigned long, std::map<double, int>> *fMCHitToLineageStatus = nullptr;
  // Safety cap on the ancestry walk; real chains are only a few generations deep.
  static const int kMaxLineageDepth = 64;

  bool fDirectParentClockTickMatching = true;
  uint16_t fPMTSimPrewindowTicks = 10;
  uint16_t fPMTSimReadoutWindowTicks = 35;



  /// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;

};


#endif
