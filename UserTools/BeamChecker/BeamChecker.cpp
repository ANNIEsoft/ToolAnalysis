// standard library includes
#include <algorithm>
#include <ctime>

// ToolAnalysis includes
#include "BeamChecker.h"
#include "BeamStatus.h"
#include "IFBeamDataPoint.h"

// Definitions local to this source file
namespace {

  // Used to convert from seconds and nanoseconds to milliseconds below
  constexpr uint64_t THOUSAND = 1000ull;
  constexpr uint64_t MILLION = 1000000ull;

  std::string make_time_string(uint64_t ms_since_epoch) {
    time_t s_since_epoch = ms_since_epoch / THOUSAND;
    std::string time_string = asctime(gmtime(&s_since_epoch));
    time_string.pop_back(); // remove the trailing \n from the time string
    return time_string;
  }

}

BeamChecker::BeamChecker() : Tool() {}

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

  // Load the map containing the ADC raw waveform data. We will use this
  // to get the timing information for each minibuffer.
  // TODO: alter the ANNIEEvent format so that you don't have to load the
  // raw waveforms in order to get the minibuffer timestamps
  std::map<ChannelKey, std::vector<Waveform<unsigned short> > >
    raw_waveform_map;

  bool got_raw_data = annie_event->Get("RawADCData", raw_waveform_map);

  // Check for problems
  if ( !got_raw_data ) {
    Log("Error: The BeamChecker tool could not find the RawADCData entry", 0,
      verbosity_);
    return false;
  }
  else if ( raw_waveform_map.empty() ) {
    Log("Error: The BeamChecker tool found an empty RawADCData entry", 0,
      verbosity_);
    return false;
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

  // Arbitrarily use channel 1 to get the minibuffer timestamps (should be good
  // enough since the beam database has millisecond time resolution)
  // TODO: consider if there's a better approach
  const auto& channel_1_raw_waveforms = raw_waveform_map.at(
    ChannelKey(subdetector::ADC, 1));

  size_t num_minibuffers = minibuffer_labels.size();
  if ( num_minibuffers != channel_1_raw_waveforms.size() ) {
    Log("Error: Mismatch between the number of minibuffer labels and the"
      " number of minibuffers", 0, verbosity_);
    return false;
  }

  // Build the vector of beam statuses (with one for each minibuffer)
  std::vector<BeamStatus> beam_statuses;

  for (size_t mb = 0; mb < num_minibuffers; ++mb) {
    const auto& waveform = channel_1_raw_waveforms.at(mb);
    uint64_t ns_since_epoch = waveform.GetStartTime().GetNs();

    const auto& mb_label = minibuffer_labels.at(mb);

    beam_statuses.push_back( get_beam_status(ns_since_epoch, mb_label) );
  }

  annie_event->Set("BeamStatuses", beam_statuses);

  return true;
}


bool BeamChecker::Finalise() {

  return true;
}

