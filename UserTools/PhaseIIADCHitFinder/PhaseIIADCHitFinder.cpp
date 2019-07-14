// ToolAnalysis includes
#include "PhaseIIADCHitFinder.h"

PhaseIIADCHitFinder::PhaseIIADCHitFinder() : Tool() {}

bool PhaseIIADCHitFinder::Initialise(std::string config_filename, DataModel& data) {

  // Load information from this tool's config file
  if ( !config_filename.empty() )  m_variables.Initialise(config_filename);

  // Assign a transient data pointer
  m_data = &data;

  // Load the default threshold settings for finding pulses
  verbosity = 3;
  adc_threshold_db = "none";
  default_adc_threshold = BOGUS_INT;
  pulse_finding_approach = "threshold";
  threshold_type = "relative";
  pulse_window_type = "fixed";
  pulse_window_start_shift = -3;
  pulse_window_end_shift = 25;

  //Load any configurables set in the config file
  m_variables.Get("verbosity",verbosity); 
  m_variables.Get("PulseFindingApproach", pulse_finding_approach); 
  m_variables.Get("ADCThresholdDB", adc_threshold_db); 
  m_variables.Get("DefaultADCThreshold", default_adc_threshold);
  m_variables.Get("DefaultThresholdType", threshold_type);
  m_variables.Get("PulseWindowType", pulse_window_type);
  m_variables.Get("PulseWindowStart", pulse_window_start_shift);
  m_variables.Get("PulseWindowEnd", pulse_window_end_shift);

  if ((pulse_window_start_shift > 0) || (pulse_window_end_shift) < 0){
    Log("PhaseIIADCHitFinder Tool: WARNING... trigger threshold crossing will not be inside pulse window.  Threshold" 
      " setting of PhaseIIADCHitFinder tool may behave improperly.", v_error,
      verbosity);
  }

  if(adc_threshold_db != "none") channel_threshold_map = this->load_channel_threshold_map(adc_threshold_db);
  
  return true;
}

bool PhaseIIADCHitFinder::Execute() {

  try {
    // Get a pointer to the ANNIEEvent Store
    auto* annie_event = m_data->Stores["ANNIEEvent"];

    if (!annie_event) {
      Log("Error: The PhaseIIADCHitFinder tool could not find the ANNIEEvent Store", v_error,
        verbosity);
      return false;
    }

    // Load the map containing the ADC raw waveform data
    std::map<unsigned long, std::vector<Waveform<unsigned short> > >
      raw_waveform_map;

    bool got_raw_data = annie_event->Get("RawADCData", raw_waveform_map);

    // Check for problems
    if ( !got_raw_data ) {
      Log("Error: The PhaseIIADCHitFinder tool could not find the RawADCData entry", v_error,
        verbosity);
      return false;
    }
    else if ( raw_waveform_map.empty() ) {
      Log("Error: The PhaseIIADCHitFinder tool found an empty RawADCData entry", v_error,
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
        " entry", v_error, verbosity);
      return false;
    }
    else if ( calibrated_waveform_map.empty() ) {
      Log("Error: The PhaseIIADCHitFinder tool found an empty CalibratedADCData entry",
        v_error, verbosity);
      return false;
    }

    // Build the map of pulses and Hit Map
    std::map<unsigned long, std::vector< std::vector<ADCPulse> > > pulse_map;
    std::map<unsigned long,std::vector<Hit>> hit_map;

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
          " waveforms.", v_error, verbosity);
        return false;
      }

      //Initialize objects that final pulse information is loaded into
      std::vector< std::vector<ADCPulse> > pulse_vec;
      std::vector<Hit> HitsOnPMT;

      if (pulse_finding_approach == "threshold"){
        // Determine the ADC threshold to use for the current channel
        unsigned short thispmt_adc_threshold = BOGUS_INT;
        thispmt_adc_threshold = this->get_db_threshold(channel_key);

        //For each minibuffer, adjust threshold for baseline calibration and find pulses
        size_t num_minibuffers = raw_waveforms.size();
        for (size_t mb = 0; mb < num_minibuffers; ++mb) {
          if (threshold_type == "relative") {
            thispmt_adc_threshold = thispmt_adc_threshold
              + std::round( calibrated_waveforms.at(mb).GetBaseline() );
          }

          if (mb == 0) Log("PhaseIIADCHitFinder: First minibuffer will use ADC threshold = "
            + std::to_string(thispmt_adc_threshold) + " for channel "
            + std::to_string( channel_key ),
            2, verbosity);

            pulse_vec.push_back(this->find_pulses_bythreshold(raw_waveforms.at(mb),
              calibrated_waveforms.at(mb), thispmt_adc_threshold, channel_key));
        }
        
        //Fill pulse map with all ADCPulses found
        pulse_map.emplace(channel_key,pulse_vec);
        //Convert ADCPulses to Hits and fill into Hit map
        HitsOnPMT = this->convert_adcpulses_to_hits(channel_key,pulse_vec);
        hit_map.emplace(channel_key, HitsOnPMT);
      
      } else if (pulse_finding_approach == "NNLS") {
        Log("PhaseIIADCHitFinder: NNLS approach is not implemented.  please use threshold.",
            0, verbosity);
      } 
    }
    
    //Store the pulse and hit maps in the ANNIEEvent store
    annie_event->Set("RecoADCHits", pulse_map);
    annie_event->Set("Hits", hit_map);
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

   

