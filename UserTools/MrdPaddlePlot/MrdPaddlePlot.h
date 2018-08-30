#ifndef MrdPaddlePlot_H
#define MrdPaddlePlot_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TClonesArray.h"
#include "MRDSubEventClass.hh"      // a class for defining subevents
#include "MRDTrackClass.hh"         // a class for defining MRD tracks
// for drawing
#include "TApplication.h"
#include "TSystem.h"
//#include "TEveLine.h"
class TEveLine;

class MrdPaddlePlot: public Tool {


 public:

	MrdPaddlePlot();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();

 private:

	// Variables stored in Config file
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	int verbosity=1;
	std::string gdmlpath;
	bool saveimages;
	const char* plotDirectory;  // where to save images and plots
	
	// Variables from ANNIEEVENT
	// ~~~~~~~~~~~~~~~~~~~~~~~~~
	std::string MCFile;     //-> currentfilestring
	uint32_t RunNumber;     // -> runnum     (keep as the type is different: TODO align types)
	uint32_t SubrunNumber;  // -> subrunnum  ( " " )
	uint32_t EventNumber;   // -> eventnum   ( " " )
	uint16_t MCTriggernum;  // -> triggernum ( " " )
	uint64_t MCEventNum;    // not yet in MRDTrackClass 
	
	// Variables from MRDTracks BoostStore
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::vector<BoostStore>* theMrdTracks; // the actual tracks
	uint16_t MrdTrackID;
	uint16_t MrdSubEventID;
	bool InterceptsTank;
	double StartTime;
	Position StartVertex;
	Position StopVertex;
	double TrackAngle;
	double TrackAngleError;
	std::vector<int> LayersHit;
	double TrackLength;
	bool IsMrdPenetrating;
	bool IsMrdStopped;
	bool IsMrdSideExit;
	double PenetrationDepth;
	double HtrackFitChi2;
	double HtrackFitCov;
	double VtrackFitChi2;
	double VtrackFitCov;
	std::vector<int> PMTsHit;
	
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
	
	// we need to subtract the an offset to draw TEveLines over gdml
	Position buildingoffset;
	
	bool printTracks;
	bool printTClonesTracks;
	bool enableTApplication;
	bool drawPaddlePlot;
	bool drawGdmlOverlay;
	TApplication* mrdPaddlePlotApp;
	
	TCanvas* gdmlcanv;
	TCanvas* mrdTrackCanv;
	TClonesArray* thesubeventarray;  // retrieve from track finder
	std::vector<TEveLine*> thiseventstracks;
	
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
	
};


#endif
