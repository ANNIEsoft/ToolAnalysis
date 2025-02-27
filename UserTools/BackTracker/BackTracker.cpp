#include "BackTracker.h"

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

  bool gotDebugPlots = m_variables.Get("DebugPlots", fDebugPlots);
  if (!gotDebugPlots) {
    fDebugPlots = false;
    logmessage = "BackTracker::Initialize: \"DebugPlots\" not set in the config, defaulting to false";
    Log(logmessage, v_error, verbosity);
  }
  if (fDebugPlots)
    SetupDebug();

  bool gotMCWaveforms = m_variables.Get("MCWaveforms", fMCWaveforms);
  if (!gotMCWaveforms) {
    fMCWaveforms = false;
    logmessage = "BackTracker::Initialize: \"MCWaveforms\" not set in the config, defaulting to false";
    Log(logmessage, v_error, verbosity);
  }


  // Set up the pointers we're going to save. No need to 
  // delete them at Finalize, the store will handle it
  fClusterToBestParticleIdx = new std::map<double, int>;
  fClusterToBestParticlePDG = new std::map<double, int>;
  fClusterEfficiency        = new std::map<double, double>;
  fClusterPurity            = new std::map<double, double>;
  fClusterTotalCharge       = new std::map<double, double>;
  
  return true;
}

