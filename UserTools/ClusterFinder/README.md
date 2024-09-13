# ClusterFinder

The `ClusterFinder` tool is based on the TankCalibrationDiffuser. Its goal is to find cluster of hits in an acquisition window.
Clusters are found using hit times.

## Data

The ClusterFinder tool needs access to the hit information within ANNIE events:

**MCHits** `map<unsigned long, vector<Hit>>`
* Takes this data from the `ANNIEEvent` store and evaluates whether observed and expected times and charges correspond to one another.

## Configuration

The following variables can be configured for the ClusterFinder tool:

```
# ClusterFinder Config File

HitStore Hits #Either MCHits or Hits (accessed in ANNIEEvent store)
OutputFile LEDRun1415S0Beam_AllPMTs_TestClusterFinder #Output root prefix name for the current run
ClusterFindingWindow 50 # in ns, size of the window used to "clusterize"
AcqTimeWindow 4000 # in ns, size of the acquisition window
ClusterIntegrationWindow 50 # in ns, all hits with +/- 1/2 of this window are considered in the cluster
MinHitsPerCluster 10 # group of hits are considered clusters above this amount of hits
MC_pulse_width 10  # width of MC "pulse" in which to integrate true photon hits - all true photon hits on a single PMT will be combined into a "pulse" of this width

verbose 1         #verbosity of the application

```

## OutputFiles

The tool produces one output file:
* a root file `Run<run_number>_AllPMTs_ClusterFinder` which contains charge & time histograms for all the clusters as well as a "DeltaT" histogram showing the time difference between the current cluster and the first cluster of the acquisition window

One of the output is a map <double, vector<Hit>> that contains the clusters sorted by mean time and their corresponding hits
