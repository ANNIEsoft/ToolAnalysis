// standard library includes
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

// ToolAnalysis includes
#include "ADCHitFinder.h"
#include "ADCPulse.h"
#include "ANNIEconstants.h"
#include "CalibratedADCWaveform.h"
#include "Waveform.h"

ADCHitFinder::ADCHitFinder() : Tool() {}

bool ADCHitFinder::Initialise(std::string config_filename, DataModel& data) {

  // Load information from this tool's config file
  if ( !config_filename.empty() )  m_variables.Initialise(config_filename);

  // Assign a transient data pointer
  m_data = &data;

  return true;
}

bool ADCHitFinder::Execute() {

  int verbosity;
  m_variables.Get("verbose", verbosity);

  try {
    // Get a pointer to the ANNIEEvent Store
    auto* annie_event = m_data->Stores["ANNIEEvent"];

    if (!annie_event) {
      Log("Error: The ADCHitFinder tool could not find the ANNIEEvent Store", 0,
        verbosity);
      return false;
    }

    // Load the map containing the ADC raw waveform data
    std::map<unsigned long, std::vector<Waveform<unsigned short> > >
      raw_waveform_map;

    bool got_raw_data = annie_event->Get("RawADCData", raw_waveform_map);

    // Check for problems
    if ( !got_raw_data ) {
      Log("Error: The ADCHitFinder tool could not find the RawADCData entry", 0,
        verbosity);
      return false;
    }
    else if ( raw_waveform_map.empty() ) {
      Log("Error: The ADCHitFinder tool found an empty RawADCData entry", 0,
        verbosity);
      return false;
    }

    // Load the map containing the ADC calibrated waveform data
    std::map<unsigned long, std::vector<CalibratedADCWaveform<double> > >
      calibrated_waveform_map;

    bool got_calibrated_data = annie_event->Get("CalibratedADCData",
      calibrated_waveform_map);

    // Check for problems
    if ( !got_calibrated_data ) {
      Log("Error: The ADCHitFinder tool could not find the CalibratedADCData"
        " entry", 0, verbosity);
      return false;
    }
    else if ( calibrated_waveform_map.empty() ) {
      Log("Error: The ADCHitFinder tool found an empty CalibratedADCData entry",
        0, verbosity);
      return false;
    }

    // Load the default threshold settings for finding pulses
    unsigned short default_adc_threshold = BOGUS_INT;
    m_variables.Get("DefaultADCThreshold", default_adc_threshold);

    std::string default_threshold_type;
    m_variables.Get("DefaultThresholdType", default_threshold_type);

    // Build the map of pulses
    std::map<unsigned long, std::vector< std::vector<ADCPulse> > > pulse_map;

    for (const auto& temp_pair : raw_waveform_map) {
      const auto& channel_key = temp_pair.first;
      const auto& raw_waveforms = temp_pair.second;

      const auto& calibrated_waveforms = calibrated_waveform_map.at(channel_key);

      // Ensure that the number of minibuffers is the same between the
      // sets of raw and calibrated waveforms for the current channel
      if ( raw_waveforms.size() != calibrated_waveforms.size() ) {
        Log("Error: The ADCHitFinder tool found a set of raw waveforms produced"
          " using a different number of minibuffers than the matching calibrated"
          " waveforms.", 0, verbosity);
        return false;
      }

      // Determine the ADC threshold to use for the current channel
      unsigned short adc_threshold = BOGUS_INT;
      // If the user has not set a specific ADC threshold for the current
      // channel, then use the default
      bool user_set_threshold = m_variables.Get("ADCThresholdForChannel"
        + std::to_string( channel_key ), adc_threshold);
      if ( !user_set_threshold ) adc_threshold = default_adc_threshold;

      std::vector< std::vector<ADCPulse> > pulse_vec;

      size_t num_minibuffers = raw_waveforms.size();
      for (size_t mb = 0; mb < num_minibuffers; ++mb) {
        // If the user hasn't set an explicit ADC threshold for this channel
        // and has set the default threshold type to "relative", then use the
        // calibrated waveform's baseline to determine the appropriate ADC
        // threshold for the current minibuffer
        if (!user_set_threshold && default_threshold_type == "relative") {
          adc_threshold = default_adc_threshold
            + std::round( calibrated_waveforms.at(mb).GetBaseline() );
        }

        if (mb == 0) Log("ADCHitFinder will use ADC threshold = "
          + std::to_string(adc_threshold) + " for channel "
          + std::to_string( channel_key ),
          2, verbosity);

          pulse_vec.push_back(find_pulses(raw_waveforms.at(mb),
            calibrated_waveforms.at(mb), adc_threshold, channel_key));
      }

      pulse_map[channel_key] = pulse_vec;
    }

    annie_event->Set("RecoADCHits", pulse_map);

    return true;
  }

  catch (const std::exception& except) {
    Log("Error: " + std::string( except.what() ), 0, verbosity);
    return false;
  }
}


