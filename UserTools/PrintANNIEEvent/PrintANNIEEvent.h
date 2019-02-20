/* vim:set noexpandtab tabstop=4 wrap */
#ifndef PrintANNIEEvent_H
#define PrintANNIEEvent_H

#include <string>
#include <iostream>
#include <sstream>

#include "Tool.h"
#include "Particle.h"
#include "Waveform.h"
#include "Hit.h"
#include "LAPPDHit.h"
#include "TriggerClass.h"
#include "TimeClass.h"
#include "BeamStatus.h"

class PrintANNIEEvent: public Tool {
	
	public:
	
	PrintANNIEEvent();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();
	
	private:
	
	int verbose=1;
	int get_ok;
	std::string inputfile;
	unsigned long NumEvents;
	
	// contents of ANNIEEvent
	uint32_t RunNumber;
	uint32_t SubrunNumber;
	uint32_t EventNumber;
	std::vector<MCParticle>* MCParticles=nullptr;
	std::vector<Particle>* RecoParticles=nullptr;
	std::map<unsigned long,std::vector<Hit>>* MCHits=nullptr;
	std::map<unsigned long,std::vector<LAPPDHit>>* MCLAPPDHits=nullptr;
	std::map<unsigned long,std::vector<Hit>>* TDCData=nullptr;
	std::map<unsigned long,std::vector<Waveform<uint16_t>>>* RawADCData=nullptr;
	std::map<unsigned long,std::vector<Waveform<uint16_t>>>* RawLAPPDData=nullptr;
	std::map<unsigned long,std::vector<Waveform<double>>>* CalibratedADCData=nullptr;
	std::map<unsigned long,std::vector<Waveform<double>>>* CalibratedLAPPDData=nullptr;
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
