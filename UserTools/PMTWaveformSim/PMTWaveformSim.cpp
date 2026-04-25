#include <vector>
#include <cmath>
#include <map>

// ANNIE includes
#include "ANNIEconstants.h"
#include "PMTWaveformSim.h"

// ROOT includes
#include "TGraph.h"

PMTWaveformSim::PMTWaveformSim():Tool(){}


bool PMTWaveformSim::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  // get config variables
  bool gotVerbosity = m_variables.Get("verbosity", verbosity);
  if (!gotVerbosity) verbosity = 1;

  bool gotPMTParamFile = m_variables.Get("PMTParameterFile", fPMTParameterFile);
  if (!gotPMTParamFile) {
    logmessage = "PMTWaveformSim: No PMTParameterFile specified! Aborting!";
    Log(logmessage, v_error, verbosity);
    return false;
  }  

  if (!LoadPMTParameters())
    return false;

  bool got_sigmamap = m_variables.Get("useTimeSmearing", fuseTimeSmearing);
  if (!got_sigmamap) {
    logmessage = "PMTWaveformSim: No useTimeSmearing specified! Aborting!";
    Log(logmessage, v_error, verbosity);
    return false;
  }  

  bool gotPrewindow = m_variables.Get("Prewindow", fPrewindow);
  if (!gotPrewindow) {
    logmessage = "PMTWaveformSim: Prewindow not defined. Using default of 10.";
    Log(logmessage, v_warning, verbosity);
    fPrewindow = 10;
  }

  bool gotReadoutWindow = m_variables.Get("ReadoutWindow", fReadoutWindow);
  if (!gotReadoutWindow) {
    logmessage = "PMTWaveformSim: ReadoutWindow not defined. Using default of 35.";
    Log(logmessage, v_warning, verbosity);
    fReadoutWindow = 35;
  }

  bool gotT0Offset = m_variables.Get("T0Offset", fT0Offset);
  if (!gotT0Offset) {
    logmessage = "PMTWaveformSim: T0Offset not defined. Using default of 25.";
    Log(logmessage, v_warning, verbosity);
    fT0Offset = 25;
  }

  bool gotShiftTimes = m_variables.Get("TimeShift", fTimeShift);
  if (!gotShiftTimes) {
    logmessage = "PMTWaveformSim: TimeShift not defined. Using default of 0.";
    Log(logmessage, v_warning, verbosity);
    fTimeShift = 0.0;
  } else {
    logmessage = "PMTWaveformSim: TimeShift = " + std::to_string(fTimeShift) + "ns will be applied to ALL MCHits.";
    Log(logmessage, v_warning, verbosity);
  }

  bool gotDebug = m_variables.Get("MakeDebugFile", fDebug);
  if (!gotDebug) fDebug = 0;
  if (fDebug)
    fOutFile = new TFile("PMTWaveforms.root", "RECREATE");

  bool gotGeometry = m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry", fGeo);
  if(!gotGeometry){
    logmessage = "PMTWaveformSim: Error retrieving Geometry from ANNIEEvent! Aborting!";
    Log(logmessage, v_error, verbosity);
    return false;
  }

  // random seed is grabbed from the system clock
  fRandom = new TRandom3();
  
  return true;
}

