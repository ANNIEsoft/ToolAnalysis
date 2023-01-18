#ifndef PhaseIITreeMaker_H
#define PhaseIITreeMaker_H

#include <string>
#include <iostream>

#include "Tool.h"
// ROOT includes
#include "TApplication.h"
#include <Math/PxPyPzE4D.h>
#include <Math/LorentzVector.h>
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TMath.h"
#include "ADCPulse.h"
#include "Waveform.h"
#include "CalibratedADCWaveform.h"
#include "Hit.h"
#include "RecoDigit.h"
#include "ANNIEalgorithms.h"
#include "TimeClass.h"
#include "BeamStatus.h"

class PhaseIITreeMaker: public Tool {


 public:

  PhaseIITreeMaker();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

  /// \brief Load MCTruth information into ROOT file variables
  //
  /// Function loads the Muon MC Truth variables with the information
  /// Saved in the RecoEvent store.
  bool FillMCTruthInfo();
  bool FillTankRecoInfo();
  int LoadMRDTrackReco(int SubEventNumber);
  void LoadAllMRDHits(bool IsData);
  void FillRecoDebugInfo();
  void FillTruthRecoDiffInfo(bool got_mc, bool got_reco);

  /// \brief Summary of Reconstructed vertex
  void RecoSummary();
  void LoadTankClusterHits(std::vector<Hit> cluster_hits);
  void LoadTankClusterHitsMC(std::vector<MCHit> cluster_hits,std::vector<unsigned long> cluster_detkeys);
  bool LoadTankClusterClassifiers(double cluster_time);
  void LoadAllTankHits(bool IsData);
  void LoadSiPMHits();

 private:

  //General variables
  bool isData;
  bool hasGenie;

  std::map<int,std::string>* AuxChannelNumToTypeMap;
  std::map<int,double> ChannelKeyToSPEMap;

  std::map<int,unsigned long> pmtid_to_channelkey;
  std::map<unsigned long, int> channelkey_to_pmtid;
  std::map<unsigned long, int> channelkey_to_mrdpmtid;
  std::map<int, unsigned long> mrdpmtid_to_channelkey_data;
  std::map<unsigned long, int> channelkey_to_faccpmtid;
  std::map<int, unsigned long> faccpmtid_to_channelkey_data;

  /// \brief Reset all variables. 
  void ResetVariables();	
 	
  /// \brief ROOT TFile that will be used to store the output from this tool
  TFile* fOutput_tfile = nullptr;

  /// \brief TTree that will be used to store output
  TTree* fPhaseIITrigTree = nullptr;
  TTree* fPhaseIITankClusterTree = nullptr;
  TTree* fPhaseIIMRDClusterTree = nullptr;
 
  std::map<double,std::vector<Hit>>* m_all_clusters = nullptr;  
  std::map<double,std::vector<MCHit>>* m_all_clusters_MC = nullptr;  
  std::map<double,std::vector<unsigned long>>* m_all_clusters_detkeys = nullptr;  
  Geometry *geom = nullptr;

  /// \brief Branch variables
  /// \brief ANNIE event number
  uint32_t fEventNumber;
  uint32_t fRunNumber;
  uint32_t fSubrunNumber;
  uint64_t fEventTimeTank;
  TimeClass* mrd_timestamp=nullptr;
  TimeClass fEventTimeMRD;
  int fRunType;
  uint64_t fStartTime;
  int fNumEntries;
  ULong64_t fEventTimeTank_Tree;
  ULong64_t fEventTimeMRD_Tree;
  ULong64_t fStartTime_Tree;
  int fExtended;	//extended window variable, 0: no extended readout, 1: CC extended readout, 2: Non-CC extended readout
  double fPot;
  int fBeamok;	//1: beam is ok, 0: beam is not ok 

  // \brief Event Status flag masks
  int fEventStatusApplied;
  int fEventStatusFlagged;

