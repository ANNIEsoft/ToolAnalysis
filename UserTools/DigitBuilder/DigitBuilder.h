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
#include <boost/algorithm/string.hpp>

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
   void CollectMCPMTHits();
 	
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

  void CollectHitsDirect(const std::vector<double>& hit_times, const std::vector<double>& hit_charges, const std::vector<int>& hit_parents, Position pos_reco, int PMTId, double window_ns = 5.0);
  double compute_median(const std::vector<double>& times);
 	
  int verbosity=1;
  std::string fInputfile;
  unsigned long fNumEvents;
  std::vector<int>* fHitLAPPDs;
  std::vector<int> fLAPPDId; ///< selected LAPPDs
  std::string fPhotodetectorConfiguration; ///< "PMTs_Only", "LAPPDs_Only", "All_Detectors"
  int fParametricModel;     ///< configures how PMTs hits for each event are accumulated into one hit per PMT 0: they are not, 1: median hit time, 2: first hit time, 3: average first 20% of hits, 4: average all hits, 5: highest charge hit time
  bool fIsMC;     ///< Configure whether to load from MCHits or Hits in boost store 
  bool fUseWaveforms; ///< Configure whether to use simulated waveforms.  Handled approximately like data, looks for Hits in boost store.
  std::string  fLAPPDIDFile="none";
  double fDigitChargeThr;
  std::string path_chankeymap;
  std::string singlePEgains;
  int striphit; //0 all LAPPD Hits as true; 1 average all times per strip; 2 use first time for each strip
  double MCPMTResolution = 0;

  bool fCollectHits;
  double fCollectionWindow=10.0;
  int AcqTimeWindow;
  double end_of_window_time_cut;
  double fWaveformshift = 0;   //shift all times to counter simulated waveform effects     TEMP if we can figure out better solution.

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
  std::map<unsigned long, std::vector<Hit>>* Hits = nullptr;
  std::map<unsigned long,std::vector<MCLAPPDHit>>* fMCLAPPDHits=nullptr;   ///< LAPPD hits
  std::map<unsigned long,std::vector<MCHit>>* fTDCData=nullptr;            ///< MRD & veto hits
  std::map<double,std::vector<Hit>>* m_all_clusters=nullptr;            ///< Clusters, from ClusterFinder tool - deprecated and to be removed
  std::map<double,std::vector<unsigned long>>* m_all_clusters_detkey=nullptr;         ///< Chankeys corresponding to clusters, from ClusterFinder tool - deprecated and to be removed

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
