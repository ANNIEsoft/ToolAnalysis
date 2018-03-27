// This tool creates a CalibratedWaveform object for each RawWaveform object
// that it finds stored under the "RawADCData" key in the ANNIEEvent store.
// It saves the CalibratedWaveform objects to the ANNIEEvent store using the
// key "CalibratedADCData".
//
// Steven Gardiner <sjgardiner@ucdavis.edu>
#pragma once

// ToolAnalysis includes
#include "CalibratedADCWaveform.h"
#include "Tool.h"
#include "Waveform.h"

class ADCCalibrator : public Tool {

  public:

    ADCCalibrator();
    bool Initialise(const std::string configfile,DataModel& data) override;
    bool Execute() override;
    bool Finalise() override;

  protected:

    void ze3ra_baseline(const std::vector< Waveform<unsigned short> >& raw_data,
      double& baseline, double& sigma_baseline, size_t num_baseline_samples);

    std::vector< CalibratedADCWaveform<double> > make_calibrated_waveforms(
      const std::vector< Waveform<unsigned short> >& raw_waveforms);
};
