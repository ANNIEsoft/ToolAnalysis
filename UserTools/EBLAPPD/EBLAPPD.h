#ifndef EBLAPPD_H
#define EBLAPPD_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "PsecData.h"
#include "BoostStore.h"

/**
 * \class EBPMT
 *
 * $Author: Yue Feng $
 * $Date: 2024/04 $
 * Contact: yuef@iaistate.edu
 *
 */

class EBLAPPD : public Tool
{

public:
    EBLAPPD();                                                ///< Simple constructor
    bool Initialise(std::string configfile, DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
    bool Execute();                                           ///< Execute function used to perform Tool purpose.
    bool Finalise();                                          ///< Finalise function used to clean up resources.
    bool LoadLAPPDData();
    bool CleanData();
    bool Matching(int targetTrigger, int matchToTrack);

private:
    int thisRunNum;
    bool matchToAllTriggers;
    int exePerMatch;

    PsecData dat;
    uint64_t LAPPDBeamgate_ns;
    uint64_t LAPPDTimestamp_ns;
    uint64_t LAPPDOffset;
    unsigned long LAPPDBeamgate_Raw;
    unsigned long LAPPDTimestamp_Raw;
    int LAPPDBGCorrection;
    int LAPPDTSCorrection;
    int LAPPDOffset_minus_ps;
    uint64_t LAPPDBG_PPSBefore;
    uint64_t LAPPDBG_PPSAfter;
    uint64_t LAPPDBG_PPSDiff;
    int LAPPDBG_PPSMissing;
    uint64_t LAPPDTS_PPSBefore;
    uint64_t LAPPDTS_PPSAfter;
    uint64_t LAPPDTS_PPSDiff;
    int LAPPDTS_PPSMissing;

    vector<uint64_t> MatchBuffer_LAPPDTimestamp_ns; // used to indexing data for unmatched

    // TODO, maybe make a new "LAPPDBuildData" class?
    vector<uint64_t> Buffer_LAPPDTimestamp_ns; // used to indexing the data
    vector<PsecData> Buffer_LAPPDData;
    vector<uint64_t> Buffer_LAPPDBeamgate_ns;
    vector<uint64_t> Buffer_LAPPDOffset;
    vector<unsigned long> Buffer_LAPPDBeamgate_Raw;
    vector<unsigned long> Buffer_LAPPDTimestamp_Raw;
    vector<int> Buffer_LAPPDBGCorrection;
    vector<int> Buffer_LAPPDTSCorrection;
    vector<int> Buffer_LAPPDOffset_minus_ps;
    vector<int> Buffer_RunCode;
    vector<uint64_t> Buffer_LAPPDBG_PPSBefore;
    vector<uint64_t> Buffer_LAPPDBG_PPSAfter;
    vector<uint64_t> Buffer_LAPPDBG_PPSDiff;
    vector<int> Buffer_LAPPDBG_PPSMissing;
    vector<uint64_t> Buffer_LAPPDTS_PPSBefore;
    vector<uint64_t> Buffer_LAPPDTS_PPSAfter;
    vector<uint64_t> Buffer_LAPPDTS_PPSDiff;
    vector<int> Buffer_LAPPDTS_PPSMissing;

    std::map<int, vector<uint64_t>> PairedCTCTimeStamps;
    std::map<int, vector<int>> PairedLAPPD_TriggerIndex;
    std::map<int, vector<uint64_t>> PairedLAPPDTimeStamps;

    int matchTargetTrigger;
    uint64_t matchTolerance_ns;
    int verbosityEBLAPPD;

    int matchedLAPPDNumber = 0;
    int exeNum = 0;
    int currentRunCode;

    int v_message = 1;
    int v_warning = 2;
    int v_error = 3;
    int v_debug = 4;
};

#endif
