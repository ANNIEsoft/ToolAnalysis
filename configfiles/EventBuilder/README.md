# EventBuilder

***********************
## Description
**********************

The `EventBuilder` toolchain is used to event build for ANNIE. The toolchain reads in RAWData files, time match, and creates processed ANNIEEvents with Tank, CTC, and MRD information (currently no LAPPD information is included). This toolchain consolidated efforts to create an official event building toolchain, and should replace the `DataDecoder` toolchain which is now considered depreciated. 

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
PhaseIIADCCalibrator
PhaseIIADCHitFinder
SaveConfigInfo
ANNIEEventBuilder
```
