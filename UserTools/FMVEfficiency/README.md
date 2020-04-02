# FMVEfficiency

FMVEfficiency determines FMV paddle efficiencies by looking at coincidences of FMV hits and PMT/MRD clusters.

The efficiencies are calculated paddle-wise by counting the number of observed and expected hits for each paddle. Furthermore, a position-dependent efficiency is attempted by extrapolating found MRD tracks and looking for the intersection of the extrapolated track and the FMV paddle.

## Input Data

FMVEfficiency uses the following objects to get necessary information about the event:

**TDCData** `map<unsigned long, vector<Hit>>*`
* Extract the information about FMV hits from TDCData.

**m_all_clusters** `map<double,vector<Hit>>*`
* Tank clusters as found by the `ClusterFinder` tool. Tank clusters will be used for FMV coincidence searches only if `UseTank` is set to 1.

**m_all_clusters_detkey**` map<double,vector<unsigned long>>*`
* Channelkeys corresponding to the tank clusters found by the `ClusterFinder` tool. Will be used for FMV coincidence searches only if `UseTank` is set to 1.

**MrdTimeClusters** `vector<vector<int>>`
* Digit IDs of MRD clusters that were found by the `TimeClustering` tool. MRD/FMV coincidences are the default mode, so this object is always loaded.

**MrdDigitTimes** `vector<double>`
* Times corresponding to the MRD clusters found by the `TimeClustering` tool.

**mrddigitchankeysthisevent** `vector<unsigned long>`
* Channelkeys corresponding to the MRD clusters found by the `TimeClustering` tool.

**theMrdTracks** `vector<BoostStore>*`
* MRD tracks as found by the `FindMrdTracks` tool. The tracks are used to get position-dependent efficiencies on the FMV paddles and get stricter constraints on the FMV-MRD coincidence searches.

## Output Data

The output of FMVEfficiency is entirely in the forms of histograms, where there are some histograms describing the general properties of the coincidence selection and some contain the results of the coincidence search.

* **General properties of coincidence selection**
  * `time_diff_LayerX`: Time difference of MRD clusters and FMV hits for layer X.
  * `time_diff_tank_LayerX`: Time difference of tank clusters and FMV hits for layer X.
  * `num_paddles_LayerX`: Number of hit FMV paddles per event.
  * `track_diff_x_LayerX`: x difference of the extrapolated MRD track and the center of the hit FMV paddle for layer X.
  * `track_diff_y_LayerX:`: y difference of the extrapolated MRD track and the center of the hit FMV paddle for layer X.
  * `track_diff_xy_LayerX:`: x-y difference of the extrapolated MRD track and the center of the hit FMV paddle for layer X.
  * `track_diff_*_strict_LayerX`: coordinate difference of the extrapolated MRD track for tracks fulfilling a strict paddle confinement condition.
  * `track_diff_*_loose_LayerX`: coordinate difference of the extrapolated MRD track for tracks fulfilling a looser paddle confinement condition, considering the MRD track reconstruction uncertainty

* **Coincidence search results**

  * `fmv_tank_observed_layerX:`: Channel-wise representation of the number of observed hits for tank-FMV coincidences in layer X
  * `fmv_tank_expected_layerX:`: Channel-wise representation of the number of expected hits for tank-FMV coincidences in layer X
  * `fmv_observed_layerX:`: Channel-wise representation of the number of observed hits for MRD-FMV coincidences in layer X
  * `fmv_expected_layerX:`: Channel-wise representation of the number of expected hits for MRD-FMV coincidences in layer X
  * `fmv_*_track_strict_layerX:`: Channel-wise representation of expected/observed hits for FMV channels fulfilling the strict paddle-track coincidence condition
  * `fmv_*_track_loose_layerX:`: Channel-wise representation of expected/observed hits for FMV channels fulfilling the looser paddle-track coincidence condition
  * `hist_observed_strict_chankeyX`: Observed hits as a function of the position on the paddle with chankey X, for paddles fulfilling the strict paddle-track coincidence condition
  * `hist_expected_strict_chankeyX`: Expected hits as a function of the position on the paddle with chankey X, for paddles fulfilling the looser paddle-track coincidence condition


## Configuration

FMVEfficiency uses the following configuration variables:

```
SinglePEGains ./configfiles/FMVEfficiency/ChannelSPEGains_BeamRun20192020.csv      #Path to single P.E. file
verbosity 0
OutputFile FMVEfficiency_RAWDataR1613p2_PMTMRD     #Name of output file
UseTank 1      # should tank clusters also be used for coincidences or only MRD clusters?
```
