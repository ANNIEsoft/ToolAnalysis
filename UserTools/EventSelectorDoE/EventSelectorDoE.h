//This tool reads information from both the Monte Carlo and
//Event reconstruction and flags each event with the
//chosen flags in the EventSelectorDoE config file.

#ifndef EventSelectorDoE_H
#define EventSelectorDoE_H

#include <string>
#include <iostream>
#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

#include "Tool.h"
// ROOT includes
#include "TFile.h"
#include "TTree.h"
#include "TMath.h"
#include "ANNIEGeometry.h"
#include "TMath.h"

class EventSelectorDoE: public Tool {


 public:

  EventSelectorDoE();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:
 	
	/// Clear reconstruction info.
 	void Reset();

 	/// \brief Event selection by trigger number
 	///
 	/// The selection is based on the trigger number for the event
 	/// in the store "ANNIEEvent".  Events are selected if they have
 	/// an MCTriggernum == 0 (i.e. they are a prompt trigger) 
 	bool PromptTriggerCheck();
 	
 	/// \brief Event selection by fidicual volume
 	///
 	/// The selection is based on the true vertex position from MC. 
 	/// If the true vertex is inside the fidicual volume, the event 
 	/// is selected.  Also requires muon has nonzero track length
  /// in the MRD.
 	/// The 
 	bool EventSelectionByMCTruthInfo();
 	
 	/// \brief MC entry number
  uint64_t fMCEventNum;
  
  /// \brief trigger number
  uint16_t fMCTriggernum;
  
  /// \brief ANNIE event number
  uint32_t fEventNumber;

	Geometry fGeometry;    ///< ANNIE Geometry
	RecoVertex* fMuonVertex = nullptr; 	 ///< true muon start vertex
  double fTrackLengthInMrd;

	//verbosity initialization
	int verbosity=1;

	std::string fInputfile;
	bool fMCTruthCut = false;
  bool fPromptTrigOnly = true;
	bool fEventCutStatus;

	/// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;

};


#endif
