// standard library includes
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

// reco-annie includes
#include "annie_math.h"
#include "Constants.h"
#include "RawAnalyzer.h"
#include "RawCard.h"
#include "RawChannel.h"
#include "RawReadout.h"

// ToolAnalysis includes
#include "ANNIEconstants.h"

// Anonymous namespace for definitions local to this source file
namespace {

  // All F-distribution probabilities below this value will pass the
  // variance consistency test in ze3ra_baseline()
  constexpr double Q_CRITICAL = 1e-4;

  // Computes the sample mean and sample variance for a vector of numerical
  // values. Based on http://tinyurl.com/mean-var-onl-alg.
  template<typename ElementType> void compute_mean_and_var(
    const std::vector<ElementType>& data, double& mean, double& var,
    size_t sample_cutoff = std::numeric_limits<size_t>::max())
  {
    if ( data.empty() || sample_cutoff == 0) {
      mean = std::numeric_limits<double>::quiet_NaN();
      var = mean;
      return;
    }
    else if (data.size() == 1 || sample_cutoff == 1) {
      mean = data.front();
      var = 0.;
      return;
    }

    size_t num_samples = 0;
    double mean_x2 = 0.;
    mean = 0.;

    for (const ElementType& x : data) {
      ++num_samples;
      double delta = x - mean;
      mean += delta / num_samples;
      double delta2 = x - mean;
      mean_x2 = delta * delta2;
      if (num_samples == sample_cutoff) break;
    }

    var = mean_x2 / (num_samples - 1);
    return;
  }

}

annie::RawAnalyzer::RawAnalyzer()
{
}

const annie::RawAnalyzer& annie::RawAnalyzer::Instance() {

  // Create the raw analyzer using a static variable. This ensures
  // that the singleton instance is only created once.
  static std::unique_ptr<annie::RawAnalyzer>
    the_instance( new annie::RawAnalyzer() );

  // Return a reference to the singleton instance
  return *the_instance;
}

void annie::RawAnalyzer::ze3ra_baseline(const annie::RawChannel& channel,
  double& baseline, double& sigma_baseline,
  size_t num_baseline_samples) const
{
  // Signal ADC means, variances, and F-distribution probability values
  // ("Q") for the first num_baseline_samples from each minibuffer
  std::vector<double> means;
  std::vector<double> variances;
  std::vector<double> Qs;

  // Compute the signal ADC mean and variance for each raw data minibuffer
  for (size_t mb = 0; mb < channel.num_minibuffers(); ++mb) {
    auto mb_data = channel.minibuffer_data(mb);

    double mean, var;
    compute_mean_and_var(mb_data, mean, var, num_baseline_samples);

    means.push_back(mean);
    variances.push_back(var);
  }

  // Compute probabilities for the F-distribution test for each minibuffer
  for (size_t j = 0; j < variances.size() - 1; ++j) {
    double sigma2_j = variances.at(j);
    double sigma2_jp1 = variances.at(j + 1);
    double F;
    if (sigma2_j > sigma2_jp1) F = sigma2_j / sigma2_jp1;
    else F = sigma2_jp1 / sigma2_j;

    double nu = (num_baseline_samples - 1) / 2.;
    double Q = std::tgamma(2*nu)
      * annie_math::Incomplete_Beta_Function(1. / (1. + F), nu, nu)
      / (2. * std::tgamma(nu));

    Qs.push_back(Q);
  }

  // Compute the mean and standard deviation of the baseline signal
  // for this RawChannel using the mean and standard deviation from
  // each minibuffer whose F-distribution probability falls below
  // the critical value.
  baseline = 0.;
  sigma_baseline = 0.;
  size_t num_passing = 0;
  for (size_t k = 0; k < Qs.size(); ++k) {
    if (Qs.at(k) < Q_CRITICAL) {
      ++num_passing;
      baseline += means.at(k);
      sigma_baseline += std::sqrt( variances.at(k) );
    }
  }

  if (num_passing > 0) {
    baseline /= num_passing;
    sigma_baseline /= num_passing;
  }
  else {
    // If none of the minibuffers passed the F-distribution test,
    // choose the one closest to passing and adopt its baseline statistics
    // TODO: consider changing this approach
    auto min_iter = std::min_element(Qs.cbegin(), Qs.cend());
    int min_index = std::distance(Qs.cbegin(), min_iter);

    baseline = means.at(min_index);
    sigma_baseline = std::sqrt( variances.at(min_index) );
  }

}

