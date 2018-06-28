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

BeamFetcher::BeamFetcher() : Tool(),
  beam_db_store_(false, BOOST_STORE_MULTIEVENT_FORMAT)
{}

bool BeamFetcher::Initialise(std::string config_filename, DataModel& data)
{
  // Load configuration file variables
  if ( !config_filename.empty() ) m_variables.Initialise(config_filename);

  // Assign a transient data pointer
  m_data = &data;

  m_variables.Get("verbose", verbosity_);

  bool got_db_filename = m_variables.Get("OutputFile", db_filename_);

  // Check for problems
  if ( !got_db_filename ) {
    Log("Error: Missing output filename in the configuration for the"
      " BeamFetcher tool", 0, verbosity_);
    return false;
  }

  // Check if the beam database file already exists using a dummy std::ifstream
  std::ifstream dummy_in_file(db_filename_);
  if ( dummy_in_file.good() ) {
    Log("Error: BeamFetcher output file \"" + db_filename_
     + "\" already exists", 0, verbosity_);
    return false;
  }
  dummy_in_file.close();


  return true;
}

bool BeamFetcher::Execute() {

  uint64_t start_ms_since_epoch;
  bool got_start_ms = m_variables.Get("StartMillisecondsSinceEpoch",
    start_ms_since_epoch);

  if ( !got_start_ms ) {
    Log("Error: Missing setting for the StartMillisecondsSinceEpoch"
      " configuration file option", 0, verbosity_);
    return false;
  }

  uint64_t end_ms_since_epoch;
  bool got_end_ms = m_variables.Get("EndMillisecondsSinceEpoch",
    end_ms_since_epoch);

  if ( !got_end_ms ) {
    Log("Error: Missing setting for the EndMillisecondsSinceEpoch"
      " configuration file option", 0, verbosity_);
    return false;
  }

  if ( start_ms_since_epoch >= end_ms_since_epoch ) {
    Log("Error: The start time for the BeamFetch tool must be less than"
      " the end time", 0, verbosity_);
    return false;
  }

  uint64_t chunk_step_ms;
  bool got_time_step = m_variables.Get("TimeChunkStepInMilliseconds",
    chunk_step_ms);

  if ( !got_time_step ) {
    Log("Error: Missing setting for the TimeChunkStepInMilliseconds"
      " configuration file option", 0, verbosity_);
    return false;
  }

  return fetch_beam_data(start_ms_since_epoch, end_ms_since_epoch,
    chunk_step_ms);
}


bool BeamFetcher::Finalise() {

  return true;
}

bool BeamFetcher::fetch_beam_data(uint64_t start_ms_since_epoch,
  uint64_t end_ms_since_epoch, uint64_t chunk_step_ms)
{
  // Get POT information backwards moving from the end time to the start
  // time in large chunks. Chunk sizes of two hours or so are recommended.
  // Chunks significantly longer than that will reliably cause some
  // database queries to fail.
  uint64_t current_time = end_ms_since_epoch;

  // Get a const reference to the beam database interface
  const auto& db = IFBeamDBInterface::Instance();

  // Storage for parsed data from the IF beam database
  std::map<std::string, std::map<uint64_t, BeamDataPoint> > beam_data;

  // Index to save in the BoostStore header. Allows easy searching of the
  // BeamDB entries later
  std::map<int, std::pair<uint64_t, uint64_t> > beam_db_index;

  int current_entry = 0;

  while (current_time >= start_ms_since_epoch) {

    if (current_entry > 0) current_time -= chunk_step_ms;

    time_t s_since_epoch = current_time / THOUSAND;
    std::string time_string = asctime(gmtime(&s_since_epoch));

    Log("Loading new beam data for " + time_string, 2, verbosity_);

    // Have a small overlap (THIRTY_SECONDS) between entries so that we
    // can be sure not to miss any time interval in the desired range
    beam_data = db.QueryBeamDB(current_time - chunk_step_ms,
      current_time + THIRTY_SECONDS);

    // TODO: remove hard-coded device name here
    const auto& pot_map = beam_data.at("E:TOR875");

    // Find the range of times for which E:TOR875 data exist in the
    // current beam database chunk
    uint64_t start_ms
      = std::numeric_limits<uint64_t>::max();
    uint64_t end_ms = 0ull;
    for (const auto& pair : pot_map) {
      if (pair.first < start_ms) start_ms = pair.first;
      if (pair.first > end_ms) end_ms = pair.first;
    }

    beam_db_index[current_entry] = std::pair<uint64_t,
      uint64_t>(start_ms, end_ms);

    beam_db_store_.Set("BeamDB", beam_data);
    beam_db_store_.Save(db_filename_);
    beam_db_store_.Delete();

    ++current_entry;
  }

  beam_db_store_.Header->Set("BeamDBIndex", beam_db_index);

  // Find the range of times covered by the entire downloaded database
  uint64_t overall_start_ms
      = std::numeric_limits<uint64_t>::max();
  uint64_t overall_end_ms = 0ull;

  for (const auto& pair : beam_db_index) {
    uint64_t temp_start_ms = pair.second.first;
    uint64_t temp_end_ms = pair.second.second;
    if (temp_start_ms < overall_start_ms) overall_start_ms = temp_start_ms;
    if (temp_end_ms > overall_end_ms) overall_end_ms = temp_end_ms;
  }

  beam_db_store_.Header->Set("StartMillisecondsSinceEpoch", overall_start_ms);
  beam_db_store_.Header->Set("EndMillisecondsSinceEpoch", overall_end_ms);

  beam_db_store_.Close();

  Log("Retrieval of beam status data complete.", 1, verbosity_);

  return true;
}
