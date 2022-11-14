# MonitorLAPPDData

The `MonitorLAPPDData` tool is a part of the official ANNIE Monitoring toolchain and produces simple monitoring plots for the raw LAPPD data, streamed directly from the ANNIE DAQ.

The LAPPD raw data is decoded prior by the `ParseDataMonitoring` tool before being analyzed by the `MonitorLAPPDData` tool.

It produces the following kinds of plots:
* pedestal fits + ADC histograms
* exemplary waveforms
* simple hit counting for LAPPD pulses
* Timing plot w.r.t. the beginning of the beamgate

## Configuration

`MonitorLAPPDData` has the following configuration variables

```
verbose 0 #verbosity
OutputPath fromStore  #output path for monitoring root files
StartTime 1970/1/1 #start time for the conversion of timestamps into dates
ReferenceDate 2021/10/20  # reference date for power up of LAPPD, deprecated
ReferenceTime 12:25:33.13 # reference time for power up of LAPPD hours:minutes:seconds, deprecated
PlotConfiguration ./configfiles/Monitoring/LAPPDDataPlotConfig.txt #file containing instructions what to plot
PathMonitoring /monitoringfiles/	#path at which the monitoring plots are going to be saved
ImageFormat png	#format in which monitoring plots are saved. Options: png, jpg, jpeg
UpdateFrequency 1. #specify frequency for the file history plot, in mins
ForceUpdate 0	#force monitor plots to be produced even if there was no new data file available
DrawMarker 0	#specify whether to use markers for the time evolution graphs or not
ACDCBoardConfiguration ./configfiles/Monitoring/LAPPDACDCConfig.txt #configure which ACDC board numbers will be expected
ThresholdPulse 3.	#3 sigma away from baseline -> pulse
```
