#ifndef LAPPDTreeMaker_H
#define LAPPDTreeMaker_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TTree.h"
#include "TFile.h"
#include "TString.h"
#include "LAPPDPulse.h"
#include "LAPPDHit.h"
#include "Geometry.h"
#include "Position.h"
#include "PsecData.h"

/**
 * \class LAPPDTreeMaker
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
 *
 * $Author: Yue Feng $
 * $Date: 2024/02/03 16:10:00 $
 * Contact: yuef@iastate.edu
 */
class LAPPDTreeMaker : public Tool
{

public:
    LAPPDTreeMaker();                                         ///< Simple constructor
    bool Initialise(std::string configfile, DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
    bool Execute();                                           ///< Execute function used to perform Tool purpose.
    bool Finalise();                                          ///< Finalise function used to clean up resources.
    void CleanVariables();
    bool FillPulseTree();
    bool FillHitTree();
    bool FillWaveformTree();
    bool FillPPSTimestamp();
    bool FillLAPPDDataTimeStamp();
    bool LoadRunInfoFromRaw();
    bool LoadRunInfoFromANNIEEvent();
    bool GroupTriggerByBeam();
    bool GroupTriggerByLaser();
    bool GroupPPSTrigger();
    bool FillTriggerTree();
    bool FillGroupedTriggerTree();
    void CleanTriggers();
    void LoadLAPPDMapInfo();

private:
    TFile *file;
    TTree *fPulse;
    TTree *fHit;
    TTree *fWaveform;
    TTree *fTimeStamp;
    TTree *fTrigger;
    TTree *fGroupedTrigger;

    int treeMakerVerbosity;
    bool LAPPDana;
    bool LoadingPPS;

    bool LoadPulse;
    bool LoadHit;
    bool LoadWaveform;
    bool LoadLAPPDDataTimeStamp;
    bool LoadPPSTimestamp;
    bool LoadRunInfoRaw;
    bool LoadRunInfoANNIEEvent;
    bool LoadTriggerInfo;
    bool LoadGroupedTriggerInfo;
    string LoadGroupOption; // using beam, or laser, or other.

    std::map<unsigned long, vector<Waveform<double>>> lappdData;
    std::map<unsigned long, vector<double>> waveformMax; // strip number+30*side, value
    std::map<unsigned long, vector<double>> waveformRMS;
    std::map<unsigned long, vector<double>> waveformMaxLast;
    std::map<unsigned long, vector<double>> waveformMaxNearing;
    std::map<unsigned long, vector<int>> waveformMaxTimeBin;

    map<uint64_t, std::vector<uint32_t>> *TriggerWordMap;
    unsigned long CTCTimeStamp;
    std::vector<uint32_t> CTCTriggerWord;
    int trigNumInThisMap;
    int trigIndexInThisMap;
    int TriggerGroupNumInThisEvent;
    int groupedTriggerType;

    vector<uint32_t> unGroupedTriggerWords;
    vector<unsigned long> unGroupedTriggerTimestamps;
    vector<vector<uint32_t>> groupedTriggerWordsVector;
    vector<vector<unsigned long>> groupedTriggerTimestampsVector;
    vector<int> groupedTriggerByType;
    vector<uint32_t> groupedTriggerWords;
    vector<unsigned long> groupedTriggerTimestamps;

    double waveformMaxValue;
    double waveformRMSValue;
    bool waveformMaxFoundNear;
    double waveformMaxNearingValue;
    int waveformMaxTimeBinValue;

    std::map<unsigned long, vector<vector<LAPPDPulse>>> lappdPulses;
    std::map<unsigned long, vector<LAPPDHit>> lappdHits;
    Geometry *_geom;
    string treeMakerInputPulseLabel;
    string treeMakerInputHitLabel;
    string treeMakerOutputFileName;

    int EventNumber; // this is for LAPPD relate event number, data or PPS, not trigger!!
    int StripNumber;

    int LAPPD_ID;
    int ChannelID;
    double PeakTime;
    double Charge;
    double PeakAmp;
    double PulseStart;
    double PulseEnd;
    double PulseSize;
    int PulseSide;
    double PulseThreshold;
    double PulseBaseline;

    double HitTime;
    double HitAmp;
    double XPosTank;
    double YPosTank;
    double ZPosTank;
    double ParallelPos;
    double TransversePos;
    double Pulse1StartTime;
    double Pulse2StartTime;
    double Pulse1LastTime;
    double Pulse2LastTime;

    // ACDC 0
    unsigned long LAPPDDataTimeStamp0_UL;
    unsigned long LAPPDDataBeamgate0_UL;
    unsigned long LAPPDDataTimestamp0_Part1;
    unsigned long LAPPDDataBeamgate0_Part1;
    double LAPPDDataTimestamp0_Part2;
    double LAPPDDataBeamgate0_Part2;

    // ACDC 1
    unsigned long LAPPDDataTimeStamp1_UL;
    unsigned long LAPPDDataBeamgate1_UL;
    unsigned long LAPPDDataTimestamp1_Part1;
    unsigned long LAPPDDataBeamgate1_Part1;
    double LAPPDDataTimestamp1_Part2;
    double LAPPDDataBeamgate1_Part2;

    vector<unsigned long> pps_vector;
    vector<unsigned long> pps_count_vector;
    Long64_t ppsDiff;
    unsigned long ppsCount0;
    unsigned long ppsCount1;
    unsigned long ppsTime0; // in clock tick, without 3.125ns
    unsigned long ppsTime1; // in clock tick, without 3.125ns

    int RunNumber;
    int SubRunNumber;
    int PartFileNumber;

    bool MultiLAPPDMapTreeMaker;
    vector<int> LAPPD_IDs;

    vector<uint64_t> LAPPDMapTimeStampRaw_0;
    vector<uint64_t> LAPPDMapBeamgateRaw_0;
    vector<uint64_t> LAPPDMapOffsets_0;
    vector<int> LAPPDMapTSCorrections_0;
    vector<int> LAPPDMapBGCorrections_0;
    vector<int> LAPPDMapOSInMinusPS_0;

    vector<uint64_t> LAPPDMapTimeStampRaw_1;
    vector<uint64_t> LAPPDMapBeamgateRaw_1;
    vector<uint64_t> LAPPDMapOffsets_1;
    vector<int> LAPPDMapTSCorrections_1;
    vector<int> LAPPDMapBGCorrections_1;
    vector<int> LAPPDMapOSInMinusPS_1;

    std::map<uint64_t, PsecData> LAPPDDataMap;
    
    std::map<uint64_t, uint64_t> LAPPDBeamgate_ns_0;
    std::map<uint64_t, uint64_t> LAPPDTimeStamps_ns_0; 
    std::map<uint64_t, uint64_t> LAPPDTimeStampsRaw_0;
    std::map<uint64_t, uint64_t> LAPPDBeamgatesRaw_0;
    std::map<uint64_t, uint64_t> LAPPDOffsets_0;
    std::map<uint64_t, int> LAPPDTSCorrection_0;
    std::map<uint64_t, int> LAPPDBGCorrection_0;
    std::map<uint64_t, int> LAPPDOSInMinusPS_0;

    std::map<uint64_t, uint64_t> LAPPDBeamgate_ns_1;
    std::map<uint64_t, uint64_t> LAPPDTimeStamps_ns_1; 
    std::map<uint64_t, uint64_t> LAPPDTimeStampsRaw_1;
    std::map<uint64_t, uint64_t> LAPPDBeamgatesRaw_1;
    std::map<uint64_t, uint64_t> LAPPDOffsets_1;
    std::map<uint64_t, int> LAPPDTSCorrection_1;
    std::map<uint64_t, int> LAPPDBGCorrection_1;
    std::map<uint64_t, int> LAPPDOSInMinusPS_1;

    uint64_t LTSRaw_0;
    uint64_t LBGRaw_0;
    uint64_t LOffset_ns_0;
    int LTSCorrection_0;
    int LBGCorrection_0;
    int LOSInMinusPS_0;

    uint64_t LTSRaw_1;
    uint64_t LBGRaw_1;
    uint64_t LOffset_ns_1;
    int LTSCorrection_1;
    int LBGCorrection_1;
    int LOSInMinusPS_1;

    uint64_t CTCPrimeTriggerTime;
};

#endif
