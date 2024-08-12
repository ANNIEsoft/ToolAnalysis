// standard library includes
#include <ctime>
#include <limits>

// ToolAnalysis includes
#include "BeamFetcherV2.h"
#include "IFBeamDBInterfaceV2.h"

namespace {
  constexpr uint64_t TWO_HOURS = 7200000ull; // ms
  constexpr uint64_t THIRTY_SECONDS = 30000ull; // ms
}

BeamFetcherV2::BeamFetcherV2():Tool()			     
{}

//------------------------------------------------------------------------------
bool BeamFetcherV2::Initialise(std::string config_filename, DataModel& data)
{
  // Load configuration file variables
  if ( !config_filename.empty() ) m_variables.Initialise(config_filename);

  // Assign a transient data pointer
  m_data = &data;

  // Get the things
  bool got_verbosity     = m_variables.Get("verbose", verbosity);
  bool got_bundleflag    = m_variables.Get("IsBundle", fIsBundle);
  bool got_devicesfile   = m_variables.Get("DevicesFile", fDevicesFile);
  bool got_saveroot      = m_variables.Get("SaveROOT", fSaveROOT);
  bool got_chunkMSec     = m_variables.Get("TimeChunkStepInMilliseconds", fChunkStepMSec);
  bool got_deletectcdata = m_variables.Get("DeleteCTCData", fDeleteCTCData);
  
  
  // Check the config parameters and set default values if necessary 
  if (!got_verbosity) verbosity = 1;

  if (!got_devicesfile) {
    logmessage = ("Error (BeamFetcherV2): You must define which devices to poll"
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
	logmessage = ("Error (BeamFetcherV2): No devices specified in your"
		      " Devices file.");
	Log(logmessage, v_error, verbosity);      
	return false;
      }
    } else{
      logmessage = ("Error (BeamFetcherV2): Devices file "
		    "\"" +  fDevicesFile + "\""
		    " does not exists");
      Log(logmessage, v_error, verbosity);      
      return false;
    }
    devicesFile.close();
  }

  if (!got_saveroot) {
    logmessage = ("Warning (BeamFetcherV2): SaveROOT was not"
		  " set in the config file. Using default \"false\"");
    Log(logmessage, v_warning, verbosity);
    fSaveROOT = false;
  }


  if (!got_deletectcdata) {
    logmessage = ("Warning (BeamFetcherV2): DeleteCTCData was not set in the "
		  "config file. If you're not running downstream tools that "
		  "remove the CTC from the CStore then you probably want to "
		  " set this to true to save memory. Using default \"false\"");
    Log(logmessage, v_warning, verbosity);
    fDeleteCTCData = false;
  }
    

  if (!got_chunkMSec) {
    logmessage = ("Warning (BeamFetcherV2): TimeChunkStepInMilliseconds was not"
		  " set in the config file. Using default \"7200000\"");
    Log(logmessage, v_warning, verbosity);
    fChunkStepMSec = 7200000;
  }


  if (!got_bundleflag || (fIsBundle !=0 && fIsBundle != 1)) {
    logmessage = ("Error (BeamFetcherV2): IsBundle flag was not set correctly"
		    " in the config file.");
    Log(logmessage, v_error, verbosity);
    return 0;
  }

  if (fSaveROOT) this->SetupROOTFile();

  // initialize the last timestamp
  fLastTimestampFetched = 0;
  fLastTimestampSaved = 0;
  BeamDataMap = new std::map<uint64_t, std::map<std::string, BeamDataPoint> >;

  m_data->CStore.Set("NewBeamDataAvailable", false);
  
  return true;
}


//------------------------------------------------------------------------------
bool BeamFetcherV2::Execute()
{
  m_data->CStore.Set("NewBeamDataAvailable", false);
  // Do the things
  bool got_ctc = m_data->CStore.Get("NewCTCDataAvailable", fNewCTCData);
  bool goodFetch = false;
  if (got_ctc && fNewCTCData) {
    logmessage = ("Message (BeamFetcherV2): New CTC data found. Fetching. ");
    Log(logmessage, v_message, verbosity);  

  goodFetch = this->FetchFromTrigger();
  } else {
    logmessage = ("Warning (BeamFetcherV2): No new CTC data found. Nothing to fetch. ");
    Log(logmessage, v_message, verbosity);    
  }

  // Save it out
  if (goodFetch) {
    logmessage = ("Debug (BeamFetcherV2): Writing out BeamDataMap, which has size: " + std::to_string(BeamDataMap->size()));
    Log(logmessage, v_debug, verbosity);    
    
    // put BeamDataMap in CStore for other tools to use
    m_data->CStore.Set("NewBeamDataAvailable", true);
    m_data->CStore.Set("BeamDataMap", BeamDataMap);
    if (fSaveROOT) this->WriteTrees();
  } else if (fNewCTCData) {
    logmessage = ("Warning (BeamFetcherV2): Bad fetch. ");
    Log(logmessage, v_message, verbosity);    
  }

  return true;
}

//------------------------------------------------------------------------------
bool BeamFetcherV2::Finalise()
{
  if (fSaveROOT) this->SaveROOTFile();
  
  std::cout << "BeamFetcherV2 tool exitting" << std::endl;
  
  return true;
}