  // SiPM Hit Info
  int fSiPM1NPulses;
  int fSiPM2NPulses;
  std::vector<double> fSiPMHitQ;
  std::vector<double> fSiPMHitT;
  std::vector<double> fSiPMHitAmplitude;
  std::vector<double> fSiPMNum;
  // Digits
  int fNHits = 0;
  std::vector<int> fIsFiltered;
  std::vector<double> fHitX;
  std::vector<double> fHitY;
  std::vector<double> fHitZ;
  std::vector<double> fHitT;
  std::vector<double> fHitQ; 
  std::vector<double> fHitPE; 
  std::vector<int> fHitType;
  std::vector<int> fHitDetID;
  std::vector<int> fHitChankey;
  std::vector<int> fHitChankeyMC;

  // MRD hit info 
  int fVetoHit;
  std::vector<double> fMRDHitT;
  std::vector<int> fMRDHitDetID;
  std::vector<int> fMRDHitChankey;
  std::vector<int> fMRDHitChankeyMC;
  std::vector<double> fFMVHitT;
  std::vector<int> fFMVHitDetID;
  std::vector<int> fFMVHitChankey;
  std::vector<int> fFMVHitChankeyMC;
  std::map<unsigned long,vector<Hit>>* TDCData=nullptr;
  std::map<unsigned long,vector<MCHit>>* TDCData_MC=nullptr;

  // ************** MRD Cluster level information ********** //
  int fMRDClusterNumber;
  int fMRDClusterHits;
  double fMRDClusterTime;
  double fMRDClusterTimeSigma;
  // Cluster properties
  std::vector<double> mrddigittimesthisevent;
  std::vector<int> mrddigitpmtsthisevent;
  std::vector<unsigned long> mrddigitchankeysthisevent;
  std::vector<std::vector<int>> MrdTimeClusters;
  
  // ************** Tank Cluster level information ********** //
  std::map<double,double> ClusterMaxPEs;
  std::map<double,Position> ClusterChargePoints;
  std::map<double,double> ClusterChargeBalances;
  int fClusterNumber;
  int fNumClusterTracks;
  int fClusterHits;
  double fClusterCharge;
  double fClusterTime;
  double fClusterPE;
  double fClusterMaxPE;
  double fClusterChargePointX;
  double fClusterChargePointY;
  double fClusterChargePointZ;
  double fClusterChargeBalance;
  std::vector<int> fADCWaveformChankeys; 
  std::vector<int> fADCWaveformSamples;  

  // ************ Muon reconstruction level information ******** //
  std::string MRDTriggertype;
  std::vector<BoostStore>* theMrdTracks;   // the reconstructed tracks
  int numtracksinev;
  std::vector<double> fMRDTrackAngle;
  std::vector<double> fMRDTrackAngleError;
  std::vector<double> fMRDPenetrationDepth;
  std::vector<double> fMRDTrackLength;
  std::vector<double> fMRDEntryPointRadius;
  std::vector<double> fMRDEnergyLoss;
  std::vector<double> fMRDEnergyLossError;
  std::vector<double> fMRDTrackStartX;
  std::vector<double> fMRDTrackStartY;
  std::vector<double> fMRDTrackStartZ;
  std::vector<double> fMRDTrackStopX;
  std::vector<double> fMRDTrackStopY;
  std::vector<double> fMRDTrackStopZ;
  std::vector<bool> fMRDSide;
  std::vector<bool> fMRDStop;
  std::vector<bool> fMRDThrough;

  // Trigger-level information
  std::map<std::string,bool> fDataStreams;
  int fTriggerword;
  int fTankMRDCoinc;
  int fNoVeto;
  int fHasTank;
  int fHasMRD;

