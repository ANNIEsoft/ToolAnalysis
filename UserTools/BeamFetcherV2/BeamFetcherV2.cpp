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
  bool got_verbosity   = m_variables.Get("verbose", verbosity);
  bool got_bundleflag  = m_variables.Get("IsBundle", fIsBundle);
  bool got_devicesfile = m_variables.Get("DevicesFile", fDevicesFile);
  bool got_saveroot    = m_variables.Get("SaveROOT", fSaveROOT);
  bool got_chunkMSec   = m_variables.Get("TimeChunkStepInMilliseconds", fChunkStepMSec);
  
  
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
  
  return true;
}


//------------------------------------------------------------------------------
bool BeamFetcherV2::Execute()
{
  // Do the things
  bool goodFetch = this->FetchFromTrigger();

  if (goodFetch) {
      // Emplace fBeamDataToSave to CStore for other tools to use
      m_data->CStore.Set("BeamData",fBeamDataToSave);      
      goodFetch = true;
    }

  if (fSaveROOT) this->WriteTrees();

  // Clear for the next Fetch
  fBeamDataToSave.clear();
  

  return goodFetch;
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
	logmessage = ("BeamFetcherV2: I'm going to query the DB");
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
	logmessage = ("Error (BeamFetcherV2): We fetched the data based on the CTC"
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
    logmessage = ("Error (BeamFetcherV2): Could not load CTC information for"
		  " timestamps. Did you run TriggerDataDecoder?");
    Log(logmessage, v_error, verbosity);    
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool BeamFetcherV2::SaveToFile()
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
void BeamFetcherV2::SaveROOTFile()
{
  fOutFile->cd();
  fOutTree->Write();
  fOutFile->Close();
}

