# Classification ToolChain configuration files

**********************
# Classification ToolChains
**********************

The Classification tools can be used to either prepare variables for the training of a Machine Learning classifier or to load the pre-trained classifier and classify events within the RecoEvent toolchain. The ToolChain names for those two use cases are

* `PrepareClassificationVariables`: Classification variables are calculated and saved in csv-files, so they can be used to train ML classifiers in a separate repository. Tools in this ToolChain:
  * `LoadWCSim`: Loads simulation data associated to PMT hits.
  * `LoadWCSimLAPPD`: Loads simulation data associated to LAPPD hits.
  * `MCRecoEventLoader`: Loads MC-Truth information into the RecoEvent store.
  * `DigitBuilder`: Builds RecoDigit objects for the RecoEvent store.
  * `HitCleaner`: Hit Cleaning cuts to throw away noise hits.
  * `TimeClustering`: Find clusters of hits in MRD data.
  * `EventSelector`: Filter events based on custom cuts.
  * `CalcClassificationVars`: Calculate classification parameters, store them in the Classification BoostStore.
  * `StoreClassificationVars`: Save the classification parameters in a csv-file/ROOT-file.
* `EventClassification`: Classification variables are calculated, pre-trained models of ML classifiers are loaded and used to classify the event. Tools in this ToolChain:
  * `LoadWCSim`: Loads simulation data associated to PMT hits.
  * `LoadWCSimLAPPD`: Loads simulation data associated to LAPPD hits.
  * `MCRecoEventLoader`: Loads MC-Truth information into the RecoEvent store.
  * `DigitBuilder`: Builds RecoDigit objects for the RecoEvent store.
  * `HitCleaner`: Hit Cleaning cuts to throw away noise hits.
  * `TimeClustering`: Find clusters of hits in MRD data.
  * `EventSelector`: Filter events based on custom cuts.
  * `CalcClassificationVars`: Calculate classification parameters, store them in the Classification BoostStore.
  * `EventClassification`: Classify events based on the calculated classification parameters and loading a pre-trained classifier.

************************
# CalcClassificationVars tool configuration
************************

The `CalcClassificationVars` tool primarily needs to know whether it is looking at MC or data. All classification variables are calculated and stored in the Classification BoostStore, while subsequent tools (`StoreClassificationVars`,`EventClassification`) then select which variables are actually used in the classification process.

```
verbosity 2
UseMCTruth 0
IsData 0
```

************************
# StoreClassificationVars tool configuration
************************

The `StoreClassificationVars` tool enables the user to choose in which form the classification variables will be saved (csv-file/ROOT-file). Furthermore, a custom set of variables can be defined by specifying a `VariableConfig` name corresponding to the desired set of variables. To use a custom variable set, one needs to specify the variables to be included within that set in a special config file. The config file needs to be located in the `VariableConfigPath` directory and follow the filename nomenclature `VariableConfig_{your_config_name}.txt`. The `Full` and `Minimal` variable configurations can serve as an example for how to construct such a variable set. 

```
verbosity 5
Filename classification_test
SaveCSV 1
SaveROOT 1
VariableConfig Full
VariableConfigPath ./configfiles/Classification/PrepareClassificationTraining
IsData 0
```

************************
# EventClassification tool configuration
************************

The `EventClassification` tool has a lot of awesome configuration possiblities `TODO` insert them here `TODO`
