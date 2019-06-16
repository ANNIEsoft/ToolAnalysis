// standard library includes
//#include <algorithm>
//#include <ctime>
//#include <fstream>
//#include <sstream>

// ToolAnalysis includes
//#include "ANNIEconstants.h"
#include "BeamChecker.h"
//#include "BeamDataPoint.h"
//#include "BeamStatus.h"
//#include "HeftyInfo.h"
//#include "TimeClass.h"

// Definitions local to this source file
namespace {

  constexpr double BOGUS_DOUBLE = -9999.;

  std::string make_time_string(uint64_t ms_since_epoch) {
    time_t s_since_epoch = ms_since_epoch / THOUSAND;
    std::string time_string = asctime(gmtime(&s_since_epoch));
    time_string.pop_back(); // remove the trailing \n from the time string
    return time_string;
  }

}

BeamChecker::BeamChecker() : Tool(),
  beam_db_store_(false, BOOST_STORE_MULTIEVENT_FORMAT)
{}

bool BeamChecker::Initialise(std::string config_filename, DataModel& data)
{
  // Load configuration file variables
  if ( !config_filename.empty() ) m_variables.Initialise(config_filename);

  // Assign a transient data pointer
  m_data = &data;

  return initialise_beam_db();
}

bool BeamChecker::Execute() {

  // Get a pointer to the ANNIEEvent Store
  auto* annie_event = m_data->Stores["ANNIEEvent"];

  if (!annie_event) {
    Log("Error: The BeamChecker tool could not find the ANNIEEvent Store", 0,
      verbosity_);
    return false;
  }

  // Load timing information for each of the minibuffers from the event.
  // The minibuffer times are represented differently for Hefty vs. non-Hefty
  // data.

  // Decide whether we're using Hefty vs. non-Hefty data by checking whether
  // the HeftyInfo object is present
  bool hefty_mode = annie_event->Has("HeftyInfo");

  HeftyInfo hefty_info;
  std::vector<TimeClass> mb_timestamps;

  size_t num_minibuffers = 0u;

  if (hefty_mode) {
    bool got_hefty_info = annie_event->Get("HeftyInfo", hefty_info);

    if ( !got_hefty_info ) {
      Log("Error: The BeamChecker tool failed to load a HeftyInfo object", 0,
        verbosity_);
      return false;
    }

    num_minibuffers = hefty_info.num_minibuffers();

  }
  else {
    // non-Hefty data
    bool got_mb_timestamps = annie_event->Get("MinibufferTimestamps",
      mb_timestamps);

    // Check for problems
    if ( !got_mb_timestamps ) {
      Log("Error: The BeamChecker tool could not find any minibuffer"
        " timestamps", 0, verbosity_);
      return false;
    }
    else if ( mb_timestamps.empty() ) {
      Log("Error: The BeamChecker tool found an empty vector of minibuffer"
        " timestamps", 0, verbosity_);
      return false;
    }

    num_minibuffers = mb_timestamps.size();
  }

  // Load the minibuffer labels for the current event
  std::vector<MinibufferLabel> minibuffer_labels;

  bool got_mb_labels = annie_event->Get("MinibufferLabels", minibuffer_labels);

  // Check for problems
  if ( !got_mb_labels ) {
    Log("Error: The BeamChecker tool could not find the minibuffer labels", 0,
      verbosity_);
    return false;
  }
  else if ( minibuffer_labels.empty() ) {
    Log("Error: The BeamChecker tool found an empty vector of minibuffer"
      " labels", 0, verbosity_);
    return false;
  }

  // Build the vector of beam statuses (with one for each minibuffer)
  std::vector<BeamStatus> beam_statuses;

  for (size_t mb = 0; mb < num_minibuffers; ++mb) {

    uint64_t ns_since_epoch = 0ull;
    if (hefty_mode) ns_since_epoch = hefty_info.time(mb);
    else ns_since_epoch = mb_timestamps.at(mb).GetNs();

    const auto& mb_label = minibuffer_labels.at(mb);

    auto temp_beam_status = get_beam_status(ns_since_epoch, mb_label);

    std::stringstream temp_ss;
    temp_ss << std::scientific << std::setprecision(4);
    temp_ss << temp_beam_status.pot();

    Log(make_beam_condition_string( temp_beam_status.condition() )
      + " minibuffer had " + temp_ss.str() + " POT", 2, verbosity_);

    beam_statuses.push_back( temp_beam_status );
  }

  annie_event->Set("BeamStatuses", beam_statuses);

  return true;
}


