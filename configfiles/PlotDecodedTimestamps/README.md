# PlotDecodedTimestamps

***********************
# Overview
**********************

The `PlotDecodedTimestamps` toolchain provides the opportunity to plot the timestamps of all datastreams in ANNIE in a visual way. The necessary information needs to be provided via the `DataSummary` tool and the `StoreDecodedTimestamps` tool. The former will provide information about matched and orphaned timestmaps while the latter can give more details on timestamps of triggerwords that are not usually considered in the EventBuilding process. The timestamps of each datastream will be plotted in different rows in order to better see correlations between the subsystems for matched events.

************************
# Configuration
************************

The configuration of the `PlotDecodedTimestamps` tool has variables for the needed input files and one variable specifying the name of the output file. The variable `SecondsPerPlot` defines how many seconds are plotted in one canvas. This value should not be chosen too large, otherwise it will be difficult to disentangle the single timestamp lines (default: 10s)

Another configuration variable is `TriggerWordConfig`, which specifies the path to a configuration file. This file specifies which additional triggerwords should be shown in the CTC stream and which color the respective triggerword should be displayed. The triggerwords 5 (beam) and 36 (MRD CR) are always shown in the CTC stream, while others need to be specified in this file.

```
verbosity 2
OutputFile RXXXSYpz_TimestampsPlots.root
InputDataSummary DataSummary_RXXXSYpz.root
InputTimestamps RXXXSYpz_DecodedTimestamps.root
SecondsPerPlot 10
TriggerWordConfig ./configfiles/PlotDecodedTimestamps/TriggerColors.txt
```

