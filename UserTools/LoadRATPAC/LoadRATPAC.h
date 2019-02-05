#ifndef LoadRATPAC_H
#define LoadRATPAC_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "time.h"

// ROOT Dependencies
#include "TChain.h"
#include "TFile.h"
#include "TTree.h"
#include "TMath.h"

// RATEvent Library dependencies
#include "Run.hh"
#include "RunStore.hh"
#include "Root.hh"
#include "MC.hh"
#include "MCParticle.hh"
#include "MCTrackStep.hh"
#include "MCTrack.hh"
#include "Calib.hh"
#include "EV.hh"
#include "PMT.hh"
#include "PMTInfo.hh"
#include "LAPPDInfo.hh"
#include "DSReader.hh"
//#include "TrackNav.hh"
//#include "TrackCursor.hh"
//#include "TrackNode.hh"

//DataModel Dependencies
#include "Particle.h"
#include "Position.h"
#include "Direction.h"
#include "Hit.h"
#include "LAPPDHit.h"
#include "ChannelKey.h"
#include "TriggerClass.h"
#include "BeamStatus.h"

//Currently the specs in WCSim; need to have RAT-PAC's
//MRD implementation match when made
#include "MRDspecs.hh"


class LoadRATPAC: public Tool {


 public:

  LoadRATPAC();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  void LoadANNIEGeometry();
  void Reset();

 private:
	
  int verbose=1;

	uint32_t RunNumber;
	uint32_t SubrunNumber;

	uint32_t EventNumber; // will need to be tracked separately, since we flatten triggers
	uint64_t MCEventNum;
	uint16_t MCTriggernum;

	TimeClass* EventTime;
	BeamStatusClass* BeamStatus;
	
  uint64_t EventTimeNs;

  RAT::DSReader *dsReader;
  RAT::DS::Root   *ds;
 
  //TChain* tri;
  TChain* runtri;
  //RAT::TrackNav *nav;
  //RAT::TrackCursor *cursor;
  //RAT::TrackNode *node;
  


  // Input file to read RATPAC Data from
  std::string filename_ratpac;
  
  // time infos
  std::clock_t start;
  double duration;
  
  TFile *f_input, *file;
  TString save, log;
  
  // Variables
  Double_t init_time, fin_time;
  Double_t disp,deltat;
  Double_t charge_tot;
  Double_t distance_nCap_muTrack, distance_nCap_muStart;
  
  
  // TTrees, TChains and all that ROOT stuff
  TTree *nutri;
  TChain *chain;
  RAT::DS::Run *run;
  RAT::DS::PMTInfo *pmtInfo;
  RAT::DS::LAPPDInfo *lappdInfo;
  
  // Fixed size dimensions of array or collections stored in the TTree if any.
  static const Int_t kMaxprocResult = 4;
  static const Int_t kMaxmc = 1;
  static const Int_t kMaxcalib = 1;
  static const Int_t kMaxev = 1;
 
	std::vector<MCParticle>* MCParticles;
	std::map<ChannelKey,std::vector<Hit>>* TDCData;
	std::map<ChannelKey,std::vector<Hit>>* MCHits;
	std::map<ChannelKey,std::vector<LAPPDHit>>* MCLAPPDHits;
  
  // TVectors

  ULong64_t entry;
  ULong64_t NbEntries;
  
};


#endif
