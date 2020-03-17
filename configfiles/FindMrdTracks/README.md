# FindMrdTracks toolchains

***********************
# General
**********************

The `FindMrdTracks` toolchains fit tracks within clusters of hits in the Muon Range Detector. There is a distinction between the toolchain on MC and on data, with the key difference being that the mapping of channelkeys is done differently: For the data files it is done by reading in the text file `MRD_Chankey_WCSimID.dat`, the mapping for MC files is mitigated via the auto-generated map by `LoadWCSim`.

Furthermore, the MC toolchain contains a lot more tools due to the fact that a lot of work was done on combining the fit results of the MRD and the tank PMTs, which is currently not present in the data toolchain.

************************
# MC
************************

The tools in the MC toolchain are the following:

* LoadWCSim
* LoadWCSimLAPPD
* MCParticleProperties
* MCRecoEventLoader
* DigitBuilder
* EventSelector
* VtxSeedGenerator
* VtxExtendedVertexFinder
* PhaseIITreeMaker
* TimeClustering
* FindMrdTracks
* TrackCombiner
* MrdPaddlePlot

************************
# Data
************************

The tools in the standard data toolchain are the following:

* LoadGeometry
* LoadANNIEEvent
* TimeClustering
* FindMrdTracks
* MrdPaddlePlot
