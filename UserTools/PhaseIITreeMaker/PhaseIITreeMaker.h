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
#include "PsecData.h"
#include "LAPPDPulse.h"
#include "LAPPDHit.h"

#include "GenieInfo.h"
#include "CLHEP/Random/RandGaussQ.h"
#include "CLHEP/Random/JamesRandom.h"
#include "Framework/Conventions/KineVar.h"
#include "Framework/EventGen/EventRecord.h"
#include "Framework/Interaction/Interaction.h"
#include "Framework/Interaction/Kinematics.h"
//#include "Framework/Messenger/Messenger.h"
#include "Framework/Utils/AppInit.h"
#include <Tools/Flux/GSimpleNtpFlux.h>
#include <Tools/Flux/GNuMIFlux.h>
#include <Framework/GHEP/GHepUtils.h>               // neut reaction codes
#include <Framework/ParticleData/PDGLibrary.h>
#include <Framework/ParticleData/PDGCodes.h>
#include <Framework/Ntuple/NtpMCEventRecord.h>
#include <Framework/Ntuple/NtpMCTreeHeader.h>
#include <Framework/Conventions/Constants.h>
#include <Framework/GHEP/GHepParticle.h>
#include <Framework/GHEP/GHepStatus.h>
#include <TParticlePDG.h>
#include "TChain.h"
#include "TVector3.h"
#include "TLorentzVector.h"

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
  void FillSimpleRecoInfo();
  void FillRingCountingInfo();
  void FillWeightInfo();
  void FillTruthRecoDiffInfo(bool got_mc, bool got_reco);
  void LoadDigitHits();

  /// \brief Summary of Reconstructed vertex
  void RecoSummary();
  void LoadTankClusterHits(std::vector<Hit> cluster_hits);
  void LoadTankClusterHitsMC(std::vector<MCHit> cluster_hits,std::vector<unsigned long> cluster_detkeys);
  bool LoadTankClusterClassifiers(double cluster_time);
  bool LoadBNBtimingMC(double cluster_time);
  void LoadAllTankHits(bool IsData, bool MCWaveform);
  void LoadSiPMHits();
  void LoadLAPPDData();
  void FillLAPPDData();

  void FillLAPPDHit();
  void FillLAPPDPulse();

 private:

  //General variables
  bool isData;
  bool MCWaveform;
  bool ApplyDeadMask;
  bool hasGenie;
  bool hasBNBtimingMC;

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

  /// \brief Copy the current cMrdMlReco Store contract into tree buffers.
  /// This is publication only: no ML selection or reconstruction is repeated here.
  bool LoadMrdMLReco();
  void ResetMrdMLReco();
  bool WriteMrdMLMetadata();
 	
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
  // Digits (Hits)
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
  
  //Digits
  int fNDigitsPMTs = 0;
  int fNDigitsLAPPDs = 0;
  std::vector<double> fdigitX;
  std::vector<double> fdigitY;
  std::vector<double> fdigitZ;
  std::vector<double> fdigitT;
  

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

  // ************** MC BNB Spill Structure ************* //
  std::map<double,double> bunchTimes;
  double fbunchTimes;

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

  //Weights
  std::map<std::string, std::vector<double>> fxsec_weights;
  std::map<std::string, std::vector<double>> fflux_weights;
  std::vector<double> fAll0;
  std::vector<double> fAll1;
  std::vector<double> fAll2;
  std::vector<double> fAll3;
  std::vector<double> fAll4;
  std::vector<double> fAll5;
  std::vector<double> fAxFFCCQEshape;
  std::vector<double> fDecayAngMEC;
  std::vector<double> fNormCCCOH;
  std::vector<double> fNorm_NCCOH;
  std::vector<double> fRPA_CCQE;
  std::vector<double> fRootinoFix;
  std::vector<double> fThetaDelta2NRad;
  std::vector<double> fTheta_Delta2Npi;
  std::vector<double> fTunedCentralValue;
  std::vector<double> fVecFFCCQEshape;
  std::vector<double> fXSecShape_CCMEC;
  std::vector<double> fpiplus;
  std::vector<double> fpiminus;
  std::vector<double> fkplus;
  std::vector<double> fkzero;
  std::vector<double> fkminus;
  std::vector<double> fhorncurrent;
  std::vector<double> fpioninexsec;
  std::vector<double> fpionqexsec;
  std::vector<double> fpiontotxsec;
  std::vector<double> fexpskin;
  std::vector<double> fnucleoninexsec;
  std::vector<double> fnucleonqexsec;
  std::vector<double> fnucleontotxsec;

  //Genie information for event
  double fTrueNeutrinoEnergy;
  int fTrueNuPDG;
  double fTrueNeutrinoMomentum_X;
  double fTrueNeutrinoMomentum_Y;
  double fTrueNeutrinoMomentum_Z;
  double fTrueNuIntxVtx_X;
  double fTrueNuIntxVtx_Y;
  double fTrueNuIntxVtx_Z;
  double fTrueNuIntxVtx_T;
  double fTrueFSLVtx_X;
  double fTrueFSLVtx_Y;
  double fTrueFSLVtx_Z;
  double fTrueFSLMomentum_X;
  double fTrueFSLMomentum_Y;
  double fTrueFSLMomentum_Z;
  double fTrueFSLTime;
  double fTrueFSLMass;
  int fTrueFSLPdg;
  double fTrueFSLEnergy;
  double fTrueQ2;
  double fTrueW2;
  double fTrueBJx;
  double fTruey;
  double fTrueq0;
  double fTrueq3;
  int fTrueTarget;
  int fTrueCC;
  int fTrueNC;
  int fTrueQEL;
  int fTrueRES;
  int fTrueDIS;
  int fTrueCOH;
  int fTrueMEC;
  int fTrueNeutrons;
  int fTrueProtons;
  int fTruePi0;
  int fTruePiPlus;
  int fTruePiPlusCher;
  int fTruePiMinus;
  int fTruePiMinusCher;
  int fTrueKPlus;
  int fTrueKPlusCher;
  int fTrueKMinus;
  int fTrueKMinusCher;

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

  // Simple Reco
  int fSimpleFlag;
  double fSimpleEnergy;
  double fSimpleVtxX;
  double fSimpleVtxY;
  double fSimpleVtxZ;
  double fSimpleStopVtxX;
  double fSimpleStopVtxY;
  double fSimpleStopVtxZ;
  double fSimpleCosTheta;
  double fSimplePt;
  int fSimpleFV;
  double fSimpleMrdEnergyLoss;
  double fSimpleTrackLengthInMRD;
  double fSimpleTrackLengthInTank;
  double fSimpleMRDStartX;
  double fSimpleMRDStartY;
  double fSimpleMRDStartZ;
  double fSimpleMRDStopX;
  double fSimpleMRDStopY;
  double fSimpleMRDStopZ;
 
  // Ring Counting
  double fRCSRPred;
  double fRCMRPred;

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

  // cMrdMlReco per-event branch variables (generated from one field table;
  // production + job-level first, then debug-only).
  int fMrdMlFailureCode = -1;
  int fMrdMlRecoSkipReason = 0;
  bool fMrdMlGatePassed = false;
  bool fMrdMlReconstructionAvailable = false;
  std::string fMrdMlMRDInputSource;
  int fMrdMlNumberMrdTimeClusters = -1;
  int fMrdMlSelectedMrdClusterIndex = -1;
  int fMrdMlSelectedTankClusterIndex = -1;
  int fMrdMlInputTankHits = -1;
  bool fMrdMlRecoUsedTankBypass = false;
  bool fMrdMlTankMrdAssociationFound = false;
  int fMrdMlTankMrdSelectedTankClusterIndex = -1;
  int fMrdMlTankMrdSelectedMrdSubEventID = -1;
  double fMrdMlTankMrdTimingResidualNs = -999.0;
  double fMrdMlHasTargetMuonProbability = -1.0;
  double fMrdMlReconstructableProbability = -1.0;
  int fMrdMlEventCategory = -1;
  int fMrdMlTopologyClass = -1;
  std::vector<double> fMrdMlTopologyProbabilities;
  double fMrdMlStoppedProbability = -1.0;
  int fMrdMlExitFaceClass = -1;
  double fMrdMlMRDTrackLengthCm = -999.0;
  double fMrdMlMRDTrackLengthSigmaCm = -999.0;
  double fMrdMlCosTheta = -999.0;
  double fMrdMlCosThetaSigma = -999.0;
  double fMrdMlThetaRad = -999.0;
  double fMrdMlThetaSigmaRad = -999.0;
  bool fMrdMlThetaSigmaValid = false;
  double fMrdMlPhiRad = -999.0;
  double fMrdMlPenetrationCm = -999.0;
  double fMrdMlPenetrationSigmaCm = -999.0;
  double fMrdMlMRDEnergyLossMeV = -999.0;
  double fMrdMlMRDEnergyLossSigmaMeV = -999.0;
  double fMrdMlMuonKEMeV = -999.0;
  double fMrdMlMuonKESigmaMeV = -999.0;
  double fMrdMlCherenkovAngleRad = -999.0;
  double fMrdMlCherenkovAngleSigmaRad = -999.0;
  std::vector<double> fMrdMlTankVertexCm;
  std::vector<double> fMrdMlTankVertexSigmaCm;
  std::vector<double> fMrdMlTankExitCm;
  std::vector<double> fMrdMlTankExitSigmaCm;
  std::vector<double> fMrdMlMRDEntryCm;
  std::vector<double> fMrdMlMRDEntrySigmaCm;
  std::vector<double> fMrdMlMRDExitCm;
  std::vector<double> fMrdMlMRDExitSigmaCm;
  std::vector<double> fMrdMlSelectedEndpointCm;
  std::vector<double> fMrdMlSelectedEndpointSigmaCm;
  std::vector<double> fMrdMlMRDScatterRad;
  std::vector<double> fMrdMlMRDScatterSigmaRad;
  std::vector<double> fMrdMlTankScatterRad;
  std::vector<double> fMrdMlTankScatterSigmaRad;
  int fMrdMlSchemaVersion = -1;
  std::string fMrdMlOutputMode;
  std::string fMrdMlModelRelease;
  // debug / smoke-test only
  int fMrdMlProtocolVersion = -1;
  std::string fMrdMlStatusText;
  std::string fMrdMlErrorText;
  bool fMrdMlInputBuildOK = false;
  bool fMrdMlGateAttempted = false;
  bool fMrdMlGateInferenceOK = false;
  bool fMrdMlRecoAttempted = false;
  bool fMrdMlRecoInferenceOK = false;
  int fMrdMlDaemonRestartCount = 0;
  double fMrdMlSelectedMrdClusterStartTimeNs = -999.0;
  double fMrdMlSelectedMrdClusterEndTimeNs = -999.0;
  double fMrdMlSelectedMrdClusterTimeSpanNs = -999.0;
  int fMrdMlInputMRDRawHits = -1;
  int fMrdMlInputMRDSelectedRawHits = -1;
  int fMrdMlInputMRDGeometryValidHitsBeforeDeadMask = -1;
  int fMrdMlInputMRDKnownDeadRemoved = -1;
  int fMrdMlInputMRDLiveHitsBeforeDedup = -1;
  int fMrdMlInputMRDLiveUniqueHits = -1;
  int fMrdMlInputMRDDedupRemoved = -1;
  int fMrdMlInputMRDUnassignedHits = -1;
  int fMrdMlInputMRDLayers = -1;
  int fMrdMlInputMRDFirstLayer = -1;
  int fMrdMlInputMRDLastLayer = -1;
  std::string fMrdMlTankInputSource;
  int fMrdMlNumberTankClusters = -1;
  double fMrdMlSelectedTankClusterTimeNs = -999.0;
  double fMrdMlSelectedTankClusterChargePE = -999.0;
  double fMrdMlSelectedTankClusterTimeSpanNs = -999.0;
  bool fMrdMlTankPacketReorderedByTime = false;
  std::vector<int> fMrdMlAdditionalDeadMRDChankeys;
  bool fMrdMlRecoStructurallyEligible = false;
  bool fMrdMlPhysicsIsRawUnstandardized = false;
  bool fMrdMlTankMrdAssociationApplied = false;
  int fMrdMlTankMrdCompatiblePairCount = 0;
  double fMrdMlTankMrdExpectedOffsetNs = -999.0;
  double fMrdMlTankMrdCoincidenceWindowNs = -999.0;
  double fMrdMlTankMrdTankMeanTimeNs = -999.0;
  double fMrdMlTankMrdMrdMeanTimeNs = -999.0;
  double fMrdMlTankMrdDtNs = -999.0;
  double fMrdMlEventCategoryConfidence = -1.0;
  double fMrdMlEventCategoryMargin = -1.0;
  std::vector<double> fMrdMlEventCategoryProbabilities;
  double fMrdMlSingleMuonCompatibilityProbability = -1.0;
  bool fMrdMlSingleMuonCompatibilityValidated = false;
  double fMrdMlTopologyConfidence = -1.0;
  double fMrdMlTopologyMargin = -1.0;
  double fMrdMlTopologyPStopped = -1.0;
  double fMrdMlTopologyPBackExit = -1.0;
  double fMrdMlTopologyPSideExit = -1.0;
  double fMrdMlExitFaceConfidence = -1.0;
  double fMrdMlExitFaceMargin = -1.0;
  std::vector<double> fMrdMlExitFaceProbabilities;
  int fMrdMlEndpointExpertUsed = -1;
  double fMrdMlSinPhi = -999.0;
  double fMrdMlCosPhi = -999.0;
  double fMrdMlPhiVectorNorm = -999.0;
  std::vector<double> fMrdMlStopEndpointCm;
  std::vector<double> fMrdMlStopEndpointSigmaCm;
  std::vector<double> fMrdMlBackExitEndpointCm;
  std::vector<double> fMrdMlBackExitEndpointSigmaCm;
  std::vector<double> fMrdMlSideExitEndpointCm;
  std::vector<double> fMrdMlSideExitEndpointSigmaCm;
  bool fMrdMlQualityBaseline = false;
  bool fMrdMlQualityTopology = false;
  bool fMrdMlQualityDirection = false;
  bool fMrdMlQualityEndpoint = false;
  int fMrdMlQualityTier = 0;
  // cMrdMlReco one-entry metadata tree (mrd_ml_metadata).  These values are
  // job-level release provenance and are intentionally not repeated per event.
  int fMrdMlMetaSchemaVersion = -1;
  std::string fMrdMlMetaOutputMode;
  std::string fMrdMlMetaAdapterVersion;
  std::string fMrdMlMetaModelRelease;
  std::string fMrdMlMetaDaemonVersion;
  std::string fMrdMlMetaRuntimeFeatureVersion;
  std::string fMrdMlMetaTensorFlowVersion;
  std::string fMrdMlMetaGateModelName;
  std::string fMrdMlMetaReconstructionModelName;
  std::string fMrdMlMetaGateModelTreeSHA256;
  std::string fMrdMlMetaReconstructionModelTreeSHA256;
  std::string fMrdMlMetaGateTrainerVersion;
  std::string fMrdMlMetaReconstructionTrainerVersion;
  std::string fMrdMlMetaGeometryVersion;
  std::string fMrdMlMetaGeometryManifestSHA256;
  double fMrdMlMetaGateHasTargetThreshold = -999.0;
  double fMrdMlMetaGateReconstructableThreshold = -999.0;
  bool fMrdMlMetaRequireGateCategoryZero = false;
  int fMrdMlMetaPrefilterMinMRDHits = -1;
  
  // MuonFitter vertex
  double fRecoMuonVtxX;
  double fRecoMuonVtxY;
  double fRecoMuonVtxZ;
  double fRecoTankTrack;
  double fRecoMuonKE;
  int fNumMrdLayers;

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

  /// \brief MRD reconstruction source: Legacy (default), MrdMLReco, or Both
  std::string MRDRecoSource = "Legacy";
  bool MrdMLReco_fill = false;
  /// \brief cMrdMlReco branch set: production or debug (from the producer handshake)
  std::string MrdMLOutputMode = "production";
  bool MrdMLDebug_fill = false;
  bool Reweight_fill = 0;
  bool SimpleReco_fill = 0;
  bool RingCounting_fill = 0;
  bool RecoDebug_fill = 0; //Outputs results of Reconstruction at each step (best fits, FOMs, etc.)
  bool muonTruthRecoDiff_fill = 0; //Output difference in tmuonruth and reconstructed values
  bool SiPMPulseInfo_fill = 0;

  // LAPPD data variables
  bool LAPPDData_fill = 0;
  int gotLAPPDNumber;
  std::map<uint64_t, PsecData> LAPPDDataMap;
  std::map<uint64_t, uint64_t> LAPPDBeamgate_ns;
  std::map<uint64_t, uint64_t> LAPPDTimeStamps_ns; // data and key are the same
  std::map<uint64_t, uint64_t> LAPPDTimeStampsRaw;
  std::map<uint64_t, uint64_t> LAPPDBeamgatesRaw;
  std::map<uint64_t, uint64_t> LAPPDOffsets;
  std::map<uint64_t, int> LAPPDTSCorrection;
  std::map<uint64_t, int> LAPPDBGCorrection;
  std::map<uint64_t, int> LAPPDOSInMinusPS;

  vector<int> fLAPPD_ID;
  vector<uint64_t> fLAPPD_Beamgate_ns;
  vector<uint64_t> fLAPPD_Timestamp_ns;
  vector<uint64_t> fLAPPD_Beamgate_Raw;
  vector<uint64_t> fLAPPD_Timestamp_Raw;
  vector<uint64_t> fLAPPD_Offset;
  vector<int> fLAPPD_TSCorrection;
  vector<int> fLAPPD_BGCorrection;
  vector<int> fLAPPD_OSInMinusPS;

  std::map<uint64_t, uint32_t> GroupedTrigger;

  vector<uint64_t> fGroupedTriggerTime;
  vector<uint32_t> fGroupedTriggerWord;


    uint64_t beamInfoTime;
    int64_t timeDiff;
  double E_TOR860, E_TOR875, THCURR, BTJT2, HP875, VP875, HPTG1, VPTG1, HPTG2, VPTG2, BTH2T2;


  bool LAPPDReco_fill = 0;
  std::map<unsigned long, vector<vector<LAPPDPulse>>> lappdPulses;
  std::map<unsigned long, vector<LAPPDHit>> lappdHits;

  vector<int> fLAPPD_IDs;
  vector<int> fChannelID;
  vector<double> fPulsePeakTime;
  vector<double> fPulseCharge;
  vector<double> fPulsePeakAmp;
  vector<double> fPulseStart;
  vector<double> fPulseEnd;
  vector<double> fPulseWidth;
  vector<int> fPulseSide;
  vector<int> fPulseStripNum;

  vector<int> fLAPPDHit_IDs;
  vector<int> fLAPPDHitChannel;
  vector<int> fLAPPDHitStrip;
  vector<double> fLAPPDHitTime;
  vector<double> fLAPPDHitAmp;
  vector<double> fLAPPDHitParallelPos;
  vector<double> fLAPPDHitTransversePos;
  vector<double> fLAPPDHitP1StartTime;
  vector<double> fLAPPDHitP2StartTime;
  vector<double> fLAPPDHitP1EndTime;
  vector<double> fLAPPDHitP2EndTime;

  bool Digit_fill = 0;
  bool MuonFitter_fill = 0;
};


#endif
