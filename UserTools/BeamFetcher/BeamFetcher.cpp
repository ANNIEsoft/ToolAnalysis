// standard library includes
#include <ctime>
#include <limits>

// ToolAnalysis includes
#include "BeamFetcher.h"
#include "IFBeamDBInterface.h"

namespace {
  constexpr uint64_t TWO_HOURS = 7200000ull; // ms
  constexpr uint64_t THIRTY_SECONDS = 30000ull; // ms
}

BeamFetcher::BeamFetcher():Tool()			     
{}

//------------------------------------------------------------------------------
bool BeamFetcher::Initialise(std::string config_filename, DataModel& data)
{
  // Load configuration file variables
  if ( !config_filename.empty() ) m_variables.Initialise(config_filename);

  // Assign a transient data pointer
  m_data = &data;

  // Get the things
  bool got_verbosity   = m_variables.Get("verbose", verbosity);
  bool got_bundleflag  = m_variables.Get("IsBundle", fIsBundle);
  bool got_devicesfile = m_variables.Get("DevicesFile", fDevicesFile);
  bool got_fetchflag   = m_variables.Get("FetchFromTimes", fFetchFromTimes);
  bool got_saveroot    = m_variables.Get("SaveROOT", fSaveROOT);
  bool got_chunkMSec   = m_variables.Get("TimeChunkStepInMilliseconds", fChunkStepMSec);
  
  // Only needed if FetchFromTimes == true
  bool got_dsflag        = m_variables.Get("DaylightSavings", fDaylightSavings);
  bool got_outfilename   = m_variables.Get("OutputFile", fOutFileName);
  bool got_tsmode        = m_variables.Get("TimestampMode", fTimestampMode);
  bool got_startfilename = m_variables.Get("StartDate", fStartDateFileName);
  bool got_stopfilename  = m_variables.Get("EndDate", fStopDateFileName);
  bool got_startMSec     = m_variables.Get("StartMillisecondsSinceEpoch", fStartMSec);
  bool got_stopMSec      = m_variables.Get("EndMillisecondsSinceEpoch", fStopMSec);
  bool got_runnum        = m_variables.Get("RunNumber", fRunNumber);

  
  // Check the config parameters and set default values if necessary 
  if (!got_verbosity) verbosity = 1;

  if (!got_devicesfile) {
    logmessage = ("Error (BeamFetcher): You must define which devices to poll"
		  " via a DevicesFile.");
    Log(logmessage, v_error, verbosity);
    return false;
  } else {
    // Grab the stuff from the file
    std::ifstream devicesFile(fDevicesFile);
    if ( devicesFile.good() ) {
      std::string line;
      while (std::getline(devicesFile, line))
	fDevices.push_back(line);

      if (!fDevices.size()) {
	logmessage = ("Error (BeamFetcher): No devices specified in your"
		      " Devices file.");
	Log(logmessage, v_error, verbosity);      
	return false;
      }
    } else{
      logmessage = ("Error (BeamFetcher): Devices file "
		    "\"" +  fDevicesFile + "\""
		    " does not exists");
      Log(logmessage, v_error, verbosity);      
      return false;
    }
    devicesFile.close();
  }

  if (!got_saveroot) {
    logmessage = ("Warning (BeamFetcher): SaveROOT was not"
		  " set in the config file. Using default \"false\"");
    Log(logmessage, v_warning, verbosity);
    fSaveROOT = false;
  }
    

  if (!got_chunkMSec) {
    logmessage = ("Warning (BeamFetcher): TimeChunkStepInMilliseconds was not"
		  " set in the config file. Using default \"7200000\"");
    Log(logmessage, v_warning, verbosity);
    fChunkStepMSec = 7200000;
  }


  if (!got_bundleflag || (fIsBundle !=0 && fIsBundle != 1)) {
    logmessage = ("Error (BeamFetcher): IsBundle flag was not set correctly"
		    " in the config file.");
    Log(logmessage, v_error, verbosity);
    return 0;
  }


  if (!got_fetchflag) {
    logmessage = ("Warning (BeamFetcher): FetchFromTimes flag was not set in"
		  " the config file. Using default \"0\"");
    Log(logmessage, v_warning, verbosity);
    fFetchFromTimes = 0;
  }


  
  // We only care about most of the config parameters if we're fetching from
  // given times instead of CTC instead
  if (fFetchFromTimes) {
    if (!got_outfilename) {
      logmessage = ("Error (BeamFetcher): You want to SaveToFile but did not"
		    " supply an OutputFile name.");
      Log(logmessage, v_error, verbosity);
      return false;
    }

    // Check if outfile exists
    std::ifstream dummyFile(fOutFileName);
    if ( dummyFile.good() ) {
      logmessage = ("Error (BeamFetcher): Output file "
		    "\"" +  fOutFileName + "\""
		    " already exists");
      Log(logmessage, v_error, verbosity);
      
      return false;
    }
    dummyFile.close();

    
    if (!got_dsflag || (fDaylightSavings != 1 && fDaylightSavings != 0) ) {
      logmessage = ("Warning (BeamFetcher): DaylightSavings flag was not set "
		    " correctly in the config file. Using default \"0\"");
      Log(logmessage, v_warning, verbosity);
      fDaylightSavings = false;
    }

    if (!got_tsmode || (fTimestampMode != "MSEC" &&
			fTimestampMode != "LOCALDATE" &&
			fTimestampMode != "DB") ) {
      logmessage = ("Warning (BeamFetcher): TimestampMode flag was not set correctly"
		    " in the config file. Using default \"MSEC\"");
      Log(logmessage, v_warning, verbosity);
      fTimestampMode = "MSEC";
    }

    // Checking timestamp inputs based on timestamp mode
    if (fTimestampMode == "MSEC") {
      if (!got_startMSec) {
	logmessage = ("Error (BeamFetcher): Using MSEC TimestampMode, but missing"
		      " StartMillisecondsSinceEpoch in config file.");
	Log(logmessage, v_error, verbosity);
	return false;
      }
      
      if (!got_stopMSec) {
	logmessage = ("Error (BeamFetcher): Using MSEC TimestampMode, but missing"
		      " EndMillisecondsSinceEpoch in config file.");
	Log(logmessage, v_error, verbosity);
	return false;
      }
    } else if (fTimestampMode == "LOCALDATE") {
      if (!got_startfilename) {
	logmessage = ("Error (BeamFetcher): Using LOCALDATE TimestampMode, but missing"
		      " StartDate in config file.");
	Log(logmessage, v_error, verbosity);
	return false;
      }
      
      if (!got_stopfilename) {
	logmessage = ("Error (BeamFetcher): Using LOCALDATE TimestampMode, but missing"
		      " EndDate in config file.");
	Log(logmessage, v_error, verbosity);
	return false;
      }

      // If we are using localdate files then grab the time in msec
      ifstream file_startdate(fStopDateFileName);
      std::string start_timestamp;
      getline(file_startdate,start_timestamp);
      file_startdate.close();
      ifstream file_stopdate(fStopDateFileName);
      std::string stop_timestamp;
      getline(file_stopdate,stop_timestamp);
      file_stopdate.close();

      this->ConvertDateToMSec(start_timestamp,stop_timestamp,fStartMSec,fStopMSec);
    } else if (fTimestampMode == "DB") {
      if (!got_runnum) {
	logmessage = ("Error (BeamFetcher): Using DB TimestampMode, but missing"
		      " RunNumber in config file.");
	Log(logmessage, v_error, verbosity);
	return false;
      }
    }// end check for fTimestampMode
  }// end if fFetchFromTimes

  if (fSaveROOT) this->SetupROOTFile();
  
  return true;
}