//------------------------------------------------------------------------------
bool PMTWaveformSim::Execute()
{
  int load_status = LoadFromStores();

  if (load_status == 0) return false;

  // The container for the data that we'll put into the ANNIEEvent
  std::map<unsigned long, std::vector<Waveform<uint16_t>> > RawADCDataMC;
  std::map<unsigned long, std::vector<CalibratedADCWaveform<double>> > CalADCDataMC;


  // If MCHits is empty (load_status == 2), create one minimal baseline waveform so that the hit finder doesn't freak out
  // while keeping the rest of the machinery the same
  if (load_status == 2) {
    logmessage = "PMTWaveformSim: Creating single minimal baseline waveform (No MCHits)...";
    Log(logmessage, v_message, verbosity);
    
    // we can put the dummy baseline in a dead PMT channel so it won't be integrated
    unsigned long dummy_chankey = 333;    // PMT ID 333 is dead
    
    // create a short baseline waveform (~50ns) so that the hit finder will be satisfied
    int num_samples = 25;   // 50ns
    double noiseSigma = fRandom->Gaus(1, 0.01);   // set the noise to basically 0
    int baseline = fRandom->Uniform(300, 350);
    
    std::vector<uint16_t> rawSamples;
    std::vector<double> calSamples;
    
    for (int i = 0; i < num_samples; i++) {
      double noise = fRandom->Gaus(0, noiseSigma);
      int sample = std::round(noise + baseline);
      sample = (sample > 4095) ? 4095 : ((sample < 0) ? 0 : sample);  // shouldn't matter
      
      rawSamples.push_back(sample);
      calSamples.push_back((sample - baseline) * ADC_TO_VOLT);
    }
    
    std::vector<Waveform<uint16_t>> rawWaveforms;
    std::vector<CalibratedADCWaveform<double>> calWaveforms;
    
    rawWaveforms.emplace_back(0, rawSamples);
    calWaveforms.emplace_back(0, calSamples, baseline, noiseSigma);
    
    RawADCDataMC.emplace(dummy_chankey, rawWaveforms);
    CalADCDataMC.emplace(dummy_chankey, calWaveforms);

  }

  // normal use case (no blank MCHits)
  for (auto mcHitsIt : *fMCHits) { // Loop over the hit PMTs
    int PMTID = mcHitsIt.first;

    std::vector<MCHit> mcHits = mcHitsIt.second;

    // Generate waveform samples from the MC hits
    // samples from hits that are close in time will be added together
    // key is hit time in clock ticks, value is amplitude
    std::map<uint16_t, uint16_t> sample_map;
    for (const auto& mcHit : mcHits) {// Loop through each MCHit in the vector

      // skip negative hit times, what does that even mean if we're not using the smeared digit time?
      // skip hit times past 70 us since that's our longest readout
      if (mcHit.GetTime() < 0) continue;
      if (mcHit.GetTime() > 70000) continue;

      if(!fPMTParamMap.count(PMTID)) continue;      //Skip inactive PMTs (those that don't have calibration parameters
                                                    //TODO: Add a default set of parameters to fall back on, so ideal case with all PMTs can be approximated

      // Grab the hit time (also converted to clock ticks) and the charge
      double hit_t0 = mcHit.GetTime() + fTimeShift;
      double hit_charge = mcHit.GetCharge();

      logmessage = "PMTWaveformSim: hit charge =  " + std::to_string(hit_charge) + " p.e., hit time =  " + std::to_string(hit_t0) + " for PMTID " + std::to_string(PMTID);
      Log(logmessage, v_message, verbosity);

      // before "digitizing", add smearing based on the uncertainty extracted in the laser analysis
      if (fuseTimeSmearing) {

        // grab the timing jitter map
		if (!TimeSmearing(PMTID)) {
		    m_data->vars.Set("StopLoop", true);    // if jitter is not found, stop the show
		    return false;
		}

        // apply time smearing by sampling normal centered at 0 with std = timing_sigma
        if (verbosity > v_warning) {
            std::cout << "PMTWaveformSim: Found timing uncertainty for PMT "                                            
                      << PMTID << " = "
                      << fTimeSmear << " ns\n";
        } 
        
        double timejitter = fRandom->Gaus(0, fTimeSmear);               
        if (verbosity > v_message) {
          std::cout << "PMTWaveformSim: Sampled timing uncertainty for PMT "                                            
                    << PMTID << " = "
                    << timejitter << " ns\n";
        }

        hit_t0 += timejitter;

      }

      // now convert to clock ticks
      uint16_t t0_ticks = uint16_t(hit_t0 / NS_PER_ADC_SAMPLE);

      // Set the readout window in clock ticks, but don't allow negative times
      uint16_t start_clocktick = (t0_ticks > fPrewindow)? t0_ticks - fPrewindow : 0;
      uint16_t end_clocktick = start_clocktick + fReadoutWindow;

      // Randomly Sample the PMT parameters for each MCHit
      SampleFitParameters(PMTID);

      // loop over clock ticks      
      for (uint16_t clocktick = start_clocktick; clocktick <= end_clocktick; clocktick += 1) {
	      uint16_t sample = CustomLogNormalPulse(hit_t0, clocktick, hit_charge);
 
        std::stringstream logmessage;
        logmessage << "    --> clocktick = " << clocktick << ", sample = " << sample;
        Log(logmessage.str(), v_debug, verbosity);
	
        // check if this hit time has been recorded
        // either set it or add to it
        if (sample_map.find(clocktick) == sample_map.end()) 
          sample_map[clocktick] = sample;
        else 
          sample_map[clocktick] += sample;		
            }// end loop over clock ticks
        }// end loop over mcHits
    
        // If there are no samples for this PMT then no need to do the rest
        if (sample_map.empty()) continue;
    
    
    // Set the noise envelope and baseline for this PMT
    // The noise std dev appears to be normally distributed around 1 with sigma 0.25
    // TODO: set accurate baseline and noise profiles for all PMTs individually (noise should be fine, baselines will vary)
    double noiseSigma = fRandom->Gaus(1, 0.25);
    int basline = fRandom->Uniform(300, 350);
    
    // convert the sample map into a vector of Waveforms and put them into the container
    std::vector<Waveform<uint16_t>> rawWaveforms;
    std::vector<CalibratedADCWaveform<double>> calWaveforms;
    ConvertMapToWaveforms(sample_map, rawWaveforms, calWaveforms, noiseSigma, basline);

    RawADCDataMC.emplace(PMTID, rawWaveforms);
    CalADCDataMC.emplace(PMTID, calWaveforms);
  }// end loop over PMTs


  // Publish the waveforms to the ANNIEEvent store if we have them
  m_data->Stores.at("ANNIEEvent")->Set("RawADCDataMC",      RawADCDataMC);
  m_data->Stores.at("ANNIEEvent")->Set("CalibratedADCData", CalADCDataMC); 
  m_data->Stores.at("ANNIEEvent")->Set("ActivePMTs",factivePMTs);
  
  if (fDebug) 
    FillDebugGraphs(RawADCDataMC);

  return true;
}

