#include "BackTracker.h"
#include "ANNIEconstants.h"
#include <algorithm>

BackTracker::BackTracker():Tool(){}

// To sort
struct sort_by_charge {
    bool operator()(const std::pair<int,int> &left, const std::pair<int,int> &right) {
        return left.second < right.second;
    }
};


bool BackTracker::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  // Load my config parameters
  bool gotVerbosity = m_variables.Get("verbosity",verbosity);
  if (!gotVerbosity) {
    verbosity = 0;
    logmessage = "BackTracker::Initialize: \"verbosity\" not set in the config, defaulting to 0";
    Log(logmessage, v_error, verbosity);
  }

  bool gotUsePulseWindowMatching = m_variables.Get("UseDirectParentClockTickMatching", fDirectParentClockTickMatching);
  if (!gotUsePulseWindowMatching) fDirectParentClockTickMatching = true;


  // Set up the pointers we're going to save. No need to 
  // delete them at Finalize, the store will handle it
  fClusterToBestParticleID  = new std::map<double, int>;
  fClusterToBestParticlePDG = new std::map<double, int>;
  fClusterEfficiency        = new std::map<double, double>;
  fClusterPurity            = new std::map<double, double>;
  fClusterTotalCharge       = new std::map<double, double>;
  fMCHitToDirectParents     = new std::map<unsigned long, std::map<double, std::vector<int>>>;
  fMCHitToNeutronAncestor   = new std::map<unsigned long, std::map<double, std::pair<int, int>>>;
  fMCHitToNeutronAncestorClass = new std::map<unsigned long, std::map<double, int>>;
  fMCHitToNeutronParent     = new std::map<unsigned long, std::map<double, std::pair<int, int>>>;
  fMCHitToIsDarknoise       = new std::map<unsigned long, std::map<double, bool>>;
  fMCHitToInteractionMode   = new std::map<unsigned long, std::map<double, int>>;
  fMCHitToImmediateAncestor = new std::map<unsigned long, std::map<double, std::pair<int, int>>>;
  fMCHitToImmediateAncestorClass = new std::map<unsigned long, std::map<double, int>>;
  // DISABLED: PrimaryAncestor (PrimaryParentID-based; see BackTracker.h)
  // fMCHitToPrimaryAncestor   = new std::map<unsigned long, std::map<double, std::pair<int, int>>>;
  fMCHitToRootAncestor      = new std::map<unsigned long, std::map<double, std::pair<int, int>>>;
  fMCHitToLineage           = new std::map<unsigned long, std::map<double, std::vector<std::pair<int, int>>>>;
  fMCHitToLineageStatus     = new std::map<unsigned long, std::map<double, int>>;

  return true;
}