//------------------------------------------------------------------------------
bool BackTracker::Execute()
{
  int load_status = LoadFromStores();
  if (load_status == 0) return false;
  if (load_status == 2) return true;
  
  
  fClusterToBestParticleIdx->clear();
  fClusterToBestParticlePDG->clear();
  fClusterEfficiency       ->clear();
  fClusterPurity           ->clear();
  fClusterTotalCharge      ->clear();
  fClusterEarliestMCTime     ->clear();
  fClusterMeanMCTime         ->clear();
  fClusterMedianMCTime       ->clear();

  fParticleToTankTotalCharge.clear();
  
  SumParticleTankCharge();

  if (fMCWaveforms) { // using clusters of Hits made from simulated PMT pulses

    // Produce the map from channel ID to pulse time to MCHit index
    //std::cout << "BT: Calling MapPulsesToParentIdxs" << std::endl;
    bool gotPulseMap = MapPulsesToParentIdxs();
    if (!gotPulseMap) {
      logmessage = "BackTracker: No good pulse map.";
      Log(logmessage, v_error, verbosity);
      return false;
    }
    
    // Loop over the clusters
    for (std::pair<double, std::vector<Hit>>&& apair : *fClusterMap) {
      // Create a vector of MCHits associated with the vector of Hits
      std::vector<MCHit> mcHits;
      for (auto hit : apair.second) {
	int channel_key = hit.GetTubeId();
	double hitTime = hit.GetTime();

	// Catches if something goes wrong
	// First make sure that we have the PMT in the outer map
	// Then make sure we have the hit time in the inner map
	if (fMapChannelToPulseTimeToMCHitIdx.find(channel_key) == fMapChannelToPulseTimeToMCHitIdx.end()) {
	  std::cout << "BackTracker: No hit on this PMT: " << channel_key << std::endl;
	  return false;
	}
	if (fMapChannelToPulseTimeToMCHitIdx.at(channel_key).find(hitTime) == fMapChannelToPulseTimeToMCHitIdx.at(channel_key).end()) {
	  std::cout << "BackTracker: No hit on this PMT: " << channel_key << " at this time: " << hitTime << std::endl;
	  return false;
	}

	// Extract all the MCHits that are in time with this cluster hit
	std::vector<int> mcHitIdxVec = fMapChannelToPulseTimeToMCHitIdx[channel_key][hitTime];
	for (auto mcHitIdx : mcHitIdxVec) 
	  mcHits.push_back((fMCHitsMap->at(channel_key)).at(mcHitIdx));
      }// end loop over cluster hits

      int prtIdx = -5;
      int prtPdg = 0;
      double eff = -5;
      double pur = -5;
      double totalCharge = 0;

      MatchMCParticle(mcHits, prtIdx, prtPdg, eff, pur, totalCharge);

      // Go through the mcHits and determine the earliest, mean, and median hit times
      double earliestTime = 99999;
      double meanTime = 0;
      double medianTime = -5;
      for (auto mcHit : mcHits) {
	double tempTime = mcHit.GetTime();
	if (tempTime < earliestTime) earliestTime = tempTime;
	meanTime += tempTime;       
      }// end loop over MCHits
      if (mcHits.size()) {
	meanTime = meanTime / mcHits.size();
	  
	if (mcHits.size() %2 != 0)
	  medianTime = mcHits.at(mcHits.size()/2).GetTime();
	else {
	  double time1 = mcHits.at((mcHits.size()-1)/2).GetTime();
	  double time2 = mcHits.at((mcHits.size())/2).GetTime();
	  medianTime = (time1 + time2) / 2;
	}
      }// end mean/median calculation

      fClusterToBestParticleIdx->emplace(apair.first, prtIdx);
      fClusterToBestParticlePDG->emplace(apair.first, prtPdg);
      fClusterEfficiency       ->emplace(apair.first, eff);
      fClusterPurity           ->emplace(apair.first, pur);
      fClusterTotalCharge      ->emplace(apair.first, totalCharge);
      fClusterEarliestMCTime   ->emplace(apair.first, earliestTime);
      fClusterMeanMCTime       ->emplace(apair.first, meanTime);
      fClusterMedianMCTime     ->emplace(apair.first, medianTime);
      
      m_data->Stores.at("ANNIEEvent")->Set("MapChannelToPulseTimeToMCHitIdx",  fMapChannelToPulseTimeToMCHitIdx );


      if (fDebugPlots) {
	// Basic cluster values
	fDbgClusterTime = apair.first;
	fDbgClusterNHits = apair.second.size();
	fDbgClusterNMCHits = mcHits.size();
	fDbgClusterBestParticleIdx = prtIdx;
	fDbgClusterBestParticlePDG = prtPdg;
	fDbgClusterBestParticleCharge = fParticleToTankTotalCharge.at(prtIdx);
	fDbgClusterBestParticleStartEnergy = fMCParticles->at(prtIdx).GetStartEnergy();
	fDbgClusterBestParticleStopEnergy = fMCParticles->at(prtIdx).GetStopEnergy();
	fDbgClusterBestParticleStartTime = fMCParticles->at(prtIdx).GetStartTime();
	fDbgClusterBestParticleStopTime = fMCParticles->at(prtIdx).GetStopTime();	    
	fDbgClusterEfficiency = eff;
	fDbgClusterPurity = pur;
	fDbgClusterTotalCharge = totalCharge;

	// Vectors for individual cluster hits
	fDbgClusterHitChannel.clear();
	fDbgClusterHitCharge.clear();
	fDbgClusterHitTime.clear();
	for (auto hit : apair.second) {
	  fDbgClusterHitChannel.push_back(hit.GetTubeId());
	  fDbgClusterHitCharge.push_back(hit.GetCharge());
	  fDbgClusterHitTime.push_back(hit.GetTime());
	}

	// Vectors for individual matched MCHits and their parent particles
	fDbgClusterMCHitChannel.clear();
	fDbgClusterMCHitCharge.clear();
	fDbgClusterMCHitTime.clear();
	fDbgClusterParticleIdx.clear();
	fDbgClusterParticleCharge.clear();
	fDbgClusterParticlePdgCode.clear();
	fDbgClusterParticleStartEnergy.clear();
	fDbgClusterParticleStopEnergy.clear();
	fDbgClusterParticleStartTime.clear();
	fDbgClusterParticleStopTime.clear();
	fDbgClusterMCHitParticleTimeDiff.clear();
	std::vector<int> seenParentIdxs;
	for (auto hit : mcHits) {
	  fDbgClusterMCHitChannel.push_back(hit.GetTubeId());
	  fDbgClusterMCHitCharge.push_back(hit.GetCharge());
	  fDbgClusterMCHitTime.push_back(hit.GetTime());

	  // Vectors for all matched particles
	  std::vector<int> parentIdxs = *(hit.GetParents());
	  if (!parentIdxs.size()) continue;
	  for (int parentIdx : parentIdxs) {
	    if (std::find(seenParentIdxs.begin(), seenParentIdxs.end(), parentIdx) != seenParentIdxs.end())
	      continue; // we've recorded info on this particle already

	    fDbgClusterParticleIdx.push_back(parentIdx);
	    fDbgClusterParticleCharge.push_back(fParticleToTankTotalCharge.at(parentIdx));
	    fDbgClusterParticlePdgCode.push_back(fMCParticles->at(parentIdx).GetPdgCode());
	    fDbgClusterParticleStartEnergy.push_back(fMCParticles->at(parentIdx).GetStartEnergy());
	    fDbgClusterParticleStopEnergy.push_back(fMCParticles->at(parentIdx).GetStopEnergy());
	    fDbgClusterParticleStartTime.push_back(fMCParticles->at(parentIdx).GetStartTime());
	    fDbgClusterParticleStopTime.push_back(fMCParticles->at(parentIdx).GetStopTime());

	    fDbgClusterMCHitParticleTimeDiff.push_back(hit.GetTime() - fMCParticles->at(parentIdx).GetStopTime());
	  }// end loop over parent particles
	}// end loop over matched MCHits

	
	fDbgClusterTree->Fill();
      }// endif debug plots
      
    }// end loop over the cluster map
  } else { // using clusters of MCHits
    // Loop over the MC clusters and do the things
    for (std::pair<double, std::vector<MCHit>>&& apair : *fClusterMapMC) {
      int prtIdx = -5;
      int prtPdg = -5;
      double eff = -5;
      double pur = -5;
      double totalCharge = 0;

      MatchMCParticle(apair.second, prtIdx, prtPdg, eff, pur, totalCharge);

      fClusterToBestParticleIdx->emplace(apair.first, prtIdx);
      fClusterToBestParticlePDG->emplace(apair.first, prtPdg);
      fClusterEfficiency       ->emplace(apair.first, eff);
      fClusterPurity           ->emplace(apair.first, pur);
      fClusterTotalCharge      ->emplace(apair.first, totalCharge);

      if (fDebugPlots) {
	// Basic cluster values
	fDbgClusterTime = apair.first;
	fDbgClusterNMCHits = apair.second.size();
	fDbgClusterBestParticleIdx = prtIdx;
	fDbgClusterBestParticlePDG = prtPdg;
	fDbgClusterBestParticleCharge = fParticleToTankTotalCharge.at(prtIdx);
	fDbgClusterBestParticleStartEnergy = fMCParticles->at(prtIdx).GetStartEnergy();
	fDbgClusterBestParticleStopEnergy = fMCParticles->at(prtIdx).GetStopEnergy();
	fDbgClusterBestParticleStartTime = fMCParticles->at(prtIdx).GetStartTime();
	fDbgClusterBestParticleStopTime = fMCParticles->at(prtIdx).GetStopTime();	    
	fDbgClusterEfficiency = eff;
	fDbgClusterPurity = pur;
	fDbgClusterTotalCharge = totalCharge;

	// Vectors for individual matched MCHits and their parent particles
	fDbgClusterMCHitChannel.clear();
	fDbgClusterMCHitCharge.clear();
	fDbgClusterMCHitTime.clear();
	fDbgClusterParticleIdx.clear();
	fDbgClusterParticleCharge.clear();
	fDbgClusterParticlePdgCode.clear();
	fDbgClusterParticleStartEnergy.clear();
	fDbgClusterParticleStopEnergy.clear();
	fDbgClusterParticleStartTime.clear();
	fDbgClusterParticleStopTime.clear();
	fDbgClusterMCHitParticleTimeDiff.clear();
	std::vector<int> seenParentIdxs;
	for (auto hit : apair.second) {
	  fDbgClusterMCHitChannel.push_back(hit.GetTubeId());
	  fDbgClusterMCHitCharge.push_back(hit.GetCharge());
	  fDbgClusterMCHitTime.push_back(hit.GetTime());

	  // Vectors for all matched particles
	  std::vector<int> parentIdxs = *(hit.GetParents());
	  if (!parentIdxs.size()) continue;
	  for (int parentIdx : parentIdxs) {
	    if (std::find(seenParentIdxs.begin(), seenParentIdxs.end(), parentIdx) != seenParentIdxs.end())
	      continue; // we've recorded info on this particle already

	    fDbgClusterParticleIdx.push_back(parentIdx);
	    fDbgClusterParticleCharge.push_back(fParticleToTankTotalCharge.at(parentIdx));
	    fDbgClusterParticlePdgCode.push_back(fMCParticles->at(parentIdx).GetPdgCode());
	    fDbgClusterParticleStartEnergy.push_back(fMCParticles->at(parentIdx).GetStartEnergy());
	    fDbgClusterParticleStopEnergy.push_back(fMCParticles->at(parentIdx).GetStopEnergy());
	    fDbgClusterParticleStartTime.push_back(fMCParticles->at(parentIdx).GetStartTime());
	    fDbgClusterParticleStopTime.push_back(fMCParticles->at(parentIdx).GetStopTime());

	    fDbgClusterMCHitParticleTimeDiff.push_back(hit.GetTime() - fMCParticles->at(parentIdx).GetStopTime());
	  }// end loop over parent particles
	}// end loop over matched MCHits

	
	fDbgClusterTree->Fill();
      }// endif debug plots

    }// end loop over the MC cluster map
  }// end if/else fMCWaveforms

  m_data->Stores.at("ANNIEEvent")->Set("ClusterToBestParticleIdx", fClusterToBestParticleIdx);
  m_data->Stores.at("ANNIEEvent")->Set("ClusterToBestParticlePDG", fClusterToBestParticlePDG);
  m_data->Stores.at("ANNIEEvent")->Set("ClusterEfficiency",        fClusterEfficiency       );
  m_data->Stores.at("ANNIEEvent")->Set("ClusterPurity",            fClusterPurity           );
  m_data->Stores.at("ANNIEEvent")->Set("ClusterTotalCharge",       fClusterTotalCharge      );
  m_data->Stores.at("ANNIEEvent")->Set("ClusterEarliestMCTime",    fClusterEarliestMCTime   );
  m_data->Stores.at("ANNIEEvent")->Set("ClusterMeanMCTime",        fClusterMeanMCTime       );
  m_data->Stores.at("ANNIEEvent")->Set("ClusterMedianMCTime",      fClusterMedianMCTime     );

  return true;
}

