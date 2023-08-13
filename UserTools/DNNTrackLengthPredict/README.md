# DNNTrackLengthPredict

The `DNNTrackLengthPredict` tool is used to reconstruct the track length of the muon in the water tank using a Deep Learning Neural Network(DNN) from the Keras library on top of Tensorflow. The tool loads the necessary information from the `EnergyReco` store. The `FindTrackLengthInWater` tool should always be placed before the `DNNTrackLengthPredict` tool in a toolchain. The scaling parameters are loaded from the `ScalingVarsStore` in order to preprocess our data for the model. This tool also loads the weights file of the model which has already been trained. The value of the reconstructed track length in the water tank is passed to the next tools through the `EnergyReco` store. Also, this value is set into the `ANNIEEvent` store with the key `RecoTrackLengthInTank`.

## Data

### Input

The following variables are obtained from the `EnergyReco` store:

**MaxTotalHitsToDNN** `int`

**lambda_vec** `std::vector<double>` Vector with all the lambda values for each event

**digit_ts_vec** `std::vector<double>` Vector with the time of all the digits of each event

**lambda_max** `double` The distance between the reconstructed vertex and last Cherenkov photon emission point along the track

**num_pmt_hits** `int` Total number of pmt digits

**num_lappd_hits** `int` Total number of lappd digits

**TrueTrackLengthInWater** `float` MC track length in the water

The following variables are obtained from the `ScalingVarsStore` store:

**features_mean_values** `std::vector<double>` Vector with the means of each feature of the training dataset

**features_std_values** `std::vector<double>` Vector with the standard deviations of each feature of the training dataset

### Output

The following variables are passed on to the next tool via the `EnergyReco` store:

**DNNRecoLength** `double` The reconstructed track length in water for a single event

## Configuration

```
TrackLengthWeightsFile UserTools/DNNTrackLength/stand_alone/weights/weights_bets.hdf5
The path of the weights file as it has been set when training the model

ScalingVarsBoostStoreFile Data_Energy_Reco/ScalingVarsStore.bs
The path of the scaling parameters file as it has been set when training the model
```
