#ifndef FindMrdTracks_H
#define FindMrdTracks_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "Geometry.h"
#include "ChannelKey.h"
#include "Hit.h"
#include "MRDSubEventClass.hh"      // a class for defining subevents
#include "MRDTrackClass.hh"         // a class for defining MRD tracks

#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TClonesArray.h"

class FindMrdTracks: public Tool {
	
public:
	FindMrdTracks();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();
	
	void StartNewFile();
	
private:
	
	// Variables stored in Config file
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	int verbosity=1;
	int minimumdigits;
	double maxsubeventduration;
	std::string outputdir="";
	bool writefile=false;
	
	// Variables retrieved from ANNIEEVENT
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::string MCFile;      //-> currentfilestring
	uint32_t RunNumber;     // -> runnum     (keep as the type is different: TODO align types)
	uint32_t SubrunNumber;  // -> subrunnum  ( " " )
	uint32_t EventNumber;   // -> eventnum   ( " " )
	uint16_t MCTriggernum;  // -> triggernum ( " " )
	uint64_t MCEventNum;    // not yet in MRDTrackClass 
	std::map<unsigned long,vector<Hit>>* TDCData;
	Geometry* geo=nullptr;  // for num MRD PMTs
	int numvetopmts=0;      // current method for separating veto / mrd pmts in TDCData
	
	// From the CStore, for converting WCSim TubeId t channelkey
	std::map<unsigned long,int> channelkey_to_mrdpmtid;
	
	// MRD TRACK RECONSTRUCTION
	// ~~~~~~~~~~~~~~~~~~~~~~~~
	// variables for file writing
	int runnum, subrunnum, eventnum, triggernum;
	std::string currentfilestring;  // raw / MC file being analyzed
	TFile* mrdtrackfile=0;
	TTree* mrdtree=0;  // mrd track reconstruction tree
	std::vector<double> mrddigittimesthisevent;
	std::vector<int> mrddigitpmtsthisevent;
	std::vector<double> mrddigitchargesthisevent;
	int nummrdsubeventsthisevent;
	int nummrdtracksthisevent;
	TBranch* mrdeventnumb;
	TBranch* mrdtriggernumb;
	TBranch* nummrdsubeventsthiseventb=0;
	TBranch* nummrdtracksthiseventb=0;
	TBranch* subeventsinthiseventb=0;
	TClonesArray* SubEventArray=0;
	
	// For saving to the BoostStore to pass between Tools
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::vector<BoostStore>* theMrdTracks;
	
	// For Debug Drawing Tracks During Looping
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	bool DrawTruthTracks;
	std::vector<MCParticle>* MCParticles=nullptr;
};


#endif
