# Classification ToolChain configuration files

**********************
## Classification ToolChains ##
**********************

The Classification tools can be used to prepare classification variables for the training of a Machine Learning classifier or to generate the input for already trained classifiers to make predictions. The first ToolChain is called `PrepareClassificationVariables` and is an example toolchain for MC files, while the second Toolchain is called `ClassificationVarsData` and can be used on data files.

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
* `ClassificationVarsData`: Classification variables are calculated and saved in csv-files on data files. The calculated variables can later be loaded by trained ML classifiers to make predictions. Tools in this ToolChain:
  * `LoadGeometry`: Load ANNIE geometry configuration.
  * `LoadANNIEEvent`: Loads ANNIEEvent data file
  * `PhaseIIADCCalibrator`: Calibrates the baseline for all PMTs.
  * `PhaseIIADCHitFinder`: Searches for hits above the baseline for all the PMTs, saves them in a `Hits` map in the `ANNIEvent` BoostStore.
  * `ClusterFinder`: Searches for clusters of hits in the PMT hits found by the `PhaseIIADCHitFinder`.
  * `DigitBuilder`: Builds RecoDigit objects for the RecoEvent store.
  * `HitCleaner`: Hit Cleaning cuts to throw away noise hits.
  * `TimeClustering`: Find clusters of hits in MRD data.
  * `EventSelector`: Filter events based on custom cuts.
  * `CalcClassificationVars`: Calculate classification parameters, store them in the Classification BoostStore.
  * `StoreClassificationVars`: Save the classification parameters in a csv-file/ROOT-file.

************************
## CalcClassificationVars tool configuration ##
************************

The `CalcClassificationVars` tool primarily needs to know whether it is looking at MC or data. All classification variables are calculated and stored in the Classification BoostStore, while subsequent tools (`StoreClassificationVars`,custom python scripts for event classification) then select which variables are actually used in the classification process.

```
verbosity 2
UseMCTruth 0
IsData 0
```

************************
## StoreClassificationVars tool configuration ##
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
## EventClassification - ML scripts ##
************************

The `EventClassification` process currently relies on custom Machine Learning scripts both for training the classifiers and for applying the classification on new datasets. A more detailed description of the tools can be found at the repository https://github.com/mnieslony/MLAlgorithms.git. The primary scripts for doing the classification are also stored in ToolAnalysis in the directory `UserTools/EventClassification/do_classification*.py`.
