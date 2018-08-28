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


 private:
 	
	/// Clear reconstruction info.
 	void Reset();

 	/// \brief Event selection by MRD reconstructed information
 	///
 	/// Loop over all the MRC tracks. Find the track with the longest track
 	/// length. If the longest track is stoped inside the MRD, the event is 
 	/// selected
 	bool EventSelectionByMRDReco();
 	
 	/// \brief Event selection by fidicual volume
 	///
 	/// The selection is based on the true vertex position from MC. 
 	/// If the true vertex is inside the fidicual volume, the event 
 	/// is selected. 
 	/// The 
 	bool EventSelectionByMCTruthInfo();
 	
 	/// \brief Find true neutrino vertex
 	///
 	/// Loop over all MC particles and find the particle with highest energy. 
 	/// This particle is the primary muon. The muon start position, time and 
 	/// the muon direction are used to initise the true neutrino vertex 
 	RecoVertex* FindTrueVertexFromMC();
 	
 	/// \brief Save true neutrino vertex
 	///
 	/// Push true neutrino vertex to "RecoVertex"
 	/// \param[in] bool savetodisk: save object to disk if savetodisk=true
 	void PushTrueVertex(bool savetodisk);

	Geometry fGeometry;    ///< ANNIE Geometry
	RecoVertex* fMuonStartVertex = nullptr; 	 ///< true muon start vertex
	RecoVertex* fMuonStopVertex = nullptr; 	 ///< true muon stop vertex
	std::vector<MCParticle>* fMCParticles=nullptr;  ///< truth tracks

	//verbosity initialization
	int verbosity=1;

	std::string fInputfile;
	unsigned long fNumEvents;
	bool fMRDRecoCut = false;
	bool fMCTruthCut = false;
	bool fEventCutStatus;

	/// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;

};


#endif
