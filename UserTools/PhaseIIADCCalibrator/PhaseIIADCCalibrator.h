// This tool creates a CalibratedWaveform object for each RawWaveform object
// that it finds stored under the "RawADCData" key in the ANNIEEvent store.
// It saves the CalibratedWaveform objects to the ANNIEEvent store using the
// key "CalibratedADCData".
//
// Phase I version by Steven Gardiner <sjgardiner@ucdavis.edu>
// Modified for Phase II by Teal Pershing <tjpershing@ucdavis.edu>
#pragma once

// ToolAnalysis includes
#include "CalibratedADCWaveform.h"
#include "Tool.h"
#include "Waveform.h"
#include "annie_math.h"
#include "ANNIEalgorithms.h"
#include "ANNIEconstants.h"

class PhaseIIADCCalibrator : public Tool {

  public:

    PhaseIIADCCalibrator();
    bool Initialise(const std::string configfile,DataModel& data) override;
    bool Execute() override;
    bool Finalise() override;

  protected:

    /// @brief Compute the baseline for a particular RawChannel
    /// object using a technique taken from the ZE3RA code.
    /// @details See section 2.2 of https://arxiv.org/pdf/1106.0808.pdf for a
    /// description of the algorithm.
    void ze3ra_baseline(const std::vector< Waveform<unsigned short> >& raw_data,
      double& baseline, double& sigma_baseline, size_t num_baseline_samples);

    std::vector< CalibratedADCWaveform<double> > make_calibrated_waveforms(
      const std::vector< Waveform<unsigned short> >& raw_waveforms);
  
    int verbosity;
    // All F-distribution probabilities above this value will pass the
    // variance consistency test in ze3ra_baseline(). That is, p_critical
    // is the maximum p-value for which we will reject the null hypothesis
    // of equal variances.
    double p_critical;
    
    size_t num_baseline_samples;
    size_t num_sub_waveforms;
};
