#ifndef HITCLEANER_H
#define HITCLEANER_H

#include <string>
#include <iostream>
#include <vector>

#include "Tool.h"
#include "RecoCluster.h"
#include "RecoClusterDigit.h"
#include "TString.h"

class HitCleaner: public Tool {

 public:

  HitCleaner();
  ~HitCleaner();
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

  static HitCleaner* Instance();

  static void Config(int config);
  static void PmtMinPulseHeight(double min);
  static void PmtNeighbourRadius(double radius);
  static void PmtNeighbourDigits(int digits);
  static void PmtClusterRadius(double radius);
  static void MinClusterDigits(int digits);
  static void PmtTimeWindowN(double windowN);
  static void PmtTimeWindowC(double windowC);

  void PrintParameters();

  void SetConfig(int config)               { fConfig = config; }
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
  std::vector<RecoDigit*>* FilterDigits(std::vector<RecoDigit*>* digitlist);
  std::vector<RecoDigit*>* FilterAll(std::vector<RecoDigit*>* digitlist);
  std::vector<RecoDigit*>* FilterByPulseHeight(std::vector<RecoDigit*>* digitlist);
  std::vector<RecoDigit*>* FilterByNeighbours(std::vector<RecoDigit*>* digitlist);
  std::vector<RecoDigit*>* FilterByClusters(std::vector<RecoDigit*>* digitlist);
  std::vector<RecoDigit*>* FilterByTruthInfo(std::vector<RecoDigit*>* digitlist); //use truth information. Only for testing the code
  std::vector<RecoCluster*>* RecoClusters(std::vector<RecoDigit*>* digitlist);


 private:
  void Reset();
  
  // running mode
  int fConfig;

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

  // p.e. conversion parameters
  std::map<int,unsigned long> pmt_tubeid_to_channelkey;
  std::map<unsigned long, double> pmt_gains;
  std::string singlePEgains;

  // Container for parameters
  std::map<std::string, double>* fHitCleaningParam = nullptr;

  // internal containers
  std::vector<Double_t> vNdigitsCluster;  
  std::vector<RecoClusterDigit*> vClusterDigitList;
  std::vector<RecoClusterDigit*> vClusterDigitCollection;

  // vectors of filtered digitss
  std::vector<RecoDigit*>* fFilterAll;
  std::vector<RecoDigit*>* fFilterByPulseHeight;
  std::vector<RecoDigit*>* fFilterByNeighbours;
  std::vector<RecoDigit*>* fFilterByClusters;
  	
  // for test only
  std::vector<RecoDigit*>* fFilterByTruthInfo;

  // vectors of clusters
  std::vector<RecoCluster*>* fClusterList;
 
  // vector of clusters (accessible to the CStore)
  std::vector<RecoCluster*>* fHitCleaningClusters = nullptr;  
 	
  // true vertex
  RecoVertex* fTrueVertex = 0; 
  
  // digit list
  std::vector<RecoDigit>* fDigitList = 0;    
  
  // hit cleaning status;
  bool fIsHitCleaningDone = false;    
  
  /// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity=1;
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
  
};


#endif
