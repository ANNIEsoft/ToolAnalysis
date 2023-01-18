# StoreDecodedTimestamps

***********************
# Overview
**********************

The `StoreDecodedTimestamps` tool can be used to access the timestamp information of the different datastreams in ANNIE. It is supposed to be a supplementary tool to the event building that can provide information about all timestamps, independently from the datastream merging process. It can for example be used to get information about the timestamps of all triggerwords, while the EventBuilding will only try to match certain trigerwords and hence not have the information about all triggerwords.


************************
# Configuration
************************

The `StoreDecodedTimestamps` toolchains consists of the following tools:

* `LoadGeometry`
* `LoadRawData`
* `PMTDataDecoder`
* `MRDDataDecoder`
* `TriggerDataDecoder`
* `StoreDecodedTimestamps`

The `StoreDecodedTimestamps` tool has the following configuration variables:

```
verbosity 2
OutputFile RXXXSYpz_DecodedTimestamps.root
SaveMRD 1
SavePMT 1
SaveCTC 1
DeleteTimestamps 1
```

It is possible to only save the timestamps of certain subsystems by setting the variables `SaveMRD`, `SavePMT` or `SaveCTC` for unwanted subsystems to 0. In this case, the corresponding Decoding tools can be omitted from the toolchain. E.g. if one is not interested in the tank PMT timestamps, one can set `SavePMT` to 0 and then omit the `PMTDataDecoder` from the `ToolsConfig` file. 

The `DeleteTimestamps` variable is used to free up memory by deleting the timestamps from memory that are already saved to the output tree.

