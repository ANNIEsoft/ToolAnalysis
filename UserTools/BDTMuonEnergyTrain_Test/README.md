# BDTMuonEnergyTrain_Test

The `BDTMuonEnergyTrain_Test` tool is used to train a Boosted Decision Tree(BDT) with Gradient Boost which will then be able to reconstruct the energy of the muon. The tool loads the necessary information from the `EnergyReco` store which have been saved to disk by the `FindTrackLengthInWater` tool. The `FindTrackLengthInWater` tool should hence always be placed before the `BDTMuonEnergyTrain_Test` tool in a toolchain. The tool also loads the values for the reconstructed track length in the water for the each of the events which are calculated by the `DNNTrackLengthTrain_Test` tool and are passed to the `BDTMuonEnergyTrain_Test` tool through the `DNNRecoLength` store. The `DNNTrackLengthTrain_Test` tool should also always be placed before the `BDTMuonEnergyTrain_Test` tool in a toolchain. After training and testing the weights are saved locally. A few plots are also drawn for visualization purposes.

## Data

### Input

The following variables are obtained from the `EnergyReco` store:

**num_pmt_hits** `int` Total number of pmt digits

**num_lappd_hits** `int` Total number of lappd digits

**recoTrackLengthInMrd** Track length of a reconstructed track in the MRD found by the `FindMrdTracks` tool

**diffDirAbs** `double` Angle difference between the reconstructed z direction and the beam direction at (0,0,1)

**recoDWallR** `double` Radial distance of the reconstructed vertex from the walls of the tank 

**recoDWallZ** `double` Axial distance of the reconstructed vertex from the walls of the tank

**vtxVec** `Position` Position of the reconstructed vertex

**trueMuonEnergy** `double` MC muon energy

The following variables are obtained from the `DNNRecoLength` store:

**DNNRecoLength** `std::vector<double>` Vector with the reconstructed track length in water for each event of the event sample

## Configuration

```
InitialiseFunction Initialise
ExecuteFunction Finalise
FinaliseFunction Execute
Everything is done in the Execute method of the tool. We need to run the Execute method in the Finalise step of the toolchain so that the FindTrackLengthInWater tool has already saved a multiple event sample for the training.

BDTMuonEnergyWeightsFile finalized_BDTmodel_forMuonEnergy.sav
The path where you want to save the weights file

BDTMuonEnergyTrainingInputBoostStoreFile EnergyReco.bs
The path of the EnergyReco boost store
```