//------------------------------------------------------------------------------
bool PMTWaveformSim::Finalise()
{
  if (fDebug)
    fOutFile->Close();
  
  return true;
}

//------------------------------------------------------------------------------
bool PMTWaveformSim::LoadPMTParameters()
{
  
  std::ifstream infile(fPMTParameterFile);
  if (!infile.is_open()) {
    logmessage = "PMTWaveformSim: Error opening CSV file: ";
    logmessage += fPMTParameterFile + "!";
    Log(logmessage, v_error, verbosity);
    return false;
  }

  int pmtid;
  // Stored fit parameters.
  // p0, p1, and p2 are the mean values of the lognorm
  // T1 and T2 are the reflection spacings
  // r1 and r2 are the reflection amplitudes (relative to the main peak amplitude)
  // the uncertainties (u*) are the sq(diagonal elements) of the fitted covariance matrix
  // offset_std [ns] is the timing jitter as observed in the laser calibration
  double p0, p1, p2, T1, T2, R1, R2,
         up0, up1, up2, uT1, uT2, uR1, uR2,
         time_jitter;
                    
  std::string comma;
  std::string line;
  while (std::getline(infile, line)) {
    if (infile.fail()) {
      logmessage = "PMTWaveformSim: Error using CSV file: ";
      logmessage += fPMTParameterFile + "!";
      Log(logmessage, v_error, verbosity);

      return false;
    }

    // Skip the header line
    if (line.find("PMT") != std::string::npos) continue;
    
    // Skip any commented lines
    if(line.rfind("#",0) != std::string::npos) continue;

    // Turn the line into a stringstream to extract the values
    std::stringstream ss(line);
    ss >> pmtid >> comma >> p0 >> comma >> p1 >> comma >> p2 >> comma >> T1 >> comma >> T2 >> comma >> R1 >> comma >> R2 >> comma 
       >> up0 >> comma >> up1 >> comma >> up2 >> comma >> uT1 >> comma >> uT2 >> comma >> uR1 >> comma >> uR2 >> comma
       >> time_jitter;

    fPMTParamMap[pmtid] = {p0, p1, p2, T1, T2, R1, R2, up0, up1, up2, uT1, uT2, uR1, uR2};
    fPMTJitterMap[pmtid] = time_jitter;

    factivePMTs++;

    logmessage = "PMTWaveformSim: Loaded parameters for PMTID " + std::to_string(pmtid) + ": ";
    logmessage += "p0 = " + std::to_string(p0);
    logmessage += " p1 = " + std::to_string(p1);
    logmessage += " p2 = " + std::to_string(p2);
    logmessage += " T1 = " + std::to_string(T1);
    logmessage += " T2 = " + std::to_string(T2);
    logmessage += " R1 = " + std::to_string(R1);
    logmessage += " R2 = " + std::to_string(R2);
    logmessage += " uncertainty_p0 = " + std::to_string(up0);
    logmessage += " uncertainty_p1 = " + std::to_string(up1);
    logmessage += " uncertainty_p2 = " + std::to_string(up2);
    logmessage += " uncertainty_T1 = " + std::to_string(uT1);
    logmessage += " uncertainty_T2 = " + std::to_string(uT2);
    logmessage += " uncertainty_R1 = " + std::to_string(uR1);
    logmessage += " uncertainty_R2 = " + std::to_string(uR2);
    logmessage += " offset_std = " + std::to_string(time_jitter);
    Log(logmessage, v_message, verbosity);
  }

  infile.close();

  return true;
}

