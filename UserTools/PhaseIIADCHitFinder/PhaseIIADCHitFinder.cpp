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
  use_led_waveforms = false;
  pulse_finding_approach = "threshold";
  adc_threshold_db = "none";
  default_adc_threshold = 5;
  threshold_type = "relative";
  pulse_window_type = "fixed";
  pulse_window_start_shift = -3;
  pulse_window_end_shift = 25;
  adc_window_db = "none"; //Used when pulse_finding_approach="fixed_windows"

  //Load any configurables set in the config file
  m_variables.Get("verbosity",verbosity); 
  m_variables.Get("UseLEDWaveforms", use_led_waveforms); 
  m_variables.Get("PulseFindingApproach", pulse_finding_approach); 
  m_variables.Get("ADCThresholdDB", adc_threshold_db);
  m_variables.Get("DefaultADCThreshold", default_adc_threshold);
  m_variables.Get("DefaultThresholdType", threshold_type);
  m_variables.Get("PulseWindowType", pulse_window_type);
  m_variables.Get("PulseWindowStart", pulse_window_start_shift);
  m_variables.Get("PulseWindowEnd", pulse_window_end_shift);
  m_variables.Get("WindowIntegrationDB", adc_window_db); 

  if ((pulse_window_start_shift > 0) || (pulse_window_end_shift) < 0){
    Log("PhaseIIADCHitFinder Tool: WARNING... trigger threshold crossing will not be inside pulse window.  Threshold" 
      " setting of PhaseIIADCHitFinder tool may behave improperly.", v_error,
      verbosity);
  }

  if(adc_window_db=="none" && pulse_finding_approach=="fixed_windows"){
    Log("PhaseIIADCHitFinder Tool ERROR: Fixed integration window approach specified, but no CSV file with" 
      " windows for any channels defined.", v_error,
      verbosity);
    return false;
  }

  auto get_geometry= m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",geom);
  if(!get_geometry){
  	Log("DigitBuilder Tool: Error retrieving Geometry from ANNIEEvent!",v_error,verbosity); 
  	return false; 
  }

  //Load window and threshold CSV files if defined
  if(adc_threshold_db != "none") channel_threshold_map = this->load_channel_threshold_map(adc_threshold_db);
  if(adc_window_db != "none") channel_window_map = this->load_integration_window_map(adc_window_db);

  //Set in CStore for tools to know and log this later 
  m_data->CStore.Set("ADCThreshold",default_adc_threshold);

  // Get the Auxiliary channel types; identifies which channels are SiPM channels
  m_data->CStore.Get("AuxChannelNumToTypeMap",AuxChannelNumToTypeMap);

  //Recreate maps that were deleted with ANNIEEvent->Delete() ANNIEEventBuilder tool
  hit_map = new std::map<unsigned long,std::vector<Hit>>;
  aux_hit_map = new std::map<unsigned long,std::vector<Hit>>;

  return true;
}

