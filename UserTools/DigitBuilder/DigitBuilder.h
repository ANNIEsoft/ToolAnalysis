/// This tool reads the raw data from the file and creates a DigitBuilder object
/// Jingbo Wang <jiowang@ucdavis.edu>

#ifndef DigitBuilder_H
#define DigitBuilder_H

#include <string>
#include <iostream>
#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>
#include <TRandom3.h>

#include "Tool.h"
// ROOT includes
#include "TFile.h"
#include "TTree.h"
#include "ANNIEGeometry.h"
#include "Detector.h"

class DigitBuilder: public Tool {

 public:

  DigitBuilder();
  ~DigitBuilder();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  static DigitBuilder* Instance();

 private:
 	/// \brief Build reconstructed object in ANNIEEvent
  ///
  /// It loops over all PMT and LAPPD hits, adds hits to a RecoDigit vector
  /// It also creates empty vertex and ring vectors
  ///
  /// \param[in] bool usetruth: buld event from MC simulation if usetruth=1
 	bool BuildRecoDigit();
 	
 	/// \brief Build PMT digits
  ///
  /// It adds PMT hits to the RecoDigit list
 	bool BuildPMTRecoDigit();
 	
 	/// \brief Build LAPPD digits
  ///
  /// It adds LAPPD hits to the RecoDigit list
 	bool BuildLAPPDRecoDigit();

 	/// \brief Find true neutrino vertex
 	///
 	/// Loop over all MC particles and find the particle with highest energy. 
 	/// This particle is the primary muon. The muon start position, time and 
 	/// the muon direction are used to initise the true neutrino vertex 
 	void FindTrueVertexFromMC();

 	/// \brief Find PionKaon Count 
 	///
 	/// Loop over all MC particles and find any particles with PDG codes
  /// Consistent with Pions or Kaons of any charges. Racks up a count
  /// of the number of each type of particle
 	
  void FindPionKaonCountFromMC();

 	/// \brief Save true neutrino vertex
 	///
 	/// Push true muon vertex to "RecoVertex"
 	/// \param[in] bool savetodisk: save object to disk if savetodisk=true
 	void PushTrueVertex(bool savetodisk);

 	/// \brief Save true neutrino vertex
 	///
 	/// Push true muon stop vertex to "RecoVertex"
 	/// \param[in] bool savetodisk: save object to disk if savetodisk=true
 	void PushTrueStopVertex(bool savetodisk);

 	/// \brief Push reco digits to ANNIEEvent
  ///
  /// It adds the vector of PMT and LAPPD digits to ANNIEEvent
 	void PushRecoDigits(bool savetodisk);
 	
 	/// \brief Reset digits
 	///
 	/// Clear digit list
 	void Reset();
 	
  ///
  /// Fills the parameter name and appropriate parameter values into
  /// the parameter container, to be used in the fit
	void ClearDigitList() {fDigitList->clear();}
 	
  int verbosity=1;
	std::string fInputfile;
	unsigned long fNumEvents;
	
	/// \brief contents of ANNIEEvent filled by LoadWCSim and LoadWCSimLAPPD
	std::string fMCFile;
	uint32_t fRunNumber;       ///< retrieved from MC file but simulations tend to only ever be run 0. 
	uint32_t fSubrunNumber;    ///< MC has no 'subrun', always 0
	uint32_t fEventNumber;     ///< flattens the 'event -> trigger' MC hierarchy
	uint64_t fMCEventNum;      ///< event number in MC file
	std::vector<int> fLAPPDId; ///< selected LAPPDs
	std::string fPhotodetectorConfiguration; ///< "PMTs_Only", "LAPPDs_Only", "All_Detectors"
	bool fParametricModel;     ///< configures if PMTs hits for each event are accumulated into one hit per PMT
  bool fGetPiKInfo = false;
	
  Geometry* fGeometry=nullptr;    ///< ANNIE Geometry
	std::map<unsigned long,std::vector<Hit>>* fMCHits=nullptr;             ///< PMT hits
	std::map<unsigned long,std::vector<LAPPDHit>>* fMCLAPPDHits=nullptr;   ///< LAPPD hits
	std::map<unsigned long,std::vector<Hit>>* fTDCData=nullptr;            ///< MRD & veto hits
	TRandom3 frand;  ///< Random number generator
	
	/// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	
	///RecoEvent information
	bool fEventCutStatus;
	/// Reconstructed information
	std::vector<RecoDigit>* fDigitList;				///< Reconstructed Hits including both LAPPD hits and PMT hits
	RecoVertex* fMuonStartVertex = nullptr; 	 ///< true muon start vertex
	RecoVertex* fMuonStopVertex = nullptr; 	 ///< true muon stop vertex
	std::vector<MCParticle>* fMCParticles=nullptr;  ///< truth tracks

	// retrieved from CStore, for mapping WCSim LAPPD IDs to unique detectorkey
	// Note: WCSim doesn't have "striplines", so while the LoadWCSim tool generates
	// the correct number of Channel (stripline) objects, all hits are on the 
	// first Channel (stripline) of the Detector (tile).
	std::map<unsigned long,int> detectorkey_to_lappdid;
	std::map<unsigned long,int> channelkey_to_pmtid;
	
};


#endif