bool BackTracker::Execute()
{
  if (!LoadFromStores())
    return false;

  fClusterToBestParticleID ->clear();
  fClusterToBestParticlePDG->clear();
  fClusterEfficiency       ->clear();
  fClusterPurity           ->clear();
  fClusterTotalCharge      ->clear();
  fMCHitToDirectParents    ->clear();
  fMCHitToNeutronAncestor  ->clear();
  fMCHitToNeutronAncestorClass->clear();
  fMCHitToNeutronParent->clear();
  fMCHitToIsDarknoise->clear();
  fMCHitToInteractionMode->clear();
  fMCHitToImmediateAncestor->clear();
  fMCHitToImmediateAncestorClass->clear();
  // DISABLED: PrimaryAncestor
  // fMCHitToPrimaryAncestor->clear();
  fMCHitToRootAncestor->clear();
  fMCHitToLineage->clear();
  fMCHitToLineageStatus->clear();

  fParticleToTankTotalCharge.clear();

  SumParticleTankCharge();

  if (fDirectParentClockTickMatching) {
    // Required tool order: PMTWaveformSim -> PhaseIIADCHitFinder -> BackTracker
    DirectParentsFromClockTickWindows();
    FindNeutronAncestors();
  }

  // Loop over the clusters and do the things
  for (std::pair<double, std::vector<MCHit>>&& apair : *fClusterMapMC) {
    int prtId = -5;
    int prtPdg = -5;
    double eff = -5;
    double pur = -5;
    double totalCharge = 0;

    MatchMCParticle(apair.second, prtId, prtPdg, eff, pur, totalCharge);

    fClusterToBestParticleID ->emplace(apair.first, prtId);
    fClusterToBestParticlePDG->emplace(apair.first, prtPdg);
    fClusterEfficiency       ->emplace(apair.first, eff);
    fClusterPurity           ->emplace(apair.first, pur);
    fClusterTotalCharge      ->emplace(apair.first, totalCharge);

  }

  m_data->Stores.at("ANNIEEvent")->Set("ClusterToBestParticleID",  fClusterToBestParticleID );
  m_data->Stores.at("ANNIEEvent")->Set("ClusterToBestParticlePDG", fClusterToBestParticlePDG);
  m_data->Stores.at("ANNIEEvent")->Set("ClusterEfficiency",        fClusterEfficiency       );
  m_data->Stores.at("ANNIEEvent")->Set("ClusterPurity",            fClusterPurity           );
  m_data->Stores.at("ANNIEEvent")->Set("ClusterTotalCharge",       fClusterTotalCharge      );
  m_data->Stores.at("ANNIEEvent")->Set("MCHitToDirectParents",     fMCHitToDirectParents    );
  m_data->Stores.at("ANNIEEvent")->Set("MCHitToNeutronAncestor",   fMCHitToNeutronAncestor  );
  m_data->Stores.at("ANNIEEvent")->Set("MCHitToNeutronAncestorClass", fMCHitToNeutronAncestorClass);
  m_data->Stores.at("ANNIEEvent")->Set("MCHitToNeutronParent", fMCHitToNeutronParent); //It stores
  m_data->Stores.at("ANNIEEvent")->Set("MCHitToIsDarknoise", fMCHitToIsDarknoise);
  m_data->Stores.at("ANNIEEvent")->Set("MCHitToInteractionMode", fMCHitToInteractionMode);
  m_data->Stores.at("ANNIEEvent")->Set("MCHitToImmediateAncestor", fMCHitToImmediateAncestor);
  m_data->Stores.at("ANNIEEvent")->Set("MCHitToImmediateAncestorClass", fMCHitToImmediateAncestorClass);
  // DISABLED: PrimaryAncestor
  // m_data->Stores.at("ANNIEEvent")->Set("MCHitToPrimaryAncestor", fMCHitToPrimaryAncestor);
  m_data->Stores.at("ANNIEEvent")->Set("MCHitToRootAncestor", fMCHitToRootAncestor);
  m_data->Stores.at("ANNIEEvent")->Set("MCHitToLineage", fMCHitToLineage);
  m_data->Stores.at("ANNIEEvent")->Set("MCHitToLineageStatus", fMCHitToLineageStatus);

  return true;
}

//------------------------------------------------------------------------------
bool BackTracker::Finalise()
{

  return true;
}

//------------------------------------------------------------------------------
void BackTracker::SumParticleTankCharge()
{
  for (auto mcHitsIt : *fMCHitsMap) {
    std::vector<MCHit> mcHits = mcHitsIt.second;
    for (uint mcHitIdx = 0; mcHitIdx < mcHits.size(); ++mcHitIdx) {

      // technically a MCHit could have multiple parents, but they don't appear to in practice
      // skip any cases we come across
      std::vector<int> parentIdxs = *(mcHits[mcHitIdx].GetParents());
      if (parentIdxs.size() != 1) continue;
      
      int particleId = -5;
      for (auto it : *fMCParticleIndexMap) {
	if (it.second == parentIdxs[0]) particleId = it.first;
      }
      if (particleId == -5) continue;
	
      double depositedCharge = mcHits[mcHitIdx].GetCharge();      
      if (!fParticleToTankTotalCharge.count(particleId)) 
	fParticleToTankTotalCharge.emplace(particleId, depositedCharge);
      else 
	fParticleToTankTotalCharge.at(particleId) += depositedCharge;
    }    
  }
}

