#include <string>

// ToolAnalysis includes
#include "ADCCalibrator.h"
#include "ANNIEalgorithms.h"
#include "ANNIEconstants.h"

ADCCalibrator::ADCCalibrator() : Tool() {}

bool ADCCalibrator::Initialise(std::string config_filename, DataModel& data)
{
  // Load configuration file variables
  if ( !config_filename.empty() ) m_variables.Initialise(config_filename);

  // Assign a transient data pointer
  m_data = &data;

  return true;
}

bool ADCCalibrator::Execute() {

  int verbosity;
  m_variables.Get("verbose", verbosity);

  // Get a pointer to the ANNIEEvent Store
  auto* annie_event = m_data->Stores["ANNIEEvent"];

  if (!annie_event) {
    Log("Error: The ADCCalibrator tool could not find the ANNIEEvent Store", 0,
      verbosity);
    return false;
  }

  // Load the map containing the ADC raw waveform data
  std::map<unsigned long, std::vector<Waveform<unsigned short> > >
    raw_waveform_map;

  bool got_raw_data = annie_event->Get("RawADCData", raw_waveform_map);

  // Check for problems
  if ( !got_raw_data ) {
    Log("Error: The ADCCalibrator tool could not find the RawADCData entry", 0,
      verbosity);
    return false;
  }
  else if ( raw_waveform_map.empty() ) {
    Log("Error: The ADCCalibrator tool found an empty RawADCData entry", 0,
      verbosity);
    return false;
  }

  // Build the calibrated waveforms
  std::map<unsigned long, std::vector<CalibratedADCWaveform<double> > >
    calibrated_waveform_map;

  for (const auto& temp_pair : raw_waveform_map) {
    const auto& channel_key = temp_pair.first;
    const auto& raw_waveforms = temp_pair.second;

    Log("Making calibrated waveforms for ADC channel " +
      std::to_string(channel_key), 3, verbosity);

    calibrated_waveform_map[channel_key] = make_calibrated_waveforms(
      raw_waveforms);
  }

  annie_event->Set("CalibratedADCData", calibrated_waveform_map);

  return true;
}


bool ADCCalibrator::Finalise() {
  return true;
}

