/* vim:set noexpandtab tabstop=4 wrap */
#ifndef LoadCCData_H
#define LoadCCData_H

#include <string>
#include <iostream>

#include "Tool.h"
//#include "PMTData.h"
//#include "TrigData.h"
//#include "RunInformation.h"
#include "MRDTree.h"

class LoadCCData: public Tool {

public:
	LoadCCData();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();

private:
	int verbosity=1;
	
	//TChain* PMTDataChain;
	//TChain* RunInformationChain;
	//TChain* TrigChain;
	TChain* MRDChain=nullptr;
	
	//PMTData* WaterPMTData;
	//RunInformation* RunInformationData;
	//TrigData* TriggerData;
	MRDTree* MRDData=nullptr;
	
	Long64_t ChainEntry;
	Long64_t NumEntries;
	
	// variables in the MRDData class
	UInt_t                     Trigger;
	UInt_t                     OutNumber;
	std::vector<std::string>*  Type=nullptr;
	std::vector<unsigned int>* Value=nullptr;
	std::vector<unsigned int>* Slot=nullptr;
	std::vector<unsigned int>* Channel=nullptr;
	ULong64_t                  TimeStamp;
	
	// ANNIEEvent variables
	std::map<ChannelKey,std::vector<std::vector<Hit>>>* TDCData=nullptr;
	
	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	int get_ok;
	
	uint32_t TubeIdFromSlotChannel(unsigned int slot, unsigned int channel);
	// map that converts TDC camac slot + channel to the corresponding MRD tube ID
	// tubeID is a 6-digit ID of XXYYZZ.
	static std::map<uint16_t,std::string> slotchantopmtid;
	
	std::vector<uint64_t> myvec;
};


#endif
