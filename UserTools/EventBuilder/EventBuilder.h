/// This tool reads the raw data from the file and creates a RecoEvent object
/// Jingbo Wang <jiowang@ucdavis.edu>

#ifndef EventBuilder_H
#define EventBuilder_H

#include <string>
#include <iostream>
#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

#include "Tool.h"

class EventBuilder: public Tool {


 public:

  EventBuilder();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  static EventBuilder* Instance();

 private:
  int fverbosity=1;
	std::string fInputfile;
	unsigned long fNumEvents;
	
	// contents of ANNIEEvent filled by LoadWCSim and LoadWCSimLAPPD
	std::string fMCFile;
	uint32_t fRunNumber;       // retrieved from MC file but simulations tend to only ever be run 0. 
	uint32_t fSubrunNumber;    // MC has no 'subrun', always 0
	uint32_t fEventNumber;     // flattens the 'event -> trigger' MC hierarchy
	uint64_t fMCEventNum;      // event number in MC file
	uint16_t fMCTriggernum;    // trigger number in MC file
	
	std::vector<MCParticle>* fMCParticles=nullptr;                      // truth tracks
	std::map<ChannelKey,std::vector<Hit>>* fMCHits=nullptr;             // PMT hits
	std::map<ChannelKey,std::vector<LAPPDHit>>* fMCLAPPDHits=nullptr;   // LAPPD hits
	std::map<ChannelKey,std::vector<Hit>>* fTDCData=nullptr;            // MRD & veto hits
	TimeClass* fEventTime=nullptr;    // NDigits trigger time in ns from when the particles were generated
	
	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int fv_error=0;
	int fv_warning=1;
	int fv_message=2;
	int fv_debug=3;
	std::string fLogmessage;
	int fget_ok;

};


#endif
