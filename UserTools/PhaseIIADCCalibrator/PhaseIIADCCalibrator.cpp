#include <string>

// ToolAnalysis includes
#include "PhaseIIADCCalibrator.h"

PhaseIIADCCalibrator::PhaseIIADCCalibrator() : Tool() {}

bool PhaseIIADCCalibrator::Initialise(std::string config_filename, DataModel& data)
{
  // Load configuration file variables
  if ( !config_filename.empty() ) m_variables.Initialise(config_filename);

  // Assign a transient data pointer
  m_data = &data;
  //Set defaults in case config file has no entries
  p_critical = 0.01;
  num_sub_waveforms = 6;
  num_baseline_samples = 5;
  adc_window_db = "none";

  m_variables.Get("verbosity", verbosity);
  m_variables.Get("PCritical", p_critical);
  m_variables.Get("NumBaselineSamples", num_baseline_samples);
  m_variables.Get("NumSubWaveforms", num_sub_waveforms);
  m_variables.Get("MakeCalLEDWaveforms",make_led_waveforms);
  m_variables.Get("WindowIntegrationDB", adc_window_db); 
  

  if(adc_window_db != "none") channel_window_map = this->load_window_map(adc_window_db);

  return true;
}

bool PhaseIIADCCalibrator::Execute() {
  
  if(verbosity) std::cout<<"Initializing Tool PhaseIIADCCalibrator"<<std::endl;

  // Get a pointer to the ANNIEEvent Store
  auto* annie_event = m_data->Stores["ANNIEEvent"];

  if (!annie_event) {
    Log("Error: The PhaseIIADCCalibrator tool could not find the ANNIEEvent Store", 0,
      verbosity);
    return false;
  }

  // Load the map containing the ADC raw waveform data
  std::map<unsigned long, std::vector<Waveform<unsigned short> > >
    raw_waveform_map;

  bool got_raw_data = annie_event->Get("RawADCData", raw_waveform_map);

  // Check for problems
  if ( !got_raw_data ) {
    Log("Error: The PhaseIIADCCalibrator tool could not find the RawADCData entry", 0,
      verbosity);
    return false;
  }
  else if ( raw_waveform_map.empty() ) {
    Log("Error: The PhaseIIADCCalibrator tool found an empty RawADCData entry", 0,
      verbosity);
    return false;
  }

  // Build the calibrated waveforms
  std::map<unsigned long, std::vector<CalibratedADCWaveform<double> > >
    calibrated_waveform_map;

  // Load the map containing the ADC raw waveform data
  std::map<unsigned long, std::vector<Waveform<unsigned short> > >
    raw_led_waveform_map;

  std::map<unsigned long, std::vector<CalibratedADCWaveform<double> > >
    calibrated_led_waveform_map;

  for (const auto& temp_pair : raw_waveform_map) {
    const auto& channel_key = temp_pair.first;
    //Default running: raw_waveforms only has one entry.  If we go to a
    //hefty-mode style of running though, this could have multiple minibuffers
    const auto& raw_waveforms = temp_pair.second;
    Log("Making calibrated waveforms for ADC channel " +
      std::to_string(channel_key), 3, verbosity);

    calibrated_waveform_map[channel_key] = make_calibrated_waveforms(
      raw_waveforms);

    if(make_led_waveforms){
      Log("Also making LED window waveforms for ADC channel " +
        std::to_string(channel_key), 3, verbosity);

      //To dos:
      //  - First, create a new vector of raw waveforms that have the 
      //    pulse window and the earlier window used to estimate background
      //  - Estimate the baseline and sigma with ze3ra in the beginning of the
      //    window
      //  - Make calibrated waveforms for each LED window
      std::vector<Waveform<unsigned short>> LEDWaveforms;
      this->make_raw_led_waveforms(channel_key,raw_waveforms,LEDWaveforms);
      raw_led_waveform_map.emplace(channel_key,LEDWaveforms);
      calibrated_led_waveform_map[channel_key] = make_calibrated_waveforms(
        LEDWaveforms);
    }
  }

  std::cout <<"Setting CalibratedADCData"<<std::endl;
  annie_event->Set("CalibratedADCData", calibrated_waveform_map);
  if(make_led_waveforms){
    std::cout <<"Setting LEDADCData"<<std::endl;
    annie_event->Set("CalibratedLEDADCData", calibrated_led_waveform_map);
    annie_event->Set("RawLEDADCData", raw_led_waveform_map);
  }
  std::cout <<"Set CalibratedADCData"<<std::endl;

  return true;
}