//------------------------------------------------------------------------------
void BackTracker::MatchMCParticle(std::vector<MCHit> const &mchits, int &prtId, int &prtPdg, double &eff, double &pur, double &totalCharge)
{
  // Loop over the hits and get all of their parents and the energy that each one contributed
  //  be sure to bunch up all neutronic contributions
  std::map<int, double> mapParticleToTotalClusterCharge;
  totalCharge = 0;

  for (auto mchit : mchits) {    
    std::vector<int> parentIdxs = *(mchit.GetParents());
    if (parentIdxs.size() != 1) {
      logmessage = "BackTracker::MatchMCParticle: this MCHit has ";
      logmessage += std::to_string(parentIdxs.size()) + " parents!";
      Log(logmessage, v_debug, verbosity);
      continue;
    }
    
    int particleId = -5;
    for (auto it : *fMCParticleIndexMap) {
      if (it.second == parentIdxs[0]) particleId = it.first;
    }
    if (particleId == -5) continue;
    
    double depositedCharge = mchit.GetCharge();
    totalCharge += depositedCharge;
    
    if (mapParticleToTotalClusterCharge.count(particleId) == 0) 
      mapParticleToTotalClusterCharge.emplace(particleId, depositedCharge);
    else
      mapParticleToTotalClusterCharge[particleId] += depositedCharge;    
  }       

  // Loop over the particleIds to find the primary contributer to the cluster
  double maxCharge = 0;
  for (auto apair : mapParticleToTotalClusterCharge) {
    if (apair.second > maxCharge) {
      maxCharge = apair.second;
      prtId = apair.first;
    }
  }

  // Check that we have some charge, if not then something is wrong so pass back all -5
  if (totalCharge > 0) {
    eff = maxCharge/fParticleToTankTotalCharge.at(prtId);
    pur = maxCharge/totalCharge;
    prtPdg = (fMCParticles->at(fMCParticleIndexMap->at(prtId))).GetPdgCode();
  } else {
    prtId = -5;
    eff = -5;
    pur = -5;
    totalCharge = -5;
  }

  logmessage = "BackTracker::MatchMCParticle: best particleId is : ";
  logmessage += std::to_string(prtId) + " which has PDG: " + std::to_string(prtPdg);
  Log(logmessage, v_message, verbosity);

}

void BackTracker::DirectParentsFromClockTickWindows()
{
  if (fPMTToDirectParentMap.empty() || fRecoADCHits.empty()) return;

  const double prewindow_ns = static_cast<double>(fPMTSimPrewindowTicks) * NS_PER_ADC_SAMPLE;
  const double readout_ns = static_cast<double>(fPMTSimReadoutWindowTicks) * NS_PER_ADC_SAMPLE;

  for (auto const& recoIt : fRecoADCHits) {
    unsigned long pmtID = recoIt.first;

    auto parentMapIt = fPMTToDirectParentMap.find(pmtID);
    if (parentMapIt == fPMTToDirectParentMap.end()) continue;

    std::map<uint16_t, std::vector<int>> const& hits_to_directparents_map = parentMapIt->second;

    for (std::vector<ADCPulse> const& minibufPulses : recoIt.second) {
      for (ADCPulse const& pulse : minibufPulses) {
        double pulseStart = pulse.start_time();
        double hitTime = pulse.peak_time();

        double tmin = pulseStart - prewindow_ns;
        double tmax = pulseStart + readout_ns;

        for (auto const& apair : hits_to_directparents_map) {
          double mchitTime = static_cast<double>(apair.first) * NS_PER_ADC_SAMPLE;

          if (mchitTime > tmin && mchitTime < tmax) {
            (*fMCHitToDirectParents)[pmtID][hitTime].insert(
              (*fMCHitToDirectParents)[pmtID][hitTime].end(),
              apair.second.begin(), apair.second.end());
          }
        }
      }
    }
  }
}