unsigned short PhaseIIADCHitFinder::get_db_threshold(unsigned long channelkey){
  unsigned short this_pmt_threshold = default_adc_threshold;
  //Look in the map and check if channelkey exists.
  if (channel_threshold_map.find(channelkey) == channel_threshold_map.end() ) {
     if (verbosity>v_warning){
       std::cout << "PhaseIIADCHitFinder Warning: no channel threshold found" <<
       "for channel_key" << channelkey <<". Using default threshold" << std::endl;
       }
  } else {
    // gottem
    this_pmt_threshold = channel_threshold_map.at(channelkey);
  }
  return this_pmt_threshold;
}

std::map<unsigned long, unsigned short> PhaseIIADCHitFinder::load_channel_threshold_map(std::string threshold_db){
  std::map<unsigned long, unsigned short> chanthreshmap;
  std::string fileline;
  ifstream myfile(threshold_db.c_str());
  if (myfile.is_open()){
    while(getline(myfile,fileline)){
      if(fileline.find("#")!=std::string::npos) continue;
      std::cout << fileline << std::endl; //has our stuff;
      std::vector<std::string> dataline;
      boost::split(dataline,fileline, boost::is_any_of(","), boost::token_compress_on);
      unsigned long chanvalue = std::stoul(dataline.at(0));
      unsigned short threshvalue = (unsigned short) std::stoul(dataline.at(1));
      if(chanthreshmap.count(chanvalue)==0) chanthreshmap.emplace(chanvalue,threshvalue);
      else {
        Log("PhaseIIADCHitFinder Error: tried loading more than one channel threshold for a "
             "single channel!  Channel num is " + std::to_string(chanvalue),
            v_error, verbosity);
      }
    }
  } else {
    Log("PhaseIIADCHitFinder Tool: Input threshold DB file not found. "
        " all channels will be assigned the default threshold. ",
        v_warning, verbosity);
  }
  return chanthreshmap;
}

