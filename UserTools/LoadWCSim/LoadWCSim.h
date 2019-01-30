/* vim:set noexpandtab tabstop=4 wrap */
#ifndef LoadWCSim_H
#define LoadWCSim_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TFile.h"
#include "TTree.h"
#include "wcsimT.h"
#include "Particle.h"
#include "Hit.h"
#include "Waveform.h"
#include "TriggerClass.h"
#include "Geometry.h"
#include "MRDspecs.hh"
#include "ChannelKey.h"
#include "BeamStatus.h"

class LoadWCSim: public Tool {

public:

	LoadWCSim();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();

private:

	int verbose=1;
	int HistoricTriggeroffset;
	// WCSim variables
	TFile* file;
	TTree* wcsimtree;
	wcsimT* WCSimEntry; // from makeclass
	WCSimRootTrigger* atrigt, *atrigm, *atrigv;
	WCSimRootGeom* wcsimrootgeom;
	WCSimRootOptions* wcsimrootopts;
	
	long NumEvents;
	
	////////////////
	// things that will be filled into the store from this WCSim file.
	// note: filling everything in the right format is a complex process;
	// just do a simple filling here: this will be properly handled by the conversion
	// from WCSim to Raw and the proper RawReader Tools
	// bool MCFlag=true; 
	std::string MCFile;
	uint64_t MCEventNum;
	uint16_t MCTriggernum;
	uint32_t RunNumber;
	uint32_t SubrunNumber;
	uint32_t EventNumber; // will need to be tracked separately, since we flatten triggers
	TimeClass* EventTime;
	uint64_t EventTimeNs;
	std::vector<MCParticle>* MCParticles;
	std::map<ChannelKey,std::vector<Hit>>* TDCData;
	std::map<ChannelKey,std::vector<Hit>>* MCHits;
	std::vector<TriggerClass>* TriggerData;
	BeamStatusClass* BeamStatus;
	
	// currently used to separate Veto/MRD PMTs
	int numvetopmts;

};


#endif
