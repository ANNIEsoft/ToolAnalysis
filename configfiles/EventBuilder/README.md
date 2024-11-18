# EventBuilder

***********************
## Description
**********************

The `EventBuilder` toolchain is used to create ANNIEEvents. The toolchain reads in RAWData files, time match, and creates processed ANNIEEvents with Tank, CTC, MRD, and Beam information (no LAPPD information is included in this toolchain). This toolchain replaces the `DataDecoder` toolchain which is now considered depreciated. 

Please consult the following ANNIE wiki page on how to event build: https://cdcvs.fnal.gov/redmine/projects/annie_experiment/wiki/Event_Building_with_ToolAnalysis

************************
## Tools
************************

The toolchain consists of the following tools:

```
LoadGeometry
LoadRawData
PMTDataDecoder
MRDDataDecoder
TriggerDataDecoder
BeamFetcherV2
BeamQuality
PhaseIIADCCalibrator
PhaseIIADCHitFinder
SaveConfigInfo
ANNIEEventBuilder
```
