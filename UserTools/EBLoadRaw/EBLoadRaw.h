#ifndef EBLoadRaw_H
#define EBLoadRaw_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "CardData.h"
#include "TriggerData.h"
#include "PsecData.h"
#include "BoostStore.h"
#include "Store.h"

/**
 * \class EBPMT
 *
 * $Author: Yue Feng $
 * $Date: 2024/04 $
 * Contact: yuef@iaistate.edu
 *
 */

class EBLoadRaw : public Tool
{

public:
    EBLoadRaw();                                              ///< Simple constructor
    bool Initialise(std::string configfile, DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
    bool Execute();                                           ///< Execute function used to perform Tool purpose.
    bool Finalise();                                          ///< Finalise function used to clean up resources.

    bool LoadPMTData();
    bool LoadMRDData();
    bool LoadLAPPDData();
    bool LoadCTCData();
    bool LoadRunInfo();

    bool LoadNextPMTData();
    bool LoadNextMRDData();
    bool LoadNextLAPPDData();
    bool LoadNextCTCData();

    bool LoadNewFile();
    int RunCode(string fileName);
    std::vector<std::string> OrganizeRunParts(std::string FileList);

private:
    std::string CurrentFile = "NONE";
    std::string InputFile;
    std::vector<std::string> OrganizedFileList;
    bool ReadTriggerOverlap;
    int RunCodeToSave;

    bool LoadCTC;
    bool LoadPMT;
    bool LoadMRD;
    bool LoadLAPPD;

    int PMTTotalEntries;
    int MRDTotalEntries;
    int LAPPDTotalEntries;
    int CTCTotalEntries;

    int LoadedPMTTotalEntries;
    int LoadedMRDTotalEntries;
    int LoadedLAPPDTotalEntries;
    int LoadedCTCTotalEntries;

    bool ProcessingComplete;
    bool FileCompleted;
    bool JumpBecauseLAPPD;
    bool PMTEntriesCompleted;
    bool MRDEntriesCompleted;
    bool LAPPDEntriesCompleted;
    bool CTCEntriesCompleted;
    bool usingTriggerOverlap;

    int CTCEntryNum;
    int PMTEntryNum;
    int MRDEntryNum;
    int LAPPDEntryNum;
    int LoadingFileNumber;

    int RunNumber;
    int SubRunNumber;
    int PartFileNumber;

    bool PMTPaused;
    bool MRDPaused;
    bool LAPPDPaused;
    bool CTCPaused;

    bool PMTMatchingForced;
    bool MRDMatchingForced;
    bool LAPPDMatchingForced;

    BoostStore *RawData = nullptr;
    BoostStore *PMTData = nullptr;
    BoostStore *MRDData = nullptr;
    BoostStore *LAPPDData = nullptr;
    BoostStore *CTCData = nullptr;

    std::vector<CardData> *CData = nullptr;
    TriggerData *TData = nullptr;
    MRDOut *MData = nullptr;
    PsecData *LData = nullptr;

    int verbosityEBLoadRaw;

    int v_message = 1;
    int v_warning = 2;
    int v_error = 3;
    int v_debug = 4;
};

#endif
