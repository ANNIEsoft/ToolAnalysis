#ifndef ProcessedLAPPDFilter_H
#define ProcessedLAPPDFilter_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "PsecData.h"
#include "Waveform.h"
#include "BoostStore.h"
#include "Store.h"
#include "ADCPulse.h"
#include "TimeClass.h"
#include "TriggerClass.h"
#include "ANNIEalgorithms.h"
#include "CalibratedADCWaveform.h"
#include "BeamStatus.h"
#include "Geometry.h"

/**
 * \class ProcessedLAPPDFilter
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
 *
 * $Author: B.Richards $
 * $Date: 2019/05/28 10:44:00 $
 * Contact: b.richards@qmul.ac.uk
 */
class ProcessedLAPPDFilter : public Tool
{

public:
    ProcessedLAPPDFilter();                                   ///< Simple constructor
    bool Initialise(std::string configfile, DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
    bool Execute();                                           ///< Execute function used to perform Tool purpose.
    bool Finalise();                                          ///< Finalise function used to clean up resources.

    bool GotANNIEEventAndSave(BoostStore *BS, string savePath);

private:
    std::map<std::string, bool> DataStreams;

    int FilterVerbosity;

    string MRDDataName;
    string MRDNoVetoDataName;
    string AllLAPPDDataName;
    string PMTClusterDataName;

    string filterType;

    BoostStore *FilteredMRD = nullptr;
    BoostStore *FilteredMRDNoVeto = nullptr;
    BoostStore *FilteredAllLAPPD = nullptr;
    BoostStore *FilteredPMTCluster = nullptr;

    bool gotEventMRD;
    bool gotEventMRDNoVeto;
    bool gotEventPMTCluster;

    bool saveAllLAPPDEvents;
    bool savePMTClusterEvents;

    int EventMRDNumber;
    int EventMRDNoVetoNumber;
    int EventPMTClusterNumber;
    int CurrentExeNumber;
    int pairedEventNumber;
    int PsecDataNumber;



    int currentRunNumber;

    double requirePulsedAmp;
    int requirePulsedStripNumber;

    int passCutEventNumber;
    int PMTClusterEventNum;
    bool RequireNoVeto;
    bool RequireCherenkovCover;


};

#endif
