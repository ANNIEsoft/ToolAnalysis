# PlotsTrackLengthAndEnergy

The `PlotsTrackLengthAndEnergy` tool is used to plot the reconstructed track length and energy from the DNN and the BDT. Therefore this tool should be run only after running the `EnergyRecoPredict` toolchain. This tool loads the `EnergyReco` boost store that is being saved locally by the `BDTMuonEnergyPredict` tool in order to make the plots we want.

## Data

### Input

The following variables are obtained from the `EnergyRecoStore`:

**lambda_max** `double` The distance between the reconstructed vertex and last Cherenkov photon emission point along the track

**TrueTrackLengthInWater** `float` MC track length in the water

**DNNRecoLength** `double` The reconstructed track length in water for a single event

**trueMuonEnergy** `double` MC muon energy

**BDTMuonEnergy** `double` The reconstructed energy of the muon for a single event

## Configuration

There aren't any configuration variables for PlotsTrackLengthAndEnergy.