//------------------------------------------------------------------------------
bool BeamFetcher::Execute()
{
  // Check for RunInfoDB if we want it
  if (fTimestampMode == "DB") {
    std::map<int,std::map<std::string,std::string>> RunInfoDB;
    bool got_runinfo_db = m_data->CStore.Get("RunInfoDB",RunInfoDB);
    if (got_runinfo_db){
      if (RunInfoDB.count(fRunNumber) > 0){
	std::string start_timestamp = RunInfoDB.at(fRunNumber)["StartTime"];
	std::string stop_timestamp = RunInfoDB.at(fRunNumber)["EndTime"];
	///Convert string start/end dates to milliseconds
	this->ConvertDateToMSec(start_timestamp,stop_timestamp,fStartMSec,fStopMSec);
      } else {
	logmessage = ("Error (BeamFetcher): Did not find entry for run "
		      + std::to_string(fRunNumber)
		      + " in RunInfoDB map. Is the run information database complete?");
	Log(logmessage, v_error, verbosity);
	return false;
      }
    } else {
      logmessage = ("Error (BeamFetcher): Did not find RunInfoDB in CStore - "
		    "Did you run the LoadRunInfo tool beforehand?");
      Log(logmessage, v_error, verbosity);
    }
  }

  
  // Do the things
  bool goodFetch = false;
  
  if (fFetchFromTimes)
    goodFetch = this->FetchFromTimes();
  else 
    goodFetch = this->FetchFromTrigger();

  if (goodFetch) {
    if (fFetchFromTimes) {
      goodFetch = this->SaveToFile();
    } else {
      // Emplace fBeamDataToSave to CStore for other tools to use
      m_data->CStore.Set("BeamData",fBeamDataToSave);      
      goodFetch = true;
    }

    if (fSaveROOT) this->WriteTrees();

    // Clear for the next Fetch
    fBeamDataToSave.clear();
  }

  return goodFetch;
}

