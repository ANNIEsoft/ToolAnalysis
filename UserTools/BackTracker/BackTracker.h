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
