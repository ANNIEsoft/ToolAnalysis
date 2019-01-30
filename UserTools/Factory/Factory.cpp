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
if (tool=="LAPPDParseACC") ret=new LAPPDParseACC;
if (tool=="LAPPDFindPeak") ret=new LAPPDFindPeak;
if (tool=="LAPPDSave") ret=new LAPPDSave;
if (tool=="LAPPDSim") ret=new LAPPDSim;
if (tool=="GenerateHits") ret=new GenerateHits;
if (tool=="LAPPDcfd") ret=new LAPPDcfd;
if (tool=="NeutronStudyReadSandbox") ret=new NeutronStudyReadSandbox;
if (tool=="NeutronStudyPMCS") ret=new NeutronStudyPMCS;
if (tool=="NeutronStudyWriteTree") ret=new NeutronStudyWriteTree;
if (tool=="RawLoader") ret=new RawLoader;
if (tool=="LAPPDSaveROOT") ret=new LAPPDSaveROOT;
if (tool=="LAPPDFilter") ret=new LAPPDFilter;
if (tool=="LAPPDIntegratePulse") ret=new LAPPDIntegratePulse;
if (tool=="MonitorReceive") ret=new MonitorReceive;
  if (tool=="MonitorSimReceive") ret=new MonitorSimReceive;
return ret;
}
