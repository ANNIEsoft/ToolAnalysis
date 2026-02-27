#ifndef BackTracker_H
#define BackTracker_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "Hit.h"
#include "Particle.h"


/**
 * \class BackTracker
 *
 * A tool to link reco info to the paticle(s) that generated the light 
*
* $Author: A.Sutton $
* $Date: 2024/06/16 $
* Contact: atcsutton@gmail.com
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
  
 private:

  // Things we need to pull out of the store
  std::map<unsigned long, std::vector<MCHit>> *fMCHitsMap = nullptr;          ///< All of the MCHits keyed by channel number
  std::map<double, std::vector<MCHit>>        *fClusterMapMC = nullptr;       ///< Clusters that we will be linking MCParticles to
  std::vector<MCParticle>                     *fMCParticles = nullptr;        ///< The true particles from the event
  std::map<int, int>                          *fMCParticleIndexMap = nullptr; ///< Map between the particle Id and it's position in MCParticles vector

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

  /// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;

};


#endif
