# RunValidation

RunValidation provides a summary of the analyzed run's properties such as the detected charge of prompt/delayed events, the rates of tank/MRD/veto hits and more. It is supposed to provide an easy check of the stability of data taking over multiple runs.

## Input Data

The RunValidation tool needs the following input objects to work correctly:

**ClusterMap** `map<double, vector<Hit>>*`
* The tank PMT clusters as found by the `ClusterFinder` tool. Charge/time properties are extracted from the `Hit` vector and plotted in this tool.

**ClusterMapDetkey** `map<double, vector<unsigned long>>*`
* The channelkeys corresponding to the clustered hits that were found by the `ClusterFinder` tool.

**MrdTimeClusters** `vector<vector<int>>`
* The number of found MRD clusters, as well as the digit IDs of MRD hits belonging to each cluster, as found by the `TimeClustering` tool. The properties of MRD clusters are also analyzed by the `RunValidation` tool.

**MrdDigitTimes** `vector<double>`
* The MRD hit times corresponding to the clusters found by the `TimeClustering` tool.

**MrdDigitChankeys** `vector<unsigned long>`
* The MRD hit channelkeys corresponding to the clusters found by the `TimeClustering` tool.

**TDCData** `map<unsigned long,vector<Hit>>*`
* The TDC data object containing general information about MRD+FMV hits. (All hits, not just clustered ones). The object is primarily used to extract information about the veto hits.

## Output

The RunValidation tool produces the following output_histograms:
* `MRD_t_clusters`: Histogram of MRD clustered times
* `PMT_t_clusters`: Histogram of tank PMT clustered times
* `PMT_t_clusters_Xpe`: Histogram of tank PMT clustered times, showing only hits with charge above X p.e.
* `PMT_t_clusters_full`: Histogram of tank PMT clustered times in full (prompt+delayed) acquisition window
* `PMT_t_clusters_Xpe_full`: Histogram of tank PMT clustered times in full (prompt+delayed) acquisition window, showing only hits with charge above X p.e.
* `MRD_PMT_t`: The correlation of mean cluster times of the MRD & PMT subsystem are shown in a 2D histogram.
* `MRD_PMT_t_100pe`: The correlation of mean cluster times of the MRD & PMT subsystem, only considering events where the tank cluster has a total charge of 100 p.e. or more.
* `MRD_PMT_Deltat`: The difference between MRD & PMT cluster times are shown in a 1D histogram.
* `MRD_PMT_Deltat_100pe`: The difference between MRD & PMT cluster times are shown in a 1D histogram, only considering events where the tank cluster has a total charge of 100 p.e. or more.
* `PMT_prompt_charge`: The cumulative charge of tank clusters in the prompt acqusition window.
* `PMT_prompt_charge_10hits`: The cumulative charge of tank clusters in the prompt acqusition window, considering only events with more than 10 PMTs hit.
* `PMT_prompt_charge_zoom`: The cumulative charge of tank clusters in the prompt acqusition window, zoomed into the lower charge region.
* `PMT_delayed_charge`: The cumulative charge of tank clusters in the extended (delayed) acqusition window.
* `PMT_delayed_charge_10hits`: The cumulative charge of tank_clusters in the extended (delayed) acquisition window, considering only events with more than 10 PMTs hit.
* `PMT_delayed_charge_zoom`: The cumulative charge of tank_clusters in the extended (delayed) acquisition window, zoomed into the lower charge region.
* `ANNIE_counts`: The number of occurrences for different event types within the run. Considered event types are:
  * `All Events`: The total number of events in this run.
  * `PMT Clusters`: The total number of events with PMT clusters in this run.
  * `PMT Clusters > 100p.e.`: The total number of events with PMT clusters above 100p.e. in this run.
  * `MRD Clusters`: The total number of events with MRD clusters in this run.
  * `PMT+MRD Clusters`: The total number of events with PMT+MRD clusters in this run.
  * `PMT+MRD, No FMV`: The total number of events with PMT+MRD clusters and no FMV veto hit.
  * `FMV`: The total number of events with veto hits.
  * `FMV+PMT`: The toal number of events with PMT clusters and a FMV veto hit.
  * `FMV+MRD`: The total number of events with MRD clusters and a FMV veto hit.
  * `FMV+PMT+MRD`: The total number of events with PMT+MRD clusters and a FMV veto hit.
* `ANNIE_Rates`: Provides the same histograms as `ANNIE_counts`, but in rates instead of total counts.

## Configuration

RunValidation has the following configuration variables:

```
verbosity 1
OutputPath /path/to/outputfile
InvertMRDTimes 0
RunNumber 1611
SubRunNumber 0
RunType 3
```

The variable `InvertMRDTimes` should only be used if the MRD hit times have not been calculated correctly due to the TDC ticks being counted backwards in Common Stop Mode (should not be the case anymore for newer processed files).

The variables `RunNumber`, `SubRunNumber`, and `RunType` will only get into effect in case this information was not stored correctly in the raw data store, otherwise the tool will automatically get the run information from the `ANNIEEvent` store.

The output file name is automatically generated, just the path to the output file needs to be specified (`OutputPath`).
