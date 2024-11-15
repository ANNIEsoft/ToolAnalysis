#include "BeamQuality.h"

BeamQuality::BeamQuality():Tool(){}

//------------------------------------------------------------------------------
bool BeamQuality::Initialise(std::string configfile, DataModel &data)
{
  // Load configuration file variables
  if ( !configfile.empty() ) m_variables.Initialise(configfile);

  // Assign a transient data pointer
  m_data = &data;

  // Get the things
  bool got_verbosity = m_variables.Get("verbose", verbosity);
  bool got_potmin    = m_variables.Get("POTMin", fPOTMin);
  bool got_potmax    = m_variables.Get("POTMax", fPOTMax);
  bool got_hornmin   = m_variables.Get("HornCurrentMin", fHornCurrMin);
  bool got_hornmax   = m_variables.Get("HornCurrentMax", fHornCurrMax);
  bool got_beamloss  = m_variables.Get("BeamLossTolerance", fBeamLoss);

  // Check the config parameters and set default values if necessary 
  if (!got_verbosity) verbosity = 1;

  if (!got_potmin) {
    fPOTMin = 5e11;

    logmessage = ("Warning (BeamQuality): POTMin not "
		  "set in the config file. Using default: 5e11");
    Log(logmessage, v_warning, verbosity);
  }

  if (!got_potmax) {
    fPOTMax = 8e12;

    logmessage = ("Warning (BeamQuality): POTMax not "
		  "set in the config file. Using default: 8e12");
    Log(logmessage, v_warning, verbosity);
  }

  if (!got_hornmin) {
    fHornCurrMin = 172;

    logmessage = ("Warning (BeamQuality): HornCurrentMin not "
		  "set in the config file. Using default: 172");
    Log(logmessage, v_warning, verbosity);
  }
  
  if (!got_hornmax) {
    fHornCurrMax = 176;

    logmessage = ("Warning (BeamQuality): HornCurrentMax not "
		  "set in the config file. Using default: 176");
    Log(logmessage, v_warning, verbosity);
  }

  if (!got_beamloss) {
    fBeamLoss = 0.05;

    logmessage = ("Warning (BeamQuality): BeamLossTolerance not "
		  "set in the config file. Using default: 0.05");
    Log(logmessage, v_warning, verbosity);
  }

  // Necessary initialization
  m_data->CStore.Set("NewBeamStatusAvailable", false);
  fLastTimestamp = 0;
  BeamStatusMap = new std::map<uint64_t, BeamStatus>;

  return true;
}

//------------------------------------------------------------------------------
bool BeamQuality::Execute()
{
  m_data->CStore.Set("NewBeamStatusAvailable", false);

  // Check for CTC timestamps and beam DB info
  bool got_ctc    = m_data->CStore.Get("NewCTCDataAvailable",  fNewCTCData);
  bool got_beamdb = m_data->CStore.Get("NewBeamDataAvailable", fNewBeamData);

  // Make sure we have the info we need
  if (!got_ctc) { 
    logmessage = ("Error (BeamQuality): Did not find NewCTCDataAvailable "
		  "entry in the CStore. Aborting.");
    Log(logmessage, v_error, verbosity);
    
    return false;
  }

  if (!got_beamdb) {
    logmessage = ("Error (BeamQuality): Did not find NewBeamDataAvailable "
		  "entry in the CStore. Make sure to run BeamFetcherV2 in "
		  "your tool chain. Aborting.");
    Log(logmessage, v_error, verbosity);
    
    return false;
  } 
    

  // Start doing the things
  if (!fNewCTCData) {
    logmessage = ("Message (BeamQuality): No new CTC data is available. "
		  "Nothing to do for now");
    Log(logmessage, v_message, verbosity);
    
    return true;
  }

  // Need to get the trigger times and loop over them
  bool got_triggers = m_data->CStore.Get("TimeToTriggerWordMap", TimeToTriggerWordMap);
  bool got_beamdata = m_data->CStore.Get("BeamDataMap", BeamDataMap);

  // Keep track of which timestamps we've seen so we can clear out the BeamDataMap
  std::set<uint64_t> completed_timestamps;
  
  // But start from the fLastTimestamp to save time  
  for (auto iterator = TimeToTriggerWordMap->lower_bound(fLastTimestamp);
	 iterator != TimeToTriggerWordMap->end(); ++iterator) {

    // Grab the timestamp and check the trigger words
    uint64_t timestamp = iterator->first;
    uint64_t timestamp_ms = timestamp/1E6;
    fLastTimestamp = timestamp;
    completed_timestamps.insert(timestamp);

    // Check for trigger word 14 (undelayed beam trigger)
    // If it's not there then it isn't a beam trigger and there's nothing more to do
    if (std::find(iterator->second.begin(), iterator->second.end(), 14) == iterator->second.end()) {
      BeamStatus tempStatus(TimeClass(timestamp), 0., BeamCondition::NonBeamMinibuffer);
      BeamStatusMap->emplace(timestamp, tempStatus);
      continue;
    }

    // Have we already saved this timestamp? If so then continue to the next one
    if (BeamStatusMap->find(timestamp) != BeamStatusMap->end())
      continue;
    
    // Check for the existence of beam DB info
    if (!fNewBeamData) {
      logmessage = ("Warning (BeamQuality): No new Beam DB data is available. "
		    "Setting it to \"Missing\". ");
      Log(logmessage, v_warning, verbosity);

      BeamStatus tempStatus(TimeClass(timestamp), 0., BeamCondition::Missing);
      BeamStatusMap->emplace(timestamp, tempStatus);
    }

    // Check for beam DB info associated with the timestamp
    if (BeamDataMap->find(timestamp_ms) != BeamDataMap->end()) {
      // Got the goods, now use it
      BeamStatus tempStatus = assess_beam_quality(timestamp);
      BeamStatusMap->emplace(timestamp, tempStatus);
      
    } else {
      logmessage = ("Warning (BeamQuality): No Beam DB info found for timestamp: "
		    + std::to_string(timestamp_ms) + ". Setting it to \"Missing\". ");
      Log(logmessage, v_warning, verbosity);

      BeamStatus tempStatus(TimeClass(timestamp), 0., BeamCondition::Missing);
      BeamStatusMap->emplace(timestamp, tempStatus);
    } // end if found timestamp in BeamDataMap
  } // end loop over TimeToTriggerWordMap

  // Clear out the BeamDataMap to free up memory
  for (auto ts : completed_timestamps)
    BeamDataMap->erase(ts/1E6);

  // Save the status
  m_data->CStore.Set("BeamStatusMap", BeamStatusMap);

  logmessage = ("Message (BeamQuality): BeamStatusMap size: "
		+ std::to_string(BeamStatusMap->size()));
  Log(logmessage, v_message, verbosity);

  
  return true;
}

