# EventClassification #

EventClassification includes tools and scripts that can be used to classify events. The `C++`-tool itself is currently empty, but can be adapted to include additional methods for event classification (such as a Likelihood approach). 

Event Classification in ANNIE currently entails on the one hand Particle Identification to distinguish between electrons and muons, and on the other hand Ring Classification to distinguish between single- and multi-ring events. 

## ML - classifiers ##

The Machine Learning classifiers are based on classification algorithms such as Random Forests or the Multi-Layer-Perceptron. The repository which is used for their training can be found at the following link: https://github.com/mnieslony/MLAlgorithms.git
The classifiers use classification variables that are generated with the `CalcClassificationVars` and `StoreClassificationVars` tools in ToolAnalysis. The scripts which are used for training are replicated in this directory and have the names `train_classification_emu.py` and `train_classification_rings.py`. In order to make predictions with already trained classifiers, one can use the `do_classification_emu.py` and `do_classification_rings.py` scripts.

## Variable configuration ##

The classifiers can be trained and used with certain subsets of variables. Those subsets need to be defined in a `txt`-file in the `variable_config` directory. When adding additional subsets, one should make sure to use a unique name that is not already used for another subset.