//------------------------------------------------------------------------------
bool BackTracker::Finalise()
{

  if (fDebugPlots) {
    fDebugFile->cd();
    fDbgClusterTree->Write();
  }
  
  return true;
}

//------------------------------------------------------------------------------
void BackTracker::SumParticleTankCharge()
{
  for (auto apair : *fMCHitsMap) {
    std::vector<MCHit> mcHits = apair.second;
    for (uint mcHitIdx = 0; mcHitIdx < mcHits.size(); ++mcHitIdx) {


      // technically a MCHit could have multiple parents, but they don't appear to in practice
      // however, if they do then split the energy equally amongg them
      std::vector<int> parentIdxs = *(mcHits[mcHitIdx].GetParents());
      if (!parentIdxs.size()) continue; // no parents recorded?

      double depositedCharge = mcHits[mcHitIdx].GetCharge() / parentIdxs.size();
      for (int parentIdx : parentIdxs) {
	if (!fParticleToTankTotalCharge.count(parentIdx)) 
	fParticleToTankTotalCharge.emplace(parentIdx, depositedCharge);
      else 
	fParticleToTankTotalCharge.at(parentIdx) += depositedCharge;
      }// end loop over parent indexes      
    }// end loop over MCHits    
  }// end loop over PMTs
}

//------------------------------------------------------------------------------
void BackTracker::MatchMCParticle(std::vector<MCHit> const &mchits, int &prtIdx, int &prtPdg, double &eff, double &pur, double &totalCharge)
{
  // Loop over the hits and get all of their parents and the energy that each one contributed
  std::map<int, double> mapParticleToTotalClusterCharge;
  totalCharge = 0;

  for (auto mchit : mchits) {    
    std::vector<int> parentIdxs = *(mchit.GetParents());
    if (!parentIdxs.size()) continue; 

    double hitCharge = mchit.GetCharge();
    totalCharge += hitCharge;
    hitCharge  = hitCharge / parentIdxs.size();
    
    for (int parentIdx : parentIdxs) {
      if (mapParticleToTotalClusterCharge.count(parentIdx) == 0) 
	mapParticleToTotalClusterCharge.emplace(parentIdx, hitCharge);
      else
	mapParticleToTotalClusterCharge[parentIdx] += hitCharge;
    }// end loop over parentIdxs
  }// end loop over MCHits    

  // Loop over the particleIds to find the primary contributer to the cluster
  double maxCharge = 0;
  for (auto apair : mapParticleToTotalClusterCharge) {
    if (apair.second > maxCharge) {
      maxCharge = apair.second;
      prtIdx = apair.first;
    }
  }// end loop over the particle charge map

  // Check that we have some charge, if not then something is wrong so pass back all -5
  if (totalCharge > 0) {
    eff = maxCharge/fParticleToTankTotalCharge.at(prtIdx);
    pur = maxCharge/totalCharge;
    prtPdg = (fMCParticles->at(prtIdx)).GetPdgCode();
  } else {
    prtIdx = -5;
    eff = -5;
    pur = -5;
    totalCharge = -5;
  }

  logmessage = "BackTracker::MatchMCParticle: best particleId is : ";
  logmessage += std::to_string(prtIdx) + " which has PDG: " + std::to_string(prtPdg);
  Log(logmessage, v_message, verbosity);
}