//------------------------------------------------------------------------------
bool BeamQuality::Finalise()
{

  return true;
}

//------------------------------------------------------------------------------
BeamStatus BeamQuality::assess_beam_quality(uint64_t timestamp)
{
  // initialize the beamstatus that we'll return
  BeamStatus retStatus;
  retStatus.set_time(TimeClass(timestamp));

  
  // Check for and grab the quatities we want from the BeamDataMap
  uint64_t timestamp_ms = timestamp/1E6;
  auto BeamDataPointMap = BeamDataMap->at(timestamp_ms);

  std::string device_name = "";
  
  // POT downstream toroid
  device_name = "E:TOR875";
  double pot_ds_toroid = 0;
  if (BeamDataPointMap.find(device_name) != BeamDataPointMap.end()) {
  auto bdp = BeamDataPointMap[device_name];
    retStatus.add_measurement(device_name, timestamp_ms, bdp);
    retStatus.set_pot(bdp.value);
    pot_ds_toroid = bdp.value;
  }
  
  // POT upstream toroid
  device_name = "E:TOR860";
  double pot_us_toroid = 0;
  if (BeamDataPointMap.find(device_name) != BeamDataPointMap.end()) {
    auto bdp = BeamDataPointMap[device_name];
    retStatus.add_measurement(device_name, timestamp_ms, bdp);
    pot_us_toroid = bdp.value;
  }

  // Horn current
  device_name = "E:THCURR";
  double horn_current = 0;
  if (BeamDataPointMap.find(device_name) != BeamDataPointMap.end()) {
    auto bdp = BeamDataPointMap[device_name];
    retStatus.add_measurement(device_name, timestamp_ms, bdp);
    horn_current = bdp.value;
  }

  
  // Perform the cuts
  retStatus.add_cut("POT in range",
		    (retStatus.pot() >= fPOTMin && retStatus.pot() <= fPOTMax));

  retStatus.add_cut("Horn current in range",
		    (horn_current >= fHornCurrMin && horn_current <= fHornCurrMax));

  double beam_loss_frac = std::abs(pot_ds_toroid - pot_us_toroid) / (pot_ds_toroid + pot_us_toroid);
  retStatus.add_cut("Beam loss acceptable",
		    (beam_loss_frac < fBeamLoss));

  
  // Finish up the beam status
  if (retStatus.passed_all_cuts()) {
    retStatus.set_condition(BeamCondition::Ok);
  } else {
    logmessage = ("Message (BeamQuality): Bad beam spill at trigger timestamp "
		  + std::to_string(timestamp));
    Log(logmessage, v_message, verbosity);
    if (verbosity >=v_debug)
      retStatus.Print();

    retStatus.set_condition(BeamCondition::Bad);
  }

  return retStatus;
}
