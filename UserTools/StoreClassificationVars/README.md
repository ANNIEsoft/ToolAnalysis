# StoreClassificationVars

The `StoreClassificationVars` tool stores the classification variables computed by the `CalcClassificationVars` tool in csv-files and ROOT-files. The csv-files can be used for training classifiers in separate code repositories, while the ROOT-files are an additional way to access the classification data.


## Data

`StoreClassificationVars` accesses the classification variables stored in the `Classification` BoostStore. There are averaged classification variables (one number for each event) and PMT-wise classification variables (a vector of numbers for each event). The latter variables are denoted by the suffix "vector" after the variable name. The `README.md`-file of the `CalcClassificationVars` tool contains more information on the classification variables and how they are calculated.


## Configuration

The tool has the following configuration possibilities:

```
verbosity 5
Filename classification_test
SaveCSV 1
SaveROOT 1
VariableConfig Full
VariableConfigPath ./configfiles/Classification/PrepareClassificationTraining
IsData 0
```

It can be chosen whether to save the information in a csv-file/ROOT-file, furthermore a custom set of variables can be defined by specifying a `VariableConfig` name corresponding to the specific variable ensemble. To use a custom variable set, one needs to specify the variables to be included within that set in a special config file. The config file needs to be located in the `VariableConfigPath` directory and follow the filename nomenclature `VariableConfig_{your_config_name}.txt`. The `Full` and `Minimal` variable configurations can serve as an example for how to construct such a variable set. 
