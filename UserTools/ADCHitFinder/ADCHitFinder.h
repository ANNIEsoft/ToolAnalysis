// This tool finds PMT pulses using both raw and calibrated ADC waveforms
//
// Steven Gardiner <sjgardiner@ucdavis.edu>
#pragma once

// ToolAnalysis includes
#include "ADCPulse.h"
#include "CalibratedADCWaveform.h"
#include "Tool.h"
#include "Waveform.h"
#include "Constants.h"

class ADCHitFinder : public Tool {

  public:

    ADCHitFinder();
    bool Initialise(const std::string configfile, DataModel& data) override;
    bool Execute() override;
    bool Finalise() override;

  protected:

    std::vector<ADCPulse> find_pulses(
      const Waveform<unsigned short>& raw_minibuffer_data,
      const CalibratedADCWaveform<double>& calibrated_minibuffer_data,
      unsigned short adc_threshold, const unsigned long& channel_key) const;
};