void BackTracker::FindNeutronAncestors() {

  std::map<int, std::pair<int, int>> trackMap; // trackId -> (DirectParentID, pdg)
  for (auto& particle : *fMCParticles) {
    int trackId = particle.GetParticleID();
    int directParentId = particle.GetDirectParentID();
    int pdg = particle.GetPdgCode();

    trackMap[trackId] = std::make_pair(directParentId, pdg);
  }

  for (const auto& pmtPair : *fMCHitToDirectParents) {
    unsigned long pmtID = pmtPair.first;
    for (const auto& hitPair : pmtPair.second) {
      double hitTime = hitPair.first;
      const std::vector<int>& directParents = hitPair.second;

      if (directParents.empty()) continue;

      // Dark-noise check: a pulse is pure dark noise iff every contributing
      // MCHit was pure noise. Noise photons carry direct parent -1 (WCSim
      // convention), so the pulse's directParents vector is all -1 only when
      // no real physics MCHit contributed within the pulse window. This is
      // equivalent to MCHit::GetIsDarknoise() set in LoadWCSim, but keyed on
      // the pulse peak_time which is what fMCHitToDirectParents uses.
      bool isDarknoise = std::all_of(directParents.begin(), directParents.end(),
                                     [](int id) { return id == -1; });
      // Temporary fix for older WCSim files (e.g. AmBe wcsim_0_999) where -1 is
      // stored as the neutron's actual track ID rather than a dark-noise sentinel.
      // If -1 exists in trackMap it is a real physics particle, not noise.
      if (isDarknoise && trackMap.find(-1) != trackMap.end())
        isDarknoise = false;
      (*fMCHitToIsDarknoise)[pmtID][hitTime] = isDarknoise;

      int neutronAncestorId = -5;
      int neutronAncestorPdg = -5;
      int neutronAncestorClass = -5;
      //Neutron Classification:
      // 0: dark noise (pure-noise pulse)
      // 1: primary neutron from initial interaction boundary
      // 2: secondary neutron from proton
      // 3: secondary neutron from neutron
      // 4: secondary neutron from other parent type
      int neutronParentTrackId = -5;
      int neutronParentPdg = -5;

      if (isDarknoise) {
        neutronAncestorClass = 0;
        (*fMCHitToNeutronAncestor)[pmtID][hitTime] = std::make_pair(neutronAncestorId, neutronAncestorPdg);
        (*fMCHitToNeutronAncestorClass)[pmtID][hitTime] = neutronAncestorClass;
        (*fMCHitToNeutronParent)[pmtID][hitTime] = std::make_pair(neutronParentTrackId, neutronParentPdg);
        (*fMCHitToInteractionMode)[pmtID][hitTime] = -999;
        (*fMCHitToImmediateAncestor)[pmtID][hitTime] = std::make_pair(-5, -5);
        (*fMCHitToImmediateAncestorClass)[pmtID][hitTime] = 0;
        // DISABLED: PrimaryAncestor
        // (*fMCHitToPrimaryAncestor)[pmtID][hitTime] = std::make_pair(-5, -5);
        (*fMCHitToRootAncestor)[pmtID][hitTime] = std::make_pair(-5, -5);
        (*fMCHitToLineage)[pmtID][hitTime] = std::vector<std::pair<int, int>>();
        (*fMCHitToLineageStatus)[pmtID][hitTime] = -5;
        continue;
      }

      int startParentID = directParents[0]; // take the first direct parent of neutron as the starting point
      int currentID = startParentID;

      
      std::set<int> visited; //Ensures that Ancestory finding doesn't get stuck in a loop if there are any circular references in the MCParticles

      while (trackMap.find(currentID) != trackMap.end()) {
        if (visited.count(currentID) > 0) {
          std::cerr << "WARNING: Circular reference detected at trackID " << currentID << std::endl;
          break;
        }
        visited.insert(currentID);

        int parentID = trackMap[currentID].first;
        int pdg = trackMap[currentID].second;

        if (pdg == 2112 && neutronAncestorId == -5) { // if it's a neutron and we haven't already found an ancestor, save it
          neutronAncestorId = currentID;  // This is the neutron's TRACK ID
          neutronAncestorPdg = pdg;       // Store the PDG code (2112 for neutron)
          neutronParentTrackId = parentID;
          neutronParentPdg = (trackMap.count(parentID) ? trackMap[parentID].second : -5);

          // Neutron classification based on parentage
          if (neutronParentTrackId == 0) {
            neutronAncestorClass = 1;      // primary neutron from initial interaction boundary
          } else if (neutronParentPdg == 2212) {
            neutronAncestorClass = 2;      // secondary neutron from proton
          } else if (neutronParentPdg == 2112) {
            neutronAncestorClass = 3;      // secondary neutron from neutron
          } else {
            neutronAncestorClass = 4;      // secondary neutron from other parent type
          }
          break; //We care about the immidiate neutron ancestor. 
                //Do we want to find the potential primary neutron ancestor, if there is one? (DJA) - If the classification portion works that this question is answered.
        }

        if (parentID == -1) break; // reached the end of the ancestry
        currentID = parentID;
      }

      (*fMCHitToNeutronAncestor)[pmtID][hitTime] = std::make_pair(neutronAncestorId, neutronAncestorPdg);
      (*fMCHitToNeutronAncestorClass)[pmtID][hitTime] = neutronAncestorClass;
      (*fMCHitToNeutronParent)[pmtID][hitTime] = std::make_pair(neutronParentTrackId, neutronParentPdg);

      // Immediate background particle: walks the same DirectParentID chain as the
      // neutron search above, but stops at the nearest ancestor that actually names a
      // species instead of at PDG 2112. Electrons/positrons are skipped over, because
      // in a water Cherenkov detector essentially every hit is mediated by an e-/e+ at
      // the last step (Cherenkov radiation, Compton scattering, pair production, the
      // photoelectric effect), so reporting "electron" carries no information.
      //
      // The skip is repeated, not single-step: an EM shower can put several e-/e+
      // generations in a row on the chain, and one step back would stop short of the
      // species that seeded the shower. If the whole recorded lineage is e-/e+ we
      // report the last one reached (class 7), and if nothing on the chain resolves at
      // all the hit stays untraced (-5).
      int immediateAncestorId = -5;
      int immediateAncestorPdg = -5;
      {
        std::set<int> skipVisited; // guards against circular DirectParentID references
        int walkId = startParentID;
        auto walkIt = trackMap.find(walkId);
        while (walkIt != trackMap.end() && skipVisited.count(walkId) == 0) {
          skipVisited.insert(walkId);
          int walkPdg = walkIt->second.second;
          immediateAncestorId = walkId;
          immediateAncestorPdg = walkPdg;
          if (walkPdg != 11 && walkPdg != -11) break; // found a species-naming ancestor
          walkId = walkIt->second.first;              // still a lepton: keep going up
          walkIt = trackMap.find(walkId);
        }
      }
      (*fMCHitToImmediateAncestor)[pmtID][hitTime] = std::make_pair(immediateAncestorId, immediateAncestorPdg);
      (*fMCHitToImmediateAncestorClass)[pmtID][hitTime] = ClassifyBackgroundPDG(immediateAncestorPdg);

      // ---------------------------------------------------------------------------------
      // DISABLED: PrimaryAncestor. Original PrimaryParentID-based lookup, kept verbatim for
      // future retrieval (it was never committed, so git cannot recover it). Superseded by
      // the DirectParentID walk below, which reaches the same top-of-tree particle using the
      // same mechanism as the neutron scheme.
      //
      // int primaryAncestorId = -5;
      // int primaryAncestorPdg = -5;
      // {
      //   auto startIt = fMCParticleIndexMap->find(startParentID);
      //   if (startIt != fMCParticleIndexMap->end()) {
      //     primaryAncestorId = fMCParticles->at(startIt->second).GetPrimaryParentID();
      //     auto primaryIt = fMCParticleIndexMap->find(primaryAncestorId);
      //     if (primaryIt != fMCParticleIndexMap->end())
      //       primaryAncestorPdg = fMCParticles->at(primaryIt->second).GetPdgCode();
      //   }
      // }
      // (*fMCHitToPrimaryAncestor)[pmtID][hitTime] = std::make_pair(primaryAncestorId, primaryAncestorPdg);
      // ---------------------------------------------------------------------------------

      // Full lineage of this optical photon, walking the SAME DirectParentID chain the
      // neutron search above uses -- no PrimaryParentID, so every branch here is derived
      // by one consistent mechanism. Ordered nearest-first: [0] is the hit's direct parent
      // (almost always an e-/e+), [1] its parent (e.g. a capture gamma), and so on up to
      // the generator primary. This is what lets a class-(-5) hit be read as a chain,
      // "e- <- gamma <- neutron <- proton", rather than as a single label.
      std::vector<std::pair<int, int>> lineage;
      int lineageStatus = 0;   // 0 = truncated at an unsaved track, until proven otherwise
      {
        std::set<int> lineageVisited;
        int walkId = startParentID;
        while (true) {
          if ((int)lineage.size() >= kMaxLineageDepth) { lineageStatus = -1; break; }
          if (lineageVisited.count(walkId)) {
            std::cerr << "WARNING: circular DirectParentID in lineage at trackID "
                      << walkId << std::endl;
            lineageStatus = -1;
            break;
          }
          auto walkIt = trackMap.find(walkId);
          if (walkIt == trackMap.end()) break;   // WCSim never saved this track
          lineageVisited.insert(walkId);
          lineage.push_back(std::make_pair(walkId, walkIt->second.second));
          int nextId = walkIt->second.first;
          if (nextId <= 0) { lineageStatus = 1; break; } // reached a generator primary
          walkId = nextId;
        }
      }
      (*fMCHitToLineage)[pmtID][hitTime] = lineage;
      (*fMCHitToLineageStatus)[pmtID][hitTime] = lineage.empty() ? 0 : lineageStatus;

      // Root of that chain: the generator-level particle the deposit descends from.
      // Trustworthy only when lineageStatus == 1.
      int rootAncestorId  = lineage.empty() ? -5 : lineage.back().first;
      int rootAncestorPdg = lineage.empty() ? -5 : lineage.back().second;
      (*fMCHitToRootAncestor)[pmtID][hitTime] = std::make_pair(rootAncestorId, rootAncestorPdg);

      int interactionMode = -9999;
      if (!directParents.empty() && !fWCSimInteractionModes.empty()) {
        auto particleIt = fMCParticleIndexMap->find(directParents[0]);
        if (particleIt != fMCParticleIndexMap->end()) {
          int trigNum = fMCParticles->at(particleIt->second).GetMCTriggerNum();
          if (trigNum >= 0 && trigNum < (int)fWCSimInteractionModes.size())
            interactionMode = fWCSimInteractionModes[trigNum];
        }
      }
      (*fMCHitToInteractionMode)[pmtID][hitTime] = interactionMode;

    }
  }
  
  std::cout << "BackTracker::FindNeutronAncestors: found " 
            << fMCHitToNeutronAncestor->size() 
            << " PMTs with neutron ancestors." << std::endl;
}