//------------------------------------------------------------------------------
bool PMTWaveformSim::SampleFitParameters(int pmtid)
{
  PMTFitParams pmtParams;
  if (fPMTParamMap.find(pmtid) != fPMTParamMap.end()) {
    pmtParams = fPMTParamMap[pmtid];
  } else {
    logmessage = "PMTWaveformSim: PMTParameters not found for " + std::to_string(pmtid);
    logmessage += ", using defaults (exemplary fit: PMT 403 Hamamatsu): p0 = 16.876, p1 = 65.448, p2 = 0.032, T1 = 6.593, T2 = 6.975, r1 = 0.174, r2 = 0.049";
    Log(logmessage, v_warning, verbosity);

    // TODO make this a random sample as well
    // TODO make the default the parameters of the *total* fit
    fP0 = 16.876;
    fP1 = 65.448;
    fP2 = 0.032;
    fT1 = 6.593;
    fT2 = 6.975;
    fR1 = 0.174; 
    fR2 = 0.049;

    return true;
  }
  
  // First sample a Gaussian with mean 0 and deviation 1
  double rr0 = fRandom->Gaus();  // p0
  double rr1 = fRandom->Gaus();  // p1
  double rr2 = fRandom->Gaus();  // p2

  // Randomly sample parameters with their associated uncertainties
  fP0 = rr0*pmtParams.up0 + pmtParams.p0;
  fP1 = rr1*pmtParams.up1 + pmtParams.p1;
  fP2 = rr2*pmtParams.up2 + pmtParams.p2;

  // for the reflection coefficients, we know they must be positive (or else its nonsense)
  double rr3, rr4, rr5, rr6;
  do {
    rr3 = fRandom->Gaus();  // T1
    rr4 = fRandom->Gaus();  // T2
    rr5 = fRandom->Gaus();  // r1
    rr6 = fRandom->Gaus();  // r2

    fT1 = rr3*pmtParams.uT1 + pmtParams.T1;
    fT2 = rr4*pmtParams.uT2 + pmtParams.T2;
    fR1 = rr5*pmtParams.uR1 + pmtParams.R1;
    fR2 = rr6*pmtParams.uR2 + pmtParams.R2;

    if (fT1 < 0.0 || fT2 < 0.0 || fR1 < 0.0 || fR2 < 0.0) {
      std::stringstream debug_msg_random;
      debug_msg_random << "PMTWaveformSim: Resampling due to negative value(s): "
                << "fT1 = " << fT1 << ", "
                << "fT2 = " << fT2 << ", "
                << "fR1 = " << fR1 << ", "
                << "fR2 = " << fR2 << std::endl;
      Log(debug_msg_random.str(), v_warning, verbosity);
    }

  } while (fT1 < 0.0 || fT2 < 0.0 || fR1 < 0.0 || fR2 < 0.0);


  std::stringstream debug_msg;
  debug_msg << "PMTWaveformSim::SampleFitParameters final sampled parameters:"
            << "\n  random p0 = " << fP0
            << "\n  random p1 = " << fP1
            << "\n  random p2 = " << fP2
            << "\n  random T1 = " << fT1
            << "\n  random T2 = " << fT2
            << "\n  random R1 = " << fR1
            << "\n  random R2 = " << fR2;
  Log(debug_msg.str(), v_debug, verbosity);

  return true;
}

