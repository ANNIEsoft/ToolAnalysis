#ifndef MrdPaddlePlot_H
#define MrdPaddlePlot_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TClonesArray.h"
#include "MRDSubEventClass.hh"      // a class for defining subevents
#include "MRDTrackClass.hh"         // a class for defining MRD tracks

class MrdPaddlePlot: public Tool {


 public:

	MrdPaddlePlot();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();

 private:

	// Variables stored in Config file
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	int verbose=1;
	std::string gdmlpath;
	bool saveimages;
	
	// Variables from ANNIEEVENT
	// ~~~~~~~~~~~~~~~~~~~~~~~~~
	std::string MCFile;      //-> currentfilestring
	uint32_t RunNumber;     // -> runnum     (keep as the type is different: TODO align types)
	uint32_t SubrunNumber;  // -> subrunnum  ( " " )
	uint32_t EventNumber;   // -> eventnum   ( " " )
	uint16_t MCTriggernum;  // -> triggernum ( " " )
	uint64_t MCEventNum;    // not yet in MRDTrackClass 
	
	// MRD TRACK DRAWING
	// ~~~~~~~~~~~~~~~~~~~~~~~~
	int numents; // running total of the number of times Execute gets called
	int numsubevs;
	int numtracksinev;
	int totnumtracks=0;
	int numstopped=0;
	int numpenetrated=0;
	int numsideexit=0;
	int numtankintercepts=0;
	int numtankmisses=0;
	
	TCanvas* gdmlcanv;
	TClonesArray* thesubeventarray;  // retrieve from track finder
	
	// Summary histograms on tracks found
	TH1F* hnumsubevs;
	TH1F* hnumtracks;
	TH1F* hrun;
	TH1F* hevent;
	TH1F* hmrdsubev;
	TH1F* htrigger;
	TH1F* hnumhclusters;
	TH1F* hnumvclusters;
	TH1F* hnumhcells;
	TH1F* hnumvcells;
	TH1F* hpaddleids;
	TH1F* hpaddleinlayeridsh;
	TH1F* hpaddleinlayeridsv;
	TH1D* hdigittimes;
	TH1F* hhangle;
	TH1F* hhangleerr;
	TH1F* hvangle;
	TH1F* hvangleerr;
	TH1F* htotangle;
	TH1F* htotangleerr;
	TH1F* henergyloss;
	TH1F* henergylosserr;
	TH1F* htracklength;
	TH1F* htrackpen;
	TH2F* htrackpenvseloss;
	TH2F* htracklenvseloss;
	TH3D* htrackstart;
	TH3D* htrackstop;
	TH3D* hpep;
	
	TCanvas c2;
	TCanvas c3;
	TCanvas c4;
	TCanvas c5;
	TCanvas c6;
	TCanvas c7;
	TCanvas c8;
	TCanvas c9;
	TCanvas c10;
	TCanvas c11;
	TCanvas c12;
	TCanvas c13;
	TCanvas c14;
	TCanvas c15;
	TCanvas c16;
	TCanvas c17;
	TCanvas c18;
	TCanvas c19;
	TCanvas c20;
	TCanvas c21;
	TCanvas c22;
	TCanvas c23;
	TCanvas c24;
	TCanvas c25;
	TCanvas c26;
	TCanvas c27;
	TCanvas c28;
	
};


#endif
