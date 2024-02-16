# FindTrackLengthInWater

The `FindTrackLengthInWater` tool uses the `find_lambda()` function to calculate the length between the reconstructed Vertex and the photon production Point(lambda) for each digit. These lengths are going to be used to reconstruct the track in the water in the next tool. In order to do that the tool requires information about the `ExtendedVertex` and the `RecoDigit` which are stored in the `RecoEvent` store from previous tools. Then the required information for the track length and energy reconstruction are passed to the next tools through the `EnergyReco` boost store. Also cuts are applied to exclude events with bad vertex reconstruction(recovtxFOM<=0) and with no reconstructed tracks in the MRD.

## Data

### Input

The following variables are obtained from the `ANNIEEvent` store:

**EventNumber** `uint32_t`

**AnnieGeometry**(Header) `Geometry` Retrieve the tank radius and the tank halfheight from the geometry object of the `ANNIEEvent` store

The following variables are obtained from the `RecoEvent` store:

**ExtendedVertex** `RecoVertex` Contains the reconstructed Vertex information

**MrdTimeClusters** `vector<vector<int>>` One vector for each subevent containing the digit IDs for clustered hits in each subevent

**TrueTrackLengthInWater** `double` MC track length in the water is retrieved from the `RecoEvent` store

**TrueMuonEnergy** `double` MC muon energy is retrieved from the `RecoEvent` store

**RecoDigit** `std::vector<RecoDigit>` One vector for each event containing the RecoDigit objects for each event

### Output

The following variables are passed on to the next tool via the `EnergyReco` store:

**MaxTotalHitsToDNN** `int`

**ThisEvtNum** `uint32_t`

**lambda_vec** `std::vector<double>` Vector with all the lambda values for each event

**digit_ts_vec** `std::vector<double>` Vector with the time of all the digits of each event

**lambda_max** `double` The distance between the reconstructed vertex and last Cherenkov photon emission point along the track

**num_pmt_hits** `int` Total number of pmt digits

**num_lappd_hits** `int` Total number of lappd digits

**TrueTrackLengthInWater** `float` MC track length in the water

**trueMuonEnergy** `double` MC muon energy

**diffDirAbs** `double` Angle difference between the reconstructed z direction and the beam direction at (0,0,1)

**recoDWallR** `double` Radial distance of the reconstructed vertex from the walls of the tank 

**recoDWallZ** `double` Axial distance of the reconstructed vertex from the walls of the tank 

**dirVec** `Direction` Direction of the reconstructed vertex

**vtxVec** `Position` Position of the reconstructed vertex

**recoTrackLengthInMrd** Track length of a reconstructed track in the MRD found by the `FindMrdTracks` tool

## Configuration

```
MaxTotalHitsToDNN 1100
This is set to 1100 as a reasonable upper limit to the total number of hits that might happen in a single event in order to have a fixed number of features for the DNN

OutputDataFile data_for_trackLength_training.csv
There exists the capabillity to create an output .csv file with the necessary information for the DNN for any inpdependent analysis or testing

DoTraining bool
It is by default set to 0. Only when you want to run the training toolchain you need to set it to 1
```
