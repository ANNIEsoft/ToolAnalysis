# BDTMuonEnergyPredict

The `BDTMuonEnergyPredict` tool is used to reconstruct the energy of the muon using a Boosted Decision Tree(BDT) with Gradient Boost. The tool loads the necessary information from the `EnergyReco` store. The `FindTrackLengthInWater` and `DNNTrackLengthPredict` tools should always be placed before the `BDTMuonEnergyPredict` tool in a toolchain. This tool loads the weights file of the model which has already been trained. The value of the reconstructed muon energy is passed to the next tools through the `EnergyReco` store. Also, this value is set into the `ANNIEEvent` store with the key `RecoMuonEnergy`.

## Data

### Input

The following variables are obtained from the `EnergyReco` store:

**DNNRecoLength** `double` The reconstructed track length in water for a single event

**num_pmt_hits** `int` Total number of pmt digits

**num_lappd_hits** `int` Total number of lappd digits

**recoTrackLengthInMrd** Track length of a reconstructed track in the MRD found by the `FindMrdTracks` tool

**diffDirAbs** `double` Angle difference between the reconstructed z direction and the beam direction at (0,0,1)

**recoDWallR** `double` Radial distance of the reconstructed vertex from the walls of the tank 

**recoDWallZ** `double` Axial distance of the reconstructed vertex from the walls of the tank

**vtxVec** `Position` Position of the reconstructed vertex

**trueMuonEnergy** `double` MC muon energy

### Output

The following variables are passed on to the next tool via the `EnergyReco` store:

**BDTMuonEnergy** `double` The reconstructed energy of the muon for a single event

## Configuration

```
BDTMuonModelFile finalized_BDTmodel_forMuonEnergy.sav
The path of the weights file as it has been set when training the model
```