//------------------------------------------------------------------------------
bool BackTracker::MapPulsesToParentIdxs()
{
  // Clear out the map
  fMapChannelToPulseTimeToMCHitIdx.clear();

  // Grab the pulses
  std::map<unsigned long, std::vector< std::vector<ADCPulse>> > adcPulseMap;
  bool goodADCPulses = m_data->Stores.at("ANNIEEvent")->Get("RecoADCHits", adcPulseMap);
  if (!goodADCPulses) {
    logmessage = "BackTracker: no RecoADCHits in the ANNIEEvent!";
    Log(logmessage, v_error, verbosity);
    return false;
  }

  // Loop over the ADCPulses and find MCHits that fall within the start and stop times
  // also record the pulse time to match the MCHits to the reco Hits
  // Pulses are indexed by PMT id then stacked in a two-deep vector

  for (auto apair: adcPulseMap) {
    int channel_key = apair.first;
      
    std::map<double, std::vector<int>> mapHitTimeToParents;
    bool goodPulses = false;
    for (auto pulseVec : apair.second) {
      for (auto pulse : pulseVec) {
	goodPulses = true;
	double pulseTime = pulse.peak_time();

	// Record the hit index if it occurred within the pulse window
	// If there is only one hit the no need to check the times
	std::vector<MCHit> mcHits = fMCHitsMap->at(channel_key);
	if (mcHits.size() == 1)
	  mapHitTimeToParents[pulseTime].push_back(0);
	else {
	  for (uint mcHitIdx = 0; mcHitIdx < mcHits.size(); ++mcHitIdx) {
	    double hitTime = mcHits[mcHitIdx].GetTime();

	    // The hit finding has to contend with noise so allow for a 10 ns 
	    // slew in the pulse start time (I know it seems large)
	    if ( hitTime + 10 >= pulse.start_time() && hitTime <= pulse.stop_time())
	      mapHitTimeToParents[pulseTime].push_back(mcHitIdx);
	  }// end loop over MCHits
	}
      }// end loop over inner pulse vector
    }// end loop over outer pulse vector

    if (mapHitTimeToParents.size() == 0 && goodPulses) {
      logmessage = "BackTracker::MapPulsesToParentIdxs: No MCHits match with this pulse! PMT channel: ";
      logmessage += std::to_string(channel_key);
      Log(logmessage, v_error, verbosity);
      return false;
    }

    fMapChannelToPulseTimeToMCHitIdx.emplace(channel_key, std::move(mapHitTimeToParents));
  }// end loop over pulse map

  return true;
}

