#ifndef EBTriggerGrouper_H
#define EBTriggerGrouper_H

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

class EBTriggerGrouper : public Tool
{

public:
    EBTriggerGrouper();                                       ///< Simple constructor
    bool Initialise(std::string configfile, DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
    bool Execute();                                           ///< Execute function used to perform Tool purpose.
    bool Finalise();                                          ///< Finalise function used to clean up resources.

    bool GroupByTolerance();
    bool FillByTolerance();
    bool SaveGroupedTriggers();
    bool CleanBuffer();

    int GroupByTrigWord(uint32_t mainTrigWord, vector<uint32_t> TrigWords, int tolerance);
    // group and save triggers to GroupedTriggersInTotal and RunCodeInTotal
    // input a vector of trigger words in that combination, and a tolerance
    // during grouping, select based on the main TrigWord + the tolerance
    // in that range, if the trigger word is in the vector, group them together
    // push the grouped trigger map into the vector at GroupedTriggersInTotal[mainTrigWord]

    int FillByTrigWord(uint32_t mainTrigWord, vector<uint32_t> TrigWords, int tolerance);
    // Fill the trigger maps at GroupedTriggersInTotal[mainTrigWord]
    // for all triggers in the buffer, if the trigger word is in the vector, and in the range of main TrigWord - the tolerance
    // push the trigger into the map

    int CleanTriggerBuffer(); // remove very early trigger in TrigTimeForGroup and TrigWordForGroup
    // only leave the latest maxNumAllowedInBuffer elements

private:
    string savePath;
    string GroupMode; // beam, Tolerance
    double GroupTolerance;
    int GroupTrigWord;

    std::map<uint64_t, std::vector<uint32_t>> *TimeToTriggerWordMap;
    std::map<uint64_t, std::vector<uint32_t>> *TimeToTriggerWordMapComplete; // Info about all triggerwords

    vector<uint64_t> TrigTimeForGroup;
    vector<uint32_t> TrigWordForGroup;
    vector<int> RunCodeBuffer;

    std::vector<std::map<uint64_t, uint32_t>> GroupedTriggers; // each map is a group of triggers, for the main target trigger
    vector<int> RunCode;                                       //!! RunCode goes with each group, always modify them together

    std::map<int, std::vector<std::map<uint64_t, uint32_t>>> GroupedTriggersInTotal; // each map is a group of triggers, with the key is the target trigger word
    std::map<int, std::vector<int>> RunCodeInTotal;                                  //!! RunCode goes with each group, always modify them together

    std::map<int, int> SkippedDuplicateTriggers; // if the trigger was duplicated in multiple entries, skip it. this record the skipped number of groups

    int verbosityEBTG;

    int v_message = 1;
    int v_warning = 2;
    int v_error = 3;
    int v_debug = 4;

    int ANNIEEventNum;
    int CurrentRunNum;
    int CurrentSubrunNum;
    int CurrentPartNum;
    int currentRunCode;
    bool usingTriggerOverlap;

    int StoreTotalEntry;

    int maxNumAllowedInBuffer;
    int removedTriggerInBuffer;

    bool groupBeam;
    bool groupCosmic;
    bool groupLaser;
    bool groupLED;
    bool groupAmBe;
    bool groupPPS;
    bool groupNuMI;

    int BeamTriggerMain;
    double BeamTolerance;
    vector<uint32_t> BeamTriggers;

    int CosmicTriggerMain;
    double CosmicTolerance;
    vector<uint32_t> CosmicTriggers;

    int LaserTriggerMain;
    double LaserTolerance;
    vector<uint32_t> LaserTriggers;

    int LEDTriggerMain;
    double LEDTolerance;
    vector<uint32_t> LEDTriggers;

    int AmBeTriggerMain;
    double AmBeTolerance;
    vector<uint32_t> AmBeTriggers;

    int PPSTriggerMain;
    double PPSTolerance;
    vector<uint32_t> PPSTriggers;

    int NuMITriggerMain;
    double NuMITolerance;
    vector<uint32_t> NuMITriggers;
};

#endif
