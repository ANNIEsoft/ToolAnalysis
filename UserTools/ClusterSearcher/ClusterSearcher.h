#ifndef CLUSTERSEARCHER_H
#define CLUSTERSEARCHER_H

#include <string>
#include <iostream>
#include <vector>

#include "Tool.h"
#include "RecoCluster.h"
#include "RecoClusterDigit.h"
#include "TString.h"

class ClusterSearcher: public Tool {

 public:

  ClusterSearcher();
  ~ClusterSearcher();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  
  typedef enum EFilterConfig {
    kNone  = 0,
    kPulseHeight = 1,
    kPulseHeightAndNeighbours = 2,
    kPulseHeightAndClusters = 3, 
    kPulseHeightAndTruthInfo = 4
  } FilterConfig_t;

  static ClusterSearcher* Instance();

  static void Config(int config);
  static void ClusterType(int ctype);
  static void PmtMinPulseHeight(double min);
  static void PmtNeighbourRadius(double radius);
  static void PmtNeighbourDigits(int digits);
  static void PmtClusterRadius(double radius);
  static void MinClusterDigits(int digits);
  static void PmtTimeWindowN(double windowN);
  static void PmtTimeWindowC(double windowC);

  void PrintParameters();

  void SetConfig(int config)               { fConfig = config; }
  void SetClusterMode(int cmode)		{fClusterMode = cmode;}
  void SetPmtMinPulseHeight(double min)       { fPmtMinPulseHeight = min; }
  void SetPmtNeighbourRadius(double radius)   { fPmtNeighbourRadius = radius; }
  void SetPmtNeighbourDigits(int digits)      { fPmtMinNeighbourDigits = digits; }
  void SetPmtClusterRadius(double radius)     { fPmtClusterRadius = radius; }
  void SetPmtTimeWindowNeighbours(double windowN)        { fPmtTimeWindowN = windowN; }
  void SetPmtTimeWindowClusters(double windowC)        { fPmtTimeWindowC = windowC; }
  
  void SetLappdMinPulseHeight(double min)       { fLappdMinPulseHeight = min; }
  void SetLappdNeighbourRadius(double radius)   { fLappdNeighbourRadius = radius; }
  void SetLappdNeighbourDigits(int digits)      { fLappdMinNeighbourDigits = digits; }
  void SetLappdClusterRadius(double radius)     { fLappdClusterRadius = radius; }
  void SetLappdTimeWindowNeighbours(double windowN)        { fLappdTimeWindowN = windowN; }
  void SetLappdTimeWindowClusters(double windowC)        { fLappdTimeWindowC = windowC; }
  void LoadConfigFile(string configfilename);
  void SetMinClusterDigits(int digits)        { fMinClusterDigits = digits; }
  

  std::vector<RecoDigit*>* Run(std::vector<RecoDigit*>* digitlist);
  std::vector<RecoDigit*>* ResetDigits(std::vector<RecoDigit*>* digitlist);
  std::vector<RecoDigit*>* SelectDigits(std::vector<RecoDigit*>* digitlist);
  void SelectDigits(std::vector<RecoDigit>* digitlist);
  std::vector<RecoDigit*>* SelectAll(std::vector<RecoDigit*>* digitlist);
  std::vector<RecoDigit*>* SelectByPulseHeight(std::vector<RecoDigit*>* digitlist);
  std::vector<RecoDigit*>* SelectByNeighbours(std::vector<RecoDigit*>* digitlist);
  std::vector<RecoDigit*>* SelectByClusters(std::vector<RecoDigit*>* digitlist);
  std::vector<RecoDigit*>* SelectByTruthInfo(std::vector<RecoDigit*>* digitlist); //use truth information. Only for testing the code
  std::vector<RecoCluster*>* RecoClusters(std::vector<RecoDigit*>* digitlist);
  


 private:
  void Reset();
  
  // running mode
  int fConfig;
  
  // clustering mode
  int fClusterMode;

  // cleaning parameters
  double fPmtMinPulseHeight;
  double fPmtNeighbourRadius;
  int    fPmtMinNeighbourDigits;
  double fPmtClusterRadius;
  double fPmtTimeWindowN;
  double fPmtTimeWindowC;
  double fPmtMinHitsPerCluster;
  
  double fLappdMinPulseHeight;
  double fLappdNeighbourRadius;
  int    fLappdMinNeighbourDigits;
  double fLappdClusterRadius;

  double fLappdTimeWindowN;
  double fLappdTimeWindowC;
  double fLappdMinHitsPerCluster;
  
  int    fMinClusterDigits;
  bool   fisMC;
  bool  fFirstRun;      //Likely temp; would be nice to have it automatically set.

  // p.e. conversion parameters
  std::map<int,unsigned long> pmt_tubeid_to_channelkey;
  std::map<unsigned long, double> pmt_gains;
  std::string singlePEgains;

  // Container for parameters
  std::map<std::string, double>* fClusteringParam = nullptr;

  // internal containers
  std::vector<Double_t> vNdigitsCluster;  
  std::vector<RecoClusterDigit*> vClusterDigitList;
  std::vector<RecoClusterDigit*> vClusterDigitCollection;

  // vectors of selected digitss
  std::vector<RecoDigit*>* fSelectAll;
  std::vector<RecoDigit*>* fSelectByPulseHeight;
  std::vector<RecoDigit*>* fSelectByNeighbours;
  std::vector<RecoDigit*>* fSelectByClusters;
  	
  // for test only
  std::vector<RecoDigit*>* fSelectByTruthInfo;

  // vectors of clusters
  std::vector<RecoCluster*>* fClusterList;
 
  // vector of clusters (accessible to the CStore)
  std::vector<RecoCluster*>* fRecoClusters = nullptr;  
  std::vector<RecoCluster*>* pre_RecoClusters = nullptr;
 	
  // true vertex
  RecoVertex* fTrueVertex = 0; 
  
  // digit list
  std::vector<RecoDigit>* fDigitList = 0;    
  
  // hit clustering status;
  bool fIsHitClusteringDone = false;    
  
  /// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity=1;
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
  
};


#endif