bool PhaseIIADCHitFinder::Execute() {
  this->ClearMaps();

  try {
    //Recreate maps that were deleted with ANNIEEvent->Delete() ANNIEEventBuilder tool
    hit_map = new std::map<unsigned long,std::vector<Hit>>;
    aux_hit_map = new std::map<unsigned long,std::vector<Hit>>;

    // Get a pointer to the ANNIEEvent Store
    auto* annie_event = m_data->Stores.at("ANNIEEvent");

    if (!annie_event) {
      Log("Error: The PhaseIIADCHitFinder tool could not find the ANNIEEvent Store", v_error,
        verbosity);
      return false;
    }

    // Load the maps containing the ADC raw waveform data
    bool got_raw_data = false;
    bool got_rawaux_data = false;
    if(use_led_waveforms){
      got_raw_data = annie_event->Get("RawLEDADCData", raw_waveform_map);
    } else {
      got_raw_data = annie_event->Get("RawADCData", raw_waveform_map);
      got_rawaux_data = annie_event->Get("RawADCAuxData", raw_aux_waveform_map);
    }
    // Check for problems
    if ( !got_raw_data ) {
      Log("Error: The PhaseIIADCHitFinder tool could not find the RawADCData entry", v_error,
        verbosity);
      return false;
    }
    if ( !got_rawaux_data ) {
      Log("Error: The PhaseIIADCHitFinder tool could not find the RawADCAuxData entry", v_error,
        verbosity);
      return false;
    }
    else if ( raw_waveform_map.empty() ) {
      Log("Error: The PhaseIIADCHitFinder tool found an empty RawADCData entry", v_error,
        verbosity);
      return false;
    }
    
    // Load the maps containing the ADC calibrated waveform data
    bool got_calibrated_data = false;
    bool got_calibratedaux_data = false;
    if(use_led_waveforms){
      got_calibrated_data = annie_event->Get("CalibratedLEDADCData",
        calibrated_waveform_map);
    } else {
      got_calibrated_data = annie_event->Get("CalibratedADCData",
        calibrated_waveform_map);
      got_calibratedaux_data = annie_event->Get("CalibratedADCAuxData", calibrated_aux_waveform_map);
    }

    // Check for problems
    if ( !got_calibrated_data ) {
      Log("Error: The PhaseIIADCHitFinder tool could not find the CalibratedADCData"
        " entry", v_error, verbosity);
      return false;
    }
    if ( !got_calibratedaux_data ) {
      Log("Error: The PhaseIIADCHitFinder tool could not find the CalibratedADCAuxData"
        " entry", v_error, verbosity);
      return false;
    }
    else if ( calibrated_waveform_map.empty() ) {
      Log("Error: The PhaseIIADCHitFinder tool found an empty CalibratedADCData entry",
        v_error, verbosity);
      return false;
    }

    //Find pulses in the raw detector data
    for (const auto& temp_pair : raw_waveform_map) {
      const auto& achannel_key = temp_pair.first;
      const auto& araw_waveforms = temp_pair.second;
      //Don't make hit objects for any offline channels
      Channel* thischannel = geom->GetChannel(achannel_key);
      if(thischannel->GetStatus() == channelstatus::OFF) continue;
      std::vector<CalibratedADCWaveform<double> > acalibrated_waveforms = calibrated_waveform_map.at(achannel_key);
      bool MadeMaps = this->build_pulse_and_hit_map(achannel_key, araw_waveforms, acalibrated_waveforms, pulse_map,*hit_map);
      if(!MadeMaps){
        Log("PhaseIIADCHitFinder Error: problem making PMT hit and pulse maps", 0, verbosity);
        return false;
      }
    }
    Log("PhaseIIADCHitFinder Tool: setting PMT RecoADCHits in annie event", v_debug, verbosity);
    annie_event->Set("RecoADCHits", pulse_map);
    Log("PhaseIIADCHitFinder Tool: setting PMT Hits in annie event", v_debug, verbosity);
    annie_event->Set("Hits", hit_map,true);

    Log("PhaseIIADCHitFinder Tool: Finding SiPM pulses in auxiliary channels", v_debug, verbosity);
    //Find pulses in the raw auxiliary channel data
    for (const auto& temp_pair : raw_aux_waveform_map) {
      const auto& achannel_key = temp_pair.first;
      if(AuxChannelNumToTypeMap->at(achannel_key) != "SiPM1" &&
        AuxChannelNumToTypeMap->at(achannel_key) != "SiPM2") continue; 
      const auto& araw_waveforms = temp_pair.second;
      std::vector<CalibratedADCWaveform<double> > acalibrated_waveforms = calibrated_aux_waveform_map.at(achannel_key);
      bool MadeAuxMaps = this->build_pulse_and_hit_map(achannel_key, araw_waveforms, acalibrated_waveforms, aux_pulse_map,*aux_hit_map);
      if(!MadeAuxMaps){
        Log("PhaseIIADCHitFinder Error: problem making  Aux hit and pulse maps", 0, verbosity);
        return false;
      }
    }
    Log("PhaseIIADCHitFinder Tool: setting RecoADCAuxHits in annie event", v_debug, verbosity);
    annie_event->Set("RecoADCAuxHits", aux_pulse_map);
    Log("PhaseIIADCHitFinder Tool: setting AuxHits in annie event", v_debug, verbosity);
    annie_event->Set("AuxHits", aux_hit_map,true);
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

std::vector<std::vector<int>> PhaseIIADCHitFinder::get_db_windows(unsigned long channelkey){
    std::vector<std::vector<int>> this_pmt_windows;
  //Look in the map and check if channelkey exists.
  if (channel_window_map.find(channelkey) == channel_window_map.end() ) {
     if (verbosity>v_debug){
       std::cout << "PhaseIIADCHitFinder Warning: no integration windows found" <<
       "for channel_key" << channelkey <<". Not finding pulses." << std::endl;
       }
  } else {
    // gottem
    this_pmt_windows = channel_window_map.at(channelkey);
  }
  return this_pmt_windows;
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

std::map<unsigned long, std::vector<std::vector<int>>> PhaseIIADCHitFinder::load_integration_window_map(std::string window_db)
{
  std::map<unsigned long, std::vector<std::vector<int>>> chanwindowmap;
  std::string fileline;
  ifstream myfile(window_db.c_str());
  if (myfile.is_open()){
    while(getline(myfile,fileline)){
      if(fileline.find("#")!=std::string::npos) continue;
      std::cout << fileline << std::endl; //has our stuff;
      std::vector<std::string> dataline;
      boost::split(dataline,fileline, boost::is_any_of(","), boost::token_compress_on);
      unsigned long chanvalue = std::stoul(dataline.at(0));
      int windowminvalue = std::stoi(dataline.at(1));
      int windowmaxvalue = std::stoi(dataline.at(2));
      std::vector<int> window{windowminvalue,windowmaxvalue};
      if(chanwindowmap.count(chanvalue)==0){ // Place window range into integral windows
        std::vector<std::vector<int>> window_vector{window};
        chanwindowmap.emplace(chanvalue,window_vector);
      }
      else { // Add this window to the windows to integrate in
        chanwindowmap.at(chanvalue).push_back(window);
      }
    }
  } else {
    Log("PhaseIIADCHitFinder Tool ERROR! Input integration window DB file not found! "
        " no integration will occur. ",
        v_warning, verbosity);
  }
  return chanwindowmap;
}

bool PhaseIIADCHitFinder::build_pulse_and_hit_map(
  unsigned long channel_key,
  std::vector<Waveform<unsigned short> > raw_waveforms, 
  std::vector<CalibratedADCWaveform<double> > calibrated_waveforms,
  std::map<unsigned long, std::vector< std::vector<ADCPulse>> > & pmap,
  std::map<unsigned long,std::vector<Hit>>& hmap)
{

  // Ensure that the number of minibuffers is the same between the
  // sets of raw and calibrated waveforms for the current channel
  if ( raw_waveforms.size() != calibrated_waveforms.size() ) {
    Log("Error: The PhaseIIPhaseIIADCHitFinder tool found a set of raw waveforms produced"
      " using a different number of waveforms than the matching calibrated"
      " waveforms.", v_error, verbosity);
    return false;
  }

  //Initialize objects that final pulse information is loaded into
  std::vector< std::vector<ADCPulse> > pulse_vec;
  std::vector<Hit> HitsOnPMT;

  size_t num_minibuffers = raw_waveforms.size();
  if (pulse_finding_approach == "full_window"){

    // Integrate each whole dang minibuffer and background subtract 
    for (size_t mb = 0; mb < num_minibuffers; ++mb) {
        Waveform<unsigned short> buffer_wave = raw_waveforms.at(mb);
        int window_end = buffer_wave.GetSamples()->size()-1;
        std::vector<int> fullwindow{0,window_end};
        std::vector<std::vector<int>> onewindowvec{fullwindow};
        pulse_vec.push_back(this->find_pulses_bywindow(raw_waveforms.at(mb),
          calibrated_waveforms.at(mb), onewindowvec, channel_key,false));
    }
  }

  if (pulse_finding_approach == "full_window_maxpeak"){
    // Integrate each whole dang minibuffer and background subtract 
    for (size_t mb = 0; mb < num_minibuffers; ++mb) {
        Waveform<unsigned short> buffer_wave = raw_waveforms.at(mb);
        int window_end = buffer_wave.GetSamples()->size()-1;
        std::vector<int> fullwindow{0,window_end};
        std::vector<std::vector<int>> onewindowvec{fullwindow};
        pulse_vec.push_back(this->find_pulses_bywindow(raw_waveforms.at(mb),
          calibrated_waveforms.at(mb), onewindowvec, channel_key,true));
    }
  }

  if (pulse_finding_approach == "fixed_windows"){
    // Integrate pulse over a fixed windows defined for channel 
    std::vector<std::vector<int>> thispmt_adc_windows;
    thispmt_adc_windows = this->get_db_windows(channel_key);

    //For each minibuffer, integrate window to get pulses
    for (size_t mb = 0; mb < num_minibuffers; ++mb) {
        pulse_vec.push_back(this->find_pulses_bywindow(raw_waveforms.at(mb),
          calibrated_waveforms.at(mb), thispmt_adc_windows, channel_key,false));
    }
  }

  else if (pulse_finding_approach == "threshold"){
    // Determine the ADC threshold to use for the current channel
    unsigned short thispmt_adc_threshold = BOGUS_INT;
    thispmt_adc_threshold = this->get_db_threshold(channel_key);

    //For each minibuffer, adjust threshold for baseline calibration and find pulses
    for (size_t mb = 0; mb < num_minibuffers; ++mb) {
      if (threshold_type == "relative") {
        thispmt_adc_threshold = thispmt_adc_threshold
          + std::round( calibrated_waveforms.at(mb).GetBaseline() );
      }

      if (mb == 0) Log("PhaseIIADCHitFinder: Waveform will use ADC threshold = "
        + std::to_string(thispmt_adc_threshold) + " for channel "
        + std::to_string( channel_key ),
        2, verbosity);

        pulse_vec.push_back(this->find_pulses_bythreshold(raw_waveforms.at(mb),
          calibrated_waveforms.at(mb), thispmt_adc_threshold, channel_key));
    }
  } 
  
  else if (pulse_finding_approach == "NNLS") {
    Log("PhaseIIADCHitFinder: NNLS approach is not implemented.  please use threshold.",
        0, verbosity);
  }

  //Fill pulse map with all ADCPulses found
  Log("PhaseIIADCHitFinder: Filling pulse map.",
      v_debug, verbosity);
  if(verbosity > v_debug) std::cout << "Number of pulses in pulse_vec's first entry: " << pulse_vec.at(0).size() << std::endl;
  pmap.emplace(channel_key,pulse_vec);
  //Convert ADCPulses to Hits and fill into Hit map
  HitsOnPMT = this->convert_adcpulses_to_hits(channel_key,pulse_vec);
  Log("PhaseIIADCHitFinder: Filling hit map.",
      v_debug, verbosity);
  for(int j=0; j < HitsOnPMT.size(); j++){
    Hit ahit = HitsOnPMT.at(j);
    if(hmap.count(channel_key)==0) hmap.emplace(channel_key, std::vector<Hit>{ahit});
    else hmap.at(channel_key).push_back(ahit);
  }
  return true;
}

std::vector<ADCPulse> PhaseIIADCHitFinder::find_pulses_bywindow(
  const Waveform<unsigned short>& raw_minibuffer_data,
  const CalibratedADCWaveform<double>& calibrated_minibuffer_data,
  std::vector<std::vector<int>> adc_windows, const unsigned long& channel_key,
  bool MaxHeightPulseOnly) const
{
  //Sanity check that raw/calibrated minibuffers are same size
  if ( raw_minibuffer_data.Samples().size()
    != calibrated_minibuffer_data.Samples().size() )
  {
    throw std::runtime_error("Size mismatch between the raw and calibrated"
      " waveforms encountered in PhaseIIADCHitFinder::find_pulses_bywindow()");
  }
  
  if (verbosity>v_debug){
    std::cout << "PhaseIIADCHitFinder integrating windows now..." <<
    "in signal of PMT ID " << channel_key << std::endl;
  }
  
  std::vector<ADCPulse> pulses;

  for(int i = 0; i<adc_windows.size();i++){
    // Integrate the pulse to get its area. Use a Riemann sum. Also get
    // the raw amplitude (maximum ADC value within the pulse) and the
    // sample at which the peak occurs.
    std::vector<int> awindow = adc_windows.at(i);
    size_t wmin = static_cast<size_t>(awindow.at(0));
    size_t wmax = static_cast<size_t>(awindow.at(1));
    unsigned short max_ADC = std::numeric_limits<unsigned short>::lowest();
    size_t peak_sample = BOGUS_INT;
    for (size_t p = wmin; p <= wmax; ++p) {
      if (max_ADC < raw_minibuffer_data.GetSample(p)) {
        max_ADC = raw_minibuffer_data.GetSample(p);
        peak_sample = p;
      }
    }

    // The amplitude of the pulse (V)
    double calibrated_amplitude
      = calibrated_minibuffer_data.GetSample(peak_sample);

    // Calculated the charge (nC) and raw ADC counts
    // using the raw and calibrated waveform
    double charge = 0.;
    unsigned long raw_area = 0; // ADC * samples

    if(MaxHeightPulseOnly){
      // From that peak sample, sum up to either side until finding a
      // sample that's 10% of the max height
      charge += calibrated_amplitude;
      raw_area += max_ADC;
      size_t pulsewinleft = peak_sample -1;
      size_t pulsewinright = peak_sample + 1;
      bool integrated_leftward = false;
      bool integrated_rightward = false;
      while (!integrated_leftward && peak_sample!=wmin){
        double sample_height = calibrated_minibuffer_data.GetSample(pulsewinleft);
        if (sample_height > (0.1 * calibrated_amplitude) && (pulsewinleft>wmin)){
          raw_area += raw_minibuffer_data.GetSample(pulsewinleft);
          charge += sample_height;
          pulsewinleft-=1;
        }
        else integrated_leftward = true;
      }
      while (!integrated_rightward && peak_sample!=wmax){
        double sample_height = calibrated_minibuffer_data.GetSample(pulsewinright);
        if (sample_height > (0.1 * calibrated_amplitude) && (pulsewinright < wmax)){
          raw_area += raw_minibuffer_data.GetSample(pulsewinright);
          charge += sample_height;
          pulsewinright+=1;
        }
        else integrated_rightward = true;
      }
      wmin = pulsewinleft;
      wmax = pulsewinright;
    } else {
      // Integrate the calibrated pulse (to get a quantity in V * samples)
      for (size_t p = wmin; p <= wmax; ++p) {
        raw_area += raw_minibuffer_data.GetSample(p);
        charge += calibrated_minibuffer_data.GetSample(p);
      }
    }

    // Convert the pulse integral to nC
    charge *= NS_PER_ADC_SAMPLE / ADC_IMPEDANCE;


    // Store the freshly made pulse in the vector of found pulses
    pulses.emplace_back(channel_key,
      ( wmin * NS_PER_SAMPLE ),
      peak_sample * NS_PER_SAMPLE,
      calibrated_minibuffer_data.GetBaseline(),
      calibrated_minibuffer_data.GetSigmaBaseline(),
      raw_area, max_ADC, calibrated_amplitude, charge);
  }
  return pulses;
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
      " waveforms encountered in PhaseIIADCHitFinder::find_pulses_bythreshold()");
  }
  
  if (verbosity>v_debug){
    std::cout << "PhaseIIADCHitFinder searcing for pulses now..." <<
    "in signal of PMT ID " << channel_key << std::endl;
  }
  
  std::vector<ADCPulse> pulses;

  unsigned short baseline_plus_one_sigma = static_cast<unsigned short>(
    std::round( calibrated_minibuffer_data.GetBaseline()
      + calibrated_minibuffer_data.GetSigmaBaseline() ));

  bool in_pulse = false;
  size_t num_samples = raw_minibuffer_data.Samples().size()-50;

  //Fixed integration window defined relative to ADC threshold crossings
  if(pulse_window_type == "fixed"){
    //Start and end of windows treated as a single "pulse"
    std::vector<int> window_starts;
    std::vector<int> window_ends;

    //First, we form a list of pulse starts and ends
    for (size_t s = 0; s < num_samples; ++s) {
      in_pulse = false;
      //check if sample is within an already defined window
      for (int i=0; i< window_starts.size(); i++){
        if ((s>window_starts.at(i)) && (s<window_ends.at(i))){
          in_pulse = true;
          if(verbosity>4) std::cout << "PhaseIIADCHitFinder: FOUND PULSE" << std::endl;
        }
      }  
      //if sample crosses threshold and isn't in a defined window, define a new window
        if (!in_pulse && (raw_minibuffer_data.GetSample(s) > adc_threshold) ) {
          window_starts.push_back(static_cast<int>(s) + pulse_window_start_shift);
          window_ends.push_back(static_cast<int>(s) + pulse_window_end_shift);
        }
    }
    //If any pulse crosses the sampling window, restrict it's value to within window
    for (int j=0; j<window_starts.size(); j++){
      if (window_starts.at(j) < 0) window_starts.at(j) = 0;
      if (window_ends.at(j) > static_cast<int>(num_samples)) window_ends.at(j) = static_cast<int>(num_samples)-1;
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
        if(verbosity>4) std::cout << "PhaseIIADCHitFinder: FOUND PULSE" << std::endl;
        if(static_cast<int>(s)-5 < 0) {
          pulse_start_sample = 0;
        } else {
          pulse_start_sample = s-5;
        }
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
          ( pulse_start_sample * NS_PER_ADC_SAMPLE ),
          peak_sample * NS_PER_ADC_SAMPLE,
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
  if(verbosity > v_debug) std::cout << "Number of pulses in channels pulse vector: " << pulses.size() << std::endl;
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

void PhaseIIADCHitFinder::ClearMaps(){
  if(!pulse_map.empty()) pulse_map.clear();
  if(!aux_pulse_map.empty()) aux_pulse_map.clear();
  //if(!hit_map->empty()) hit_map->clear();
  //if(!aux_hit_map->empty()) aux_hit_map->clear();
}   
