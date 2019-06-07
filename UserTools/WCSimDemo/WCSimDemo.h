/* vim:set noexpandtab tabstop=4 wrap */
#ifndef WCSimDemo_H
#define WCSimDemo_H

#include <string>
#include <iostream>
#include <sstream>

#include "Tool.h"
#include "Particle.h"
#include "Hit.h"
#include "LAPPDHit.h"
#include "TriggerClass.h"
#include "TimeClass.h"

class WCSimDemo: public Tool {
	
	public:
	
	WCSimDemo();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();
	
	private:
	
	int verbosity=0;
	std::string inputfile;
	unsigned long NumEvents;
	
	// contents of ANNIEEvent filled by LoadWCSim and LoadWCSimLAPPD
	std::string MCFile;
	uint32_t RunNumber;       // retrieved from MC file but simulations tend to only ever be run 0. 
	uint32_t SubrunNumber;    // MC has no 'subrun', always 0
	uint32_t EventNumber;     // flattens the 'event -> trigger' MC hierarchy
	uint64_t MCEventNum;      // event number in MC file
	uint16_t MCTriggernum;    // trigger number in MC file
	
	std::vector<MCParticle>* MCParticles=nullptr;                      // truth tracks
	std::map<unsigned long,std::vector<Hit>>* MCHits=nullptr;             // PMT hits
	std::map<unsigned long,std::vector<LAPPDHit>>* MCLAPPDHits=nullptr;   // LAPPD hits
	std::map<unsigned long,std::vector<Hit>>* TDCData=nullptr;            // MRD & veto hits
	TimeClass* EventTime=nullptr;    // NDigits trigger time in ns from when the particles were generated
	Geometry* anniegeom=nullptr;
	
	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	int get_ok;
	
};


#endif
