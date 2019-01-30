#pragma once

#include "Waveform.h"

template <typename T> class CalibratedADCWaveform : public Waveform<T> {

  friend class boost::serialization::access;

  public:

    CalibratedADCWaveform() : Waveform<T>(), fBaseline(0.),
      fSigmaBaseline(0.) {}
    CalibratedADCWaveform(const double& tc, const std::vector<T>& samples,
      double baseline, double sigma_bl)
      : Waveform<T>(tc, samples), fBaseline(baseline),
      fSigmaBaseline(sigma_bl) {}

    inline double GetBaseline() const { return fBaseline; }
    inline double GetSigmaBaseline() const { return fSigmaBaseline; }

  protected:

    /// @brief Estimated baseline (ADC counts) determined when performing
    /// the calibration
    double fBaseline;

    /// @brief Uncertainty (standard deviation) of the estimated baseline
    /// (ADC counts)
    double fSigmaBaseline;

    // Override the Waveform class's serialize() method so that we
    // can store the extra calibration data
    template<class Archive> void serialize(Archive & ar,
      const unsigned int version)
    {
      Waveform<T>::template serialize<Archive>(ar, version);

      ar & fBaseline;
      ar & fSigmaBaseline;
    }

};