void ADCCalibrator::ze3ra_baseline(
  const std::vector< Waveform<unsigned short> >& raw_data,
  double& baseline, double& sigma_baseline, size_t num_baseline_samples)
{
  int verbosity;
  m_variables.Get("verbose", verbosity);

  // All F-distribution probabilities above this value will pass the
  // variance consistency test in ze3ra_baseline(). That is, p_critical
  // is the maximum p-value for which we will reject the null hypothesis
  // of equal variances.
  double p_critical;
  m_variables.Get("PCritical", p_critical);

  // Signal ADC means, variances, and F-distribution probability values
  // ("P") for the first num_baseline_samples from each minibuffer
  // (in Hefty mode) or from each sub-minibuffer (in non-Hefty mode)
  std::vector<double> means;
  std::vector<double> variances;
  std::vector<double> Ps;

  // Hefty mode uses multiple minibuffers, non-Hefty mode uses a single
  // minibuffer
  // TODO: check that raw_data is not empty
  bool hefty_mode = raw_data.size() > 1u;

  if (hefty_mode) {
    // Compute the signal ADC mean and variance for each raw data minibuffer
    for (size_t mb = 0; mb < raw_data.size(); ++mb) {
      const auto& mb_data = raw_data.at(mb).Samples();

      double mean, var;
      ComputeMeanAndVariance(mb_data, mean, var, num_baseline_samples);

      means.push_back(mean);
      variances.push_back(var);
    }
  }
  else {
    size_t num_sub_minibuffers;
    m_variables.Get("NumSubMinibuffers", num_sub_minibuffers);

    // For non-Hefty data, split the early part of the single minibuffer
    // into sub-minibuffers and compute the mean and variance of each one.
    const auto& data = raw_data.front().Samples();

    // TODO: remove hard-coded stuff here
    // TODO: rather than copying the data to a new vector here, adapt your
    // mean and variance function appropriately
    for (size_t sub_mb = 0u; sub_mb < num_sub_minibuffers; ++sub_mb) {
      std::vector<unsigned short> sub_mb_data(
        data.cbegin() + sub_mb * num_baseline_samples,
        data.cbegin() + (1u + sub_mb) * num_baseline_samples);

      double mean, var;
      ComputeMeanAndVariance(sub_mb_data, mean, var, num_baseline_samples);

      means.push_back(mean);
      variances.push_back(var);
    }
  }

  // Compute probabilities for the F-distribution test for each minibuffer
  for (size_t j = 0; j < variances.size() - 1; ++j) {
    double sigma2_j = variances.at(j);
    double sigma2_jp1 = variances.at(j + 1);
    double F;
    if (sigma2_j > sigma2_jp1) F = sigma2_j / sigma2_jp1;
    else F = sigma2_jp1 / sigma2_j;

    double nu = (num_baseline_samples - 1) / 2.;
    double P = annie_math::Regularized_Beta_Function(1. / (1. + F), nu, nu);

    // Two-tailed hypothesis test (we need to exclude unusually small values
    // as well as unusually large ones). The tails have equal sizes, so we
    // may use symmetry and simply multiply our earlier result by 2.
    P *= 2.;

    // I've never seen this problem (the numerical values for the regularized
    // beta function that I've checked all fall within [0,1]), but the book
    // Numerical Recipes includes this check in a similar block of code,
    // so I'll add it just in case.
    if (P > 1.) P = 2. - P;

    Ps.push_back(P);
  }

  // Compute the mean and standard deviation of the baseline signal
  // for this RawChannel using the mean and standard deviation from
  // each minibuffer whose F-distribution probability falls below
  // the critical value.
  baseline = 0.;
  sigma_baseline = 0.;
  double variance_baseline = 0.;
  size_t num_passing = 0;
  for (size_t k = 0; k < Ps.size(); ++k) {
    if (Ps.at(k) > p_critical) {
      ++num_passing;
      baseline += means.at(k);
      variance_baseline += variances.at(k);
    }
  }

  if (num_passing > 1) {
    baseline /= num_passing;

    variance_baseline *= static_cast<double>(num_baseline_samples - 1)
      / (num_passing*num_baseline_samples - 1);
    // Now that we've combined the sample variances correctly, take the
    // square root to get the standard deviation
    sigma_baseline = std::sqrt( variance_baseline );
  }
  else if (num_passing == 1) {
    // We only have one set of sample statistics, so all we need to
    // do is take the square root of the variance to get the standard
    // deviation.
    sigma_baseline = std::sqrt( variance_baseline );
  }
  else {
    // If none of the minibuffers passed the F-distribution test,
    // choose the one closest to passing (i.e., the one with the largest
    // P-value) and adopt its baseline statistics. For a sufficiently large
    // number of minibuffers (e.g., 40), such a situation should be very rare.
    // TODO: consider changing this approach
    auto max_iter = std::max_element(Ps.cbegin(), Ps.cend());
    int max_index = std::distance(Ps.cbegin(), max_iter);

    baseline = means.at(max_index);
    sigma_baseline = std::sqrt( variances.at(max_index) );
  }

  std::string mb_temp_string = "minibuffer";
  if ( !hefty_mode ) mb_temp_string = "sub-minibuffer";

  if (verbosity >= 4) {
    for ( size_t x = 0; x < Ps.size(); ++x ) {
      Log("  " + mb_temp_string + " " + std::to_string(x) + ", mean = "
        + std::to_string(means.at(x)) + ", var = "
        + std::to_string(variances.at(x)) + ", p-value = "
        + std::to_string(Ps.at(x)), 4, verbosity);
    }
  }

  Log(std::to_string(num_passing) + " " + mb_temp_string + " pairs passed the"
    " F-test", 3, verbosity);
  Log("Baseline estimate: " + std::to_string(baseline) + " ± "
    + std::to_string(sigma_baseline) + " ADC counts", 3, verbosity);

}

std::vector< CalibratedADCWaveform<double> >
ADCCalibrator::make_calibrated_waveforms(
  const std::vector< Waveform<unsigned short> >& raw_waveforms)
{
  size_t num_baseline_samples;
  m_variables.Get("NumBaselineSamples", num_baseline_samples);

  // Determine the baseline for the set of raw waveforms (assumed to all
  // come from the same readout for the same channel)
  double baseline, sigma_baseline;
  ze3ra_baseline(raw_waveforms, baseline, sigma_baseline,
    num_baseline_samples);

  std::vector< CalibratedADCWaveform<double> > calibrated_waveforms;
  for (const auto& raw_waveform : raw_waveforms) {

    std::vector<double> cal_data;
    const std::vector<unsigned short>& raw_data = raw_waveform.Samples();

    for (const auto& sample : raw_data) {
      cal_data.push_back((static_cast<double>(sample) - baseline)
        * ADC_TO_VOLT);
    }

    calibrated_waveforms.emplace_back(raw_waveform.GetStartTime(),
      cal_data, baseline, sigma_baseline);
  }

  return calibrated_waveforms;
}
