#ifndef FitRWMWaveform_H
#define FitRWMWaveform_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TFile.h"
#include "TH1D.h"
#include <numeric>  // for std::accumulate
#include <TGraph.h>
#include <TF1.h>

/**
 * \class FitRWMWaveform
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
 *
 * $Author: B.Richards $
 * $Date: 2019/05/28 10:44:00 $
 * Contact: b.richards@qmul.ac.uk
 */
class FitRWMWaveform : public Tool
{

public:
    FitRWMWaveform();                                         ///< Simple constructor
    bool Initialise(std::string configfile, DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
    bool Execute();                                           ///< Execute function used to perform Tool purpose.
    bool Finalise();                                          ///< Finalise function used to clean up resources.

    void FitRWM();
    void FitBRF();

private:
    int verbosityFitRWMWaveform;
    int v_message = 1;
    int v_warning = 2;
    int v_error = 3;
    int v_debug = 4;

    bool printToRootFile;
    int maxPrintNumber;
    std::string output_filename;

    std::vector<uint16_t> RWMRawWaveform;
    std::vector<uint16_t> BRFRawWaveform;

    std::map<uint64_t, std::vector<uint16_t>> ToBePrintedRWMWaveforms;
    std::map<uint64_t, std::vector<uint16_t>> ToBePrintedBRFWaveforms;

    double RWMRisingStart;
    double RWMRisingEnd;
    double RWMHalfRising;
    double RWMFHWM;
    double RWMFirstPeak;

    double BRFFirstPeak;
    double BRFAveragePeak;
    double BRFFirstPeakFit;
};

#endif
