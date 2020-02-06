# FindMrdTracks

The `FindMrdTracks` tool uses the `MRDTrackLib` libraries to reconstruct tracks within the MRD. The tool is applied to time clustered hits in the MRD, which are found by the `TimeClustering` tool and subsequently passed to `FindMrdTracks` throught the `CStore`. The `TimeClustering` tool should hence always be placed before the `FindMrdTracks` tool in a toolchain.

`FindMrdTracks` has the ability to apply the track finding algorithm only for certain MRD trigger types (Cosmic/Beam). Information about the found tracks can be written to an output root file.

# MRDTrackLib
cMRDSubEvent and cMRDTrack class library needed to construct MRD Track reconstruction classes

# Input

The following variables are obtained from the `CStore`:

**ClusterStartTimes** `vector<float>` Vector containing the start times of all subevents

**MrdTimeClusters** `vector<vector<int>>` One vector for each subevent containing the digit IDs for clustered hits in each subevent

**NumMrdTimeClusters** `int` Number of time clusters within this event

**MrdDigitTimes** `vector<double>` A vector containing all the recorded times for all digit IDs in this event

**MrdDigitPmts** `vector<int>` A vector containing all the WCSim PMT IDs for all digit IDs in this event

**MrdDigitCharges** `vector<double>` A vector containing all the recorded charges for all digit IDs in this event (remnant from MC, not actually useful since we don't have charge information in data)

# Output

The following variables are passed on to the next tool via the `CStore`:

**SubEventArray** `TClonesArray` Array containing all the subevents with the associated track information

# Configuration

```
verbosity 5
IsData 1
OutputDirectory .
OutputFile MRDTracks_MRDTest28_cluster30ns
DrawTruthTracks 0 # whether to add MC Truth track info for drawing in MrdPaddlePlot Tool
                  ## note you need to run that tool to actually view the tracks!
WriteTracksToFile 1     # should the track information be written to a ROOT-file?
SelectTriggerType 1     #should the loaded data be filtered by trigger type?
TriggerType Cosmic      #options: Cosmic, Beam, No Loopback
```
