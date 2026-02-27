// Object representing a reconstructed pulse on a single RawChannel
//
// Steven Gardiner <sjgardiner@ucdavis.edu>
#ifndef RECOPULSE_H
#define RECOPULSE_H

// standard library includes
#include <cstddef>

namespace annie {

  class RecoPulse {

    public:

      inline RecoPulse() {}

      RecoPulse(size_t start_time, size_t peak_time, double baseline,
        double sigma_baseline, unsigned long area,
        unsigned short raw_amplitude, double calibrated_amplitude,
        double charge);

      // @brief Returns the start time (ns) of the pulse relative to the
      // start of its minibuffer
      inline size_t start_time() const { return start_time_; }

      // @brief Returns the peak time (ns) of the pulse relative to the
      // start of its minibuffer
      inline size_t peak_time() const { return peak_time_; }

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
      inline double charge() const { return charge_; }

      // @brief Returns the amplitude (V) of the calibrated
      // (baseline-subtracted) pulse
      inline double amplitude() const { return calibrated_amplitude_; }

    protected:

      size_t start_time_; // ns
      size_t peak_time_; // ns
      double baseline_; // mean (ADC)
      double sigma_baseline_; // standard deviation (ADC)
      unsigned long raw_area_; // (ADC * samples)

      unsigned short raw_amplitude_; // ADC
      double calibrated_amplitude_; // V

      double charge_; // nC
      //size_t fwhm_; // ns
  };

}

#endif
