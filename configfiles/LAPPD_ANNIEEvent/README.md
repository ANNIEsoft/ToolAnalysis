# LAPPD_ANNIEEvent toolchain

The toolchain in this directory are exemplary toolchains which should showcase how one can access the LAPPD data in the raw and in the processed BoostStore data files from the official DAQ.

************************
# GetLAPPDEvents
************************

This toolchain operates on the raw data files of ANNIE and counts the number of LAPPD events in the data file and writes them to an output text file.

************************
# checkLAPPDStatus
************************

The `checkLAPPDStatus` toolchain is supposed to show how LAPPD data can be accessed in the processed data files. It first checks whether an event actually contains LAPPD data, then parses the LAPPD data to access whether any hits were seen in the beam window period, and runs the reconstruction of the MRD to evaluate whether the LAPPD events were happening for an event with a reconstructed MRD track and no veto activity. The information is written to a csv output file, but could also be used downstream by further tools to run additional reconstruction.

It contains the following tools

```
LoadANNIEEvent
LoadGeometry
TimeClustering
FindMrdTracksConfig
ClusterFinderConfig
ClusterClassifiersConfig
EventSelectorConfig
checkLAPPDStatus
parseLAPPDData
saveLAPPDInfo
```