bool PhaseIIADCCalibrator::Finalise() {
  return true;
}

void PhaseIIADCCalibrator::ze3ra_baseline(
  const  Waveform<unsigned short> raw_data,
  double& baseline, double& sigma_baseline, size_t num_baseline_samples)
{

  // Signal ADC means, variances, and F-distribution probability values
  // ("P") for the first num_baseline_samples from each minibuffer
  // (in Hefty mode) or from each sub-minibuffer (in non-Hefty mode)
  std::vector<double> means;
  std::vector<double> variances;
  std::vector<double> Ps;

  // Using the Phase I non-hefty algorithm. Split the early part of the waveform 
  // into sub-minibuffers and compute the mean and variance of each one.
  const auto& data = raw_data.Samples();
  for (size_t sub_mb = 0u; sub_mb < num_sub_waveforms; ++sub_mb) {
    std::vector<unsigned short> sub_mb_data(
      data.cbegin() + sub_mb * num_baseline_samples,
      data.cbegin() + (1u + sub_mb) * num_baseline_samples);

    double mean, var;
    ComputeMeanAndVariance(sub_mb_data, mean, var, num_baseline_samples);

    means.push_back(mean);
    variances.push_back(var);
  }

  // Compute probabilities for the F-distribution test for each waveform chunk
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
PhaseIIADCCalibrator::make_calibrated_waveforms(
  const std::vector< Waveform<unsigned short> >& raw_waveforms)
{

  // Determine the baseline for the set of raw waveforms (assumed to all
  // come from the same readout for the same channel)
  std::vector< CalibratedADCWaveform<double> > calibrated_waveforms;
  for (const auto& raw_waveform : raw_waveforms) {
    double baseline, sigma_baseline;
    ze3ra_baseline(raw_waveform, baseline, sigma_baseline,
      num_baseline_samples);
      
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

void PhaseIIADCCalibrator::make_raw_led_waveforms(unsigned long channel_key,
  const std::vector< Waveform<unsigned short> > raw_waveforms,
  std::vector< Waveform<unsigned short> >& raw_led_waveforms)
{
  //Get the windows for this channel key
  std::vector<std::vector<int>> thispmt_adc_windows;
  thispmt_adc_windows = this->get_db_windows(channel_key);
  for(int j=0; j<raw_waveforms.size(); j++){
    for(int i = 0; i<thispmt_adc_windows.size();i++){
      // Make a waveform out of this subwindow, plus the number of samples
      // used for background estimation
      std::vector<int> awindow = thispmt_adc_windows.at(i);
      int windowmin = awindow.at(0) - ((int)num_baseline_samples*(int)num_sub_waveforms);
      if(windowmin<0){
        std::cout << "PhaseIIADCCalibrator Tool WARNING: when making an " <<
            "LED window, there was not enough room prior to the window to " <<
            "include samples for a background estimate.  Don't put your window "<<
           " too close to zero ADC counts." << std::endl;
        windowmin = 0;
      }
      int windowmax = awindow.at(1);

      std::vector<uint16_t> led_waveform;
      //FIXME: want start time of window, not of the full raw waveform
      uint64_t start_time = raw_waveforms.at(j).GetStartTime();
      for (size_t s = windowmin; s < windowmax; ++s) {
        led_waveform.push_back(raw_waveforms.at(j).GetSample(s));
      }
      Waveform<uint16_t> led_rawwave{start_time,led_waveform};
      raw_led_waveforms.push_back(led_rawwave);
    }
  }
  return;
}

std::vector<std::vector<int>> PhaseIIADCCalibrator::get_db_windows(unsigned long channelkey){
    std::vector<std::vector<int>> this_pmt_windows;
  //Look in the map and check if channelkey exists.
  if (channel_window_map.find(channelkey) == channel_window_map.end() ) {
     if (verbosity>3){
       std::cout << "PhaseIIADCHitFinder Warning: no integration windows found" <<
       "for channel_key" << channelkey <<". Not finding pulses." << std::endl;
       }
  } else {
    // gottem
    this_pmt_windows = channel_window_map.at(channelkey);
  }
  return this_pmt_windows;
}

std::map<unsigned long, std::vector<std::vector<int>>> PhaseIIADCCalibrator::load_window_map(std::string window_db)
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
    Log("PhaseIIADCHitFinder Tool: Input integration window DB file not found. "
        " no integration will occur. ",
        1, verbosity);
  }
  return chanwindowmap;
}
