# DataDecoderwLAPPD

***********************
# Description
**********************

The `DataDecoderwLAPPD` toolchain combines the timestamps of the different data streams in the scope of the event building process. In comparison to the `DataDecoder` toolchain, this toolchain also includes the LAPPD data.

************************
# Tools
************************

It includes the following tools:

```
LoadGeometry LoadGeometry ./configfiles/LoadGeometry/LoadGeometryConfig
LoadRawData LoadRawData ./configfiles/DataDecoderwLAPPD/LoadRawDataConfig
PMTDataDecoder PMTDataDecoder ./configfiles/DataDecoderwLAPPD/PMTDataDecoderConfig
MRDDataDecoder MRDDataDecoder ./configfiles/DataDecoderwLAPPD/MRDDataDecoderConfig
TriggerDataDecoder TriggerDataDecoder ./configfiles/DataDecoderwLAPPD/TriggerDataDecoderConfig
LAPPDDataDecoder LAPPDDataDecoder ./configfiles/DataDecoderwLAPPD/LAPPDDataDecoderConfig
BeamDecoder BeamDecoder ./configfiles/DataDecoderwLAPPD/BeamDecoderConfig
PhaseIIADCCalibrator PhaseIIADCCalibrator ./configfiles/DataDecoderwLAPPD/PhaseIIADCCalibratorConfig
PhaseIIADCHitFinder PhaseIIADCHitFinder ./configfiles/DataDecoderwLAPPD/PhaseIIADCHitFinderConfig
ANNIEEventBuilder ANNIEEventBuilder ./configfiles/DataDecoderwLAPPD/ANNIEEventBuilderConfig
```

************************
# Specifics for LAPPD event building
************************

The main additional configuration variable for event building with LAPPD data is included in the `ANNIEEventBuilder` tool. The global timestamp for the LAPPD needs to be inferred from the time alignment of the LAPPD PPS and beam timestamps with the CTC PPS and beam timestamps. Usually, the best alignment is searched for automatically by the `ANNIEEventBuilder` tool, but there are occasions where this alignment is ambiguous. In such a case, the timestamp offset needs to be inferred from previous and subsequent files and needs to be specified in a configuration file as follows:

```
LAPPDOffsetFile ./configfiles/DataDecoderwLAPPD/LAPPDOffsetFile_R3823-R3844.txt
```

The configuration file takes the following form:
```
1656371768.29742527008056640625 #constant offset c
0.00000562631824812537 #drift m
```
In the configuration file, the first line specifies the constant offset (`c`) which needs to be added to the LAPPD timestamp to obtain the global timestamp, while the second line (`m`) accounts for a small drift of the LAPPD timestamps and the CTC timestamps. The expected offset is calculated based on the first PPS timestamp `PPS_first` in the data file as follows:
```
global_offset = c + m*PPS_first
```

In contrast, if the global offset should be inferred automatically, one needs to specify 
```
LAPPDOffsetFile None
```

************************
# Offset calculation
************************

An illustration of how the offsets can be calculated based on the offsets from previous and subsequent files can be found in the following repository:

https://github.com/mnieslony/LAPPD_QuickAnalysis.git