//------------------------------------------------------------------------------
bool BeamFetcher::Finalise()
{
  if (fSaveROOT) this->SaveROOTFile();
  
  std::cout << "BeamFetcher tool exitting" << std::endl;

  
  return true;
}

bool BeamFetcher::FetchFromTimes()
{
  // TODO: Reassess all of this...
  
  logmessage = "BeamFetcher: StartMSec = " + std::to_string(fStartMSec);
  Log(logmessage, v_message, verbosity);
  logmessage = "BeamFetcher: StopMSec = " + std::to_string(fStopMSec);
  Log(logmessage, v_message, verbosity);

  if ( fStartMSec >= fStopMSec ) {
    logmessage = ("Error (BeamFetcher): The start time must be less than"
		  " the end time");
    Log(logmessage, v_error, verbosity);
    return false;
  }
  
  // Get POT information backwards moving from the end time to the start
  // time in large chunks. Chunk sizes of two hours or so are recommended.
  // Chunks significantly longer than that will reliably cause some
  // database queries to fail.
  uint64_t current_time = fStopMSec;

  // Get a const reference to the beam database interface
  const auto& db = IFBeamDBInterface::Instance();

  int current_entry = 0;

  while (current_time >= fStartMSec) {

    if (current_entry > 0) current_time -= fChunkStepMSec;

    time_t s_since_epoch = current_time / THOUSAND;
    std::string time_string = asctime(gmtime(&s_since_epoch));

    logmessage = "Loading new beam data for " + time_string;
    Log(logmessage, v_message, verbosity);

    // Have a small overlap (THIRTY_SECONDS) between entries so that we
    // can be sure not to miss any time interval in the desired range
    if (fIsBundle) {
      auto tempMap = db.QueryBeamDBBundleSpan(fDevices[0], current_time - fChunkStepMSec,
					      current_time + THIRTY_SECONDS);

      fBeamData.insert(tempMap.begin(), tempMap.end());
    } else {
      for (auto device : fDevices) {
	auto tempMap = db.QueryBeamDBSingleSpan(device, current_time - fChunkStepMSec,
						current_time + THIRTY_SECONDS);
	
	fBeamData.insert(tempMap.begin(), tempMap.end());
      }
    }
    
    // TODO: remove hard-coded device name here
    uint64_t start_ms = std::numeric_limits<uint64_t>::max();
    uint64_t end_ms = 0ull;
    
    // TODO: make this work again asutton
    // if (fBeamData.count("E:TOR875")>0) {
    //   //Account for two hour timeframes where there's no beam
    //   const auto& pot_map = fBeamData.at("E:TOR875");

    //   // Find the range of times for which E:TOR875 data exist in the
    //   // current beam database chunk
    //   for (const auto& pair : pot_map) {
    //     if (pair.first < start_ms) start_ms = pair.first;
    //     if (pair.first > end_ms) end_ms = pair.first;
    //   }
    // } else {
    //   start_ms = current_time;
    //   end_ms = current_time + fChunkStepMSec;
    // } 
      
    fBeamDBIdx[current_entry] = std::make_pair(start_ms, end_ms);

    ++current_entry;
  }

  return true;
}

