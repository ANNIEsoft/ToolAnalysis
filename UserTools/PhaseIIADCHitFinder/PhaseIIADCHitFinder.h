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

class PhaseIIADCHitFinder : public Tool {

  public:

    PhaseIIADCHitFinder();
    bool Initialise(const std::string configfile, DataModel& data) override;
    bool Execute() override;
    bool Finalise() override;
    std::map(unsigned long, unsigned short) channel_threshold_map;
  protected:

    //Configurables for HitFinding tool; see README for details
    std::string pulse_finding_approach;
    unsigned short default_adc_threshold;
    std::string threshold_type;
    std::string adc_threshold_db;
    size_t pulse_window_start;
    size_t pulse_window_end;

    std::vector<ADCPulse> find_pulses_bythreshold(
      const Waveform<unsigned short>& raw_minibuffer_data,
      const CalibratedADCWaveform<double>& calibrated_minibuffer_data,
      unsigned short adc_threshold, const unsigned long& channel_key) const;
};
