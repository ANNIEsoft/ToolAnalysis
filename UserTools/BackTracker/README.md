# BackTracker

`BackTracker` links reconstructed clusters (vectors of hits) to the MC particle that contributed the most energy to said cluster. Right now, it takes in a cluster of form std::map<double, std::vector<MCHit>>, specifically it uses the CStore cluster produced by the ClusterFinder tool, which has the label "ClusterMapMC". That map is indexed by the cluster time and that index is in turn used to ID all of the data products that BackTracker produces. 

## Data

`BackTracker` produces a number of maps to link MC information to the associated cluster and puts them into the ANNIEEvent store (all map keys are the cluster time):

**fClusterToBestParticleID**  `std::map<double, int>`
* The MCParticle ID (ie. the ParticleID from the MCParticle class, which is returned via the `MCParticle::GetParticleID()` function) of the best matched particle

**fClusterToBestParticlePDG**  `std::map<double, int>`
* The PDG value of the best matched particle

**fClusterEfficiency** `std::map<double, double>` 
* Efficiency of the clustering where the numerator is the number total charge in the cluster contributed by the best matched particle, and the denominator is the total charge that the best matched particle deposited anywhere in the tank

**fClusterPurity** `std::map<double, double>` 
* Purity of the clustering where the numerator is the number total charge in the cluster contributed by the best matched particle, and the denominator is the total charge of all hits in the cluster

**fClusterCharge** `std::map<double, double>` 
* Total deposited charge of all hits in the cluster

With `UseDirectParentClockTickMatching 1`, `BackTracker` also stores per-hit truth for the reconstructed PMT pulses of simulated waveforms. It needs `PMTWaveformSim` and `PhaseIIADCHitFinder` earlier in the toolchain: each pulse in `RecoADCHits` is matched to the simulated MC hits on the same PMT (`PMTToDirectParentMap` from `PMTWaveformSim`) whose time lies between `Prewindow` ticks before and `ReadoutWindow` ticks after the pulse start. These maps are keyed by PMT ID, then by the pulse peak time:

**MCHitToDirectParents** `std::map<unsigned long, std::map<double, std::vector<int>>>`
* Geant4 track IDs of the particles that emitted the matched photons (-1 dark noise)

**MCHitToIsDarknoise** `std::map<unsigned long, std::map<double, bool>>`
* True if all matched photons are dark noise

**MCHitToNeutronAncestor**, **MCHitToNeutronParent** `std::map<unsigned long, std::map<double, std::pair<int, int>>>`
* (track ID, PDG) of the nearest neutron in the ancestry, and of that neutron's parent

**MCHitToNeutronAncestorClass** `std::map<unsigned long, std::map<double, int>>`
* 0 dark noise, 1 primary neutron, 2/3/4 secondary neutron from a proton/neutron/other, -5 no neutron in the ancestry

**MCHitToImmediateAncestor**, **MCHitToImmediateAncestorClass**
* (track ID, PDG) of the first ancestor that is not an e-/e+, and its class (see `BackTracker.h`)

**MCHitToRootAncestor** `std::map<unsigned long, std::map<double, std::pair<int, int>>>`
* (track ID, PDG) at the top of the ancestry (the generator-level particle)

**MCHitToLineage** `std::map<unsigned long, std::map<double, std::vector<std::pair<int, int>>>>`, **MCHitToLineageStatus** `std::map<unsigned long, std::map<double, int>>`
* The full ancestry, nearest first, as (track ID, PDG); status 1 complete, 0 stopped at a track WCSim did not save, -1 loop or depth cap, -5 dark noise

The ancestry is complete only for WCSim files that save every track behind a PMT hit (ANNIEsoft/WCSim#27).


## Configuration
```
verbosity 1 # Verbosity level of the tool
UseDirectParentClockTickMatching 0 # 1: per-hit truth for waveform-simulated pulses (needs PMTWaveformSim and PhaseIIADCHitFinder); default 0
```
