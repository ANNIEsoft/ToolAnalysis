// Class that represents a single pulse from an ADC waveform
//
// Steven Gardiner <sjgardiner@ucdavis.edu>
#pragma once

// standard library includes
#include <cstddef>

// ToolAnalysis includes
#include "ChannelKey.h"
#include "Hit.h"

class ADCPulse : public Hit {

  friend class boost::serialization::access;

  public:

    inline ADCPulse() {}

    // TODO: consider using a ChannelKey object instead of the Hit class's
    // int TubeId member
    ADCPulse(int TubeId, double start_time, double peak_time,
      double baseline, double sigma_baseline, unsigned long raw_area,
      unsigned short raw_amplitude, double calibrated_amplitude,
      double charge);

    // @brief Returns the start time (ns) of the pulse relative to the
    // start of its minibuffer
    inline double start_time() const { return start_time_; }

    // @brief Returns the peak time (ns) of the pulse relative to the
    // start of its minibuffer
    inline double peak_time() const { return peak_time_; }

    // @brief Returns the approximate baseline (ADC) used to calibrate the
    // pulse
    inline double baseline() const { return baseline_; }

    // @brief Returns the approximate error on the baseline (ADC) used to
    // calibrate the pulse
    inline double sigma_baseline() const { return sigma_baseline_; }

    // @brief Returns the area (ADC * samples) of the uncalibrated pulse
    inline unsigned long raw_area() const { return raw_area_; }

    // @brief Returns the amplitude (ADC) of the uncalibrated pulse
    inline unsigned short raw_amplitude() const { return raw_amplitude_; }

    // @brief Returns the charge (nC) of the calibrated (baseline-subtracted)
    // pulse
    // @details No official units have been established for the Charge member
    // of the Hit class yet. If a convention is established that is different
    // than that used for the ADCPulse class (nC), then this function should
    // be modified to perform any necessary conversion to nC.
    inline double charge() const { return GetCharge(); }

    // @brief Returns the amplitude (V) of the calibrated
    // (baseline-subtracted) pulse
    inline double amplitude() const { return calibrated_amplitude_; }

    template <class Archive> void serialize(Archive& ar,
      const unsigned int version)
    {
      Hit::serialize(ar, version);

      ar & start_time_;
      ar & peak_time_;
      ar & baseline_;
      ar & sigma_baseline_;
      ar & raw_area_;
      ar & raw_amplitude_;
      ar & calibrated_amplitude_;
    }

  protected:

    double start_time_; // ns since beginning of minibuffer
    double peak_time_; // ns since beginning of minibuffer
    double baseline_; // mean (ADC)
    double sigma_baseline_; // standard deviation (ADC)
    unsigned long raw_area_; // (ADC * samples)

    unsigned short raw_amplitude_; // ADC
    double calibrated_amplitude_; // V
};
