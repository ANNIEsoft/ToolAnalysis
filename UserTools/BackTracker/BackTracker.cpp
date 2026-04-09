#include "BackTracker.h"
#include "ANNIEconstants.h"

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
  fMCHitToDirectParents       = new std::map<unsigned long, std::map<double, std::vector<int>>>;
  fMCHitToNeutronAncestor     = new std::map<unsigned long, std::map<double, std::pair<int, int>>>;
  
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
  //  fMCHitToNeutronAncestor  ->clear();

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

  std::map<int, std::pair<int, int>> trackMap; // trackId -> (ParentID, pdg)
  for (auto& particle : *fMCParticles) {
    int trackId = particle.GetParticleID();
    int parentId = particle.GetDirectParentID();
    int pdg = particle.GetPdgCode();

    trackMap[trackId] = std::make_pair(parentId, pdg);
  }

  for (const auto& pmtPair : *fMCHitToDirectParents) {
    unsigned long pmtID = pmtPair.first;
    for (const auto& hitPair : pmtPair.second) {
      double hitTime = hitPair.first;
      const std::vector<int>& directParents = hitPair.second;

      if (directParents.empty()) continue;

      int neutronAncestorId = -5;
      int neutronAncestorPdg = -5;
      int startParentID = directParents[0]; // take the first direct parent as the starting point
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
          break; //We care about the immidiate neutron ancestor. 
                //Do we want to find the potential primary neutron ancestor, if there is one? (DJA)
        }

        if (parentID == -1) break; // reached the end of the ancestry
        currentID = parentID;
      }

      (*fMCHitToNeutronAncestor)[pmtID][hitTime] = std::make_pair(neutronAncestorId, neutronAncestorPdg);

    }
  }
  
  std::cout << "BackTracker::FindNeutronAncestors: found " 
            << fMCHitToNeutronAncestor->size() 
            << " PMTs with neutron ancestors." << std::endl;
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

  return true;
}