//------------------------------------------------------------------------------
uint16_t PMTWaveformSim::CustomLogNormalPulse(double hit_t0, uint16_t clocktick, double hit_charge)
{
  //p0*exp( -0.5 * (log(x/p1)/p2)^2) (main peak) + sum_{i = 1}^{3}[(pi*p0)*exp( -0.5 * (log(x - (i * Ti)/p1)/p2)^2)] (3 reflections)
  
  // The fit was performed in time units of ns, but we pass samples in clock ticks
  double x = (clocktick + fT0Offset) * NS_PER_ADC_SAMPLE - hit_t0;

  // debug
  double logarg_main = x / fP1;
  double logarg_r1   = (x - fT1) / fP1;
  double logarg_r2   = (x - 2*fT2) / fP1;
  std::stringstream debug_msg;
  debug_msg << "PMTWaveformSim::CustomLogNormalPulse debug:"
            << "\n  clocktick = " << clocktick
            << "\n  hit_t0 = " << hit_t0
            << "\n  x = " << x
            << "\n  logarg_main = " << logarg_main
            << "\n  logarg_r1   = " << logarg_r1
            << "\n  logarg_r2   = " << logarg_r2
            << "\n  fP0 = " << fP0
            << ", fP1 = " << fP1
            << ", fP2 = " << fP2
            << ", fT1 = " << fT1
            << ", fT2 = " << fT2
            << ", fR1 = " << fR1
            << ", fR2 = " << fR2
            << ", hit_charge = " << hit_charge;
  Log(debug_msg.str(), v_debug, verbosity);

  // main peak parameters 
  double numerator = log(x/fP1);
  numerator = numerator * numerator;
  double denom = fP2 * fP2;            // main peak shape + width is used for all reflection peaks; only calculate once
  double amplitude = fP0 * exp(-0.5 * numerator/denom) * hit_charge;

  // reflection 1 parameters
  double numerator_r1 = log((x - fT1)/fP1);
  numerator_r1 = numerator_r1 * numerator_r1;
  amplitude += (fP0 * fR1) * exp(-0.5 * numerator_r1/denom) * hit_charge;

  // reflection 2 parameters
  double numerator_r2 = log((x - (2*fT2))/fP1);
  numerator_r2 = numerator_r2 * numerator_r2;
  amplitude += (fP0 * fR2) * exp(-0.5 * numerator_r2/denom) * hit_charge;

  // Clip at 4095 and digitize to an integer
  return uint16_t((amplitude > 4095) ? 4095 : amplitude);
}