//------------------------------------------------------------------------------
void BackTracker::SetupDebug()
{
  fDebugFile = new TFile("BackTracker_Debug.root", "RECREATE");
  fDbgClusterTree = new TTree("clusterTree", "clusterTree");

  fDbgClusterTree->Branch("Time", &fDbgClusterTime);
  fDbgClusterTree->Branch("NHits", &fDbgClusterNHits);
  fDbgClusterTree->Branch("NMCHits", &fDbgClusterNMCHits);
  fDbgClusterTree->Branch("BestParticleIdx", &fDbgClusterBestParticleIdx);
  fDbgClusterTree->Branch("BestParticlePDG", &fDbgClusterBestParticlePDG);
  fDbgClusterTree->Branch("BestParticleCharge", &fDbgClusterBestParticleCharge);
  fDbgClusterTree->Branch("BestParticleStartEnergy", &fDbgClusterBestParticleStartEnergy);
  fDbgClusterTree->Branch("BestParticleStopEnergy", &fDbgClusterBestParticleStopEnergy);
  fDbgClusterTree->Branch("BestParticleStartTime", &fDbgClusterBestParticleStartTime);
  fDbgClusterTree->Branch("BestParticleStopTime", &fDbgClusterBestParticleStopTime);
  fDbgClusterTree->Branch("Efficiency", &fDbgClusterEfficiency);
  fDbgClusterTree->Branch("Purity", &fDbgClusterPurity);
  fDbgClusterTree->Branch("TotalCharge", &fDbgClusterTotalCharge);
  fDbgClusterTree->Branch("HitChannel", &fDbgClusterHitChannel);
  fDbgClusterTree->Branch("HitCharge", &fDbgClusterHitCharge);
  fDbgClusterTree->Branch("HitTime", &fDbgClusterHitTime);
  fDbgClusterTree->Branch("MCHitChannel", &fDbgClusterMCHitChannel);
  fDbgClusterTree->Branch("MCHitCharge", &fDbgClusterMCHitCharge);
  fDbgClusterTree->Branch("MCHitTime", &fDbgClusterMCHitTime);
  fDbgClusterTree->Branch("ParticleIdx", &fDbgClusterParticleIdx);
  fDbgClusterTree->Branch("ParticleCharge", &fDbgClusterParticleCharge);
  fDbgClusterTree->Branch("ParticlePDG", &fDbgClusterParticlePdgCode);
  fDbgClusterTree->Branch("ParticleStartEnergy", &fDbgClusterParticleStartEnergy);
  fDbgClusterTree->Branch("ParticleStopEnergy", &fDbgClusterParticleStopEnergy);
  fDbgClusterTree->Branch("ParticleStartTime", &fDbgClusterParticleStartTime);
  fDbgClusterTree->Branch("ParticleStopTime", &fDbgClusterParticleStopTime);
  fDbgClusterTree->Branch("MCHitParticleTimeDiff", &fDbgClusterMCHitParticleTimeDiff);
  
}


