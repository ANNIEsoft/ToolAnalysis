# BeamClusterAnalysisMC

***********************
## Description
**********************

The `BeamClusterAnalysisMC` toolchain produces a ntuple ROOT-tree from simulated events, e.g. simulated with the annie version of `WCSim`. This ntuple is created via the `PhaseIITreeMaker` tool, which generates separate trees to store the information about the trigger data, tank PMT clusters, and MRD clusters. It can be used in combination with the `EventSelector` tool to only save events to the ntuple file if they meet certain criteria.


************************
## Tools
************************

The toolchain typically consists of the following tools:

```
LoadGeometry
LoadWCSim
LoadWCSimLAPPD
LoadGenieEvent
MCParticleProperties
MCRecoEventLoader
TimeClustering
FindMrdTracks
ClusterFinder
ClusterClassifiers
EventSelector
PhaseIITreeMaker
```

