#ifndef EBMRD_H
#define EBMRD_H

#include <string>
#include <iostream>

#include "Tool.h"

/**
 * \class EBPMT
 *
 * $Author: Yue Feng $
 * $Date: 2024/04 $
 * Contact: yuef@iaistate.edu
 *
 */

class EBMRD : public Tool
{

public:
    EBMRD();                                                  ///< Simple constructor
    bool Initialise(std::string configfile, DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
    bool Execute();                                           ///< Execute function used to perform Tool purpose.
    bool Finalise();                                          ///< Finalise function used to clean up resources.
    bool Matching(int targetTrigger, int matchToTrack);

private:
    int verbosityEBMRD;
    int matchTargetTrigger;
    uint64_t matchTolerance_ns;

    int currentRunCode;

    int v_message = 1;
    int v_warning = 2;
    int v_error = 3;
    int v_debug = 4;

    int matchedMRDNumber = 0;
    int exeNum = 0;

    std::map<uint64_t, std::vector<std::pair<unsigned long, int>>> MRDEvents; // Key: {MTCTime}, value: "WaveMap" with key (CardID,ChannelID), value FinishedWaveform
    std::map<uint64_t, std::string> MRDEventTriggerTypes;                     // Key: {MTCTime}, value: string noting what type of trigger occured for the event
    std::map<uint64_t, int> MRDBeamLoopback;                                  // Key: {MTCTime}, value: string noting what type of trigger occured for the event
    std::map<uint64_t, int> MRDCosmicLoopback;                                // KEY: {MTCTime}, value: Cosmic loopback TDC value

    std::map<uint64_t, std::vector<std::pair<unsigned long, int>>> MRDEventsBuffer;

    bool NewMRDDataAvailable;

    std::map<int, vector<uint64_t>> PairedCTCTimeStamps;
    std::map<int, vector<int>> PairedMRD_TriggerIndex;
    std::map<int, vector<uint64_t>> PairedMRDTimeStamps;
    std::map<uint64_t, int> MRDHitMapRunCode; // Key: {MTCTime}, value: RunCode

    bool matchToAllTriggers;

    int thisRunNum;
    int exePerMatch;
};

#endif
