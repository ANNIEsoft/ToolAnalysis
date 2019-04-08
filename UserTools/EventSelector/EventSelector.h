//This tool reads information from both the Monte Carlo and
//Event reconstruction and flags each event with the
//chosen flags in the EventSelector config file.

#ifndef EventSelector_H
#define EventSelector_H

#include <string>
#include <iostream>
#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

#include "Tool.h"
// ROOT includes
#include "TFile.h"
#include "TTree.h"
#include "ANNIEGeometry.h"

class EventSelector: public Tool {


 public:

  EventSelector();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

  typedef enum EventFlags {
   kFlagNone         = 0x00, //0
   kFlagMCFV         = 0x01, //1
   kFlagMCMRD        = 0x02, //2
   kFlagMCPiK        = 0x04, //4
   kFlagRecoMRD      = 0x08, //8
   kFlagPromptTrig   = 0x10, //16
   kFlagNHit         = 0x20, //32
   kFlagRecoFV       = 0x30, //64
  } EventFlags_t;

 private:
 	
	/// Clear reconstruction info.
 	void Reset();

 	/// \brief Event selection by MRD reconstructed information
 	///
 	/// Loop over all the MRC tracks. Find the track with the longest track
 	/// length. If the longest track is stoped inside the MRD, the event is 
 	/// selected
 	bool EventSelectionByMRDReco();

 	/// \brief Event selection by trigger number
 	///
 	/// The selection is based on the trigger number for the event
 	/// in the store "ANNIEEvent".  Events are selected if they have
 	/// an MCTriggernum == 0 (i.e. they are a prompt trigger) 
 	bool PromptTriggerCheck();

 	/// \brief Event selection by number of digits hit
 	///
  /// Require a minimum amount of digits to be hit in the event.
  /// If the criteria is not met, the event is flagged.
 	bool NHitCountCheck(int NHitCount);

 	/// \brief Event selection by fidicual volume
 	///
 	/// The selection is based on the muon interaction point. 
 	/// If isMC is true, checks the Event using muon truth info. 
    /// If False, the reconstructed vertex is used. 
 	bool EventSelectionByFV(bool isMC);

 	/// \brief Event selection by Muon MRD stop position
  /////
 	/// The selection is based on the true vertex stop position from MC. 
 	/// If the true muon vertex stops inside the MRD, the event 
 	/// is selected. 
 	/// The 
 	bool EventSelectionByMCTruthMRD();

 	/// \brief Event selection by Pion Kaon count
  /////
 	/// This event selection criteria requires that no pions or 
 	/// kaons are parent particles in the event.  This will help
  /// Select the CC0Pi events when testing reconstruction.
 	bool EventSelectionNoPiK();

 	/// \brief MC entry number
  uint64_t fMCEventNum;
  
  /// \brief trigger number
  uint16_t fMCTriggernum;
  
  /// \brief ANNIE event number
  uint32_t fEventNumber;

  // \brief Event Status bitwords
  int fEventApplied; //Integer indicates what event cleaning flags were checked for the event
  int fEventFlagged; //Integer indicates what evt. cleaning flags the event was flagged with

	Geometry fGeometry;    ///< ANNIE Geometry
	RecoVertex* fMuonStartVertex = nullptr; 	 ///< true muon start vertex
	RecoVertex* fMuonStopVertex = nullptr; 	 ///< true muon stop vertex
	std::vector<RecoDigit>* fDigitList;				///< Reconstructed Hits including both LAPPD hits and PMT hits
	RecoVertex* fRecoVertex = nullptr; 	 ///< Reconstructed Vertex 

	//verbosity initialization
	int verbosity=1;

	std::string fInputfile;
	bool fMRDRecoCut = false;
	bool fMCFVCut = false;
	bool fRecoFVCut = false;
	bool fMCMRDCut = false;
	bool fMCPiKCut = false;
  bool fNHitCut = true;
  bool fPromptTrigOnly = true;
	bool fEventCutStatus;


    bool fSaveStatusToStore = true;
	/// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;

};


#endif
