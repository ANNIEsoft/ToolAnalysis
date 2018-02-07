#include "../Unity.cpp"

Tool* Factory(std::string tool){
Tool* ret=0;

// if (tool=="Type") tool=new Type;
if (tool=="DummyTool") ret=new DummyTool;
if (tool=="ExampleGenerateData") ret=new ExampleGenerateData;
if (tool=="ExampleSaveStore") ret=new ExampleSaveStore;
if (tool=="ExampleSaveRoot") ret=new ExampleSaveRoot;
if (tool=="ExampleloadStore") ret=new ExampleloadStore;
if (tool=="ExamplePrintData") ret=new ExamplePrintData;
if (tool=="ExampleLoadRoot") ret=new ExampleLoadRoot;
if (tool=="PythonScript") ret=new PythonScript;
if (tool=="LAPPDParseScope") ret=new LAPPDParseScope;
if (tool=="LAPPDFindPeak") ret=new LAPPDFindPeak;
if (tool=="LAPPDSave") ret=new LAPPDSave;
if (tool=="LAPPDSim") ret=new LAPPDSim;
return ret;
}
