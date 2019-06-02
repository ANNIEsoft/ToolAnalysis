#include <PulseSimulation.h>

std::vector<std::string>* PulseSimulation::GetTemplateRunInfo(){
	// RunInformation tree in RAW file is essentially a map of strings.
	// It stores (at present) 11 entries, each with a 'key' in InfoTitle branch, 
	// and a corresponding xml object string in the InfoMessage branch.
	// the xml string may represent multiple variables:
	
	// see the end of this function for a slightly clearer presentation
	// this function returns a 'template' set of entries, taken from RAWDataR897S0p0.root
	static std::vector<std::string> TemplateInfoMessages{};
	if (TemplateInfoMessages.size()!=0) return &TemplateInfoMessages;
	
	std::string ToolChainVariables = "{\"Inline\":\"0\",\"Interactive\":\"0\",\"Remote\":\"1\",\"Status\":\"\", \"Tools_File\":\"configfiles/Hefty_Tools\",\"attempt_recover\":\"1\",\"error_level\":\"2\", \"log_local_path\":\"/data/logs/ANNIE_DAQ_log\",\"log_mode\":\"Local\",\"log_port\":\"24010\",\"log_service\":\"LogStore\", \"remote_port\":\"24002\",\"service_discovery_address\":\"239.192.1.1\",\"service_discovery_port\":\"5000\", \"service_kick_sec\":\"60\",\"service_name\":\"ANNIE_DAQ_Hefty\",\"service_publish_sec\":\"5\",\"verbose\":\"1\"}";
	
	std::string InputVariables = "{\"RunType\":\"6\"}";
	
	std::string PostgresVariables = "{\"RunNumber\":\"897\",\"RunType\":\"6\",\"StarTime\":\"1506987657494\",\"SubRunNumber\":\"0\", \"Verbose\":\"1\",\"dbname\":\"annie\",\"hostaddr1\":\"192.168.163.11\",\"hostaddr2\":\"192.168.163.12\",\"port\":\"5432\"}";
	
	std::string  SlackBotVariables = "{}";
	
	std::string  HVComs = "{\"msg_id\":\"1\",\"msg_type\":\"run\",\"run\":\"897\",\"subrun\":\"0\"}{\"msg_ack\": \"1\"}{\"msg_id\":\"1\",\"run\":\"success\"}";
	
	std::string TriggerVariables = "{\"AdvancedTriggerMode\":\"0\",\"BeamDelay\":\"12780\",\"BeamTriggerEnable\":\"1\",\"Ch1SelfTriggerEnable\":\"0\",\"CosmicTriggerEnable\":\"1\",\"DISABLE_TRIGGER_CARD\":\"0\",\"DelaySamples(10,0)\":\"250\",\"DelaySamples(10,1)\":\"250\",\"DelaySamples(10,2)\":\"250\",\"DelaySamples(10,3)\":\"250\",\"DelaySamples(11,0)\":\"250\",\"DelaySamples(11,1)\":\"250\",\"DelaySamples(11,2)\":\"250\",\"DelaySamples(11,3)\":\"250\",\"DelaySamples(13,0)\":\"250\",\"DelaySamples(13,1)\":\"250\",\"DelaySamples(13,2)\":\"250\",\"DelaySamples(13,3)\":\"250\",\"DelaySamples(14,0)\":\"250\",\"DelaySamples(14,1)\":\"250\",\"DelaySamples(14,2)\":\"250\",\"DelaySamples(14,3)\":\"250\",\"DelaySamples(15,0)\":\"250\",\"DelaySamples(15,1)\":\"250\",\"DelaySamples(15,2)\":\"250\",\"DelaySamples(15,3)\":\"250\",\"DelaySamples(16,0)\":\"250\",\"DelaySamples(16,1)\":\"250\",\"DelaySamples(16,2)\":\"250\",\"DelaySamples(16,3)\":\"250\",\"DelaySamples(18,0)\":\"250\",\"DelaySamples(18,1)\":\"250\",\"DelaySamples(18,2)\":\"250\",\"DelaySamples(18,3)\":\"250\",\"DelaySamples(19,0)\":\"250\",\"DelaySamples(19,1)\":\"250\",\"DelaySamples(19,2)\":\"250\",\"DelaySamples(19,3)\":\"250\",\"DelaySamples(20,0)\":\"250\",\"DelaySamples(20,1)\":\"250\",\"DelaySamples(20,2)\":\"250\",\"DelaySamples(20,3)\":\"250\",\"DelaySamples(21,0)\":\"250\",\"DelaySamples(21,1)\":\"250\",\"DelaySamples(21,2)\":\"250\",\"DelaySamples(21,3)\":\"250\",\"DelaySamples(3,0)\":\"250\",\"DelaySamples(3,1)\":\"250\",\"DelaySamples(3,2)\":\"250\",\"DelaySamples(3,3)\":\"250\",\"DelaySamples(4,0)\":\"250\",\"DelaySamples(4,1)\":\"250\",\"DelaySamples(4,2)\":\"250\",\"DelaySamples(4,3)\":\"250\",\"DelaySamples(5,0)\":\"250\",\"DelaySamples(5,1)\":\"250\",\"DelaySamples(5,2)\":\"250\",\"DelaySamples(5,3)\":\"250\",\"DelaySamples(6,0)\":\"250\",\"DelaySamples(6,1)\":\"250\",\"DelaySamples(6,2)\":\"250\",\"DelaySamples(6,3)\":\"250\",\"DelaySamples(8,0)\":\"250\",\"DelaySamples(8,1)\":\"250\",\"DelaySamples(8,2)\":\"250\",\"DelaySamples(8,3)\":\"250\",\"DelaySamples(9,0)\":\"250\",\"DelaySamples(9,1)\":\"250\",\"DelaySamples(9,2)\":\"250\",\"DelaySamples(9,3)\":\"250\",\"DownsampleMode\":\"0\",\"DownsampleOffset\":\"0\",\"ExternalTriggerEnable\":\"0\",\"LEDStartOnMinRate\":\"0\",\"LEDStartOnPeriodic\":\"0\",\"LEDWidths1\":\"32776\",\"MRDDelay\":\"12720\",\"MinRateTriggerEnable\":\"1\",\"MinRateTriggerPeriod\":\"20000000\",\"PeriodicTriggerEnable\":\"1\",\"SelfTrigger(10,0)\":\"0\",\"SelfTrigger(10,1)\":\"0\",\"SelfTrigger(10,2)\":\"0\",\"SelfTrigger(10,3)\":\"0\",\"SelfTrigger(11,0)\":\"0\",\"SelfTrigger(11,1)\":\"0\",\"SelfTrigger(11,2)\":\"0\",\"SelfTrigger(11,3)\":\"0\",\"SelfTrigger(13,0)\":\"0\",\"SelfTrigger(13,1)\":\"0\",\"SelfTrigger(13,2)\":\"0\",\"SelfTrigger(13,3)\":\"0\",\"SelfTrigger(14,0)\":\"0\",\"SelfTrigger(14,1)\":\"0\",\"SelfTrigger(14,2)\":\"0\",\"SelfTrigger(14,3)\":\"0\",\"SelfTrigger(15,0)\":\"0\",\"SelfTrigger(15,1)\":\"0\",\"SelfTrigger(15,2)\":\"0\",\"SelfTrigger(15,3)\":\"0\",\"SelfTrigger(16,0)\":\"0\",\"SelfTrigger(16,1)\":\"0\",\"SelfTrigger(16,2)\":\"0\",\"SelfTrigger(16,3)\":\"0\",\"SelfTrigger(18,0)\":\"0\",\"SelfTrigger(18,1)\":\"0\",\"SelfTrigger(18,2)\":\"0\",\"SelfTrigger(18,3)\":\"0\",\"SelfTrigger(19,0)\":\"0\",\"SelfTrigger(19,1)\":\"0\",\"SelfTrigger(19,2)\":\"0\",\"SelfTrigger(19,3)\":\"0\",\"SelfTrigger(20,0)\":\"0\",\"SelfTrigger(20,1)\":\"0\",\"SelfTrigger(20,2)\":\"0\",\"SelfTrigger(20,3)\":\"0\",\"SelfTrigger(21,0)\":\"0\",\"SelfTrigger(21,1)\":\"0\",\"SelfTrigger(21,2)\":\"0\",\"SelfTrigger(21,3)\":\"0\",\"SelfTrigger(3,0)\":\"0\",\"SelfTrigger(3,1)\":\"0\",\"SelfTrigger(3,2)\":\"0\",\"SelfTrigger(3,3)\":\"0\",\"SelfTrigger(4,0)\":\"0\",\"SelfTrigger(4,1)\":\"0\",\"SelfTrigger(4,2)\":\"0\",\"SelfTrigger(4,3)\":\"0\",\"SelfTrigger(5,0)\":\"0\",\"SelfTrigger(5,1)\":\"0\",\"SelfTrigger(5,2)\":\"0\",\"SelfTrigger(5,3)\":\"0\",\"SelfTrigger(6,0)\":\"0\",\"SelfTrigger(6,1)\":\"0\",\"SelfTrigger(6,2)\":\"0\",\"SelfTrigger(6,3)\":\"0\",\"SelfTrigger(8,0)\":\"0\",\"SelfTrigger(8,1)\":\"0\",\"SelfTrigger(8,2)\":\"0\",\"SelfTrigger(8,3)\":\"0\",\"SelfTrigger(9,0)\":\"0\",\"SelfTrigger(9,1)\":\"0\",\"SelfTrigger(9,2)\":\"0\",\"SelfTrigger(9,3)\":\"0\",\"SelfTriggerOut(18,0)\":\"1\",\"SelfTriggerOut(4,1)\":\"1\",\"Soft_Trigger\":\"0\",\"TotalSamples\":\"10000\",\"TriggerPeriod\":\"373536129\",\"TriggerSamples\":\"250\",\"TriggerThreshold(10,0)\":\"400\",\"TriggerThreshold(10,1)\":\"400\",\"TriggerThreshold(10,2)\":\"400\",\"TriggerThreshold(10,3)\":\"400\",\"TriggerThreshold(11,0)\":\"400\",\"TriggerThreshold(11,1)\":\"400\",\"TriggerThreshold(11,2)\":\"400\",\"TriggerThreshold(11,3)\":\"400\",\"TriggerThreshold(13,0)\":\"400\",\"TriggerThreshold(13,1)\":\"400\",\"TriggerThreshold(13,2)\":\"400\",\"TriggerThreshold(13,3)\":\"400\",\"TriggerThreshold(14,0)\":\"400\",\"TriggerThreshold(14,1)\":\"400\",\"TriggerThreshold(14,2)\":\"400\",\"TriggerThreshold(14,3)\":\"400\",\"TriggerThreshold(15,0)\":\"400\",\"TriggerThreshold(15,1)\":\"400\",\"TriggerThreshold(15,2)\":\"400\",\"TriggerThreshold(15,3)\":\"400\",\"TriggerThreshold(16,0)\":\"400\",\"TriggerThreshold(16,1)\":\"400\",\"TriggerThreshold(16,2)\":\"400\",\"TriggerThreshold(16,3)\":\"400\",\"TriggerThreshold(18,0)\":\"357\",\"TriggerThreshold(18,1)\":\"400\",\"TriggerThreshold(18,2)\":\"400\",\"TriggerThreshold(18,3)\":\"400\",\"TriggerThreshold(19,0)\":\"400\",\"TriggerThreshold(19,1)\":\"400\",\"TriggerThreshold(19,2)\":\"400\",\"TriggerThreshold(19,3)\":\"400\",\"TriggerThreshold(20,0)\":\"400\",\"TriggerThreshold(20,1)\":\"400\",\"TriggerThreshold(20,2)\":\"400\",\"TriggerThreshold(20,3)\":\"400\",\"TriggerThreshold(21,0)\":\"400\",\"TriggerThreshold(21,1)\":\"400\",\"TriggerThreshold(21,2)\":\"400\",\"TriggerThreshold(21,3)\":\"400\",\"TriggerThreshold(3,0)\":\"400\",\"TriggerThreshold(3,1)\":\"400\",\"TriggerThreshold(3,2)\":\"400\",\"TriggerThreshold(3,3)\":\"400\",\"TriggerThreshold(4,0)\":\"400\",\"TriggerThreshold(4,1)\":\"357\",\"TriggerThreshold(4,2)\":\"400\",\"TriggerThreshold(4,3)\":\"400\",\"TriggerThreshold(5,0)\":\"400\",\"TriggerThreshold(5,1)\":\"400\",\"TriggerThreshold(5,2)\":\"400\",\"TriggerThreshold(5,3)\":\"400\",\"TriggerThreshold(6,0)\":\"400\",\"TriggerThreshold(6,1)\":\"400\",\"TriggerThreshold(6,2)\":\"400\",\"TriggerThreshold(6,3)\":\"400\",\"TriggerThreshold(8,0)\":\"400\",\"TriggerThreshold(8,1)\":\"400\",\"TriggerThreshold(8,2)\":\"400\",\"TriggerThreshold(8,3)\":\"400\",\"TriggerThreshold(9,0)\":\"400\",\"TriggerThreshold(9,1)\":\"400\",\"TriggerThreshold(9,2)\":\"400\",\"TriggerThreshold(9,3)\":\"400\",\"VME_port\":\"24020\",\"VME_service_name\":\"VME_service\",\"WindowStartOnBeamDelay1\":\"1\",\"WindowStartOnCosmic\":\"1\",\"WindowTriggerEnable\":\"1\",\"WindowTriggerOnSelf\":\"1\",\"WindowWidth\":\"3200\",\"numVME\":\"1\",\"verbose\":\"1\"}";
	
	std::string  NetworkReceiveDataVariables = "{\"address\":\"192.168.163.36\",\"buffersize\":\"40000\",\"cards\":\"1\",\"channels\":\"4\",\"port\":\"24011\"}";
	
	std::string LoggerVariables = "{\"log_port\":\"24010\"}";
	
	std::string RootDataRecorderVariables = "{\"MRDEnable\":\"1\",\"OutputPath\":\"/data/output/\",\"TFileTTreeCap\":\"1\",\"TTreeEventCap\":\"6120\"}";
	
	std::string MonitoringVariables = "{\"MonitorLevel\":\"40\",\"OutputPath\":\"/data/monitoringplots/\"}";
	
	std::string MRDVariables = "{\"errorlevel\":\"0\",\"kick_time\":\"-1\",\"log_mode\":\"Interactive\",\"log_path\":\"./MRDlog\", \"log_port\":\"4567\",\"log_service\":\"test\",\"multicastaddress\":\"hh\",\"multicastport\":\"test\", \"remote_port\":\"24020\",\"toolsfile\":\"configfiles/MRD_tools\",\"verbose\":\"1\"}";
	
	TemplateInfoMessages = std::vector<std::string>{
	ToolChainVariables,
	InputVariables,
	PostgresVariables,
	SlackBotVariables,
	HVComs,
	TriggerVariables,
	NetworkReceiveDataVariables,
	LoggerVariables,
	RootDataRecorderVariables,
	MonitoringVariables,
	MRDVariables
	};
	
	return &TemplateInfoMessages;
}