std::vector<annie::RecoPulse> annie::RawAnalyzer::find_pulses(
  const std::vector<unsigned short>& minibuffer_waveform,
  double baseline, double sigma_baseline, unsigned short adc_threshold) const
{
  std::vector<annie::RecoPulse> pulses;

  unsigned short baseline_plus_one_sigma = static_cast<unsigned short>(
    std::round(baseline + sigma_baseline));

  size_t pulse_start_sample = BOGUS_INT;
  size_t pulse_end_sample = BOGUS_INT;

  size_t num_samples = minibuffer_waveform.size();

  bool in_pulse = false;

  for (size_t s = 0; s < num_samples; ++s) {
    if ( !in_pulse && minibuffer_waveform[s] > adc_threshold ) {
      in_pulse = true;
      pulse_start_sample = s;
    }
    // TODO: consider whether you should force a pulse to end
    // if you reach the end of the minibuffer (note that you
    // only store pulses that have a defined endpoint)
    else if ( in_pulse && ((minibuffer_waveform[s] < baseline_plus_one_sigma)
      || (s == num_samples - 1)) )
    {
      in_pulse = false;
      pulse_end_sample = s;

      // Integrate the pulse to get its area. Use a Riemann sum. Also get
      // the raw amplitude (maximum ADC value within the pulse) and the
      // sample at which the peak occurs.
      unsigned long raw_area = 0; // ADC * samples
      unsigned short max_ADC = std::numeric_limits<unsigned short>::lowest();
      size_t peak_sample = BOGUS_INT;
      for (size_t p = pulse_start_sample; p <= pulse_end_sample; ++p) {
        raw_area += minibuffer_waveform[p];
        if (max_ADC < minibuffer_waveform[p]) {
          max_ADC = minibuffer_waveform[p];
          peak_sample = p;
        }
      }

      // The amplitude of this pulse (V)
      double calibrated_amplitude = (max_ADC - baseline) * ADC_TO_VOLT;

      // The charge detected in this pulse (nC)
      double charge = (raw_area - baseline*(pulse_end_sample
        - pulse_start_sample)) * ADC_TO_VOLT * NS_PER_SAMPLE / ADC_IMPEDANCE;

      // TODO: consider adding code to merge pulses if they occur
      // very close together (the end of one is just a few samples away
      // from the start of another)

      // Store the freshly made pulse in the vector of found pulses
      pulses.emplace_back(pulse_start_sample * NS_PER_SAMPLE,
        peak_sample * NS_PER_SAMPLE, baseline, sigma_baseline,
        raw_area, max_ADC, calibrated_amplitude, charge);
    }
  }

  return pulses;
}

std::vector<annie::RecoPulse> annie::RawAnalyzer::find_pulses(
  const annie::RawChannel& channel, unsigned short adc_threshold) const
{
  std::vector<annie::RecoPulse> pulses;

  // Get estimates for the mean and standard deviation of the baseline in ADC
  // counts
  double baseline, sigma_baseline;
  ze3ra_baseline(channel, baseline, sigma_baseline);

  // Search for pulses within minibuffers, not the full buffer in Hefty mode
  for (size_t mb = 0; mb < channel.num_minibuffers(); ++mb) {
    const auto& data = channel.minibuffer_data(mb);

    std::vector<annie::RecoPulse> pulses_in_minibuffer = find_pulses(data,
      baseline, sigma_baseline, adc_threshold);

    for (const auto& pulse : pulses_in_minibuffer) pulses.push_back(pulse);
  }

  return pulses;
}

std::unique_ptr<annie::RecoReadout> annie::RawAnalyzer::find_pulses(
  const annie::RawReadout& raw_readout) const
{
  // TODO: Switch to using
  // auto reco_readout = std::make_unique<annie::RecoReadout>(
  //   raw_readout.sequence_id());
  // when our Docker image has C++14 support
  auto reco_readout = std::unique_ptr<annie::RecoReadout>(
    new annie::RecoReadout( raw_readout.sequence_id() ));

  for (const auto& card_pair : raw_readout.cards()) {
    const auto& card = card_pair.second;
    for (const auto& channel_pair : card.channels()) {
      const auto& channel = channel_pair.second;

      int card_id = card_pair.first;
      int channel_id = channel_pair.first;

      // Get estimates for the mean and standard deviation of the baseline in
      // ADC counts
      double baseline, sigma_baseline;
      ze3ra_baseline(channel, baseline, sigma_baseline);

      // TODO: Do something better here
      unsigned short adc_threshold = static_cast<unsigned short>(
        std::round(baseline) ) + 7; // baseline + roughly 4.1 mV
      if ( (card_id == 18 && channel_id == 0)
        || (card_id == 4 && channel_id == 1) ) adc_threshold = 357u; // Hefty
      // RWM signals are large square pulses
      if ( card_id == 21 && channel_id == 2 ) adc_threshold = 2000u;

      // Search for pulses within minibuffers, not the full buffer in Hefty
      // mode
      for (size_t mb = 0; mb < channel.num_minibuffers(); ++mb) {
        const auto& data = channel.minibuffer_data(mb);
        auto found_pulses = find_pulses(data, baseline, sigma_baseline,
          adc_threshold);
        reco_readout->add_pulses( card_id, channel_id, mb, found_pulses);
      }
    }
  }

  return reco_readout;
}