//------------------------------------------------------------------------------
int BackTracker::LoadFromStores()
{
  // Grab the stuff we need from the stores

  bool goodAnnieEvent = m_data->Stores.count("ANNIEEvent");
  if (!goodAnnieEvent) {
    logmessage = "BackTracker: no ANNIEEvent store!";
    Log(logmessage, v_error, verbosity);
    return 0;
  }

  bool skip = false;
  bool goodSkipStatus = m_data->Stores.at("ANNIEEvent")->Get("SkipExecute", skip);
  if (goodSkipStatus && skip) {
    logmessage = "BackTracker: An upstream tool told me to skip this event.";
    Log(logmessage, v_warning, verbosity);
    return 2;
  }

  bool goodMCHits = m_data->Stores.at("ANNIEEvent")->Get("MCHits", fMCHitsMap);
  if (!goodMCHits) {
    logmessage = "BackTracker: no MCHits in the ANNIEEvent!";
    Log(logmessage, v_error, verbosity);
    return 0;
  }

  if (fMCWaveforms) {
    bool goodClusters = m_data->CStore.Get("ClusterMap", fClusterMap);
    if (!goodClusters) {
      logmessage = "BackTracker: no ClusterMap in the CStore!";
      Log(logmessage, v_error, verbosity);
      return 0;
    }

    bool goodRecoHits = m_data->Stores.at("ANNIEEvent")->Get("Hits", fRecoHitsMap);
    if (!goodRecoHits) {
      logmessage = "BackTracker: no Hits in the ANNIEEvent!";
      Log(logmessage, v_error, verbosity);
      return 0;
    }
    
  } else {
    bool goodMCClusters = m_data->CStore.Get("ClusterMapMC", fClusterMapMC);
    if (!goodMCClusters) {
      logmessage = "BackTracker: no ClusterMapMC in the CStore!";
      Log(logmessage, v_error, verbosity);
      return 0;
    }
  }// end if/else fMCWaveforms
  
  bool goodMCParticles = m_data->Stores.at("ANNIEEvent")->Get("MCParticles", fMCParticles);
  if (!goodMCParticles) {
    logmessage = "BackTracker: no MCParticles in the ANNIEEvent!";
    Log(logmessage, v_error, verbosity);
    return 0;
  }

  bool goodMCParticleIndexMap = m_data->Stores.at("ANNIEEvent")->Get("TrackId_to_MCParticleIndex", fMCParticleIndexMap);
  if (!goodMCParticleIndexMap) {
    logmessage = "BackTracker: no TrackId_to_MCParticleIndex in the ANNIEEvent!";
    Log(logmessage, v_error, verbosity);
    return 0;
  }

  return 1;
}

