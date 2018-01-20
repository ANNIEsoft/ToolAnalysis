#ifndef FindMrdTracks_H
#define FindMrdTracks_H

#include <string>
#include <iostream>

#include "Tool.h"
//#include "ANNIEEvent.h"  // ANNIEEvent is a BoostStore not a class
#include "Geometry.h"
#include "ChannelKey.h"
#include "TimeClass.h"
#include "Hit.h"
#include "MRDSubEventClass.hh"      // a class for defining subevents
#include "MRDTrackClass.hh"         // a class for defining MRD tracks

#include "TFile.h"
//#include "TChain.h"
#include "TTree.h"
//#include "TBranch.h"
#include "TClonesArray.h"

class FindMrdTracks: public Tool {
	
public:
	FindMrdTracks();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();
	
	void StartNewFile();
	
private:
	//ANNIEEvent* annieevent=nullptr; // used for retrieving the current event
	// things to pull from the ANNIEEvent BoostStore
	int runnum, subrunnum, eventnum, triggernum;
	std::string currentfilestring;  // raw / MC file being analyzed
	std::map<ChannelKey,vector<Hit>>* TDCData;
	Geometry* geo=nullptr;
	int numvetopmts=0;              // current method for separating veto / mrd pmts in TDCData
	
	// MRD TRACK RECONSTRUCTION
	// ~~~~~~~~~~~~~~~~~~~~~~~~
	// variables for file writing
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
	
	// Variables stored in Config file
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	int verbose;
	int minimumdigits;
	double maxsubeventduration;
	std::string outputdir="";
	bool writefile=false;
};


#endif