bool BeamChecker::initialise_beam_db() {

  m_variables.Get("verbose", verbosity_);

  bool got_bad_pot_max = m_variables.Get("BadPOTMax", bad_pot_max_);

  if ( !got_bad_pot_max ) {
    Log("Error: Missing setting for the BadPOTMax configuration file option",
      0, verbosity_);
    return false;
  }

  std::string db_filename;
  bool got_db_filename = m_variables.Get("BeamDBFile", db_filename);

  // Check for problems
  if ( !got_db_filename ) {
    Log("Error: Missing beam database filename in the configuration for the"
      " BeamChecker tool", 0, verbosity_);
    return false;
  }

  // TODO: switch to std::make_unique when our Docker container has a compiler
  // that is C++14 compliant
  beam_db_tfile_ = std::unique_ptr<TFile>(new TFile(db_filename.c_str(),
    "read"));

  if ( beam_db_tfile_->IsZombie() ) {
    Log("Error: Could not open the beam database file \"" + db_filename
     + '\"', 0, verbosity_);
    return false;
  }

  db_tree_ = nullptr;
  beam_db_tfile_->GetObject("BeamData", db_tree_);

  if ( !db_tree_ ) {
    Log("Error: Could not find the BeamData TTree in the beam database"
      " file \"" + db_filename + '\"', 0, verbosity_);
    return false;
  }
  else if ( !db_tree_->GetBranch("beam_data") ) {
    Log("Error: Could not find the \"beam_data\" branch in the BeamData"
      " TTree in the beam database file \"" + db_filename + '\"', 0,
      verbosity_);
    return false;
  }

  beam_db_index_ = nullptr;
  beam_db_tfile_->GetObject("BeamIndex", beam_db_index_);

  if ( !beam_db_index_ ) {
    Log("Error: Could not find the BeamIndex map in the beam database"
      " file \"" + db_filename + '\"', 0, verbosity_);
    return false;
  }
  if ( beam_db_index_->empty() ) {
    Log("Error: The BeamIndex map from the beam database"
      " file \"" + db_filename + "\" is empty", 0, verbosity_);
    return false;
  }

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

  // TODO: consider making the static variables used here into members of
  // the BeamChecker class instead
  static std::map<std::string, std::map<unsigned long long, IFBeamDataPoint> >*
    beam_data = NULL;
  static int current_beam_db_entry = 0;
  static bool got_first_beam_entry = false;

  db_tree_->SetBranchAddress("beam_data", &beam_data);

  if ( !got_first_beam_entry ) {
    db_tree_->GetEntry(current_beam_db_entry);
    got_first_beam_entry = true;
  }

  // The beam database uses timestamps with ms precision
  unsigned long long ms_since_epoch = ns_since_epoch / MILLION;

  Log("Finding beam status information for "
    + make_time_string(ms_since_epoch), 2, verbosity_);

  // Find the beam database entry that contains POT information for the
  // moment of interest
  auto iter = std::find_if(beam_db_index_->cbegin(),
    beam_db_index_->cend(),
    [ms_since_epoch](const std::pair<int, std::pair<unsigned long long,
      unsigned long long> >& pair) -> bool {
      unsigned long long start_ms = pair.second.first;
      unsigned long long end_ms = pair.second.second;
      return ms_since_epoch >= start_ms && ms_since_epoch <= end_ms;
    }
  );

  // If a suitable entry could not be found, then complain and return
  // a BeamStatus object that indicates that the data were missing
  bool found_pot_entry = ( iter != beam_db_index_->cend() );

  if ( !found_pot_entry ) {
    Log("WARNING: unable to find a suitable entry for "
      + make_time_string(ms_since_epoch) + " (" + std::to_string(ms_since_epoch)
      + " ms since the Unix epoch) in the beam database file", 0, verbosity_);
    return BeamStatus( TimeClass(ns_since_epoch), 0., BeamCondition::Missing );
  }

  // If we need to load a new entry from the beam database TTree, do so.
  // Avoid loading a new entry if you don't have to (the maps stored in
  // each entry are fairly large)
  int new_entry_number = iter->first;
  if ( new_entry_number != current_beam_db_entry ) {
    db_tree_->GetEntry(new_entry_number);
    current_beam_db_entry = new_entry_number;
  }

  // Temporary storage for this function's return value
  BeamStatus beam_status;

  // Find the POT value for the current event
  try {

    // TODO: remove hard-coded device name here
    // Get protons-on-target (POT) information from the parsed data
    const std::map<unsigned long long, IFBeamDataPoint>& pot_map
      = beam_data->at("E:TOR875");

    // Find the POT entry with the closest time to that requested by the
    // user, and use it to create the BeamStatus object that will be returned
    std::map<unsigned long long, IFBeamDataPoint>::const_iterator
      low = pot_map.lower_bound(ms_since_epoch);

    if ( low == pot_map.cend() ) {

      Log("WARNING: IF beam database did not have any information"
        " for " + std::to_string(ms_since_epoch) + " ms after the Unix epoch ("
        + make_time_string(ms_since_epoch) + ')', 0, verbosity_);

      return BeamStatus( TimeClass(ns_since_epoch), 0.,
        BeamCondition::Missing );
    }

    else if ( low == pot_map.cbegin() ) {
      beam_status = BeamStatus(TimeClass( low->first * MILLION ),
        low->second.value, BeamCondition::Ok);
    }

    // We're between two time values, so we need to figure out which is
    // closest to the value requested by the user
    else {

      std::map<unsigned long long, IFBeamDataPoint>::const_iterator
        prev = low;
      --prev;

      if ((ms_since_epoch - prev->first) < (low->first - ms_since_epoch)) {
        beam_status = BeamStatus(TimeClass( prev->first * MILLION ),
          prev->second.value, BeamCondition::Ok);
      }
      else {
        beam_status = BeamStatus(TimeClass( low->first * MILLION ),
          low->second.value, BeamCondition::Ok);
      }
    }

  }

  catch (const std::exception& e) {
    Log("WARNING: problem encountered while querying IF beam"
      " database: " + std::string( e.what() ), 0, verbosity_);

    // There was a problem, so create a BeamStatus object that assumes that
    // the data were missing for some reason.
    return BeamStatus( TimeClass(ns_since_epoch), 0., BeamCondition::Missing );
  }

  // TODO: add more checks of the BeamStatus object before returning it
  // (and change its condition to BeamCondition::Bad) if it fails one of those
  // checks
  if ( beam_status.pot() <= bad_pot_max_ )
    beam_status.set_condition( BeamCondition::Bad );

  return beam_status;
}
