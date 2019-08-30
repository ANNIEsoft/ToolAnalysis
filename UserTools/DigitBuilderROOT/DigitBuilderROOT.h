#ifndef DigitBuilderROOT_H
#define DigitBuilderROOT_H

#include <string>
#include <vector>
#include <iostream>

#include "Tool.h"
#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include "TChain.h"
#include "TMath.h"

#include "Position.h"
#include "Direction.h"
#include "RecoVertex.h"
#include "RecoDigit.h"

class DigitBuilderROOT: public Tool {


 public:

  DigitBuilderROOT();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

  void PushTrueVertex(bool savetodisk);
  void PushRecoDigits(bool savetodisk);
  void PushTrueWaterTrackLength(double WaterT);
  void PushTrueMRDTrackLength(double MRDT);
  
  void Reset();

 private:
  
  int verbosity=1;
  /// \brief verbosity levels: if 'verbosity' > this level, the message type will be logged.
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;
  
  std::string NtupleFile;
  TChain* P2Chain;
  int TotalEntries;
  int EntryNum = 0;

  std::vector<RecoDigit>* fDigitList;				///< Reconstructed Hits including both LAPPD hits and PMT hits
  
  RecoVertex* fMuonVertex = nullptr; 	 ///< true muon start vertex

  // Digits
  int fNhits = 0;
  std::vector<double> *fDigitX = nullptr;
  std::vector<double> *fDigitY = nullptr;
  std::vector<double> *fDigitZ = nullptr;
  std::vector<double> *fDigitT = nullptr;
  std::vector<double> *fDigitQ = nullptr;    
  std::vector<int> *fDigitType = nullptr;
  std::vector<int> *fDigitDetID = nullptr;
  	
  // True muon
  double fTrueVtxX;
  double fTrueVtxY;
  double fTrueVtxZ;
  double fTrueVtxTime;
  double fTrueDirX;
  double fTrueDirY;
  double fTrueDirZ;
  double fTrueMuonEnergy; 
  double fTrueTrackLengthInWater;
  double fTrueTrackLengthInMRD;

  int fPi0Count;
  int fPiPlusCount;
  int fPiMinusCount;
  int fK0Count;
  int fKPlusCount;
  int fKMinusCount;

  bool fEventCutStatus;
  int fEventStatusApplied;
  int fEventStatusFlagged;

  uint64_t fMCEventNum;
  uint16_t fMCTriggerNum;
  uint32_t fEventNumber; // will need to be tracked separately, since we flatten triggers
  uint32_t fRunNumber; 
  uint32_t fSubRunNumber; 
};


#endif
