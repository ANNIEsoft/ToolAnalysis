/* vim:set noexpandtab tabstop=4 wrap */
#ifndef LoadWCSimLAPPD_H
#define LoadWCSimLAPPD_H

#include <string>
#include <iostream>

#include "Tool.h"
//#include "LAPPDTree.h"
class LAPPDTree;

// for drawing
class TApplication;
class TCanvas;
class TPolyMarker3D;
class TH1D;

class LoadWCSimLAPPD: public Tool {


	public:

	LoadWCSimLAPPD();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();


	private:
	// config file variables
	int verbose=1;
	std::string MCFile;
	double Rinnerstruct;    // cm octagonal inner structure radius
	int triggeroffset;      // whether the historic 950ns offset was included
	
	// LAPPDTree variables
	TFile* file=nullptr;
	int FILE_VERSION;
	TTree* lappdtree=nullptr;
	LAPPDTree* LAPPDEntry=nullptr;  // tree event as a class object from MakeClass
	long NumEvents;
	
	// retrieved from Store, populated by LoadWCSim tool
	WCSimRootGeom* geo=nullptr;     // used to calculate global pos in files where it is not saved
	uint64_t MCEventNum;
	uint16_t MCTriggernum;
	int pretriggerwindow;
	int posttriggerwindow;
	TimeClass* EventTime=nullptr;
	
	// internal things to keep between loops
	std::vector<LAPPDHit> unassignedhits;  // lappd hits not yet assigned to a trigger
	
	bool DEBUG_DRAW_LAPPD_HITS;
	TApplication* lappdRootDrawApp;
	TCanvas* lappdRootCanvas;
	TPolyMarker3D* lappdhitshist;
	TH1D *digixpos, *digiypos, *digizpos;
	
	////////////////
	// things that will be filled into the store from this WCSim LAPPD file.
	// LAPPD files necessarily have a 1:1 mapping of events to WCSim files,
	// so we do not need to do any matching of run, subrun, etc.
	// Note though that these are the "RAW" hits (photons+noise), pre-digitizer-integration and
	// pre-trigger-selection (so no WCSim trigger association is included)
	std::map<ChannelKey,std::vector<LAPPDHit>>* MCLAPPDHits;
	std::map<ChannelKey,std::vector<LAPPDHit>> MCLAPPDHitsnonp;
	
};


#endif
