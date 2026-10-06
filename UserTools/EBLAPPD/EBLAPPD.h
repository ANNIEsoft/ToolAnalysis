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
 * Updated by: Anuj gupta (2026.09.03)
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
    
    uint64_t LAPPDBeamgate_ns_0;
    uint64_t LAPPDTimestamp_ns_0;
    uint64_t LAPPDOffset_0;
    unsigned long LAPPDBeamgate_Raw_0;
    unsigned long LAPPDTimestamp_Raw_0;
    int LAPPDBGCorrection_0;
    int LAPPDTSCorrection_0;
    int LAPPDOffset_minus_ps_0;
    uint64_t LAPPDBG_PPSBefore_0;
    uint64_t LAPPDBG_PPSAfter_0;
    uint64_t LAPPDBG_PPSDiff_0;
    int LAPPDBG_PPSMissing_0;
    uint64_t LAPPDTS_PPSBefore_0;
    uint64_t LAPPDTS_PPSAfter_0;
    uint64_t LAPPDTS_PPSDiff_0;
    int LAPPDTS_PPSMissing_0;

    uint64_t LAPPDBeamgate_ns_1;
    uint64_t LAPPDTimestamp_ns_1;
    uint64_t LAPPDOffset_1;
    unsigned long LAPPDBeamgate_Raw_1;
    unsigned long LAPPDTimestamp_Raw_1;
    int LAPPDBGCorrection_1;
    int LAPPDTSCorrection_1;
    int LAPPDOffset_minus_ps_1;    
    uint64_t LAPPDBG_PPSBefore_1;
    uint64_t LAPPDBG_PPSAfter_1;
    uint64_t LAPPDBG_PPSDiff_1;
    int LAPPDBG_PPSMissing_1;
    uint64_t LAPPDTS_PPSBefore_1;
    uint64_t LAPPDTS_PPSAfter_1;
    uint64_t LAPPDTS_PPSDiff_1;
    int LAPPDTS_PPSMissing_1;

    vector<uint64_t> MatchBuffer_LAPPDTimestamp_ns_0; // used to indexing data for unmatched
    vector<uint64_t> MatchBuffer_LAPPDTimestamp_ns_1;

    // TODO, maybe make a new "LAPPDBuildData" class?
    // Shared buffers 
    vector<PsecData> Buffer_LAPPDData;
    vector<int> Buffer_RunCode;

    vector<uint64_t> Buffer_LAPPDTimestamp_ns_0; 
    vector<uint64_t> Buffer_LAPPDBeamgate_ns_0;
    vector<uint64_t> Buffer_LAPPDOffset_0;
    vector<unsigned long> Buffer_LAPPDBeamgate_Raw_0;
    vector<unsigned long> Buffer_LAPPDTimestamp_Raw_0;
    vector<int> Buffer_LAPPDBGCorrection_0;
    vector<int> Buffer_LAPPDTSCorrection_0;
    vector<int> Buffer_LAPPDOffset_minus_ps_0;
    vector<uint64_t> Buffer_LAPPDBG_PPSBefore_0;
    vector<uint64_t> Buffer_LAPPDBG_PPSAfter_0;
    vector<uint64_t> Buffer_LAPPDBG_PPSDiff_0;
    vector<int> Buffer_LAPPDBG_PPSMissing_0;
    vector<uint64_t> Buffer_LAPPDTS_PPSBefore_0;
    vector<uint64_t> Buffer_LAPPDTS_PPSAfter_0;
    vector<uint64_t> Buffer_LAPPDTS_PPSDiff_0;
    vector<int> Buffer_LAPPDTS_PPSMissing_0;


    vector<uint64_t> Buffer_LAPPDTimestamp_ns_1; 
    vector<uint64_t> Buffer_LAPPDBeamgate_ns_1;
    vector<uint64_t> Buffer_LAPPDOffset_1;
    vector<unsigned long> Buffer_LAPPDBeamgate_Raw_1;
    vector<unsigned long> Buffer_LAPPDTimestamp_Raw_1;
    vector<int> Buffer_LAPPDBGCorrection_1;
    vector<int> Buffer_LAPPDTSCorrection_1;
    vector<int> Buffer_LAPPDOffset_minus_ps_1;
    vector<uint64_t> Buffer_LAPPDBG_PPSBefore_1;
    vector<uint64_t> Buffer_LAPPDBG_PPSAfter_1;
    vector<uint64_t> Buffer_LAPPDBG_PPSDiff_1;
    vector<int> Buffer_LAPPDBG_PPSMissing_1;
    vector<uint64_t> Buffer_LAPPDTS_PPSBefore_1;
    vector<uint64_t> Buffer_LAPPDTS_PPSAfter_1;
    vector<uint64_t> Buffer_LAPPDTS_PPSDiff_1;
    vector<int> Buffer_LAPPDTS_PPSMissing_1;
    
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
