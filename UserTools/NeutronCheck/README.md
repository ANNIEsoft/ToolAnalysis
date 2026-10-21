# NeutronCheck

NeutronCheck
Output tool for RecoCluster information and parameters.  Creates NeutCheckTree in the outfile.root, for use making analysis and comparison plots.

## Data


From ANNIEEvent store
**MCEventNum**
*simple event number

**ClusterToBestParticleID**
**ClusterToBestParticlePDG**
**ClusterEfficiency**
**ClusterPurity**
**ClusterTotalCharge**
**ClusterMapMC**
*ClusterFinder-made cluster maps from BackTracker tool for comparison

**MCParticles**
*vector of all MC Particles in the event

**TrueMuonEnergy**
*True energy of the muon for inclusion in the output

**TrackId_to_MCParticleIndex**	DEPRECATED
*map of track ID to MCParticle Index.  No longer used, as MCParticle Index is used directly in all relevant cases.



From RecoEvent store
**EventFlagged**
*Which event selection cuts were triggered by the current event.

**ClusterIndiccesNeutron**
**ClusterTimesNeutron**
**ClusterChargesNeutron**
**ClusterCBNeutron**
*ClusterFinder-made cluster maps of neutrons specifically for comparison

**ClusterTimes**
**ClusterCharges**
**ClusterCB**
*ClusterFinder-made cluster maps of all particles for comparison

**RecoClusters**
*vector list of all RecoClusters generated from all iterations of ClusterSearcher



From GenieInfo Store
**NeutrinoEnergy
*True neutrino energy for inclusion in the output

Output		.root file
**NeutCheckTree**
*Tree written into output file


## Configuration

Describe any configuration variables for NeutronCheck.

```
verbosity 1
outfile /path/to/output/file	#'.root' is added to the end.
UseClean 0	#whether to exclude events that do not pass event selection
FinderCompare 0 #whether to add similar plots from ClusterFinder map-clusters to output for comparison purposes (not all parameters are written into Finder clusters)
ParticleInfo 1	#whether to add true particle information directly from MCParticles list to output
DelayThreshold 10000 #time in ns that delayed window starts.  Defaults to 10000 if excluded.
```