bool BeamChecker::Finalise() {

  return true;
}

bool BeamChecker::initialise_beam_db() {

  m_variables.Get("verbose", verbosity_);

  std::string db_filename;
  bool got_db_filename = m_variables.Get("BeamDBFile", db_filename);

  // Check for problems
  if ( !got_db_filename ) {
    Log("Error: Missing beam database filename in the configuration for the"
      " BeamChecker tool", 0, verbosity_);
    return false;
  }

  // Check that the beam database file exists using a dummy std::ifstream
  std::ifstream dummy_in_file(db_filename);
  if ( !dummy_in_file.good() ) {
    Log("Error: Could not open the beam database file \"" + db_filename
     + '\"', 0, verbosity_);
    return false;
  }
  dummy_in_file.close();

  beam_db_store_.Initialise( db_filename );

  bool got_index = beam_db_store_.Header->Get("BeamDBIndex", beam_db_index_);

  if ( !got_index ) {
    Log("Error: Could not find the BeamDBIndex entry in the beam database"
      " BoostStore header stored in the file \"" + db_filename + '\"', 0,
      verbosity_);
    return false;
  }

  bool got_start = beam_db_store_.Header->Get("StartMillisecondsSinceEpoch",
    start_ms_since_epoch_);

  if ( !got_start ) {
    Log("Error: Could not find the StartMillisecondsSinceEpoch entry in the"
      " beam database BoostStore header stored in the file \"" + db_filename
      + '\"', 0, verbosity_);
    return false;
  }

  bool got_end = beam_db_store_.Header->Get("EndMillisecondsSinceEpoch",
    end_ms_since_epoch_);

  if ( !got_end ) {
    Log("Error: Could not find the EndMillisecondsSinceEpoch entry in the"
      " beam database BoostStore header stored in the file \"" + db_filename
      + '\"', 0, verbosity_);
    return false;
  }

  if ( !beam_db_store_.Has("BeamDB") ) {
    Log("Error: Missing BeamDB key in the beam database BoostStore stored"
      " in the file \"" + db_filename
      + '\"', 0, verbosity_);
    return false;
  }

  Log("Loaded beam database entries for times between "
    + make_time_string(start_ms_since_epoch_) + " and "
    + make_time_string(end_ms_since_epoch_), 1, verbosity_);

  return true;
}

