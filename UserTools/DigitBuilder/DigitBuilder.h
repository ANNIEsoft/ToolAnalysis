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
// ROOT includes #include "TFile.h"
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
   bool BuildMCRecoDigit();
   bool BuildDataRecoDigit();	///> Same as BuildMRDRegoDigit, but for data. Uses clusters found by the ClusterFinder tool. 
	
  /// \brief Build PMT digits
  ///
  /// It adds PMT hits to the RecoDigit list
   bool BuildMCPMTRecoDigit();
   bool BuildDataPMTRecoDigit();   ///> Same as BuildMCPMTRecoDigit, but applicable for data.
 	
  /// \brief Build LAPPD digits
  ///
  /// It adds LAPPD hits to the RecoDigit list
   bool BuildMCLAPPDRecoDigit();


  /// \brief Push reco digits to ANNIEEvent
  ///
  /// It adds the vector of PMT and LAPPD digits to RecoEvent
  void PushRecoDigits(bool savetodisk);
  
  /// \brief Reset digits
  ///
  /// Clear digit list
  void Reset();
  
  /// \brief Read LAPPD ID File for LAPPDs to load digits from
  ///
  void ReadLAPPDIDFile();

  ///
  /// Fills the parameter name and appropriate parameter values into
  /// the parameter container, to be used in the fit
  void ClearDigitList() {fDigitList->clear();}
 	
  int verbosity=1;
  std::string fInputfile;
  unsigned long fNumEvents;
  
  std::vector<int> fLAPPDId; ///< selected LAPPDs
  std::string fPhotodetectorConfiguration; ///< "PMTs_Only", "LAPPDs_Only", "All_Detectors"
  bool fParametricModel;     ///< configures if PMTs hits for each event are accumulated into one hit per PMT
  bool fIsMC;     ///< Configure whether to load from MCHits or Hits in boost store 
  std::string  fLAPPDIDFile="none";
  double fDigitChargeThr;
  std::string path_chankeymap;
  std::string singlePEgains;

  Geometry* fGeometry=nullptr;    ///< ANNIE Geometry
  TRandom3 frand;  ///< Random number generator
  
  /// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;
  
  //Shifts needed for simulation package in use (in cm)
  //Defaults to values needed for WCSim MC data
  double xshift = 0.0;
  double yshift = 14.46469;
  double zshift = -168.1;

  ///RecoEvent information
  bool fEventCutStatus;
  
  /// Reconstructed information
  std::vector<RecoDigit>* fDigitList;				///< Reconstructed Hits including both LAPPD hits and PMT hits
  std::map<unsigned long,std::vector<MCHit>>* fMCPMTHits=nullptr;             ///< PMT hits
  std::map<unsigned long,std::vector<MCLAPPDHit>>* fMCLAPPDHits=nullptr;   ///< LAPPD hits
  std::map<unsigned long,std::vector<MCHit>>* fTDCData=nullptr;            ///< MRD & veto hits
  std::map<double,std::vector<Hit>>* m_all_clusters=nullptr;            ///< Clusters, from ClusterFinder tool
  std::map<double,std::vector<unsigned long>>* m_all_clusters_detkey=nullptr;         ///< Chankeys corresponding to clusters, from ClusterFinder tool

  std::map<unsigned long, double> pmt_gains;

  // retrieved from CStore, for mapping WCSim LAPPD IDs to unique detectorkey
  // Note: WCSim doesn't have "striplines", so while the LoadWCSim tool generates
  // the correct number of Channel (stripline) objects, all hits are on the 
  // first Channel (stripline) of the Detector (tile).
  std::map<unsigned long,int> detectorkey_to_lappdid;
  std::map<unsigned long,int> channelkey_to_pmtid;
  std::map<int,unsigned long> pmtid_to_channelkey;
};


#endif