  // ************ MC Truth Information **************** //
  uint64_t fMCEventNum;
  uint16_t fMCTriggerNum;
  int fiMCTriggerNum;
  // True muon
  double fTrueVtxX;
  double fTrueVtxY;
  double fTrueVtxZ;
  double fTrueVtxTime;
  double fTrueDirX;
  double fTrueDirY;
  double fTrueDirZ;
  double fTrueAngle;
  double fTruePhi;
  double fTrueMuonEnergy;
  int fTruePrimaryPdg;
  double fTrueTrackLengthInWater; 
  double fTrueTrackLengthInMRD; 
  std::vector<int> *fTruePrimaryPdgs = nullptr;
  std::vector<double> *fTrueNeutCapVtxX = nullptr;
  std::vector<double> *fTrueNeutCapVtxY = nullptr;
  std::vector<double> *fTrueNeutCapVtxZ = nullptr;
  std::vector<double> *fTrueNeutCapNucleus = nullptr;
  std::vector<double> *fTrueNeutCapTime = nullptr;
  std::vector<double> *fTrueNeutCapGammas = nullptr;
  std::vector<double> *fTrueNeutCapE = nullptr;
  std::vector<double>* fTrueNeutCapGammaE = nullptr;
  int fTrueMultiRing;

  //Genie information for event
  double fTrueNeutrinoEnergy;
  double fTrueQ2;
  int fTrueCC;
  int fTrueQEL;
  int fTrueRES;
  int fTrueDIS;
  int fTrueCOH;
  int fTrueMEC;
  int fTrueNeutrons;
  int fTrueProtons;

  // Pion and kaon counts for event
  int fPi0Count;
  int fPiPlusCount;
  int fPiMinusCount;
  int fK0Count;
  int fKPlusCount;
  int fKMinusCount;

  // **************** Full reco chain information ************* //
  //  seed vertices
  std::vector<double> fSeedVtxX;
  std::vector<double> fSeedVtxY;
  std::vector<double> fSeedVtxZ;
  std::vector<double> fSeedVtxFOM;
  double fSeedVtxTime;
  
  // Reco vertex
  // Point Position Vertex
  double fPointPosX;
  double fPointPosY;
  double fPointPosZ;
  double fPointPosTime;
  double fPointPosFOM;
  int fPointPosStatus;
  double fPointDirX;
  double fPointDirY;
  double fPointDirZ;
  double fPointDirTime;
  double fPointDirFOM;
  int fPointDirStatus;
  
  // Point Vertex Finder
  double fPointVtxPosX;
  double fPointVtxPosY;
  double fPointVtxPosZ;
  double fPointVtxTime;
  double fPointVtxDirX;
  double fPointVtxDirY;
  double fPointVtxDirZ;
  double fPointVtxFOM;
  int fPointVtxStatus;
 
  // Extended Vertex
  double fRecoVtxX;
  double fRecoVtxY;
  double fRecoVtxZ;
  double fRecoVtxTime;
  double fRecoDirX;
  double fRecoDirY;
  double fRecoDirZ;
  double fRecoVtxFOM;
  double fRecoAngle;
  double fRecoPhi;
  int fRecoStatus;
  
  // ************* Difference between MC and Truth *********** //
  double fDeltaVtxX; 
  double fDeltaVtxY;
  double fDeltaVtxZ;
  double fDeltaVtxR;
  double fDeltaVtxT;
  double fDeltaParallel;
  double fDeltaPerpendicular;
  double fDeltaAzimuth;
  double fDeltaZenith;  
  double fDeltaAngle;
  

  /// \brief Integer that determines the level of logging to perform
  int verbosity = 0;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;		
  int get_ok;	

  /// \Integer flags that control additional output to the PhaseIITree
  bool TankClusterProcessing = 0;
  bool MRDClusterProcessing = 0;
  bool TriggerProcessing = 1;
  bool TankHitInfo_fill = 0;
  bool MRDHitInfo_fill = 0;
  bool fillCleanEventsOnly = 0; //Only output events not flagged by EventSelector tool
  bool MCTruth_fill = 0; //Output the MC truth information
  bool TankReco_fill = 0;
  bool MRDReco_fill = 0;
  bool RecoDebug_fill = 0; //Outputs results of Reconstruction at each step (best fits, FOMs, etc.)
  bool muonTruthRecoDiff_fill = 0; //Output difference in tmuonruth and reconstructed values
  bool SiPMPulseInfo_fill = 0;
};


#endif
