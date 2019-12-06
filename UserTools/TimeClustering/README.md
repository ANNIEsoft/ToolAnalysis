# TimeClustering

The TimeClustering tool separates MRD paddle hit times into clusters. The clusters will then be passed on to further tools downstream that can search for tracks (`FindMrdTracks`) or plot the MRD Event Display (`MrdPaddlePlot`) or determine paddle efficiencies (`MrdPaddleEfficiency` [to be added]).

`TimeClustering` can deal with both MC and data input. The channelkeys are converted into WCSim IDs that are needed by the `MrdTrackLib` libraries that actually incorporate all the machinery to do the track fitting and plotting. Conversion of channelkeys to WCSim IDs happens via a text file for data and via a passed-on-map by the `LoadWCSim` tool for MC.

The found clusters are loaded into the `CStore` for the subsequent tools. The following section describes more in detail what objects are passed on by the `TimeClustering` tool. 

## Input objects

**TDCData** `map<unsigned long,vector<Hit>>` TDCData object for data

**TDCData** `map<unsigned long,vector<MCHit>>` TDCData object for MC

**MRDTriggertype** `string` MRD Triggertype determined by the MRD loopback channel [Beam/Cosmic/No Loopback]

## Output objects

The following output objects that were created by `TimeClustering` are stored in the `CStore`:

**ClusterStartTimes** `vector<float>` Vector containing the start times of all subevents

**MrdTimeClusters** `vector<vector<int>>` One vector for each subevent containing the digit IDs for clustered hits in each subevent

**NumMrdTimeClusters** `int` Number of time clusters within this event

**MrdDigitTimes** `vector<double>` A vector containing all the recorded times for all digit IDs in this event

**MrdDigitPmts** `vector<int>` A vector containing all the WCSim PMT IDs for all digit IDs in this event

**MrdDigitCharges** `vector<double>` A vector containing all the recorded charges for all digit IDs in this event (remnant from MC, not actually useful since we don't have charge information in data)


## Configuration

Describe any configuration variables for TimeClustering.

```
verbosity 2
MinDigitsForTrack 4             # minimum MRD paddles to constitute a cluster
MaxMrdSubEventDuration 30       # maximum subevent duration only important if we only had 1 subevent
MinSubeventTimeSep 30           # minimum time separation between clusters
MakeMrdDigitTimePlot 0          # make debugging plots
LaunchTApplication 0            # launch TApplication for on-the-fly plots?
IsData 1                        # Data/MC
OutputROOTFile TimeClustering_MRDTest28_cluster30ns         # Output ROOT file for time clustering histograms
MapChankey_WCSimID ./configfiles/MrdPaddleEfficiency/MRD_Chankey_WCSimID.dat  # map from Channelkey to WCSim IDs for data files
```
