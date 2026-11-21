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
    void FitBRF();		// Original method for BRF Fit
    void FitBRF_v1();		// New method for BRF Fit (Default)
    void FitBES();

    void WriteHist(const std::map<uint64_t, std::vector<uint16_t>> &waveforms, const std::string &prefix);
    double BRFEdgeCrossing(const std::vector<uint16_t> &w, int start, int stop);   // 50% crossing (samples) of one rising edge

private:
    int verbosityFitRWMWaveform;
    int v_error = 0;
    int v_warning = 1;
    int v_message = 2;
    int v_debug = 3;

    bool printToRootFile;
    int maxPrintNumber;
    std::string output_filename;

    int BRFFitVersion = 1;   	// 0 = FitBRF() (original), 1 = FitBRF_v1() (new, default)

    std::vector<uint16_t> RWMRawWaveform;
    std::vector<uint16_t> BRFRawWaveform;
    std::vector<uint16_t> BESRawWaveform;

    std::map<uint64_t, std::vector<uint16_t>> ToBePrintedRWMWaveforms;
    std::map<uint64_t, std::vector<uint16_t>> ToBePrintedBRFWaveforms;
    std::map<uint64_t, std::vector<uint16_t>> ToBePrintedBESWaveforms;

    double RWMRisingStart;      // in ns, sample before the rising edge
    double RWMRisingEnd;        // in ns, top of the rising edge
    double RWMHalfRising;       // in ps, 50% point on the rising edge (interpolated)
    double RWMFHWM;             // in ps, full width at half maximum (interpolated)
    double RWMFirstPeak;        // in ns, first sample above 1600 ADC

    double BRFFirstPeak;        // in ns, highest sample in the first 10 samples
    double BRFAveragePeak;      // in ns, average peak position in 10-sample blocks
    double BRFFirstPeakFit;     // in ps, 50% point on the first full rising edge (interpolated)
    double BRFPeriodFit;        // in ps, BRF period from the 1st and 2nd rising edges, corrected for skipped cycles

    double BESstartTime;        // in ps, 50% point on the negative pulse's leading edge
    double BESendTime;          // in ps, 50% point on the positive pulse's leading edge
    double BESwidth;            // in ps, BESendTime - BESstartTime
};

#endif
