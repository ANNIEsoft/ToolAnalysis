// Singleton class that contains reconstruction algorithms to apply
// to RawReadout objects.
//
// Steven Gardiner <sjgardiner@ucdavis.edu>
#ifndef RAWANALYZER_H
#define RAWANALYZER_H

// standard library includes
#include <vector>

// reco-annie includes
#include "RawReadout.h"
#include "RecoPulse.h"
#include "RecoReadout.h"

namespace annie {

  // Forward-declare the RawChannel class
  class RawChannel;

  /// @brief Singleton analyzer class for reconstructing ANNIE events
  /// from the raw data
  class RawAnalyzer {

    public:

      /// @brief Deleted copy constructor
      RawAnalyzer(const RawAnalyzer&) = delete;

      /// @brief Deleted move constructor
      RawAnalyzer(RawAnalyzer&&) = delete;

      /// @brief Deleted copy assignment operator
      RawAnalyzer& operator=(const RawAnalyzer&) = delete;

      /// @brief Deleted move assignment operator
      RawAnalyzer& operator=(RawAnalyzer&&) = delete;

      /// @brief Get a const reference to the singleton instance of the
      /// RawAnalyzer
      static const RawAnalyzer& Instance();

      std::vector<RecoPulse> find_pulses(const annie::RawChannel& channel,
        unsigned short adc_threshold) const;

      std::vector<annie::RecoPulse> find_pulses(
        const std::vector<unsigned short>& minibuffer_waveform,
        double baseline, double sigma_baseline, unsigned short adc_threshold)
        const;

      std::unique_ptr<annie::RecoReadout> find_pulses(
        const annie::RawReadout& raw_readout) const;

    protected:

      /// @brief Create the singleton RawAnalyzer object
      RawAnalyzer();

      // The default number of samples to use when computing the baseline
      // for each minibuffer (or each pre-trigger subregion in non-Hefty mode)
      // using the ZE3RA method
      static constexpr size_t DEFAULT_NUM_BASELINE_SAMPLES = 25;

      /// @brief Compute the baseline for a particular RawChannel
      /// object using a technique taken from the ZE3RA code.
      /// @details See section 2.2 of https://arxiv.org/pdf/1106.0808.pdf for a
      /// full description of the algorithm.
      void ze3ra_baseline(const annie::RawChannel& channel, double& baseline,
        double& sigma_baseline, size_t num_baseline_samples
        = DEFAULT_NUM_BASELINE_SAMPLES) const;
  };

}

#endif
