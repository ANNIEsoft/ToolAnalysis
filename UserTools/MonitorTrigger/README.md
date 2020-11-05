# MonitorTrigger

The `MonitorTrigger` tool creates online monitoring plots for `TriggerData` information. In particular, the average rates of different triggerwords are recorded and plotted. In addition, the time evolutions of said rates are plotted for a user-specified timeframe, which has a default value of 24 hours. Additional plots are produced to specifically monitor the behavior of the last datafile. Such plots include the timestamps at which the different triggerwords are recorded as well as the time alignment between different triggerwords. The triggerwords that are monitored for the last datafile and with respect to their time alignment can be specified by the user in dedicated configuration files (see "Configuration" section).

## Data

The `MonitorTrigger` tool uses the trigger information as provided by the `TriggerDataDecoder` tool. The `TriggerData` objects are transferred to the `TriggerDataDecoder` tool from the `MonitorReceive` and `MonitorSimReceive` tools, which constitute the first tools in all Monitoring toolchains. The `TriggerDataDecoder` tool then fills the map `TimeToTriggerWordMap` that can be evaluated by the `MonitorTrigger` tool.

**TimeToTriggerWordMap** `map<uint64_t, uint32_t>`
* The map of triggerwords and their respective timestamps at which they occurred. The `MonitorTrigger` tool uses the values in this map to calculate the rates of the respective triggerwords.

## Configuration

MonitorTrigger can be configured in a few regards, which will be discussed in this section. The main configuration files of interest are the `TriggerMaskFile`, `TriggerWordFile`, and the `TriggerAlignFile`, which configure the following things:
* `TriggerMaskFile`: The TriggerMask file defines the triggerwords for which the detailed last-file plots are created. The TriggerMask file contains one triggerword per line.
* `TriggerWordFile`: The TriggerWord file contains a mapping of triggerword numbers to their respective triggerword names. Each line contains one triggerword number and the corresponding triggerword name.
* `TriggerAlignFile`: Triggerword pairs for which the triggerword timestamp alignment should be monitored are listed in this file. Each line contains two triggerword numbers and two numbers depicting the minimal and maximal time differences that should be monitored.

The complete configuration options are summarized in what follows:

```
# MonitorTrigger configuration file

verbose 5
#OutputPath ./monitoringplots/
OutputPath fromStore        #Output path for the monitoringplots. If "fromStore" is specified, the desired output path is taken from the CStore.
StartTime 1970/1/1 #start time for the conversion of timestamps into dates
PlotConfiguration configfiles/Monitoring/TriggerTimePlotConfig.txt #file containing instructions what to plot
PathMonitoring ./monitoringfiles/       #path at which the monitoring plots are going to be saved
ImageFormat png #format in which monitoring plots are saved. Options: png, jpg, jpeg
UpdateFrequency 1. #specify frequency for the file history plot, in mins
ForceUpdate 0   #force monitor plots to be produced even if there was no new data file available
DrawMarker 0    #specify whether to use markers for the time evolution graphs or not
TriggerMaskFile ./configfiles/Monitoring/MonitoringTriggerMask.txt
TriggerWordFile ./configfiles/Monitoring/TriggerWords.txt
TriggerAlignFile ./configfiles/Monitoring/TriggerAlign.txt
```