int BackTracker::ClassifyBackgroundPDG(int pdg) const {
  switch (pdg) {
    case 2112:  return 1;  // neutron
    case 13: case -13:   return 2; // muon
    case 211: case -211: return 3; // charged pion
    case 2212:  return 4;  // proton
    case 22:    return 5;  // photon
    case 321: case -321: case 311: case -311: return 6; // kaon
    case 11: case -11:   return 7; // electron/positron (skip landed on another lepton)
    case -5:    return -5; // untraced
    default:    return 8;  // other identified species
  }
}

bool BackTracker::LoadFromStores()
{
  // Grab the stuff we need from the stores
  bool goodMCClusters = m_data->CStore.Get("ClusterMapMC", fClusterMapMC);
  if (!goodMCClusters) {
    std::cerr<<"BackTracker: no ClusterMapMC in the CStore!"<<endl;
    return false;
  }

  bool goodAnnieEvent = m_data->Stores.count("ANNIEEvent");
  if (!goodAnnieEvent) {
    std::cerr<<"BackTracker: no ANNIEEvent store!"<<endl;
    return false;
  }
    
  bool goodMCHits = m_data->Stores.at("ANNIEEvent")->Get("MCHits", fMCHitsMap);
  if (!goodMCHits) {
    std::cerr<<"BackTracker: no MCHits in the ANNIEEvent!"<<endl;
    return false;
  }
  
  bool goodMCParticles = m_data->Stores.at("ANNIEEvent")->Get("MCParticles", fMCParticles);
  if (!goodMCParticles) {
    std::cerr<<"BackTracker: no MCParticles in the ANNIEEvent!"<<endl;
    return false;
  }

  bool goodMCParticleIndexMap = m_data->Stores.at("ANNIEEvent")->Get("TrackId_to_MCParticleIndex", fMCParticleIndexMap);
  if (!goodMCParticleIndexMap) {
    std::cerr<<"BackTracker: no TrackId_to_MCParticleIndex in the ANNIEEvent!"<<endl;
    return false;
  }

  if (fDirectParentClockTickMatching) {
    fPMTToDirectParentMap.clear();
    fRecoADCHits.clear();

    bool gotDirectParentMap = m_data->Stores.at("ANNIEEvent")->Get("PMTToDirectParentMap", fPMTToDirectParentMap);
    if (!gotDirectParentMap) {
      logmessage = "BackTracker: PMTToDirectParentMap missing, disabling pulse-window matching for this event.";
      Log(logmessage, v_warning, verbosity);
    }

    bool gotRecoADCHits = m_data->Stores.at("ANNIEEvent")->Get("RecoADCHits", fRecoADCHits);
    if (!gotRecoADCHits) {
      logmessage = "BackTracker: RecoADCHits missing, disabling pulse-window matching for this event.";
      Log(logmessage, v_warning, verbosity);
    }

    uint16_t prewindowTicks = fPMTSimPrewindowTicks;
    uint16_t readoutTicks = fPMTSimReadoutWindowTicks;
    if (m_data->Stores.at("ANNIEEvent")->Get("PMTSimPrewindowTicks", prewindowTicks)) {
      fPMTSimPrewindowTicks = prewindowTicks;
    }
    if (m_data->Stores.at("ANNIEEvent")->Get("PMTSimReadoutWindowTicks", readoutTicks)) {
      fPMTSimReadoutWindowTicks = readoutTicks;
    }
  }

  fWCSimInteractionModes.clear();
  bool gotModes = m_data->Stores.at("ANNIEEvent")->Get("WCSimInteractionModes", fWCSimInteractionModes);
  if (!gotModes) {
    logmessage = "BackTracker: WCSimInteractionModes not in ANNIEEvent; interaction mode will be -9999 for all hits.";
    Log(logmessage, v_warning, verbosity);
  }

  return true;
}
