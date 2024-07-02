#include "Factory.h"

Tool* Factory(std::string tool) {
Tool* ret=0;

// if (tool=="Type") tool=new Type;
if (tool=="DummyTool") ret=new DummyTool;
if (tool=="MonitorReceive") ret=new MonitorReceive;
if (tool=="MonitorSimReceive") ret=new MonitorSimReceive;
if (tool=="MonitorMRDTime") ret=new MonitorMRDTime;
if (tool=="MonitorMRDLive") ret=new MonitorMRDLive;
if (tool=="MonitorMRDEventDisplay") ret=new MonitorMRDEventDisplay;
if (tool=="LoadGeometry") ret=new LoadGeometry;
if (tool=="MonitorTankTime") ret=new MonitorTankTime;
if (tool=="PMTDataDecoder") ret=new PMTDataDecoder;
if (tool=="TriggerDataDecoder") ret=new TriggerDataDecoder;
if (tool=="MonitorTrigger") ret=new MonitorTrigger;
if (tool=="MonitorDAQ") ret=new MonitorDAQ;
if (tool=="MonitorLAPPDSC") ret=new MonitorLAPPDSC;
if (tool=="MonitorLAPPDData") ret=new MonitorLAPPDData;
if (tool=="MonitorLAPPDDataSingle") ret=new MonitorLAPPDDataSingle;
if (tool=="ParseDataMonitoring") ret=new ParseDataMonitoring;

return ret;
}
