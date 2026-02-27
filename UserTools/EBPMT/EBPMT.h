#ifndef EBPMT_H
#define EBPMT_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "Hit.h"
#include "ADCPulse.h"

/**
 * \class EBPMT
 *
 * $Author: Yue Feng $
 * $Date: 2024/04 $
 * Contact: yuef@iaistate.edu
 *
 */

class EBPMT : public Tool
{

public:
    EBPMT();                                                  ///< Simple constructor
    bool Initialise(std::string configfile, DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
    bool Execute();                                           ///< Execute function used to perform Tool purpose.
    bool Finalise();                                          ///< Finalise function used to clean up resources.
    bool Matching(int targetTrigger, int matchToTrack);
    void CorrectVMEOffset();

private:
    int verbosityEBPMT;
    int matchTargetTrigger;
    uint64_t matchTolerance_ns;

    int currentRunCode;

    std::map<uint64_t, std::map<unsigned long, std::vector<Hit>> *> *FinishedHits; // Key: {MTCTime}, value: map of  Hit distributions
    std::map<uint64_t, vector<uint16_t>> *FinishedRWMWaveforms;                    // Key: {MTCTime}, value: RWM waveform
    std::map<uint64_t, vector<uint16_t>> *FinishedBRFWaveforms;                    // Key: {MTCTime}, value: BRF waveform

    std::map<uint64_t, int> AlmostCompleteWaveforms;

    std::map<uint64_t, std::map<unsigned long, std::vector<Hit>> *> *InProgressHits; // Key: {MTCTime}, value: map of  Hit distributions
    std::map<uint64_t, std::vector<unsigned long>> *InProgressChkey;                 // Key: {MTCTime}, value: vector of in progress chankeys

    // only used for VME offset correction
    std::map<uint64_t, std::map<unsigned long, std::vector<Hit>> *> *InProgressHitsAux;                        // Key: {MTCTime}, value: map of  Hit distributions
    std::map<uint64_t, std::map<unsigned long, std::vector<std::vector<ADCPulse>>>> *InProgressRecoADCHits;    // Key: {MTCTime}, value: map of found pulses
    std::map<uint64_t, std::map<unsigned long, std::vector<std::vector<ADCPulse>>>> *InProgressRecoADCHitsAux; // Key: {MTCTime}, value: map of found pulses
    std::map<uint64_t, std::vector<uint16_t>> *RWMRawWaveforms;                                                // Key: MTCTime, Value: RWM waveform
    std::map<uint64_t, std::vector<uint16_t>> *BRFRawWaveforms;                                                // Key: MTCTime, Value: BRF waveform

    std::map<int, vector<uint64_t>> PairedCTCTimeStamps;
    std::map<int, vector<int>> PairedPMT_TriggerIndex;
    std::map<int, vector<uint64_t>> PairedPMTTimeStamps;
    std::map<uint64_t, int> PMTHitmapRunCode; // Key: {MTCTime}, value: RunCode

    int v_message = 1;
    int v_warning = 2;
    int v_error = 3;
    int v_debug = 4;

    int MaxObservedNumWaves = 0;
    int NumWavesInCompleteSet = 140;
    bool max_waves_adapted = false;

    int matchedHitNumber = 0;
    int exeNum = 0;
    int thisRunNum;
    int exePerMatch = 500;
    bool matchToAllTriggers;

    bool saveRWMWaveforms;
    bool saveBRFWaveforms;
};

#endif
