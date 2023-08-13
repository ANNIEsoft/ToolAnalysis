# DNNTrackLengthTrain_Test

The `DNNTrackLengthTrain_Test` tool is used to train a Deep Learning Neural Network(DNN) using the Keras library on top of Tensorflow to reconstruct the track length of the muon in the water. The tool loads the necessary information from the `EnergyReco` store which have saved to disk by the `FindTrackLengthInWater` tool. The `FindTrackLengthInWater` tool should hence always be placed before the `DNNTrackLengthTrain_Test` tool in a toolchain. The input dataset is split into half in order to use one half for training and testing the model and the second half for reconstructing the track length in the water and use these events to train the BDT for the muon energy in the next tool. After training and testing the weights are saved locally. The values of the reconstructed track length in the water for the second half of the dataset are saved in the `DNNRecoLength` store to be loaded by the next tool. Also, some parameters concerning the training dataset that will need to be used in the prediction toolchain to scale our data are saved locally in the `ScalingVarsStore`. A few plots are also drawn for visualization purposes.

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

### Output

**features_mean_values** `std::vector<double>` Vector with the means of each feature of the training dataset

**features_std_values** `std::vector<double>` Vector with the standard deviations of each feature of the training dataset

**DNNRecoLength** `std::vector<double>` Vector with the reconstructed track length in water for each event of the second half of the event sample

## Configuration

```
InitialiseFunction Initialise
ExecuteFunction Finalise
FinaliseFunction Execute
Everything is done in the Execute method of the tool. We need to run the Execute method in the Finalise step of the toolchain so that the FindTrackLengthInWater tool has already saved a multiple event sample for the training.

TrackLengthOutputWeightsFile UserTools/DNNTrackLength/stand_alone/weights/weights_bets.hdf5
The path where you want to save the weights file

TrackLengthTrainingInputBoostStoreFile EnergyReco.bs
The path of the EnergyReco boost store

ScalingVarsBoostStoreFile Data_Energy_Reco/ScalingVarsStore.bs
The path where you want to save the scaling parameters file
```
