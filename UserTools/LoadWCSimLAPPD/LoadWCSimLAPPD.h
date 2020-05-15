/* vim:set noexpandtab tabstop=4 wrap */
#ifndef LoadWCSimLAPPD_H
#define LoadWCSimLAPPD_H

#include <string>
#include <iostream>
#include <WCSimRootGeom.hh>
#include "Tool.h"
#include "TTree.h"
#include "TFile.h"

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
	int verbosity=1;
	std::string MCFile;
	double Rinnerstruct;    // cm octagonal inner structure radius
	
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
	
	// converting between LAPPDID and channelkey
	Geometry* anniegeom=nullptr;
	std::map<int,unsigned long> lappd_tubeid_to_detectorkey;
	
	// internal things to keep between loops
	std::vector<MCLAPPDHit> unassignedhits;  // lappd hits not yet assigned to a trigger
	
	bool DEBUG_DRAW_LAPPD_HITS;
	TApplication* rootTApp;
	TCanvas* lappdRootCanvas;
	TPolyMarker3D* lappdhitshist;
	TH1D *digixpos, *digiypos, *digizpos, *digits;
	
	////////////////
	// things that will be filled into the store from this WCSim LAPPD file.
	// LAPPD files necessarily have a 1:1 mapping of events to WCSim files,
	// so we do not need to do any matching of run, subrun, etc.
	// Note though that these are the "RAW" hits (photons+noise), pre-digitizer-integration and
	// pre-trigger-selection (so no WCSim trigger association is included)
	std::map<unsigned long,std::vector<MCLAPPDHit>>* MCLAPPDHits;
	std::map<int,int>* TrackId_to_MCParticleIndex=nullptr; // maps WCSim trackId to index in MCParticles
	
	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	int get_ok;
	
};


#endif
