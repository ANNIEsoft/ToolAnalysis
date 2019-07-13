// standard library includes
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

// ToolAnalysis includes
#include "PhaseIIADCHitFinder.h"
#include "ADCPulse.h"
#include "ANNIEconstants.h"
#include "CalibratedADCWaveform.h"
#include "Waveform.h"

PhaseIIADCHitFinder::PhaseIIADCHitFinder() : Tool() {}

bool PhaseIIADCHitFinder::Initialise(std::string config_filename, DataModel& data) {

  // Load information from this tool's config file
  if ( !config_filename.empty() )  m_variables.Initialise(config_filename);

  // Assign a transient data pointer
  m_data = &data;

  // Load the default threshold settings for finding pulses
  adc_threshold_db = "none";
  default_adc_threshold = BOGUS_INT;
  pulse_finding_approach = "threshold";
  threshold_type = "relative";
  pulse_window_start = -5;
  pulse_window_end = 30;

  m_variables.Get("PulseFindingApproach", pulse_finding_approach); 
  m_variables.Get("ADCThresholdDB", adc_threshold_db); 
  m_variables.Get("DefaultADCThreshold", default_adc_threshold);
  m_variables.Get("DefaultThresholdType", threshold_type);
  m_variables.Get("PulseWindowStart", pulse_window_start);
  m_variables.Get("PulseWindowEnd", pulse_window_end);


  //TODO: Load the channel_threshold_map with values from the ADCThresholdDB file.
  this->load_channel_threshold_map(adc_threshold_db);
  return true;
}

