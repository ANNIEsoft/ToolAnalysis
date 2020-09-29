# CalcClassificationVars

CalcClassificationVars calculates properties of events for classification purposes and stores them in the `Classification` BoostStore object. The properties can be accessed and read out by other tools to enable the training and application of ML classifier algorithms on `ANNIEEvent` data files.

## Data

CalcClassificationVars uses the `ANNIEEvent` store to read out the MRD data and the `RecoEvent` store to read out PMT and LAPPD data. It will only calculate the classification variables for events that passed the selection cut in the `EventSelector` tool. 

The CalcClassificationVars tool has the option to include Monte Carlo truth information by setting the `UseMCTruth` boolean in the configfile to `true`. If it is set to `false`, only information directly accessible from the PMT and LAPPD data is used to calculate the classification variables.

The calculated variables comprise angular properties such as the RMS/variance of the angular distribution of PMT/LAPPD hits, the total amount of charge seen, the fraction of PMT hits with a low charge, the fraction of PMT hits at early/late times, etc. The full list of variables that are calculated can be reviewed in the code of the CalcClassificationVars tool.

## Configuration

Describe any configuration variables for CalcClassificationVars.

```
verbosity 1     # verbosity output settings
UseMCTruth 0    # use true information from Monte Carlo to calculate the classification variables
```
