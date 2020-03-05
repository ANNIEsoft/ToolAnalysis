# MrdPaddleEfficiency

***********************
# Description
**********************

The `MrdPaddleEfficiency` toolchain is supposed to determine the efficiency of MRD paddles by evaluating fitted MRD tracks which traverse the whole MRD (e.g. cosmic muons) and check how often the paddles on the reconstructed track actually saw a hit, hence determining their efficiency. There are two separate toolchains for data and MRD, for the reason that the data files need a different mapping of channelkeys to WCSim PMT IDs than the MC files. 

The mapping of channelkeys for the data files is done by reading in the text file `MRD_Chankey_WCSimID.dat`, the mapping for MC files is mitigated via the auto-generated map by `LoadWCSim`.

The toolchains for data and MC differ also in the respect that MC files are read in via `LoadWCSim` and data files via `LoadANNIEEvent` (the ANNIEEvent files should be created in advance from raw data files by running the `MRDDataDecoder` tool together with the `ANNIEEventBuilder` tool).

The `MRDPaddleEfficiency` tool will be added in a future PR, the toolchain can however already be used to fit tracks for both data and MC MRD data.

************************
# ToolChains
************************

**Data**

```
myLoadGeometry LoadGeometry configfiles/MrdPaddleEfficiency/Data/LoadGeometryConfig
myLoadANNIEEvent LoadANNIEEvent configfiles/MrdPaddleEfficiency/Data/LoadANNIEEventConfig
myTimeClustering TimeClustering configfiles/MrdPaddleEfficiency/Data/TimeClusteringConfig
myFindMrdTracks FindMrdTracks configfiles/MrdPaddleEfficiency/Data/FindMrdTracksConfig
myPlotMrdTracks MrdPaddlePlot configfiles/MrdPaddleEfficiency/Data/MrdPaddlePlotConfig
myMrdPaddleEfficiencyPreparer MrdPaddleEfficiencyPreparer configfiles/MrdPaddleEfficiency/Data/MrdPaddleEfficiencyPreparerConfig
```

**MC**

```
myLoadWCSim LoadWCSim configfiles/MrdPaddleEfficiency/MC/LoadWCSimConfig		#choose LoadWCSim for MC files
myTimeClustering TimeClustering configfiles/MrdPaddleEfficiency/MC/TimeClusteringConfig
myFindMrdTracks FindMrdTracks configfiles/MrdPaddleEfficiency/MC/FindMrdTracksConfig
myPlotMrdTracks MrdPaddlePlot configfiles/MrdPaddleEfficiency/MC/MrdPaddlePlotConfig
myMrdPaddleEfficiencyPreparer MrdPaddleEfficiencyPreparer configfiles/MrdPaddleEfficiency/MC/MrdPaddleEfficiencyPreparerConfig
```