//------------------------------------------------------------------------------
void PMTWaveformSim::ConvertMapToWaveforms(const std::map<uint16_t, uint16_t> &sample_map,
					   std::vector<Waveform<uint16_t>> &rawWaveforms,
					   std::vector<CalibratedADCWaveform<double>> &calWaveforms,
					   double noiseSigma, int baseline)
{
  // Clear the output waveforms just in case
  rawWaveforms.clear();
  calWaveforms.clear();
  
  // All MC has extended readout, but it's time consuming to draw 35000 noise samples
  // Instead, only sample up to the maximum tick time. Then start with all baseline and 
  // add noise only to relevant ticks. The intermediate space between pulses will have no noise

  uint16_t maxTick = sample_map.rbegin()->first;
  std::vector<uint16_t> rawSamples(maxTick+1, baseline);
  std::vector<double> calSamples(maxTick+1, 0);
  for (auto sample_pair : sample_map) {
    uint16_t tick = sample_pair.first;
    
    // Generate noise for each sample based on the std dev of the noise envelope
    double noise = fRandom->Gaus(0, noiseSigma);
    int sample = std::round(noise + baseline);

    sample += sample_pair.second;

    rawSamples[tick] = (sample > 4095) ? 4095 : sample;
    calSamples[tick] = (rawSamples[tick] - baseline) * ADC_TO_VOLT;
  }

  // The start time in data is a trigger timestamp. Don't have that for MC so just set to 0. 
  rawWaveforms.emplace_back(0, rawSamples);
  calWaveforms.emplace_back(0, calSamples, baseline, noiseSigma);
}
 
//------------------------------------------------------------------------------
int PMTWaveformSim::LoadFromStores()
{
  bool goodAnnieEvent = m_data->Stores.count("ANNIEEvent");
  if (!goodAnnieEvent) {
    logmessage = "PMTWaveformSim: no ANNIEEvent store!";
    Log(logmessage, v_error, verbosity);
    return 0;    
  }

  bool goodMCHits = m_data->Stores.at("ANNIEEvent")->Get("MCHits", fMCHits);
  if (!goodMCHits) {
    logmessage = "PMTWaveformSim: no MCHits in the ANNIEEvent! ";
    Log(logmessage, v_error, verbosity);
    return 0;
  }

  if (fMCHits->empty()) {
    logmessage = "PMTWaveformSim: The MCHits map is empty! Will fill a single PMT with a minimal waveform.";
    Log(logmessage, v_warning, verbosity);
    return 2;
  }

  
  return 1;
}

//------------------------------------------------------------------------------
void PMTWaveformSim::FillDebugGraphs(const std::map<unsigned long, std::vector<Waveform<uint16_t>> > &RawADCDataMC)
{
  for (auto itpair : RawADCDataMC) {
    std::string chanString = std::to_string(itpair.first);

    // Get/make the directory for this PMT
    TDirectory* dir = fOutFile->GetDirectory(chanString.c_str());
    if (!dir) 
      dir = fOutFile->mkdir(chanString.c_str());
      
    // Hop into that directory and save the graph
    dir->cd();

    // loop over waveforms and make graphs
    for (uint wfIdx = 0; wfIdx < itpair.second.size(); ++wfIdx) {
      auto waveform = itpair.second.at(wfIdx);

      uint32_t evtNum = 0;
      m_data->Stores.at("ANNIEEvent")->Get("EventNumber",evtNum);
      std::string grName = ("wf_" + std::to_string(evtNum) + "_" + std::to_string(wfIdx));

      // Make the graph
      std::vector<uint16_t> samples = waveform.Samples();
      TGraph grTemp = TGraph();
      double sampleX = waveform.GetStartTime();
      for(auto sample : samples) {
        grTemp.AddPoint(sampleX, sample);
        ++sampleX;
      }
      
      grTemp.Write(grName.c_str());	
    }// end loop over waveforms
  }// end loop over PMTs
}

bool PMTWaveformSim::TimeSmearing(int pmtid)
{
  if (fPMTJitterMap.find(pmtid) != fPMTJitterMap.end()) {
    fTimeSmear = fPMTJitterMap[pmtid];
  } else {
    logmessage = "PMTWaveformSim: PMT timing jitter not found for " + std::to_string(pmtid);
    logmessage += ", exiting...";
    Log(logmessage, v_error, verbosity);
    return false;
  }

  return true;

}
				     