bool PhaseIIADCHitFinder::Execute() {

  int verbosity;
  m_variables.Get("verbose", verbosity);

  try {
    // Get a pointer to the ANNIEEvent Store
    auto* annie_event = m_data->Stores["ANNIEEvent"];

    if (!annie_event) {
      Log("Error: The PhaseIIADCHitFinder tool could not find the ANNIEEvent Store", 0,
        verbosity);
      return false;
    }

    // Load the map containing the ADC raw waveform data
    std::map<unsigned long, std::vector<Waveform<unsigned short> > >
      raw_waveform_map;

    bool got_raw_data = annie_event->Get("RawADCData", raw_waveform_map);

    // Check for problems
    if ( !got_raw_data ) {
      Log("Error: The PhaseIIADCHitFinder tool could not find the RawADCData entry", 0,
        verbosity);
      return false;
    }
    else if ( raw_waveform_map.empty() ) {
      Log("Error: The PhaseIIADCHitFinder tool found an empty RawADCData entry", 0,
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
      Log("Error: The PhaseIIADCHitFinder tool could not find the CalibratedADCData"
        " entry", 0, verbosity);
      return false;
    }
    else if ( calibrated_waveform_map.empty() ) {
      Log("Error: The PhaseIIADCHitFinder tool found an empty CalibratedADCData entry",
        0, verbosity);
      return false;
    }

    // Build the map of pulses and Hit Map
    std::map<unsigned long, std::vector< std::vector<ADCPulse> > > pulse_map;
    std::map<unsigned long,std::vector<Hit>> hits;

    //Loop through map and find pulses for each PMT channel
    for (const auto& temp_pair : raw_waveform_map) {
      const auto& channel_key = temp_pair.first;
      const auto& raw_waveforms = temp_pair.second;

      const auto& calibrated_waveforms = calibrated_waveform_map.at(channel_key);

      // Ensure that the number of minibuffers is the same between the
      // sets of raw and calibrated waveforms for the current channel
      if ( raw_waveforms.size() != calibrated_waveforms.size() ) {
        Log("Error: The PhaseIIPhaseIIADCHitFinder tool found a set of raw waveforms produced"
          " using a different number of minibuffers than the matching calibrated"
          " waveforms.", 0, verbosity);
        return false;
      }

      //Initialize objects that final pulse information is loaded into
      std::vector< std::vector<ADCPulse> > pulse_vec;
      std::vector<Hit> HitsOnPMT;

      if (pulse_finding_approach == "threshold"){
        // Determine the ADC threshold to use for the current channel
        unsigned short thispmt_adc_threshold = BOGUS_INT;
        // TODO: have a method that gets the PMT's threshold from the
        // Threshold DB.  If the threshold is not available, use the
        // default threshold
        
        size_t num_minibuffers = raw_waveforms.size();
        for (size_t mb = 0; mb < num_minibuffers; ++mb) {
          if (threshold_type == "relative") {
            thispmt_adc_threshold = thispmt_adc_threshold
              + std::round( calibrated_waveforms.at(mb).GetBaseline() );
          }

          if (mb == 0) Log("PhaseIIADCHitFinder will use ADC threshold = "
            + std::to_string(thispmt_adc_threshold) + " for channel "
            + std::to_string( channel_key ),
            2, verbosity);

            pulse_vec.push_back(find_pulses_bythreshold(raw_waveforms.at(mb),
              calibrated_waveforms.at(mb), adc_threshold, channel_key));
        }

        pulse_map[channel_key] = pulse_vec;
      }
    }

    annie_event->Set("RecoADCHits", pulse_map);

    return true;
  }

  catch (const std::exception& except) {
    Log("Error: " + std::string( except.what() ), 0, verbosity);
    return false;
  }
}


bool PhaseIIADCHitFinder::Finalise() {

  return true;
}

bool PhaseIIADCHitFinder::get_db_threshold(unsigned long channelkey,
  unsigned short& this_pmt_threshold){
  //Look in the map and check if channelkey exists.
  if (channel_threshold_map.find(channelkey) == channel_threshold_map.end() ) {
    //not found
    this_pmt_threshold = default_adc_threshold;
    threshold_found = false;
  } else {
    this_pmt_threshold = channel_threshold_map.at(channelkey);
    threshold_found = true;
  }
  return threshold_found;
}

void load_channel_threshold_map(std::string threshold_db);
  std::string fileline;
  ifstream myfile(threshold_db.c_str());
  if (myfile.is_open()){
    //First, get to where data starts
    while(getline(myfile,fileline)){
      if(fileline.find("#")!=std::string::npos) continue;
      std::cout << fileline << std::endl; //has our stuff;
      std::vector<std::string> dataline;
      boost::split(dataline,fileline, boost::is_any_of(","), boost::token_compress_on);
      //TODO: Convert to our data formats and fill into channel map.
      //channel_threshold_map.emplace...
    }
}

// Create a vector of ADCPulse objects using the raw and calibrated signals
// from a given minibuffer. Note that the vectors of raw and calibrated
// samples are assumed to be the same size. This function will throw an
// exception if this assumption is violated.
std::vector<ADCPulse> PhaseIIADCHitFinder::find_pulses_bythreshold(
  const Waveform<unsigned short>& raw_minibuffer_data,
  const CalibratedADCWaveform<double>& calibrated_minibuffer_data,
  unsigned short adc_threshold, const unsigned long& channel_key) const
{
  if ( raw_minibuffer_data.Samples().size()
    != calibrated_minibuffer_data.Samples().size() )
  {
    throw std::runtime_error("Size mismatch between the raw and calibrated"
      " waveforms encountered in PhaseIIADCHitFinder::find_pulses()");
  }

  std::vector<ADCPulse> pulses;

  unsigned short baseline_plus_one_sigma = static_cast<unsigned short>(
    std::round( calibrated_minibuffer_data.GetBaseline()
      + calibrated_minibuffer_data.GetSigmaBaseline() ));

  size_t pulse_start_sample = BOGUS_INT;
  size_t pulse_end_sample = BOGUS_INT;

  size_t num_samples = raw_minibuffer_data.Samples().size();

  bool in_pulse = false;

  //First, we form a list of pulse starts and ends
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

std::vector<ADCPulse> PhaseIIADCHitFinder::find_pulses_byNNLS(
  const Waveform<unsigned short>& raw_minibuffer_data,
  const CalibratedADCWaveform<double>& calibrated_minibuffer_data,
  unsigned short adc_threshold, const unsigned long& channel_key) const
{
  std::cout << " PMT NNLS implementation not complete!!!!!" << std::endl;
  std::cout << " Defaulting to find_pulses_bythreshold" << std::endl;
  std::vector<ADCPulse> thepulses;
  thepulses = this->find_pulses_bythreshold(raw_minibuffer_data, calibrated_minibuffer_data,
  adc_threshold, channel_key);
  return thepulses;
}