bool ADCHitFinder::Finalise() {

  return true;
}

// Create a vector of ADCPulse objects using the raw and calibrated signals
// from a given minibuffer. Note that the vectors of raw and calibrated
// samples are assumed to be the same size. This function will throw an
// exception if this assumption is violated.
std::vector<ADCPulse> ADCHitFinder::find_pulses(
  const Waveform<unsigned short>& raw_minibuffer_data,
  const CalibratedADCWaveform<double>& calibrated_minibuffer_data,
  unsigned short adc_threshold, const unsigned long& channel_key) const
{
  if ( raw_minibuffer_data.Samples().size()
    != calibrated_minibuffer_data.Samples().size() )
  {
    throw std::runtime_error("Size mismatch between the raw and calibrated"
      " waveforms encountered in ADCHitFinder::find_pulses()");
  }

  std::vector<ADCPulse> pulses;

  unsigned short baseline_plus_one_sigma = static_cast<unsigned short>(
    std::round( calibrated_minibuffer_data.GetBaseline()
      + calibrated_minibuffer_data.GetSigmaBaseline() ));

  size_t pulse_start_sample = BOGUS_INT;
  size_t pulse_end_sample = BOGUS_INT;

  size_t num_samples = raw_minibuffer_data.Samples().size();

  bool in_pulse = false;

  for (size_t s = 0; s < num_samples; ++s) {
    if ( !in_pulse && raw_minibuffer_data.GetSample(s) > adc_threshold ) {
      in_pulse = true;
      pulse_start_sample = s;
    }
    // In the second test below, we force a pulse to end if we reach the end of
    // the minibuffer.
    // TODO: revisit whether this is the best thing to do
    else if ( in_pulse && ((raw_minibuffer_data.GetSample(s)
      < baseline_plus_one_sigma) || (s == num_samples - 1)) )
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
        raw_area += raw_minibuffer_data.GetSample(p);
        if (max_ADC < raw_minibuffer_data.GetSample(p)) {
          max_ADC = raw_minibuffer_data.GetSample(p);
          peak_sample = p;
        }
      }

      // The amplitude of the pulse (V)
      double calibrated_amplitude
        = calibrated_minibuffer_data.GetSample(peak_sample);

      // Calculated the charge detected in this pulse (nC)
      // using the calibrated waveform
      double charge = 0.;
      // Integrate the calibrated pulse (to get a quantity in V * samples)
      for (size_t p = pulse_start_sample; p <= pulse_end_sample; ++p) {
        charge += calibrated_minibuffer_data.GetSample(p);
      }

      // Convert the pulse integral to nC
      charge *= NS_PER_ADC_SAMPLE / ADC_IMPEDANCE;

      // TODO: consider adding code to merge pulses if they occur
      // very close together (the end of one is just a few samples away
      // from the start of another)

      // Store the freshly made pulse in the vector of found pulses
      pulses.emplace_back(channel_key,
        ( pulse_start_sample * NS_PER_SAMPLE ),
        peak_sample * NS_PER_SAMPLE,
        calibrated_minibuffer_data.GetBaseline(),
        calibrated_minibuffer_data.GetSigmaBaseline(),
        raw_area, max_ADC, calibrated_amplitude, charge);
    }
  }

  return pulses;
}
