#include <string>

#include "TROOT.h"
#include "TSystem.h"
#include "TApplication.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TH1.h"
#include "TF1.h"
#include "TFitResult.h"

#include <thread>
#include <chrono>

// ToolAnalysis includes
#include "PhaseIIADCCalibrator.h"

PhaseIIADCCalibrator::PhaseIIADCCalibrator() : Tool() {}

bool PhaseIIADCCalibrator::Initialise(std::string config_filename, DataModel& data)
{
  // Load configuration file variables
  if ( !config_filename.empty() ) m_variables.Initialise(config_filename);

  // Assign a transient data pointer
  m_data = &data;
  
  // get config variables
  m_variables.Get("drawBaselineRootFit",draw_baseline_fit);
  m_variables.Get("BaselineFitStartSample",baseline_start_sample);
  get_ok = m_variables.Get("BaselineFitOrder",baseline_fit_order);
  if(not get_ok) baseline_fit_order = 1; // default to linear fit
  get_ok = m_variables.Get("RedoFitWithoutOutliers",redo_fit_without_outliers);
  if(not get_ok) redo_fit_without_outliers = false; // whether to redo the fit after removing outliers
  get_ok = m_variables.Get("RefitThresholdMillivolts", refit_threshold); // but only if wfrm range exceeds this
  if(not get_ok) refit_threshold = 50; // something suitable: TODO tune me.
  use_ze3ra_algorithm = true; // default
  get_ok = m_variables.Get("UseZe3raAlgorithm", use_ze3ra_algorithm);
  use_root_algorithm = false; // default
  get_ok = m_variables.Get("UseRootFitAlgorithm",use_root_algorithm);
  if((!use_ze3ra_algorithm)&&(!use_root_algorithm)) use_ze3ra_algorithm=true;
  
  // get or make the ROOT TApplication to show the fit, debug only
  if(draw_baseline_fit){
    int myargc=0;
    intptr_t tapp_ptr=0;
    get_ok = m_data->CStore.Get("RootTApplication",tapp_ptr);
    if(not get_ok){
      Log("PhaseIIADCCalibrator Tool: making global TApplication",v_debug,verbosity);
      rootTApp = new TApplication("rootTApp",&myargc,0);
      tapp_ptr = reinterpret_cast<intptr_t>(rootTApp);
      m_data->CStore.Set("RootTApplication",tapp_ptr);
    } else {
      Log("PhaseIIADCCalibrator Tool: Retrieving global TApplication",v_debug,verbosity);
      rootTApp = reinterpret_cast<TApplication*>(tapp_ptr);
    }
    // register ourself as a user
    int tapplicationusers;
    get_ok = m_data->CStore.Get("RootTApplicationUsers",tapplicationusers);
    if(not get_ok) tapplicationusers=1;
    else tapplicationusers++;
    m_data->CStore.Set("RootTApplicationUsers",tapplicationusers);
  }

  //Set defaults in case config file has no entries
  p_critical = 0.01;
  num_sub_waveforms = 6;
  num_baseline_samples = 5;
  adc_window_db = "none";
  make_led_waveforms = false;
  BEType = "ze3ra";

  m_variables.Get("verbosity", verbosity);
  m_variables.Get("PCritical", p_critical);
  m_variables.Get("BaselineEstimationType", BEType);
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

    if(use_ze3ra_algorithm){
      calibrated_waveform_map[channel_key] = make_calibrated_waveforms_ze3ra(raw_waveforms);
    } else if(use_root_algorithm){
      calibrated_waveform_map[channel_key] = make_calibrated_waveforms_rootfit(raw_waveforms);
    }

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
      if(use_ze3ra_algorithm){
        calibrated_led_waveform_map[channel_key] = make_calibrated_waveforms_ze3ra(LEDWaveforms);
      } else if(use_root_algorithm){
        calibrated_led_waveform_map[channel_key] = make_calibrated_waveforms_rootfit(LEDWaveforms);
      }
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

// version based on the ze3bra algorithm; assumes a DC offset is sufficient
std::vector< CalibratedADCWaveform<double> >
PhaseIIADCCalibrator::make_calibrated_waveforms_ze3ra(
  const std::vector< Waveform<unsigned short> >& raw_waveforms)
{

  // Determine the baseline for the set of raw waveforms (assumed to all
  // come from the same readout for the same channel)
  std::vector< CalibratedADCWaveform<double> > calibrated_waveforms;
  for (const auto& raw_waveform : raw_waveforms) {
    double baseline, sigma_baseline;
    if(BEType == "ze3ra"){
      ze3ra_baseline(raw_waveform, baseline, sigma_baseline,
        num_baseline_samples);
    }
    std::vector<double> cal_data;
    const std::vector<unsigned short>& raw_data = raw_waveform.Samples();
    if(BEType == "simple"){
      ComputeMeanAndVariance(raw_data, baseline, sigma_baseline, num_baseline_samples);
    } 
  
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

// version based on a polynomial fit done via ROOT
std::vector< CalibratedADCWaveform<double> >
PhaseIIADCCalibrator::make_calibrated_waveforms_rootfit(
  const std::vector< Waveform<unsigned short> >& raw_waveforms){
  
  // Apparently the input waveforms given to us represent all the minibuffers for one channel,
  // obtained in one particular ADC readout. But I don't expect that matters much to us.
  
  // create a TGraph if we don't have one
  if(gROOT->FindObject("calibrated_waveform_tgraph")==nullptr){
    // we need to know how many points the tgraph will hold
    num_waveform_points = raw_waveforms.front().Samples().size();
    calibrated_waveform_tgraph = new TGraph(num_waveform_points);
    // honestly i have no idea how to stop ROOT interfering with object deletion after drawing
    // see https://root.cern.ch/root/htmldoc/guides/users-guide/ObjectOwnership.html
    // and https://root.cern.ch/root/roottalk/roottalk01/2060.html
    calibrated_waveform_tgraph->SetName("calibrated_waveform_tgraph");
    gROOT->Add(calibrated_waveform_tgraph); // make gROOT know about it
    //calibrated_waveform_tgraph->GetHistogram()->SetName("calibrated_waveform_tgraph_histo"); better to use this?
  }
  
  // create a TF1 if we don't have one
  if(calibrated_waveform_fit==nullptr){
    // take waveform range to fit either from minibuffer length or user if less and >0
    if((num_baseline_samples<=0) || (num_baseline_samples>(num_waveform_points-baseline_start_sample)))
        num_baseline_samples = num_waveform_points;
    if(baseline_start_sample<0) baseline_start_sample = 0;
    calibrated_waveform_fit = new TF1("calibrated_waveform_fit",TString::Format("pol%d",baseline_fit_order),
      baseline_start_sample,num_baseline_samples);
  }
  
  // Loop over raw waveforms
  std::vector< CalibratedADCWaveform<double> > calibrated_waveforms;
  for (const auto& raw_waveform : raw_waveforms){
    
    // retrieve the raw samples
    const std::vector<unsigned short>& raw_data = raw_waveform.Samples();
    
    // update our TGraph's datapoints with the new datapoints
    for(int samplei=0; samplei<raw_data.size(); ++samplei){
      calibrated_waveform_tgraph->SetPoint(samplei,samplei,raw_data.at(samplei));
    }
    
    
    // fit the graph
    TFitResultPtr fit_result = calibrated_waveform_tgraph->Fit(calibrated_waveform_fit,"RCFS");
    // R to use range of TF1
    // F option uses minuit fitter for polN... better?
    // C skips calculation of chi2 to save time
    // S prevents conversion of the TFitResultPtr to a plain integer, which can only be used to check status
    // in this case we need to check the status of the fit manually: just by converting the pointer
    
    if(draw_baseline_fit){
      Log("PhaseIIADCCalibrator Tool: Drawing initial baseline fit",v_message,verbosity);
      if(gROOT->FindObject("baselineFitCanvas")==nullptr){
        baselineFitCanvas = new TCanvas("baselineFitCanvas");
      } else {
        baselineFitCanvas->cd();
      }
      calibrated_waveform_tgraph->Draw("AP");
      calibrated_waveform_tgraph->GetHistogram()->SetDirectory(0); // canvas should not take ownership
      baselineFitCanvas->Modified();
      baselineFitCanvas->Update();
      gSystem->ProcessEvents();
      while(gROOT->FindObject("baselineFitCanvas")!=nullptr){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }
    
    Int_t fit_succeeded = fit_result;
    double baseline=0;
    if(not fit_succeeded){
      Log("PhaseIIADCCalibrator Tool: polynomial fit of baseline failed!",v_warning,verbosity);
    } else {
      baseline = fit_result->Value(0); // DC offset. FIXME this doesn't fully capture the correction applied
    }
    
    // convert samples from ADC count to volts, and subtract the baseline fit if fit succeeded
    std::vector<double> cal_data;
    // keep track of the max and min
    double cal_data_min=std::numeric_limits<double>::min();
    double cal_data_max=std::numeric_limits<double>::max();
    // loop over samples
    for(uint samplei=0; samplei<raw_data.size(); ++samplei){
      const unsigned short& sample = raw_data.at(samplei);
      double baseline_val = (fit_succeeded) ? calibrated_waveform_fit->Eval(samplei) : 0;
      double cal_val = (static_cast<double>(sample) - baseline_val)*ADC_TO_VOLT;
      cal_data.push_back(cal_val);
      
      if(cal_val<cal_data_min) cal_data_min = cal_val;
      if(cal_val>cal_data_max) cal_data_max = cal_val;
    }
    
    // do we need to calculate sigma_baseline? Does anyone use it? TODO
    double sigma_baseline = 0;
    
    // if we're worried about our baseline fit being skewed by the presence of large pulses,
    // remove outliers and do the baseline fit again. We can skip this step if the range is sufficiently
    // small that we know there aren't any pulses to skew the data
    double cal_data_range = cal_data_max - cal_data_min;
    if((redo_fit_without_outliers) && (cal_data_range>refit_threshold) && (fit_succeeded)){
      // first make a note of the current fit parameters
      std::vector<double> fitpars(baseline_fit_order+1);
      for(uint orderi=0; orderi<(baseline_fit_order+1); ++orderi){
        fitpars.at(orderi) = fit_result->Value(orderi);
        //calibrated_waveform_fit->SetParameter(orderi,0); // clear initial vals...? maybe not necessary?
      }
      
      // find and remove outliers
      std::vector<double> non_outlier_points(cal_data);
      // We'll use interquartile range as a definition of data excluding outliers.
      // First we need to find the interquartile range. Do it the lazy way: with ROOT.
      if(gROOT->FindObject("raw_datapoint_hist")==nullptr){
        raw_datapoint_hist = new TH1D("raw_datapoint_hist","Raw Data Histogram",200,cal_data_min,cal_data_max);
      } else {
        raw_datapoint_hist->Reset(); raw_datapoint_hist->SetBins(200, cal_data_min, cal_data_max);
      }
      for(double& cal_sample : cal_data){ raw_datapoint_hist->Fill(cal_sample); }
      // get the quantile thresholds. We'll take the interquartile range,
      // although we could take arbitrary thresholds
      std::vector<double> threshold_probabilities{0.25,0.75};
      std::vector<double> threshold_values(threshold_probabilities.size());
      // get the quantile thresholds. Note GetQuantiles asks for it's input arrays backwards...
      raw_datapoint_hist->GetQuantiles(threshold_probabilities.size(),
          threshold_values.data(),threshold_probabilities.data());
      
      // since we know it:
      sigma_baseline = raw_datapoint_hist->GetStdDev();
      
      // now we can remove any outliers
      auto newend = std::remove_if(non_outlier_points.begin(), non_outlier_points.end(),
         [&threshold_values](double& dataval){
           return ( (dataval<threshold_values.front()) || (dataval>threshold_values.back()) );
         }
      );
      non_outlier_points.erase(newend, non_outlier_points.end());
      
      // update the contents of the TGraph
      for(uint samplei=0; samplei<non_outlier_points.size(); ++samplei){
        calibrated_waveform_tgraph->SetPoint(samplei,samplei,non_outlier_points.at(samplei));
      }
      
      if(draw_baseline_fit){
        Log("PhaseIIADCCalibrator Tool: Drawing outlier-subtracted baseline fit",v_message,verbosity);
        if(gROOT->FindObject("baselineFitCanvas")==nullptr){
          baselineFitCanvas = new TCanvas("baselineFitCanvas");
        } else {
          baselineFitCanvas->cd();
        }
        calibrated_waveform_tgraph->Draw("AP");
        calibrated_waveform_tgraph->GetHistogram()->SetDirectory(0); // canvas should not take ownership
        baselineFitCanvas->Modified();
        baselineFitCanvas->Update();
        gSystem->ProcessEvents();
        while(gROOT->FindObject("baselineFitCanvas")!=nullptr){
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      }
      
      // note that this time we have fewer datapoints than before,
      // so we will need to restrict the range of our fit
      calibrated_waveform_fit->SetRange(0, non_outlier_points.size());
      calibrated_waveform_fit->SetMinimum(0);
      calibrated_waveform_fit->SetMaximum(non_outlier_points.size());
      
      // now redo the fit as we did before but with the remaining data
      fit_result = calibrated_waveform_tgraph->Fit(calibrated_waveform_fit,"RCFS");
      fit_succeeded = fit_result;
      if(not fit_succeeded){
        Log("PhaseIIADCCalibrator Tool: polynomial re-fit of baseline failed!",v_warning,verbosity);
      } else {
        // get the new fit parameters and add them to the previous ones.
        for(uint orderi=0; orderi<(baseline_fit_order+1); ++orderi){
          double new_parameter_val = fit_result->Value(orderi) + fitpars.at(orderi);
          calibrated_waveform_fit->SetParameter(orderi, new_parameter_val);
        }
        baseline = calibrated_waveform_fit->GetParameter(0); // DC offset. FIXME doesn't fully capture correction
        
        // update our calibrated values
        for(uint samplei=0; samplei<raw_data.size(); ++samplei){
          const unsigned short& sample = raw_data.at(samplei);
          double baseline_val = calibrated_waveform_fit->Eval(samplei);
          double cal_val = (static_cast<double>(sample) - baseline_val)*ADC_TO_VOLT;
          cal_data.push_back(cal_val);
        }
      }
      
      // revert our fit range for the next one before we forget
      calibrated_waveform_fit->SetRange(baseline_start_sample,num_baseline_samples);
      calibrated_waveform_fit->SetMinimum(baseline_start_sample);
      calibrated_waveform_fit->SetMaximum(num_baseline_samples);
    }
    
    // construct the calibrated waveform
    calibrated_waveforms.emplace_back(raw_waveform.GetStartTime(), cal_data, baseline, sigma_baseline);
  }
  
  return calibrated_waveforms;
}
