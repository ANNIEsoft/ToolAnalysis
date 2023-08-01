# FilterEvents toolchain

***********************
# Description
**********************

The `FilterEvents` toolchain uses cuts from the `EventSelector` tool to filter out events passing the selected cuts. The filtered events are then saved to a new dedicated output BoostStore that only contains the filtered events. The output BoostStore file will contain all the variables from the `ANNIEEvent` BoostStore and can be used in a toolchain like a regular processed data file.

Filters that define the specific event selection cuts should be defined in the `configfiles/Filters/` directory. Currently the following filters are defined:

* NeutrinoCandidate
* DirtCandidate
* Throughgoing
* BeamTrigger
* CosmicTrigger

Each filter is defined by a unique name, which is passed on to the `FilterEvents` tool via the common `CStore`. The variable is then used as a part of the output BoostStore file. This guarantees that the output BoostStore will contain the name of the official Filter and it is easily recognizable which filter was used for a specific file.

************************
# ToolsConfig
************************

The following tools are in the `FilterEvents` toolchain:

```
myLoadANNIEEvent LoadANNIEEvent ./configfiles/FilterEvents/LoadANNIEEventConfig
myLoadGeometry LoadGeometry ./configfiles/LoadGeometry/LoadGeometryConfig
myTimeClustering TimeClustering configfiles/FilterEvents/TimeClusteringConfig
myFindMrdTracks FindMrdTracks configfiles/FilterEvents/FindMrdTracksConfig
myClusterFinder ClusterFinder ./configfiles/FilterEvents/ClusterFinderConfig
myClusterClassifiers ClusterClassifiers ./configfiles/FilterEvents/ClusterClassifiersConfig
myEventSelector EventSelector ./configfiles/Filters/NeutrinoCandidateFilter
myFilterEvents FilterEvents ./configfiles/FilterEvents/FilterEventsConfig
```

The `EventSelector` row can be adjusted if a different filter should be used.

New filters can be defined in the `configfiles/Filters` directory. However, old filters should not be changed in any way to preserve the uniqueness of filters. If a filter needs to be updated, one should rather define a new filter with a unique name, e.g. `NeutrinoCandidate_v2` for updated neutrino candidate cuts.
