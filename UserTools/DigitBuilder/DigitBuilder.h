/// This tool reads the raw data from the file and creates a DigitBuilder object
/// Jingbo Wang <jiowang@ucdavis.edu>

#ifndef DigitBuilder_H
#define DigitBuilder_H

#include <string>
#include <iostream>
#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

#include "Tool.h"

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
 	
 	/// \brief Push reco digits to ANNIEEvent
  ///
  /// It adds the vector of PMT and LAPPD digits to ANNIEEvent
 	void PushRecoDigits(bool savetodisk);
 	
 	/// \brief Reset digits
 	///
 	/// Clear digit list
 	void Reset();
 	
  int fverbosity=1;
	std::string fInputfile;
	unsigned long fNumEvents;
	
	/// contents of ANNIEEvent filled by LoadWCSim and LoadWCSimLAPPD
	std::string fMCFile;
	uint32_t fRunNumber;       ///< retrieved from MC file but simulations tend to only ever be run 0. 
	uint32_t fSubrunNumber;    ///< MC has no 'subrun', always 0
	uint32_t fEventNumber;     ///< flattens the 'event -> trigger' MC hierarchy
	uint64_t fMCEventNum;      ///< event number in MC file
	uint16_t fMCTriggernum;    ///< trigger number in MC file
	
	Geometry fGeometry;    ///< ANNIE Geometry
	std::map<ChannelKey,std::vector<Hit>>* fMCHits=nullptr;             ///< PMT hits
	std::map<ChannelKey,std::vector<LAPPDHit>>* fMCLAPPDHits=nullptr;   ///< LAPPD hits
	std::map<ChannelKey,std::vector<Hit>>* fTDCData=nullptr;            ///< MRD & veto hits
	TimeClass* fEventTime=nullptr;    ///< NDigits trigger time in ns from when the particles were generated
	
	/// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int fv_error=0;
	int fv_warning=1;
	int fv_message=2;
	int fv_debug=3;
	std::string flogmessage;
	int fget_ok;	
	
	/// Reconstructed information
	std::vector<RecoDigit*>* fDigitList;				///< Reconstructed Hits including both LAPPD hits and PMT hits
	
	/// \brief Clear reconstructed hits
  ///
  /// Fills the parameter name and appropriate parameter values into
  /// the parameter container, to be used in the fit
	void ClearDigitList() {fDigitList->clear();}
	
};


#endif