BeamStatus BeamChecker::get_beam_status(uint64_t ns_since_epoch,
  MinibufferLabel mb_label)
{
  // Return immediately if the current minibuffer does not correspond to a beam
  // trigger (we don't want to double-count POT using self-trigger or
  // cosmic trigger minibuffers, for example)
  if ( mb_label != MinibufferLabel::Beam ) {
    return BeamStatus(TimeClass( ns_since_epoch ), 0.,
      BeamCondition::NonBeamMinibuffer);
  }

  // Retrieve device names needed for beam quality checks from the
  // configuration file
  //
  // The "first toroid" should be the one farther upstream from the target
  // (E:TOR860 in the current configuration)
  std::string first_toroid;
  m_variables.Get("FirstToroid", first_toroid);

  std::string second_toroid;
  m_variables.Get("SecondToroid", second_toroid);

  std::string horn_current_device;
  m_variables.Get("HornCurrentDevice", horn_current_device);

  // TODO: consider making the static variables used here into members of
  // the BeamChecker class instead
  static std::map<std::string, std::map<uint64_t, BeamDataPoint> >
    beam_data;
  static int current_beam_db_entry = 0;
  static bool got_first_beam_entry = false;

  if ( !got_first_beam_entry ) {
    beam_db_store_.GetEntry(current_beam_db_entry);
    beam_db_store_.Get("BeamDB", beam_data);
    got_first_beam_entry = true;
  }

  // The beam database uses timestamps with ms precision
  uint64_t ms_since_epoch = ns_since_epoch / MILLION;

  Log("Finding beam status information for "
    + make_time_string(ms_since_epoch), 2, verbosity_);

  // Find the beam database entry that contains POT information for the
  // moment of interest
  auto iter = std::find_if(beam_db_index_.cbegin(),
    beam_db_index_.cend(),
    [ms_since_epoch](const std::pair<int, std::pair<uint64_t,
      uint64_t> >& pair) -> bool {
      uint64_t start_ms = pair.second.first;
      uint64_t end_ms = pair.second.second;
      return ms_since_epoch >= start_ms && ms_since_epoch <= end_ms;
    }
  );

  // If a suitable entry could not be found, then complain and return
  // a BeamStatus object that indicates that the data were missing
  bool found_pot_entry = ( iter != beam_db_index_.cend() );

  if ( !found_pot_entry ) {
    Log("WARNING: unable to find a suitable entry for "
      + make_time_string(ms_since_epoch) + " (" + std::to_string(ms_since_epoch)
      + " ms since the Unix epoch) in the beam database file", 0, verbosity_);
    return BeamStatus( TimeClass(ns_since_epoch), 0., BeamCondition::Missing );
  }

  // If we need to load a new entry from the beam database, do so.
  // Avoid loading a new entry if you don't have to (the maps stored in
  // each entry are fairly large)
  int new_entry_number = iter->first;
  if ( new_entry_number != current_beam_db_entry ) {

    beam_db_store_.GetEntry(new_entry_number);
    beam_db_store_.Get("BeamDB", beam_data);

    current_beam_db_entry = new_entry_number;
  }

  // Temporary storage for this function's return value
  BeamStatus beam_status;

  // Retrieve the beam monitoring measurements for the current event, and
  // calculate a POT value
  try {

    // Retrieve a measurement for each of the monitoring devices included
    // in the bundle from the Intensity Frontier beam database
    for (const auto& device_pair : beam_data) {

      const std::string& device_name = device_pair.first;

      // Obtain a map of measurements for the current device.
      // Keys are timestamps (in ms since the Unix epoch),
      // while values are BeamDataPoint objects.
      const std::map<uint64_t, BeamDataPoint>& measurement_map
        = device_pair.second;

      // Find the measurement entry with the closest time to that requested by
      // the user, and add it to the BeamStatus object that will be returned
      std::map<uint64_t, BeamDataPoint>::const_iterator
        low = measurement_map.lower_bound(ms_since_epoch);

      if ( low == measurement_map.cend() ) {
        Log("WARNING: IF beam database did not have any information"
          " for device " + device_name + " at " + std::to_string(ms_since_epoch)
          + " ms after the Unix epoch (" + make_time_string(ms_since_epoch)
          + ')', 1, verbosity_);
      }

      else if ( low == measurement_map.cbegin() ) {
        beam_status.add_measurement(device_name, low->first, low->second );
      }

      // We're between two time values, so we need to figure out which is
      // closest to the value requested by the user
      else {

        std::map<uint64_t, BeamDataPoint>::const_iterator
          prev = low;
        --prev;

        if ((ms_since_epoch - prev->first) < (low->first - ms_since_epoch)) {
          beam_status.add_measurement(device_name, prev->first, prev->second);
        }
        else {
          beam_status.add_measurement(device_name, low->first, low->second);
        }
      }
    }

    if ( verbosity_ >= 4 ) {
      Log("Beam monitoring measurements", 4, verbosity_);
      for (const auto& pair : beam_status.data()) {
        Log(pair.first + " at " + std::to_string(pair.second.first)
          + " ms since epoch: " + std::to_string(pair.second.second.value)
          + " " + pair.second.second.unit, 4, verbosity_);
      }
    }

    // Now that we've retrieved all of the needed measurements, calculate a POT
    // value and a timestamp for the BeamStatus object as a whole.

    // We're catching exceptions, so if one of the required devices doesn't
    // exist, things will be handled properly.
    const auto& first_toroid_pair = beam_status.data().at(first_toroid);
    double pot_first_toroid = first_toroid_pair.second.value;
    int64_t ms_since_epoch_first_toroid = first_toroid_pair.first;

    const auto& second_toroid_pair = beam_status.data().at(second_toroid);
    double pot_second_toroid = second_toroid_pair.second.value;
    int64_t ms_since_epoch_second_toroid = second_toroid_pair.first;

    // Get the horn current measurement
    const auto& horn_pair = beam_status.data().at(horn_current_device);
    double peak_horn_current = horn_pair.second.value; // kA
    int64_t ms_since_epoch_horn = horn_pair.first;

    // TODO: perhaps add something more sophisticated here than just asking for
    // a POT value and a time from a single device.
    beam_status.set_pot( pot_second_toroid );
    beam_status.set_time( TimeClass(ms_since_epoch_second_toroid * MILLION) );

    // Wait to set the beam condition until we've applied quality cuts

    // Get beam quality cut parameters
    double pot_min = BOGUS_DOUBLE;
    double pot_max = BOGUS_DOUBLE;
    m_variables.Get("CutPOTMax", pot_max);
    m_variables.Get("CutPOTMin", pot_min);

    double horn_current_min = BOGUS_DOUBLE;
    double horn_current_max = BOGUS_DOUBLE;
    m_variables.Get("CutPeakHornCurrentMax", horn_current_max);
    m_variables.Get("CutPeakHornCurrentMin", horn_current_min);

    double toroid_tol = BOGUS_DOUBLE;
    m_variables.Get("CutToroidAgreement", toroid_tol);

    int64_t t_tol = BOGUS_INT;
    m_variables.Get("CutTimestampAgreement", t_tol);

    // TODO: add beam targeting efficiency cut

    BeamCondition bc = BeamCondition::Ok;

    // POT cut
    beam_status.add_cut("POT in range", (beam_status.pot() >= pot_min)
      && (beam_status.pot() <= pot_max));

    // Peak horn current cut
    beam_status.add_cut("peak horn current in range",
      (peak_horn_current >= horn_current_min)
      && (peak_horn_current <= horn_current_max));

    // Toroid disagreement cut
    double tor_diff_frac = 2.*std::abs( pot_second_toroid - pot_first_toroid )
      / ( pot_second_toroid + pot_first_toroid );
    beam_status.add_cut("toroids agree", tor_diff_frac <= toroid_tol);

    // Timestamp cut. Make sure measurements within the timing tolerance
    // were available for all required devices
    int64_t ms_since_epoch = static_cast<int64_t>( ns_since_epoch / MILLION );
    beam_status.add_cut("timestamps agree",
      ( std::abs(ms_since_epoch_horn - ms_since_epoch) <= t_tol )
      &&  ( std::abs(ms_since_epoch_first_toroid - ms_since_epoch) <= t_tol )
      &&  ( std::abs(ms_since_epoch_second_toroid - ms_since_epoch) <= t_tol ));

    Log("ANNIE DAQ ms since epoch = " + std::to_string(ms_since_epoch), 4,
      verbosity_);
    Log(horn_current_device + " ms since epoch = "
      + std::to_string(ms_since_epoch_horn), 4, verbosity_);
    Log(first_toroid + " ms since epoch = "
      + std::to_string(ms_since_epoch_first_toroid), 4, verbosity_);
    Log(second_toroid + " ms since epoch = "
      + std::to_string(ms_since_epoch_second_toroid), 4, verbosity_);

    // Flag the beam spill as "bad" if it failed any of the cuts above
    if ( !beam_status.passed_all_cuts() )
    {
      bc = BeamCondition::Bad;

      Log("WARNING: bad beam spill", 1, verbosity_);
    }

    if ( verbosity_ > 3 ) {
      for ( const auto& pair : beam_status.cuts() ) {
        std::string temp_string;
        if (pair.second) temp_string = "PASSED ";
        else temp_string = "FAILED ";
        Log(temp_string + pair.first + " beam quality cut", 4, verbosity_);
      }
    }

    beam_status.set_condition( bc );
  }

  catch (const std::exception& e) {
    Log("WARNING: problem encountered while querying IF beam"
      " database: " + std::string( e.what() ), 0, verbosity_);

    // There was a problem, so create a BeamStatus object that assumes that
    // the data were missing for some reason.
    return BeamStatus( TimeClass(ns_since_epoch), 0., BeamCondition::Missing );
  }

  return beam_status;
}
