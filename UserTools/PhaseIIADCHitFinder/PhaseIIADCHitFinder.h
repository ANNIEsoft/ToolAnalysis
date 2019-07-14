/* vim:set noexpandtab tabstop=4 wrap */
#ifndef PhaseIIADCHitFinder_H
#define PhaseIIADCHitFinder_H

#pragma once

// standard library includes
#include <string>
#include <iostream>
#include <cmath>
#include <limits>
#include <stdexcept>

// ToolAnalysis includes
#include "ADCPulse.h"
#include "CalibratedADCWaveform.h"
#include "Hit.h"

#include "ANNIEconstants.h"
#include "Geometry.h"
#include "Tool.h"
#include "Waveform.h"
#include "Constants.h"
#include <boost/algorithm/string.hpp>

class PhaseIIADCHitFinder : public Tool {

  public:

    PhaseIIADCHitFinder();
    bool Initialise(const std::string configfile, DataModel& data) override;
    bool Execute() override;
    bool Finalise() override;
    
    // verbosity levels: if 'verbosity' < this level, the message type will be logged.
    int verbosity;
    int v_error=0;
    int v_warning=1;
    int v_message=2;
    int v_debug=3;
  protected:

    //Configurables for HitFinding tool; see README for details
    std::string pulse_finding_approach;
    unsigned short default_adc_threshold;
    std::string threshold_type;
    std::string adc_threshold_db;
    std::string pulse_window_type;
    size_t pulse_window_start_shift;
    size_t pulse_window_end_shift;
    std::map<unsigned long, unsigned short> channel_threshold_map;
    
    // Load a PMT's threshold from the channel_threshold_map. If none, returns default ADC threshold
    unsigned short get_db_threshold(unsigned long channelkey);
    
    // load a channel threshold map from the source file given
    std::map<unsigned long, unsigned short> load_channel_threshold_map(std::string threshold_db);

    // Create a vector of ADCPulse objects using the raw and calibrated signals
    // from a given minibuffer. Note that the vectors of raw and calibrated
    // samples are assumed to be the same size. This function will throw an
    // exception if this assumption is violated.
    std::vector<ADCPulse> find_pulses_bythreshold(
      const Waveform<unsigned short>& raw_minibuffer_data,
      const CalibratedADCWaveform<double>& calibrated_minibuffer_data,
      unsigned short adc_threshold, const unsigned long& channel_key) const;

    //Takes the ADC pulse vectors (one per minibuffer) and converts them to a vector of hits
    std::vector<Hit> convert_adcpulses_to_hits(unsigned long channel_key,std::vector<std::vector<ADCPulse>> pulses);

};

#endif