//------------------------------------------------------------------------------
bool BeamFetcherV2::FetchFromTrigger()
{
  // Get a const reference to the beam database interface
  const auto& db = IFBeamDBInterfaceV2::Instance();

  // Need to get the trigger times
  std::map<uint64_t,std::vector<uint32_t>>* TimeToTriggerWordMap=nullptr;
  bool got_triggers = m_data->CStore.Get("TimeToTriggerWordMap",TimeToTriggerWordMap);

  // Now loop over the CTC timestamps
  // But start at the fLastTimeStampFetched to prevent double counting if the timestamp data wasn't deleted
  if (got_triggers && TimeToTriggerWordMap) {
    for (auto iterator = TimeToTriggerWordMap->lower_bound(fLastTimestampFetched+1);
	 iterator != TimeToTriggerWordMap->end(); ++iterator) {

      // We only care about beam triggers here - grab the undelayed beam trigger 14
      if (std::find(iterator->second.begin(), iterator->second.end(), 14) == iterator->second.end()) {
	continue;
      }
      
      // Grab the timestamp
      uint64_t trigTimestamp = iterator->first;
      fLastTimestampFetched = trigTimestamp;

      // Need to drop from ns to ms. This means that some timestamps will
      // already be recorded. We can skip these cases
      trigTimestamp = trigTimestamp/1E6;
      if (BeamDataMap->find(trigTimestamp) != BeamDataMap->end()) {
	logmessage = ("Debug (BeamFetcherV2): BeamDataMap already has this timstamp: "
		      + std::to_string(trigTimestamp) + ", skipping.");
	Log(logmessage, vv_debug, verbosity);    

	continue;
      }

      // Check if we already have the info we need
      bool fetch = false;
      std::map<uint64_t, std::map<std::string, BeamDataPoint> >::iterator low, prev;
      low = BeamDataQuery.lower_bound(trigTimestamp);
      if (low == BeamDataQuery.end()) {
	fetch = true;
	logmessage = ("BeamFetcherV2: I'm going to query the DB");
	Log(logmessage, v_message, verbosity);
      }
      
      // We'll pull fChunkStepMSec worth of data at a time to avoid rapid queries      
      if (fetch) {
	if (fIsBundle) {
	  BeamDataQuery = db.QueryBeamDBBundleSpan(fDevices[0], trigTimestamp, trigTimestamp+fChunkStepMSec);
	} else {
	  std::map<uint64_t, std::map<std::string, BeamDataPoint> > tempMap;
	  for (auto device : fDevices) {
	    auto tempMap = db.QueryBeamDBSingleSpan(device, trigTimestamp, trigTimestamp+fChunkStepMSec);
	    BeamDataQuery.insert(tempMap.begin(), tempMap.end());
	  }	
	}
      }	
	
      // Now we can match the Beam info to CTC timestamps for saving to the CStore
      low = BeamDataQuery.lower_bound(trigTimestamp);
      if (low == BeamDataQuery.end()) {
	logmessage = ("Error (BeamFetcherV2): We fetched the data based on the CTC"
		      " but somehow didn't turn anything up!?");
	Log(logmessage, v_error, verbosity);
	return false;
      } else if (low == BeamDataQuery.begin()) {
	BeamDataMap->emplace(trigTimestamp, low->second);
      } else {
	// Check the previous DB timestamp to see if it's closer in time
	prev = std::prev(low);
	if ((trigTimestamp - prev->first) < (low->first - trigTimestamp))
	  BeamDataMap->emplace(trigTimestamp, prev->second);
	else
	  BeamDataMap->emplace(trigTimestamp, low->second);
      }
    }// end loop over trigger times
  } else {
    logmessage = ("Error (BeamFetcherV2): Could not load CTC information for"
		  " timestamps. Did you run TriggerDataDecoder?");
    Log(logmessage, v_error, verbosity);    
    return false;
  }

  if (fDeleteCTCData)
    TimeToTriggerWordMap->clear();

  return true;
}

//------------------------------------------------------------------------------
bool BeamFetcherV2::SaveToFile()
{
  BoostStore fBeamDBStore(false, BOOST_STORE_MULTIEVENT_FORMAT);
  fBeamDBStore.Set("BeamData", BeamDataQuery);
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
void BeamFetcherV2::SetupROOTFile()
{
  fOutFile = new TFile("beamfetcher_tree.root", "RECREATE");
  fOutTree = new TTree("BeamTree", "BeamTree");
  fOutTree->Branch("Timestamp", &fTimestamp);
}

//------------------------------------------------------------------------------
void BeamFetcherV2::WriteTrees()
{
  // Loop over timestamps
  // But start at the fLastTimeStampSaved to prevent double counting if the timestamp data wasn't deleted
  int devCounter = 0;  
  for (auto iterTS = BeamDataMap->lower_bound(fLastTimestampSaved+1);
       iterTS != BeamDataMap->end(); ++iterTS) {

    fTimestamp = iterTS->first;
    fLastTimestampSaved = fTimestamp;

    std::cout << "Timestamp: " << fTimestamp << std::endl;
    
    // Loop over devices
    for (const auto iterDev : iterTS->second) {
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

// clear BeamDataMap after filling Tree
BeamDataMap->clear();

}

//------------------------------------------------------------------------------
void BeamFetcherV2::SaveROOTFile()
{
  fOutFile->cd();
  fOutTree->Write();
  fOutFile->Close();
}

