# DAQFramework

****************************
#Concept
****************************

The main executable creates a ToolChain which is an object that holds Tools. Tools are added to the ToolChain and then the ToolChain can be told to Initialise Execute and Finalise each tool in the chain.

The ToolChain also holds a uesr defined DataModel which each tool has access too and can read ,update and modify. This is the method by which data is passed between Tools.

User Tools can be generated for use in the tool chain by incuding a Tool header. This can be done manually or by use of the newTool.sh script.

For more information consult the ToolDAQ doc.pdf

https://github.com/ToolDAQ/ToolDAQFramework/blob/master/ToolDAQ%20doc.pdf