//------------------------------------------------------------------------------
bool BeamFetcher::FetchFromTrigger()
{
  // Get a const reference to the beam database interface
  const auto& db = IFBeamDBInterface::Instance();

  // Need to get the trigger times
  std::map<uint64_t,std::vector<uint32_t>>* TimeToTriggerWordMap=nullptr;
  bool get_ok = m_data->CStore.Get("TimeToTriggerWordMap",TimeToTriggerWordMap);

  // Now loop over the CTC timestamps
  if (get_ok && TimeToTriggerWordMap) {
    for (auto iterator : *TimeToTriggerWordMap) {
      // We only want to get beam info for beam triggers (word 5)
      bool hasBeamTrig = false;
      for (auto word : iterator.second) 
	if (word == 5) hasBeamTrig = true;
      if (!hasBeamTrig) continue;
      
      uint64_t trigTimestamp = iterator.first;

      // Need to drop from ns to ms. This means that some timestamps will
      // already be recorded. We can skip these cases
      trigTimestamp = trigTimestamp/1E6;
      if (fBeamDataToSave.find(trigTimestamp) != fBeamDataToSave.end()) 
	continue;

      // Check if we already have the info we need
      bool fetch = false;
      std::map<uint64_t, std::map<std::string, BeamDataPoint> >::iterator low, prev;
      low = fBeamData.lower_bound(trigTimestamp);
      if (low == fBeamData.end()) {
	fetch = true;
	logmessage = ("BeamFetcher: I'm going to query the DB");
	Log(logmessage, v_message, verbosity);
      }
      
      // We'll pull fChunkStepMSec worth of data at a time to avoid rapid queries      
      if (fetch) {
	if (fIsBundle) {
	  fBeamData = db.QueryBeamDBBundleSpan(fDevices[0], trigTimestamp, trigTimestamp+fChunkStepMSec);
	} else {
	  std::map<uint64_t, std::map<std::string, BeamDataPoint> > tempMap;
	  
	  for (auto device : fDevices) {
	    auto tempMap = db.QueryBeamDBSingleSpan(device, trigTimestamp, trigTimestamp+fChunkStepMSec);
	    fBeamData.insert(tempMap.begin(), tempMap.end());
	  }	
	}
      }	
	
      // Now we can match the Beam info to CTC timestamps for saving to the CStore
      low = fBeamData.lower_bound(trigTimestamp);
      if (low == fBeamData.end()) {
	logmessage = ("Error (BeamFetcher): We fetched the data based on the CTC"
		      " but somehow didn't turn anything up!?");
	Log(logmessage, v_error, verbosity);
	return false;
      } else if (low == fBeamData.begin()) {
	fBeamDataToSave[trigTimestamp] = low->second;
      } else {
	// Check the previous DB timestamp to see if it's closer in time
	prev = std::prev(low);
	if ((trigTimestamp - prev->first) < (low->first - trigTimestamp))
	  fBeamDataToSave[trigTimestamp] = prev->second;
	else
	  fBeamDataToSave[trigTimestamp] = low->second;
      }
    }// end loop over trigger times
  } else {
    logmessage = ("Error (BeamFetcher): Could not load CTC information for"
		  " timestamps. Did you run TriggerDataDecoder?");
    Log(logmessage, v_error, verbosity);    
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool BeamFetcher::SaveToFile()
{
  BoostStore fBeamDBStore(false, BOOST_STORE_MULTIEVENT_FORMAT);
  fBeamDBStore.Set("BeamData", fBeamData);
  fBeamDBStore.Save(fOutFileName);
  fBeamDBStore.Delete();

  fBeamDBStore.Header->Set("BeamDBIndex", fBeamDBIdx);

  // Find the range of times covered by the entire downloaded database
  uint64_t overall_start_ms = std::numeric_limits<uint64_t>::max();
  uint64_t overall_end_ms = 0ull;

  for (const auto& pair : fBeamDBIdx) {
    uint64_t temp_start_ms = pair.second.first;
    uint64_t temp_end_ms = pair.second.second;
    if (temp_start_ms < overall_start_ms) overall_start_ms = temp_start_ms;
    if (temp_end_ms > overall_end_ms) overall_end_ms = temp_end_ms;
  }

  fBeamDBStore.Header->Set("StartMillisecondsSinceEpoch", overall_start_ms);
  fBeamDBStore.Header->Set("EndMillisecondsSinceEpoch", overall_end_ms);

  fBeamDBStore.Close();

  logmessage = "Retrieval of beam status data complete";
  Log(logmessage, v_warning, verbosity);

  return true;
}

//------------------------------------------------------------------------------
void BeamFetcher::SetupROOTFile()
{
  fOutFile = new TFile("beamfetcher_tree.root", "RECREATE");
  fOutTree = new TTree("BeamTree", "BeamTree");
  fOutTree->Branch("Timestamp", &fTimestamp);
}

//------------------------------------------------------------------------------
void BeamFetcher::WriteTrees()
{
  // Loop over timestamps
  int devCounter = 0;
  for (const auto iterTS : fBeamDataToSave) {
    fTimestamp = iterTS.first;
    
    // Loop over devices
    for (const auto iterDev : iterTS.second) {
      std::string device = iterDev.first;
      std::replace( device.begin(), device.end(), ':', '_');
      
      BeamDataPoint tempPoint = iterDev.second;

      // Dynamically create branches for each new device
      if (fDevIdx.find(device) == fDevIdx.end()) {
	fDevIdx[device] = devCounter;
	fOutTree->Branch(device.c_str(),
			 &fValues[fDevIdx.at(device)]);
	++devCounter;
      }

      fValues[fDevIdx.at(device)] = tempPoint.value;
    }// end loop over devices

    fOutTree->Fill();
  }// end loop over timestamps

}

//------------------------------------------------------------------------------
void BeamFetcher::SaveROOTFile()
{
  fOutFile->cd();
  fOutTree->Write();
  fOutFile->Close();
}

//------------------------------------------------------------------------------
// bool BeamFetcher::fetch_beam_data(uint64_t fStartMSec,
//   uint64_t fStopMSec, uint64_t chunk_step_ms)
// {
// }

void BeamFetcher::ConvertDateToMSec(std::string start_str,std::string end_str,uint64_t &start_ms,uint64_t &end_ms){

  uint64_t TimeZoneShift = 21600000;
  if (fDaylightSavings) TimeZoneShift = 18000000;
  
  std::string epoch_start = "1970/1/1";
  boost::posix_time::ptime Epoch(boost::gregorian::from_string(epoch_start));
  if (verbosity > 2) std::cout <<"BeamFetcher: Convert Date To Msec: start_str: "<<start_str<<", end_str: "<<end_str<<std::endl;
  boost::posix_time::ptime ptime_start(boost::posix_time::time_from_string(start_str));
  boost::posix_time::time_duration start_duration;
  start_duration = boost::posix_time::time_duration(ptime_start - Epoch);
  start_ms = start_duration.total_milliseconds()+TimeZoneShift;
  boost::posix_time::time_duration end_duration;
  boost::posix_time::ptime ptime_end(boost::posix_time::time_from_string(end_str));
  end_duration = boost::posix_time::time_duration(ptime_end - Epoch);
  end_ms = end_duration.total_milliseconds()+TimeZoneShift;

  //logmessage = "BeamFetcher: start_ms: " + start_ms + ", end_ms: " + end_ms;
  //Log(logmessage, v_message, verbosity);
}


