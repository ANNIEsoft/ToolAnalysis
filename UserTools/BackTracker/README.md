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


## Configuration
```
verbosity 1 # Verbosity level of the tool
```
