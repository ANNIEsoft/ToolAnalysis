# ToolAnalysis

ToolAnalysis is a modular analysis program written for the ANNIE collaboration. It is based on ToolDAQ Application[1] which is an open source general DAQ Application template built using the modular ToolDAQ Framework core[2] to give separation between core and implementation code.

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
