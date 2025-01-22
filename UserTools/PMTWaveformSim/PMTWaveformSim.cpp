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
    logmessage = "PMTWaveformSim: T0Offset not defined. Using default of 7.";
    Log(logmessage, v_warning, verbosity);
    fT0Offset = 7;
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

  // Could set a seed here for repeatability
  // Though would probably want to seed it in the Execute function based on the run/part/event numbers
  fRandom = new TRandom3();
  
  return true;
}

//------------------------------------------------------------------------------
bool PMTWaveformSim::Execute()
{
  int load_status = LoadFromStores();

  
  // The container for the data that we'll put into the ANNIEEvent
  std::map<unsigned long, std::vector<Waveform<uint16_t>> > RawADCDataMC;
  std::map<unsigned long, std::vector<CalibratedADCWaveform<double>> > CalADCDataMC;

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

      // Grab the hit time (also converted to clock ticks) and the charge
      double hit_t0 = mcHit.GetTime();
      uint16_t t0_ticks = uint16_t(hit_t0 / NS_PER_ADC_SAMPLE);
      double hit_charge = mcHit.GetCharge();

      // Set the readout window in clock ticks, but don't allow negative times
      uint16_t start_clocktick = (t0_ticks > fPrewindow)? t0_ticks - fPrewindow : 0;
      uint16_t end_clocktick = start_clocktick + fReadoutWindow;

      // Randomly Sample the PMT parameters for each MCHit
      SampleFitParameters(PMTID);

      // loop over clock ticks      
      for (uint16_t clocktick = start_clocktick; clocktick <= end_clocktick; clocktick += 1) {
	uint16_t sample = CustomLogNormalPulse(hit_t0, clocktick, hit_charge);
	
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
   if (RawADCDataMC.size()) {     
    m_data->Stores.at("ANNIEEvent")->Set("RawADCDataMC",      RawADCDataMC);
    m_data->Stores.at("ANNIEEvent")->Set("CalibratedADCData", CalADCDataMC);
   } else {
     logmessage = "PMTWaveformSim: No waveforms produced. Skipping!";
     Log(logmessage, v_warning, verbosity);
     m_data->Stores.at("ANNIEEvent")->Set("SkipExecute", true);
     return true;
   }
     
  
  if (fDebug) 
    FillDebugGraphs(RawADCDataMC);

  m_data->Stores.at("ANNIEEvent")->Set("SkipExecute", false);
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
  // the u's are for sampling in a way that follows the fit covariance
  double p0, p1, p2, u00, u10, u11, u20, u21, u22;
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
    ss >> pmtid >> comma >> p0 >> comma >> p1 >> comma >> p2 >> comma
       >> u00 >> comma
       >> u10 >> comma >> u11 >> comma 
       >> u20 >> comma >> u21 >> comma >> u22;

    fPMTParamMap[pmtid] = {p0, p1, p2, u00, u10, u11, u20, u21, u22};

    logmessage = "PMTWaveformSim: Loaded parameters for PMTID " + std::to_string(pmtid) + ": ";
    logmessage += "p0 = " + std::to_string(p0);
    logmessage += "p1 = " + std::to_string(p1);
    logmessage += "p2 = " + std::to_string(p2);
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
    logmessage += ", using defaults: p0 = 17.49, p1 = 3.107, p2 = 0.1492";
    Log(logmessage, v_warning, verbosity);

    // TODO make this a random sample as well
    fP0 = 17.49;
    fP1 = 3.107;
    fP2 = 0.1492;

    return true;
  }
  
  // First sample a Gaussian with mean 0 and deviation 1
  double r0 = fRandom->Gaus();
  double r1 = fRandom->Gaus();
  double r2 = fRandom->Gaus();

  // Convert to parameters that follow the fitted covariance matrix
  fP0 = r0*pmtParams.u00 + pmtParams.p0;
  fP1 = r0*pmtParams.u10 + r1*pmtParams.u11 + pmtParams.p1;
  fP2 = r0*pmtParams.u20 + r1*pmtParams.u21 + r2*pmtParams.u22 + pmtParams.p2;

  return true;
}

//------------------------------------------------------------------------------
uint16_t PMTWaveformSim::CustomLogNormalPulse(double hit_t0, uint16_t clocktick, double hit_charge)
{
  //p0*exp( -0.5 * (log(x/p1)/p2)^2)
  
  // The fit was performed in time units of ns, but we pass samples in clock ticks
  double x = (clocktick + fT0Offset) * NS_PER_ADC_SAMPLE - hit_t0;

  double numerator = log(x/fP1);
  numerator = numerator * numerator;
  double denom = fP2 * fP2;
  double amplitude = fP0 * exp(-0.5 * numerator/denom) * hit_charge;

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
  // add noise only to relvant ticks. The intermediate space between pulses will have no noise

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
    logmessage = "PMTWaveformSim: The MCHits map is empty! Skipping!";
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
				     




