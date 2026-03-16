#ifndef EBSaver_H
#define EBSaver_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "BoostStore.h"
#include "PsecData.h"
#include "TimeClass.h"
#include "TriggerClass.h"
#include "Waveform.h"
#include "ANNIEalgorithms.h"
#include "ADCPulse.h"
#include "CalibratedADCWaveform.h"
#include "BeamStatus.h"
#include "TFile.h"
#include "TTree.h"

/**
 * \class EBPMT
 *
 * $Author: Yue Feng $
 * $Date: 2024/04 $
 * Contact: yuef@iaistate.edu
 *
 */

class EBSaver : public Tool
{

public:
    EBSaver();                                                ///< Simple constructor
    bool Initialise(std::string configfile, DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
    bool Execute();                                           ///< Execute function used to perform Tool purpose.
    bool Finalise();                                          ///< Finalise function used to clean up resources.

    bool GotAllDataFromOriginalBuffer();
    void SetDataObjects();

    bool SaveToANNIEEvent(string saveFileName, int runCode, int triggerTrack, int trackIndex); // save the data to the ANNIEEvent, include all other saving functions.
    bool SaveRunInfo(int runCode);                                                             // get run numbers from the RunCode in trigger grouper
    bool SaveGroupedTriggers(int triggerTrack, int groupIndex);
    // use the targetTrigWord to get the trigger track, use the groupIndex to get the group of triggers in that track
    // save triggers from std::map<int, std::vector<std::map<uint64_t,uint32_t>>> GroupedTriggersInTotal; //each map is a group of triggers, with the key is the target trigger word

    bool SavePMTData(uint64_t PMTTime);
    bool SaveMRDData(uint64_t MRDTime);
    bool SaveLAPPDData(uint64_t LAPPDTime);
    bool SaveBeamInfo(uint64_t TriggerTime);

    bool SaveOrphan(int runCode);
    bool SaveOrphanPMT(int runCode);
    bool SaveOrphanMRD(int runCode);
    bool SaveOrphanCTC(int runCode);
    bool SaveOrphanLAPPD(int runCode);

    void BuildEmptyPMTData();
    void BuildEmptyMRDData();
    void BuildEmptyLAPPDData();

    bool LoadBeamInfo();

private:
    int saveRunNumber;
    int saveSubrunNumber;
    int savePartFileNumber;
    int savingRunCode;

    bool saveTriggerGroups = true;
    bool saveAllTriggers;
    bool savePMT = true;
    bool saveMRD;
    bool saveLAPPD;
    bool saveOrphan;
    bool saveBeamInfo;

    bool saveRawRWMWaveform;
    bool saveRawBRFWaveform;

    string savePath;
    string saveName;

    string beamInfoFileName;

    int verbosityEBSaver;
    int v_message = 1;
    int v_warning = 2;
    int v_error = 3;
    int v_debug = 4;

    BoostStore *ANNIEEvent;

    int exeNumber;
    int savedTriggerGroupNumber;
    int savedPMTHitMapNumber;
    int savedMRDNumber;
    int savedLAPPDNumber;

    std::map<uint64_t, int> TriggerTimeWithoutMRD; // trigger time, 8 or 36

    int thisRunNum;
    int thisSubrunNum;
    int thisPartFileNum;

    int savePerExe;

    int TotalBuiltEventsNumber = 0;
    int TotalBuiltPMTNumber;
    int TotalBuiltMRDNumber;
    int TotalBuiltLAPPDNumber;
    int TotalBuiltTriggerGroupNumber;

    std::map<uint64_t, int> PMTRunCodeToRemove;
    std::map<uint64_t, int> MRDRunCodeToRemove;
    std::map<uint64_t, int> LAPPDRunCodeToRemove;
    std::map<int, int> GroupedTriggerIndexToRemove;

    std::map<int, vector<uint64_t>> PMTPairInfoToRemoveTime;
    std::map<int, vector<uint64_t>> MRDPairInfoToRemoveTime;
    std::map<int, vector<uint64_t>> LAPPDPairInfoToRemoveTime;

    std::map<int, std::vector<std::map<uint64_t, uint32_t>>> GroupedTriggersInTotal; // each map is a group of triggers, with the key is the target trigger word
    std::map<int, std::vector<int>> RunCodeInTotal;

    int BeamTriggerMain;
    int LaserTriggerMain;
    int CosmicTriggerMain;
    int LEDTriggerMain;
    int AmBeTriggerMain;
    int PPSMain;

    // PMT related data object
    std::map<uint64_t, std::map<unsigned long, std::vector<Hit>> *> *InProgressHits;                           // Key: {MTCTime}, value: map of  Hit distributions
    std::map<uint64_t, std::vector<unsigned long>> *InProgressChkey;                                           // Key: {MTCTime}, value: vector of in progress chankeys
    std::map<uint64_t, std::map<unsigned long, std::vector<std::vector<ADCPulse>>>> *InProgressRecoADCHits;    // Key: {MTCTime}, value: map of found pulses
    std::map<uint64_t, std::map<unsigned long, std::vector<std::vector<ADCPulse>>>> *InProgressRecoADCHitsAux; // Key: {MTCTime}, value: map of found pulses
    std::map<uint64_t, std::map<unsigned long, std::vector<Hit>> *> *InProgressHitsAux;                        // Key: {MTCTime}, value: map of  Hit distributions
    std::map<uint64_t, std::map<unsigned long, std::vector<int>>> *FinishedRawAcqSize;                         // Key: {MTCTime}, value: map of acquisition time window sizes

    std::map<uint64_t, std::vector<uint16_t>> *RWMRawWaveforms; // Key: MTCTime, Value: RWM waveform
    std::map<uint64_t, std::vector<uint16_t>> *BRFRawWaveforms; // Key: MTCTime, Value: BRF waveform

    std::map<int, vector<uint64_t>> PairedPMTTriggerTimestamp;
    std::map<int, vector<int>> PairedPMT_TriggerIndex;
    std::map<int, vector<uint64_t>> PairedPMTTimeStamps;
    std::map<uint64_t, int> PMTHitmapRunCode; // Key: {MTCTime}, value: RunCode

    // MRD related data object
    std::map<uint64_t, std::vector<std::pair<unsigned long, int>>> MRDEvents; // Key: {MTCTime}, value: "WaveMap" with key (CardID,ChannelID), value FinishedWaveform
    std::map<uint64_t, std::string> MRDEventTriggerTypes;                     // Key: {MTCTime}, value: string noting what type of trigger occured for the event
    std::map<uint64_t, int> MRDBeamLoopback;                                  // Key: {MTCTime}, value: string noting what type of trigger occured for the event
    std::map<uint64_t, int> MRDCosmicLoopback;                                // KEY: {MTCTime}, value: Cosmic loopback TDC value

    std::map<int, vector<uint64_t>> PairedMRDTriggerTimestamp;
    std::map<int, vector<int>> PairedMRD_TriggerIndex;
    std::map<int, vector<uint64_t>> PairedMRDTimeStamps;
    std::map<uint64_t, int> MRDHitMapRunCode; // Key: {MTCTime}, value: RunCode

    // LAPPD related data object
    vector<int> Buffer_IndexOfData;            // index of unpaired data
    vector<uint64_t> Buffer_LAPPDTimestamp_ns; // used to match

    vector<PsecData> Buffer_LAPPDData;
    vector<uint64_t> Buffer_LAPPDBeamgate_ns;
    vector<uint64_t> Buffer_LAPPDOffset;
    vector<unsigned long> Buffer_LAPPDBeamgate_Raw;
    vector<unsigned long> Buffer_LAPPDTimestamp_Raw;
    vector<int> Buffer_LAPPDBGCorrection;
    vector<int> Buffer_LAPPDTSCorrection;
    vector<int> Buffer_LAPPDOffset_minus_ps;
    vector<uint64_t> Buffer_LAPPDBG_PPSBefore;
    vector<uint64_t> Buffer_LAPPDBG_PPSAfter;
    vector<uint64_t> Buffer_LAPPDBG_PPSDiff;
    vector<int> Buffer_LAPPDBG_PPSMissing;
    vector<uint64_t> Buffer_LAPPDTS_PPSBefore;
    vector<uint64_t> Buffer_LAPPDTS_PPSAfter;
    vector<uint64_t> Buffer_LAPPDTS_PPSDiff;
    vector<int> Buffer_LAPPDTS_PPSMissing;

    vector<int> Buffer_LAPPDRunCode;

    std::map<int, vector<uint64_t>> PairedLAPPDTriggerTimestamp;
    std::map<int, vector<int>> PairedLAPPD_TriggerIndex;
    std::map<int, vector<uint64_t>> PairedLAPPDTimeStamps;

    // beam info related objects
    vector<uint64_t> BeamInfoTimestamps;
    std::map<uint64_t, double> E_TOR860_map;
    std::map<uint64_t, double> E_TOR875_map;
    std::map<uint64_t, double> THCURR_map;
    std::map<uint64_t, double> BTJT2_map;
    std::map<uint64_t, double> HP875_map;
    std::map<uint64_t, double> VP875_map;
    std::map<uint64_t, double> HPTG1_map;
    std::map<uint64_t, double> VPTG1_map;
    std::map<uint64_t, double> HPTG2_map;
    std::map<uint64_t, double> VPTG2_map;
    std::map<uint64_t, double> BTH2T2_map;
    std::map<uint64_t, double> B_BRRMPL_map;
    std::map<uint64_t, double> B_BRRMPS_map;
    std::map<uint64_t, double> B_BRRMPQ_map;
    std::map<uint64_t, double> B_BRRMP_map;

};

#endif
