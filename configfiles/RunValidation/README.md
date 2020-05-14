# RunValidation ToolChain

***********************
# Description
**********************

The `RunValidation` ToolChain creates validation plots for phase II data runs to track stability of data taking and the quality of the acquired data.

************************
# Tools in ToolChain
************************

The following tools are in the `RunValidation` Toolchain:
* LoadGeometry
* LoadANNIEEvent
* PhaseIIADCCalibrator
* PhaseIIADCHitFinder
* ClusterFinder
* TimeClustering
* RunValidation

*********************************************
# Configuration options of RunValidation tool
*********************************************

```
verbosity 0
OutputPath ./
InvertMRDTimes 0
RunNumber 1627
SubRunNumber 0
RunType 3
SinglePEGains ./configfiles/RunValidation/ChannelSPEGains_BeamRun20192020.csv
```

The variables `RunNumber`, `SubRunNumber` and `RunType` only need to be set when the run information was not stored in the raw data for some reason.