std::vector<ADCPulse> PhaseIIADCHitFinder::find_pulses_bythreshold(
  const Waveform<unsigned short>& raw_minibuffer_data,
  const CalibratedADCWaveform<double>& calibrated_minibuffer_data,
  unsigned short adc_threshold, const unsigned long& channel_key) const
{
  //Sanity check that raw/calibrated minibuffers are same size
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

  bool in_pulse = false;
  size_t num_samples = raw_minibuffer_data.Samples().size();

  //Fixed integration window defined relative to ADC threshold crossings
  if(pulse_window_type == "fixed"){
    //Start and end of windows treated as a single "pulse"
    std::vector<int> window_starts;
    std::vector<int> window_ends;

    //First, we form a list of pulse starts and ends
    for (size_t s = 0; s < num_samples; ++s) {
      //check if sample is within an already defined window
      for (int i=0; i< window_starts.size(); i++){
        if ((s>window_starts.at(i)) && (s<window_starts.at(i))) in_pulse = true;
      //if sample crosses threshold and isn't in a defined window, define a new window
        if (!in_pulse && (raw_minibuffer_data.GetSample(s) > adc_threshold) ) {
          window_starts.push_back(static_cast<int>(s) + pulse_window_start_shift);
          window_ends.push_back(static_cast<int>(s) + pulse_window_end_shift);
        }
      }  
    }
    //If any pulse crosses the sampling window, restrict it's value to within window
    for (int j=0; j<window_starts.size(); j++){
      if (window_starts.at(j) < 0) window_starts.at(j) = 0;
      if (window_ends.at(j) < static_cast<int>(num_samples)) window_ends.at(j) = static_cast<int>(num_samples);
    }
    // Integrate the pulse to get its area. Use a Riemann sum. Also get
    // the raw amplitude (maximum ADC value within the pulse) and the
    // sample at which the peak occurs.
    for (int i = 0; i< window_starts.size(); i++){
      size_t pulse_start_sample = static_cast<size_t>(window_starts.at(i));
      size_t pulse_end_sample = static_cast<size_t>(window_ends.at(i));
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

      // Store the freshly made pulse in the vector of found pulses
      pulses.emplace_back(channel_key,
        ( pulse_start_sample * NS_PER_SAMPLE ),
        peak_sample * NS_PER_SAMPLE,
        calibrated_minibuffer_data.GetBaseline(),
        calibrated_minibuffer_data.GetSigmaBaseline(),
        raw_area, max_ADC, calibrated_amplitude, charge);
    }
  
  // Peak windows are defined only by crossing and un-crossing of ADC threshold
  } else if(pulse_window_type == "dynamic"){
    size_t pulse_start_sample = BOGUS_INT;
    size_t pulse_end_sample = BOGUS_INT;
    for (size_t s = 0; s < num_samples; ++s) {
      if ( !in_pulse && raw_minibuffer_data.GetSample(s) > adc_threshold ) {
        in_pulse = true;
        pulse_start_sample = s;
      }
      // In the second test below, we force a pulse to end if we reach the end of
      // the minibuffer.
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
        // FIXME: We need a static database with each PMT's impedance
        charge *= NS_PER_ADC_SAMPLE / ADC_IMPEDANCE;

        // TODO: consider adding code to merge pulses if they occur
        // very close together (i.e. if the end of one is just a few samples away
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
  } else {
    if(verbosity > v_error){
      std::cout << "PhaseIIADCHitFinder Tool error: Pulse window type not recognized. Please pick fixed or dynamic" << std::endl;
    } 
  }
  return pulses;
}

std::vector<Hit> PhaseIIADCHitFinder::convert_adcpulses_to_hits(unsigned long channel_key,std::vector<std::vector<ADCPulse>> pulses){
  std::vector<Hit> thispmt_hits;
  for(int i=0; i < pulses.size(); i++){
    std::vector<ADCPulse> apulsevector = pulses.at(i);
    for(int j=0; j < apulsevector.size(); j++){
      ADCPulse apulse = apulsevector.at(j);
      //Get the time and charge
      double time = apulse.peak_time();
      double charge = apulse.charge();
      Hit ahit(channel_key, time, charge);
      thispmt_hits.push_back(ahit);
    } 
  }
  return thispmt_hits;
}
 
