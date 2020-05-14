#ifndef FindMrdTracks_H
#define FindMrdTracks_H

#include <string>
#include <iostream>
#include <fstream>

#include "Tool.h"
#include "Geometry.h"
#include "Hit.h"
#include "MRDSubEventClass.hh"      // a class for defining subevents
#include "MRDTrackClass.hh"         // a class for defining MRD tracks

#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TClonesArray.h"
#include "TObjectTable.h"

// for drawing
class TApplication;
class TCanvas;
class TH1D;

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
	std::string outputdir="";
	bool writefile=false;
	std::string outputfile;
	bool triggertype_selection;
	std::string triggertype;
	bool isData;
	
	// Variables retrieved from ANNIEEVENT
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::string MCFile;      //-> currentfilestring
	uint32_t RunNumber;     // -> runnum     (keep as the type is different: TODO align types)
	uint32_t SubrunNumber;  // -> subrunnum  ( " " )
	uint32_t EventNumber;   // -> eventnum   ( " " )
	uint16_t MCTriggernum;  // -> triggernum ( " " )
	uint64_t MCEventNum;    // not yet in MRDTrackClass 
	Geometry* geo=nullptr;  // for num MRD PMTs
	int numvetopmts=0;      // current method for separating veto / mrd pmts in TDCData
	std::string file_chankeymap;
	
	// From the CStore, for converting WCSim TubeId to channelkey
	std::map<int,unsigned long> mrd_tubeid_to_channelkey;
	
	// Store information regarding MRD Time clusters
	std::vector<std::vector<int>> MrdTimeClusters;
	std::string MRDTriggertype;
	
	
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
	
	bool MakeMrdDigitTimePlot=false;
	TApplication* rootTApp=nullptr;
	TCanvas* findMrdRootCanvas=nullptr;
	TH1D* mrddigitts=nullptr;
	Double_t canvwidth;
	Double_t canvheight;
	
	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int verbosity=1;
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	int get_ok;
};


#endif
