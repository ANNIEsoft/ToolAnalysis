# FMVEfficiency ToolChain

***********************
# Description
**********************

The FMVEfficiency toolchain calculates FMV paddle efficiencies by evaluating coincidences between the FMV layers and PMT/MRD clusters.

************************
# Usage
************************

The FMVEfficiency toolchain consists of the following tools:

* LoadGeometry
* LoadANNIEEvent
* PhaseIIADCCalibrator
* PhaseIIADCHitFinder
* ClusterFinder
* TimeClustering
* FindMrdTracks
* FMVEfficiency

If one wants to calculate efficiencies based on MRD/FMV coincidences only, the tools `PhaseIIADCCalibrator`, `PhaseIIADCHitFinder` & `ClusterFinder` are not needed.
