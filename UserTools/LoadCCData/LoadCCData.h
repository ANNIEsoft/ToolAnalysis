/* vim:set noexpandtab tabstop=4 wrap */
#ifndef LoadCCData_H
#define LoadCCData_H

#include <string>
#include <iostream>
#include <fstream>
#include "Tool.h"
class PMTData;
class MRDTree;
//class TrigData;
//class RunInformation;

// for drawing
class TApplication;
class TCanvas;
class TH1D;

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
	
	Long64_t TDCChainEntry;
	Long64_t NumCCDataEntries;
	
	// variables in the MRDData class
	UInt_t                     Trigger;
	UInt_t                     OutNumber;
	std::vector<std::string>*  Type=nullptr;
	std::vector<unsigned int>* Value=nullptr;
	std::vector<unsigned int>* Slot=nullptr;
	std::vector<unsigned int>* Channel=nullptr;
	ULong64_t                  TimeStamp;
	
	// variables used for timestamp alignment
	bool useHeftyTimes;
	Long64_t ADCChainEntry;
	Long64_t NumADCEntries;
	TChain* ADCTimestampChain;
	uint64_t nextreadoutfirstminibufstart;
	std::vector<uint64_t> minibufTS=std::vector<uint64_t>{};      // minibuffer times taken from the alignment files
	std::vector<uint64_t> nextminibufTs=std::vector<uint64_t>{};  // next event's minibuffer times " " files
	uint64_t maxtimediff;
	
	// relevant variables in PMTData class
	PMTData* thePMTData=nullptr;
	unsigned long long LastSync;
	int SequenceID;
	int StartTimeSec;
	int StartTimeNSec;
	unsigned long long StartCount;
	int TriggerNumber;
	
	// alternatively variables for the HeftyReader class
	annie::HeftyTreeReader* theHeftyData = nullptr;
	
	// ANNIEEvent variables
	std::map<ChannelKey,std::vector<std::vector<Hit>>>* TDCData=nullptr;
	
	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	int get_ok;
	
	bool LoadPMTDataEntries();
	bool PerformMatching(std::vector<unsigned long long> currentminibufts);
	std::vector<uint64_t> ConvertTimeStamps(unsigned long long LastSync, int StartTimeSec, 
		int StartTimeNSec, unsigned long long StartCount, std::vector<unsigned long long> TriggerCounts);
	uint32_t TubeIdFromSlotChannel(unsigned int slot, unsigned int channel);
	// map that converts TDC camac slot + channel to the corresponding MRD tube ID
	// tubeID is a 6-digit ID of XXYYZZ.
	static std::map<uint16_t,std::string> slotchantopmtid;
	
	// for debug drawing
	bool DEBUG_DRAW_TDC_HITS;
	TApplication* tdcRootDrawApp;
	TCanvas* tdcRootCanvas;
	TH1D *hTDCHitTimes, *hTDCTimeDiffs;
	
	TFile* tdcDebugRootFileOut=nullptr;
	TTree* tdcDebugTreeOut=nullptr;
	UInt_t camacslot;
	UInt_t camacchannel;
	UInt_t mrdpmtxnum, mrdpmtynum, mrdpmtznum;
	Long64_t mrdtimeinreadout;
	UInt_t mrdreadoutindex;
	ULong64_t mrdreadouttime;
	UInt_t adcreadoutindex;
	UInt_t adcminibufferinreadout;
	UInt_t adcminibufferindex;
	ULong64_t adcminibuffertime;
	Long64_t tdcadctimediff;
	UInt_t tdcnumhitsthismb;
	UInt_t tdchitnum;
	
	std::ofstream debugtimesdump;
	uint32_t ExecuteIteration;
	
//	uint32_t oldest_adc_timestamp;
//	uint16_t oldest_adc_timestamp_set;
//	std::vector<std::valarray<ULong64_t>> ADC_timestamps_vector;  // this will store the ADC timestamps
//	int firstminibufnum;
//	int numminibuffers;
//	std::unordered_multimap<int,Hit> cachedmaches;  // temporary storage for holding hits
//	std::vector<uint64_t> myvec;
};


#endif
