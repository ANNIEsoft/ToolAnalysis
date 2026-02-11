#ifndef ANNIEEventTreeMaker_H
#define ANNIEEventTreeMaker_H

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
#include "Geometry.h"

/**
 * \class ANNIEEventTreeMaker
 *
 *
 * $Author: Yue Feng $
 * $Date: 2024/8 $
 * Contact: yuef@iastate.edu
 */
class ANNIEEventTreeMaker : public Tool
{

public:
    ANNIEEventTreeMaker();                                    ///< Simple constructor
    bool Initialise(std::string configfile, DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
    bool Execute();                                           ///< Execute function used to perform Tool purpose.
    bool Finalise();                                          ///< Finalise function used to clean up resources.

    bool LoadEventInfo();
    void LoadBeamInfo();

    void LoadRWMBRFInfo();

    void LoadAllTankHits();
    void LoadSiPMHits();

    void LoadLAPPDInfo();
    void FillLAPPDInfo();
    void FillLAPPDHit();
    void FillLAPPDPulse();

    bool LoadClusterInfo();

    void LoadMRDCluster();

    void LoadMRDInfo();
    void LoadLAPPDRecoInfo();
    void FillLAPPDRecoInfo();
    void FillLAPPDWaveform();
    void FillLAPPDMCHitInfo();

    bool FillMCTruthInfo();
    bool FillTankRecoInfo();
    int LoadMRDTrackReco(int SubEventNumber);
    void LoadAllMRDHits(bool isData);
    void FillRecoDebugInfo();
    void FillTruthRecoDiffInfo(bool got_mc, bool got_reco);

    void RecoSummary();
    void LoadTankClusterHits(std::vector<Hit> cluster_hits);
    void LoadTankClusterHitsMC(std::vector<MCHit> cluster_hits, std::vector<unsigned long> cluster_detkeys);
    bool LoadTankClusterClassifiers(double cluster_time);

    void ResetVariables();

private:
    // General variables
    bool isData = 1;
    bool hasGenie;

    int ANNIEEventTreeMakerVerbosity = 0;
    int v_error = 0;
    int v_warning = 1;
    int v_message = 2;
    int v_debug = 3;

    int EventNumberIndex = 0;

    // What events will be filled - Triggers
    bool fillAllTriggers; // if true, fill all events with any major trigger
    // if fillAllTriggers = false:
    bool fill_singleTrigger; // if true, only fill events with selected trigger, for example 14
    int fill_singleTriggerWord;
    // if fillAllTriggers = false and fill_singleTrigger = false:
    vector<int> fill_TriggerWord; // fill events with any of these triggers in this vector

    // What events will be filled - status
    bool fillCleanEventsOnly = 0; // Only output events not flagged by EventSelector tool
    bool fillLAPPDEventsOnly = 0; // Only fill events with LAPPD data

    // What information will be filled
    bool TankHitInfo_fill = 1;
    bool TankCluster_fill = 1;
    bool cluster_TankHitInfo_fill = 1;
    bool MRDHitInfo_fill = 1;
    bool LAPPDData_fill = 1;
    bool RWMBRF_fill = 1;
    bool LAPPD_PPS_fill = 1;
    bool LAPPD_Waveform_fill = 1;
    bool LAPPD_MC_fill = 0;

    // What reco information will be filled
    bool MCTruth_fill = 0; // Output the MC truth information
    bool TankReco_fill = 0;
    bool MRDReco_fill = 1;
    bool RecoDebug_fill = 0;         // Outputs results of Reconstruction at each step (best fits, FOMs, etc.)
    bool muonTruthRecoDiff_fill = 0; // Output difference in tmuonruth and reconstructed values
    bool SiPMPulseInfo_fill = 0;
    bool LAPPDReco_fill = 1;
    bool BeamInfo_fill = 1;
    bool RingCounting_fill = 0;

    TFile *fOutput_tfile = nullptr;
    TTree *fANNIETree = nullptr;
    Geometry *geom = nullptr;

    int processedEvents = 0;

    // Variables for filling the tree

    // EventInfo
    int fRunNumber;
    int fSubrunNumber;
    int fPartFileNumber;
    int fRunType = 0;
    int fEventNumber;
    int fPrimaryTriggerWord;
    int trigword;
    uint64_t fPrimaryTriggerTime;
    vector<uint64_t> fGroupedTriggerTime;
    vector<uint32_t> fGroupedTriggerWord;
    int fTriggerword;
    int fExtended;
    int fTankMRDCoinc;
    int fNoVeto;
    int fHasTank;
    int fHasMRD;
    int fHasLAPPD;
    std::string fMRDTriggerType;
    int fMRDTriggerTypeInt;

    ULong64_t fEventTimeTank;
    ULong64_t fEventTimeMRD;

    // beam information
    double fPot;
    int fBeamok;
    int fBunchRotationOn;
    double beam_E_TOR860;
    double beam_E_TOR875;
    double beam_THCURR;
    double beam_BTJT2;
    double beam_HP875;
    double beam_VP875;
    double beam_HPTG1;
    double beam_VPTG1;
    double beam_HPTG2;
    double beam_VPTG2;
    double beam_BTH2T2;
    double beam_B_BRRMPL;
    double beam_B_BRRMPQ;
    double beam_B_BRRMPS;
    double beam_B_BRRMP;
    uint64_t fBeamInfoTime;
    int64_t fBeamInfoTimeToTriggerDiff;

    // RWM and BRF information
    double fRWMRisingStart; // start of the rising of the waveform
    double fRWMRisingEnd;   // maximum of the rising of the waveform
    double fRWMHalfRising;  // half of the rising of the waveform, from start to maximum
    double fRWMFHWM;        // full width at half maximum of the waveform
    double fRWMFirstPeak;   // first peak of the waveform

    double fBRFFirstPeak;    // first peak of the waveform
    double fBRFAveragePeak;  // average peak of the waveform
    double fBRFFirstPeakFit; // first peak of the waveform, Gaussian fit

    // TankHitInfo_fill
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
    std::vector<int> fHitPMTType;

    // SiPMPulseInfo_fill
    int fSiPM1NPulses;
    int fSiPM2NPulses;
    std::vector<double> fSiPMHitQ;
    std::vector<double> fSiPMHitT;
    std::vector<double> fSiPMHitAmplitude;
    std::vector<double> fSiPMNum;

    // LAPPDData_fill
    int fLAPPD_Count;
    vector<int> fLAPPD_ID;
    vector<uint64_t> fLAPPD_Beamgate_ns;
    vector<uint64_t> fLAPPD_Timestamp_ns;
    vector<uint64_t> fLAPPD_Beamgate_Raw;
    vector<uint64_t> fLAPPD_Timestamp_Raw;
    vector<uint64_t> fLAPPD_Offset;
    vector<int> fLAPPD_TSCorrection;
    vector<int> fLAPPD_BGCorrection;
    vector<int> fLAPPD_OSInMinusPS;
    vector<uint64_t> fLAPPD_BGPPSBefore;
    vector<uint64_t> fLAPPD_BGPPSAfter;
    vector<uint64_t> fLAPPD_BGPPSDiff;
    vector<int> fLAPPD_BGPPSMissing;
    vector<uint64_t> fLAPPD_TSPPSBefore;
    vector<uint64_t> fLAPPD_TSPPSAfter;
    vector<uint64_t> fLAPPD_TSPPSDiff;
    vector<int> fLAPPD_TSPPSMissing;
    vector<int> fLAPPD_BG_switchBit0;
    vector<int> fLAPPD_BG_switchBit1;

    // LAPPD Reco Fill
    vector<uint64_t> fLAPPDPulseTimeStampUL;
    vector<uint64_t> fLAPPDPulseBeamgateUL;
    vector<int> fLAPPD_IDs;
    vector<int> fChannelID;
    vector<double> fPulsePeakTime;
    vector<double> fPulseHalfHeightTime;
    vector<double> fPulseCharge;
    vector<double> fPulsePeakAmp;
    vector<double> fPulseStart;
    vector<double> fPulseEnd;
    vector<double> fPulseWidth;
    vector<int> fPulseSide;
    vector<int> fPulseStripNum;
    vector<double> fPulseBaseline;
    vector<double> fPulseFollowTime;
    vector<double> fPulseFollowCharge;

    vector<uint64_t> fLAPPDHitTimeStampUL;
    vector<uint64_t> fLAPPDHitBeamgateUL;
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
    vector<double> fLAPPDHitP1PeakTime;
    vector<double> fLAPPDHitP2PeakTime;
    vector<double> fLAPPDHitP1PeakAmp;
    vector<double> fLAPPDHitP2PeakAmp;
    vector<double> fLAPPDHitP1HalfHeightTime;
    vector<double> fLAPPDHitP2HalfHeightTime;
    vector<double> fLAPPDHitP1HalfEndTime;
    vector<double> fLAPPDHitP2HalfEndTime;
    vector<double> fLAPPDHitP1Charge;
    vector<double> fLAPPDHitP2Charge;
    vector<double> fLAPPDHitP1Baseline;
    vector<double> fLAPPDHitP2Baseline;
    vector<double> fLAPPDHitP1FollowTime;
    vector<double> fLAPPDHitP2FollowTime;
    vector<double> fLAPPDHitP1FollowCharge;
    vector<double> fLAPPDHitP2FollowCharge;

    // waveform
    vector<int> LAPPDWaveformChankey;
    vector<double> waveformMaxValue;
    vector<double> waveformRMSValue;
    vector<bool> waveformMaxFoundNear;
    vector<double> waveformMaxNearingValue;
    vector<int> waveformMaxTimeBinValue;

    // LAPPD waveform Fill
    string LAPPDWaveformInputLabel;
    vector<vector<double>> fLAPPDWaveforms;

    // LAPPD MC Hit info, loaded from WCSim
    vector<int> fLAPPDMCHitTubeIDs;
    vector<unsigned long> fLAPPDMCHitChankeys;
    vector<double> fLAPPDMCHitTime;
    vector<double> fLAPPDMCHitCharge;
    vector<double> fLAPPDMCHitX;
    vector<double> fLAPPDMCHitY;
    vector<double> fLAPPDMCHitZ;
    vector<double> fLAPPDMCHitParallelPos;
    vector<double> fLAPPDMCHitTransversePos;

    // finished ****************************************************

    // tank cluster information

    int fNumberOfClusters; // how many clusters in this event
    // vector<int> fClusterIndex; // index of which cluster this data belongs to
    vector<int> fClusterHits; // how many hits in this cluster
    vector<double> fClusterChargeV;
    vector<double> fClusterTimeV;
    vector<double> fClusterPEV;

    vector<vector<double>> fCluster_HitX; // each vector is a cluster, each element is a hit in that cluster
    vector<vector<double>> fCluster_HitY;
    vector<vector<double>> fCluster_HitZ;
    vector<vector<double>> fCluster_HitT;
    vector<vector<double>> fCluster_HitQ;
    vector<vector<double>> fCluster_HitPE;
    vector<vector<int>> fCluster_HitType;
    vector<vector<int>> fCluster_HitDetID;
    vector<vector<int>> fCluster_HitChankey;
    vector<vector<int>> fCluster_HitChankeyMC;
    vector<vector<int>> fCluster_HitPMTType;

    vector<double> fClusterMaxPEV;
    vector<double> fClusterChargePointXV;
    vector<double> fClusterChargePointYV;
    vector<double> fClusterChargePointZV;
    vector<double> fClusterChargeBalanceV;

    // MRD cluster information
    ULong64_t fEventTimeMRD_Tree;
    int fMRDClusterNumber;
    std::vector<int> fMRDClusterHitNumber;
    std::vector<double> fMRDClusterTime;
    std::vector<double> fMRDClusterTimeSigma;

    // MRDHitInfo_fill
    int fVetoHit;
    std::vector<int> fMRDHitClusterIndex;
    std::vector<double> fMRDHitT;
    std::vector<double> fMRDHitCharge;
    std::vector<int> fMRDHitDigitPMT;
    std::vector<int> fMRDHitDetID;
    std::vector<int> fMRDHitChankey;
    std::vector<int> fMRDHitChankeyMC;

    std::vector<double> fFMVHitT;
    std::vector<int> fFMVHitDetID;
    std::vector<int> fFMVHitChankey;
    std::vector<int> fFMVHitChankeyMC;

    // MRDReco_fill
    int fNumMRDClusterTracks;
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
    std::vector<int> fMRDClusterIndex;

    std::vector<int> fNumClusterTracks;

    // fillCleanEventsOnly
    int fEventStatusApplied;
    int fEventStatusFlagged;

    // MCTruth_fill
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
    std::vector<double> *fTrueNeutCapGammaE = nullptr;
    int fTrueMultiRing;

    // Genie information for event
    double fTrueNeutrinoEnergy;
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

    // TankReco_fill
    double fRecoVtxX;
    double fRecoVtxY;
    double fRecoVtxZ;
    double fRecoVtxTime;
    double fRecoVtxFOM;
    double fRecoDirX;
    double fRecoDirY;
    double fRecoDirZ;
    double fRecoAngle;
    double fRecoPhi;
    int fRecoStatus;

    // RingCounting_fill
    double fRC_singleRingP;
    double fRC_multiRingP;

    // RecoDebug_fill
    //  **************** Full reco chain information ************* //
    //   seed vertices
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

    // muonTruthRecoDiff_fill
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

    // Pion and kaon counts for event
    int fPi0Count;
    int fPiPlusCount;
    int fPiMinusCount;
    int fK0Count;
    int fKPlusCount;
    int fKMinusCount;

    //********************************************************************************************************************

    // data variables for filling the tree
    // Event Info
    std::map<std::string, bool> fDataStreams;
    std::map<uint64_t, uint32_t> GroupedTrigger;

    // LAPPDData_fill
    std::map<uint64_t, PsecData> LAPPDDataMap;
    std::map<uint64_t, uint64_t> LAPPDBeamgate_ns;
    std::map<uint64_t, uint64_t> LAPPDTimeStamps_ns; // data and key are the same
    std::map<uint64_t, uint64_t> LAPPDTimeStampsRaw;
    std::map<uint64_t, uint64_t> LAPPDBeamgatesRaw;
    std::map<uint64_t, uint64_t> LAPPDOffsets;
    std::map<uint64_t, int> LAPPDTSCorrection;
    std::map<uint64_t, int> LAPPDBGCorrection;
    std::map<uint64_t, int> LAPPDOSInMinusPS;
    std::map<uint64_t, uint64_t> LAPPDBG_PPSBefore;
    std::map<uint64_t, uint64_t> LAPPDBG_PPSAfter;
    std::map<uint64_t, uint64_t> LAPPDBG_PPSDiff;
    std::map<uint64_t, int> LAPPDBG_PPSMissing;
    std::map<uint64_t, uint64_t> LAPPDTS_PPSBefore;
    std::map<uint64_t, uint64_t> LAPPDTS_PPSAfter;
    std::map<uint64_t, uint64_t> LAPPDTS_PPSDiff;
    std::map<uint64_t, int> LAPPDTS_PPSMissing;
    std::map<int, vector<int>> SwitchBitBG; 

    std::map<unsigned long, vector<vector<LAPPDPulse>>> lappdPulses;
    std::map<unsigned long, vector<LAPPDHit>> lappdHits;

    //********************************************************************************************************************

    // detector maps, don't clear
    std::map<int, std::string> *AuxChannelNumToTypeMap;
    std::map<int, double> ChannelKeyToSPEMap;

    std::map<int, unsigned long> pmtid_to_channelkey;
    std::map<unsigned long, int> channelkey_to_pmtid;
    std::map<unsigned long, int> channelkey_to_mrdpmtid;
    std::map<int, unsigned long> mrdpmtid_to_channelkey_data;
    std::map<unsigned long, int> channelkey_to_faccpmtid;
    std::map<int, unsigned long> faccpmtid_to_channelkey_data;

    //********************************************************************************************************************
    // some left not used objects

    // left for raw data
    std::vector<int> fADCWaveformChankeys;
    std::vector<int> fADCWaveformSamples;

    // ************ Muon reconstruction level information ******** //
    std::string MRDTriggertype;

    // ************* LAPPD RecoInfo *********** //
    // left for LAPPD waveforms if needed
    std::map<unsigned long, vector<double>> waveformMax; // strip number+30*side, value
    std::map<unsigned long, vector<double>> waveformRMS;
    std::map<unsigned long, vector<double>> waveformMaxLast;
    std::map<unsigned long, vector<double>> waveformMaxNearing;
    std::map<unsigned long, vector<int>> waveformMaxTimeBin;
};

#endif
