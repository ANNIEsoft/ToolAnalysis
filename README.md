# ToolAnalysis

ToolAnalysis is a modular analysis program written for the ANNIE collaboration. It is based on ToolDAQ Application[1] which is an open source general DAQ Application template built using the modular ToolDAQ Framework core[2] to give separation between core and implementation code.

In each new shell environment, begin by running: 
```
. Setup.sh
```

To compile everything run:
```
make -j$(nproc)
```

To compile only the DataModel objects and those Tools needed for a toolchain 'MyToolChain' (where MyToolChain is the name of the appropriate directory in configfiles)`, run:
```
make -j$(nproc) MyToolChain
```

or for a ToolChain within a hierarchy, e.g. a toolchain described by `./configfiles/EnergyReco/Predict/ToolChainConfig`
```
make -j$(nproc) EnergyReco/Predict
```

Note that compilation can take some time. After the first initial build:
* If only files in the UserTools have been modified, only the modified Tools will be rebuilt, along with libMyTools.so and the Analyse application.
* If anything in the DataModel has been modified, those DataModel objects will need to be rebuilt, along with libDataModel.so, ALL Tools, libMyTool.so and Analyse. This will be considerably slower (building all Tools takes a long time), and is where building a specific toolchain can help speed things up.

Note that it is known that building and running on windows and mac is slower, due to filesystem differences. For best performance, run under linux. :)

****************************
#Concept
****************************

The main executable creates a ToolChain which is an object that holds Tools. Tools are added to the ToolChain and then the ToolChain can be told to Initialise Execute and Finalise each tool in the chain.

The ToolChain also holds a uesr defined DataModel which each tool has access too and can read ,update and modify. This is the method by which data is passed between Tools.

User Tools can be generated for use in the tool chain by incuding a Tool header. This can be done manually or by use of the newTool.sh script.

For more information consult the ToolDAQ doc.pdf

https://github.com/ToolDAQ/ToolDAQFramework/blob/master/ToolDAQ%20doc.pdf

Copyright (c) 2018 ANNIE collaboration 

[1] Benjamin Richards. (2018, November 11). ToolDAQ Application v2.1.2 (Version V2.1.2). Zenodo. http://doi.org/10.5281/zenodo.1482772

[2] Benajmin Richards. (2018, November 11). ToolDAQ Framework v2.1.1 (Version V2.1.1). Zenodo. http://doi.org/10.5281/zenodo.1482767