// slightly more readable version with linebreaks and '...' sections put in
// where many values are duplicated:
/*
======> EVENT:0
 InfoTitle       = ToolChainVariables
 InfoMessage     = {"Inline":"0","Interactive":"0","Remote":"1","Status":"",
 "Tools_File":"configfiles/Hefty_Tools","attempt_recover":"1","error_level":"2",
 "log_local_path":"/data/logs/ANNIE_DAQ_log","log_mode":"Local","log_port":"24010","log_service":"LogStore",
 "remote_port":"24002","service_discovery_address":"239.192.1.1","service_discovery_port":"5000",
 "service_kick_sec":"60","service_name":"ANNIE_DAQ_Hefty","service_publish_sec":"5","verbose":"1"}
======> EVENT:1
 InfoTitle       = InputVariables
 InfoMessage     = {"RunType":"6"}
======> EVENT:2
 InfoTitle       = PostgresVariables
 InfoMessage     = {"RunNumber":"897","RunType":"6","StarTime":"1506987657494","SubRunNumber":"0",
 "Verbose":"1","dbname":"annie","hostaddr1":"192.168.163.11","hostaddr2":"192.168.163.12","port":"5432"}
======> EVENT:3
 InfoTitle       = SlackBotVariables
 InfoMessage     = {}
======> EVENT:4
 InfoTitle       = HVComs
 InfoMessage     = {"msg_id":"1","msg_type":"run","run":"897","subrun":"0"}{"msg_ack": "1"}{"msg_id":"1","run":"success"}
======> EVENT:5
 InfoTitle       = TriggerVariables
 InfoMessage     = {"AdvancedTriggerMode":"0","BeamDelay":"12780","BeamTriggerEnable":"1","Ch1SelfTriggerEnable":"0",
 "CosmicTriggerEnable":"1","DISABLE_TRIGGER_CARD":"0",
"DelaySamples(10,0)":"250",...,"DelaySamples(9,3)":"250",
"DownsampleMode":"0","DownsampleOffset":"0","ExternalTriggerEnable":"0","LEDStartOnMinRate":"0",
"LEDStartOnPeriodic":"0","LEDWidths1":"32776","MRDDelay":"12720","MinRateTriggerEnable":"1",
"MinRateTriggerPeriod":"20000000","PeriodicTriggerEnable":"1",
"SelfTrigger(10,0)":"0",...,"SelfTrigger(9,3)":"0",
"SelfTriggerOut(18,0)":"1","SelfTriggerOut(4,1)":"1",
"Soft_Trigger":"0","TotalSamples":"10000","TriggerPeriod":"373536129","TriggerSamples":"250",
"TriggerThreshold(10,0)":"400",...,"TriggerThreshold(9,3)":"400","VME_port":"24020",
"VME_service_name":"VME_service","WindowStartOnBeamDelay1":"1","WindowStartOnCosmic":"1",
"WindowTriggerEnable":"1","WindowTriggerOnSelf":"1","WindowWidth":"3200","numVME":"1","verbose":"1"}
======> EVENT:6
 InfoTitle       = NetworkReceiveDataVariables
 InfoMessage     = {"address":"192.168.163.36","buffersize":"40000","cards":"1","channels":"4","port":"24011"}
======> EVENT:7
 InfoTitle       = LoggerVariables
 InfoMessage     = {"log_port":"24010"}
======> EVENT:8
 InfoTitle       = RootDataRecorderVariables
 InfoMessage     = {"MRDEnable":"1","OutputPath":"/data/output/","TFileTTreeCap":"1","TTreeEventCap":"6120"}
======> EVENT:9
 InfoTitle       = MonitoringVariables
 InfoMessage     = {"MonitorLevel":"40","OutputPath":"/data/monitoringplots/"}
======> EVENT:10
 InfoTitle       = MRDVariables
 InfoMessage     = {"errorlevel":"0","kick_time":"-1","log_mode":"Interactive","log_path":"./MRDlog",
 "log_port":"4567","log_service":"test","multicastaddress":"hh","multicastport":"test",
 "remote_port":"24020","toolsfile":"configfiles/MRD_tools","verbose":"1"}
*/
