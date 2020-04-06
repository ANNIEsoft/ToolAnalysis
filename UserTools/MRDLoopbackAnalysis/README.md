# MRDLoopbackAnalysis

`MRDLoopbackAnalysis` is a small tool that prints information about the MRD loopback channels. The TDC values of the two loopback channels (beam,cosmic) are drawn as a function of the time difference between events and as a function of the clustered hit times. Furthermore, the time differences w.r.t. the previous event are shown are shown independently of the loopback TDC value.

## Input Data

MRDLoopbackAnalysis uses the clustered MRD information as found by the `TimeClustering` tool, and furthermore the information about the `MRDTriggerType` and the `MRDLoopbackTDC` from the ANNIEEvent BoostStore.

**MrdTimeClusters** `vector<vector<int>>`
* Obtain the information about digit ids of MRD clusters from `MrdTimeClusters`

**MrdDigitTimes** `vector<double>`
* Obtain information about the hit times of clustered MRD hits.

**MRDTriggertype** `string`
* Get the trigger type as determined by the loopback channel to classify the events into cosmic events and beam events.

**MRDLoopbackTDC** `map<string,int>`
* Get the TDC value of each loopback channel, the two keywords being `BeamLoopbackTDC` and `CosmicLoopbackTDC`.

## Configuration

`MRDLoopbackAnalysis` has the following configuration variables:

```
verbosity 1
OutputROOTFile MRDLoopbackAnalysis_RunXX
```
