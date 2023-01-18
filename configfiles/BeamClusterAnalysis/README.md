# BeamClusterAnalysis

***********************
## Description
**********************

The `BeamClusterAnalysis` toolchain produces a ntuple ROOT-tree from data events, i.e. processed data files from the ANNIE DAQ. This ntuple is created via the `PhaseIITreeMaker` tool, which generates separate trees to store the information about the trigger data, tank PMT clusters, and MRD clusters. It can be used in combination with the `EventSelector` tool to only save events to the ntuple file if they meet certain criteria.


************************
## Tools
************************

The toolchain typically consists of the following tools:

```
LoadANNIEEvent
LoadGeometry
#PhaseIIADCCalibrator
#PhaseIIADCHitFinder
TimeClustering
FindMrdTracks
ClusterFinder
ClusterClassifiers
EventSelector
PhaseIITreeMaker
```
