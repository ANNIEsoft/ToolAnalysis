# Configure files

***********************
# Description
**********************

The `EnergyReco` toolchains are used to reconstruct the track length in the water tank and the tank energy of a muon by using a Deep Learning Neural Network(DNN) and a Boosted Decision Tree(BDT) with Gradient Boost. The `Train_Test` toolchain is used to train and test the models that we want to use later for the reconstruction. The `Predict` toolchain is used after we've trained the models in order to reconstruct the track length in the water tank and the tank energy of a muon for different events.

Both of these toolchains require information from the digit, the MRD and the vertex reconstruction so the appropriate tools need to be used.

************************
# Tools
************************

************************
# Train_Test
************************

The tools in the `Train_Test` toolchain are the following:

* LoadWCSim
* LoadWCSimLAPPD
* MCParticleProperties
* MCRecoEventLoader
* DigitBuilder
* HitCleaner
* ClusterFinder
* TimeClustering
* EventSelector
* FindMrdTracks
* VtxSeedGenerator
* VtxSeedFineGrid
* VtxExtendedVertexFinder
* FindTrackLengthInWater
* DNNTrackLengthTrain_Test
* BDTMuonEnergyTrain_Test

************************
# Predict
************************

The tools in the `Predict` toolchain are the following:

* LoadWCSim
* LoadWCSimLAPPD
* MCParticleProperties
* MCRecoEventLoader
* DigitBuilder
* HitCleaner
* ClusterFinder
* TimeClustering
* EventSelector
* FindMrdTracks
* VtxSeedGenerator
* VtxSeedFineGrid
* VtxExtendedVertexFinder
* FindTrackLengthInWater
* DNNTrackLengthPredict
* BDTMuonEnergyPredict

