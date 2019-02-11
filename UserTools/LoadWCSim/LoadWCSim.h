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
#include "Detector.h"
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
	int use_smeared_digit_time;   // digit_time = (T): first photon smeared time, (F): first photon true time
	// WCSim variables
	TFile* file;
	TTree* wcsimtree;
	wcsimT* WCSimEntry; // from makeclass
	WCSimRootTrigger* atrigt, *atrigm, *atrigv;
	WCSimRootGeom* wcsimrootgeom;
	WCSimRootOptions* wcsimrootopts;
	int FILE_VERSION;   // WCSim version
	
	long NumEvents;

	int numtankpmts;
	int numlappds;
	int nummrdpmts;

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

  std::map<unsigned long, Detector>* tanklappds;
  std::map<unsigned long, Detector>* tankpmts;
  std::map<unsigned long, Detector>* mrdpmts;
  std::map<unsigned long, Detector>* vetopmts;

	// currently used to separate Veto/MRD PMTs
	int numvetopmts;

};


#endif
