/* vim:set noexpandtab tabstop=4 wrap */
#ifndef PrintANNIEEvent_H
#define PrintANNIEEvent_H

#include <string>
#include <iostream>
#include <sstream>

#include "Tool.h"

class PrintANNIEEvent: public Tool {
	
	public:
	
	PrintANNIEEvent();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();
	
	private:
	
	int verbose;
	std::string inputfile;
	unsigned long NumEvents;
	
	// contents of ANNIEEVENT
	uint32_t RunNumber;
	uint32_t SubrunNumber;
	uint32_t EventNumber;
	std::vector<Particle>* MCParticles=nullptr;
	std::vector<Particle>* RecoParticles=nullptr;
	std::map<ChannelKey,std::vector<Hit>>* MCHits=nullptr;
	std::map<ChannelKey,std::vector<Hit>>* TDCData=nullptr;
	std::map<ChannelKey,std::vector<Waveform<uint16_t>>>* RawADCData=nullptr;
	std::map<ChannelKey,std::vector<Waveform<uint16_t>>>* RawLAPPDData=nullptr;
	std::map<ChannelKey,std::vector<Waveform<double>>>* CalibratedADCData=nullptr;
	std::map<ChannelKey,std::vector<Waveform<double>>>* CalibratedLAPPDData=nullptr;
	std::vector<TriggerClass>* TriggerData=nullptr;
	bool MCFlag;
	TimeClass* EventTime=nullptr;
	uint64_t MCEventNum;
	uint16_t MCTriggernum;
	std::string MCFile;
	BeamStatusClass* BeamStatus=nullptr;
	
	std::stringstream logmessage;
};

#endif
