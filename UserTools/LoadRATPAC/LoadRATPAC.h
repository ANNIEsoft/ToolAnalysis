#ifndef LoadRATPAC_H
#define LoadRATPAC_H

#include <string>
#include <iostream>
#include <cstdint>

#include "Tool.h"
#include "time.h"

// ROOT Dependencies
#include "TChain.h"
#include "TFile.h"
#include "TVector3.h"
#include "TTree.h"
#include "TMath.h"
#include "TMatrixD.h"

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
#include "TriggerClass.h"
#include "BeamStatus.h"
#include "Geometry.h"
#include "Detector.h"

//Currently the specs in WCSim; need to have RAT-PAC's
//MRD implementation match when made
#include "MRDspecs.hh"

namespace RC{
	//PMTs
	constexpr int ADC_CHANNELS_PER_CARD=4;
	constexpr int ADC_CARDS_PER_CRATE=20;
	constexpr int MT_CHANNELS_PER_CARD=4;
	constexpr int MT_CARDS_PER_CRATE=20;
	//LAPPDs
	constexpr int ACDC_CHANNELS_PER_CARD=30;
	constexpr int ACDC_CARDS_PER_CRATE=20;
	constexpr int ACC_CHANNELS_PER_CARD=8;
	constexpr int ACC_CARDS_PER_CRATE=20;
	//TDCs
	//constexpr int TDC_CHANNELS_PER_CARD=32;
	//constexpr int TDC_CARDS_PER_CRATE=6;
	//HV
	constexpr int CAEN_HV_CHANNELS_PER_CARD=16;
	constexpr int CAEN_HV_CARDS_PER_CRATE=10;
	//constexpr int LECROY_HV_CHANNELS_PER_CARD=16;
	//constexpr int LECROY_HV_CARDS_PER_CRATE=16;
	constexpr int LAPPD_HV_CHANNELS_PER_CARD=4; // XXX ??? XXX
	constexpr int LAPPD_HV_CARDS_PER_CRATE=10;  // XXX ??? XXX
}


class LoadRATPAC: public Tool {


 public:

  LoadRATPAC();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  void LoadANNIEGeometry();
  void Reset();

 private:

  TMatrixD Rotateatob(TVector3 a, TVector3 b);
	uint32_t RunNumber;
	uint32_t SubrunNumber;

	uint32_t EventNumber; // will need to be tracked separately, since we flatten triggers
	uint64_t MCEventNum;
	uint16_t MCTriggernum;

	TimeClass* EventTime;
	BeamStatusClass* BeamStatus;
	
  int EventTimeNs;

  RAT::DSReader *dsReader;
  RAT::DS::Root   *ds;
 
  //TChain* tri;
  TChain* runtri;
  //RAT::TrackNav *nav;
  //RAT::TrackCursor *cursor;
  //RAT::TrackNode *node;
  


  // Input file to read RATPAC Data from
  std::string filename_ratpac;
	std::string logmessage;
  
  // time infos
  std::clock_t start;
  double duration;
  
  // TTrees, TChains and all that ROOT stuff
  TTree *nutri;
  TChain *chain;
  RAT::DS::Run *run;
  RAT::DS::PMTInfo *pmtInfo;
  RAT::DS::LAPPDInfo *lappdInfo;

	int LappdNumStrips;           // number of Channels per LAPPD
	double LappdStripLength;      // [mm] for calculating relative x position for dual-ended readout
	double LappdStripSeparation;  // [mm] for calculating relative y position of each stripline

	std::vector<MCParticle>* MCParticles;
	std::map<unsigned long,std::vector<MCHit>>* MCHits;
	std::map<unsigned long,std::vector<MCLAPPDHit>>* MCLAPPDHits;
  
  ULong64_t entry;
  ULong64_t NbEntries;

	//verbosity initialization
	int verbosity=1;

  Geometry* anniegeom;

	// For constructing ToolChain Geometry
	//////////////////////////////////////
	std::map<int,unsigned long> lappd_tubeid_to_detectorkey;
	std::map<int,unsigned long> pmt_tubeid_to_channelkey;
	std::map<int,unsigned long> lappd_tubeid_to_channelkey;
	std::map<int,unsigned long> mrd_tubeid_to_channelkey;
	std::map<int,unsigned long> facc_tubeid_to_channelkey;
	// inverse
	std::map<unsigned long,int> detectorkey_to_lappdid;
	std::map<unsigned long,int> channelkey_to_pmtid;
	std::map<unsigned long,int> channelkey_to_lappdid;
	std::map<unsigned long,int> channelkey_to_mrdpmtid;
	std::map<unsigned long,int> channelkey_to_faccpmtid;

  /// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;

  //Position shifts needed for simulation package used
  double xtankcenter = 0.0;
  double ytankcenter = 133.0;
  double ztankcenter = -1724.0;
};


#endif
