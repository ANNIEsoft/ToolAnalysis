// ToolAnalysis includes
#include "BeamChecker.h"

BeamChecker::BeamChecker() : Tool() {}

bool BeamChecker::Initialise(std::string config_filename, DataModel& data)
{
  // Load configuration file variables
  if ( !config_filename.empty() ) m_variables.Initialise(config_filename);

  // Assign a transient data pointer
  m_data = &data;

  return initialize_beam_db();
}


bool BeamChecker::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool BeamChecker::Execute() {

  int verbosity;
  m_variables.Get("verbose", verbosity);

  // Get a pointer to the ANNIEEvent Store
  auto* annie_event = m_data->Stores["ANNIEEvent"];

  if (!annie_event) {
    Log("Error: The BeamChecker tool could not find the ANNIEEvent Store", 0,
      verbosity);
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
      verbosity);
    return false;
  }
  else if ( raw_waveform_map.empty() ) {
    Log("Error: The BeamChecker tool found an empty RawADCData entry", 0,
      verbosity);
    return false;
  }

  // Load the minibuffer labels for the current event
  std::vector<MinibufferLabel> minibuffer_labels;

  bool got_mb_labels = annie_event->Get("MinibufferLabels", minibuffer_labels);

  // Check for problems
  if ( !got_mb_labels ) {
    Log("Error: The BeamChecker tool could not find the minibuffer labels", 0,
      verbosity);
    return false;
  }
  else if ( minibuffer_labels.empty() ) {
    Log("Error: The BeamChecker tool found an empty vector of minibuffer"
      " labels", 0, verbosity);
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
      " number of minibuffers", 0, verbosity);
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

bool BeamChecker::initialize_beam_db() {
  std::string db_filename;
  bool got_db_filename = m_variables.Get("BeamDBFile", db_filename);

  // Check for problems
  if ( !got_db_data ) {
    Log("Error: Missing beam database filename in the configuration for the"
      " BeamChecker tool", 0, verbosity);
    return false;
  }

  // TODO: switch to std::make_unique when our Docker container has a compiler
  // that is C++14 compliant
  beam_db_file_ = std::unique_ptr<TFile>(new TFile(db_filename.c_str(),
    "read"));

  if ( beam_db_file_->IsZombie() ) {
    Log("Error: Could not open the beam database file \"" + db_filename
     + '\"', 0 verbosity);
    return false;
  }

  db_tree_ = nullptr;
  beam_db_file_->GetObject("BeamData", db_tree);

  if ( !db_tree_ ) {
    Log("Error: Could not find the BeamData TTree in the beam database"
      " file \"" + db_filename + '\"', 0 verbosity);
    return false;
  }

  return true;
}

BeamStatus BeamChecker::get_beam_status(uint64_t ns_since_epoch,
  MinibufferLabel mb_label)
{
}

void build_beam_db_index() {

  std::string beam_data_filename = "/annie/app/users/gardiner/"
    "sorted_phase1_pot_maps.root";

  TFile beam_db_file_(beam_data_filename.c_str(), "read");
  TTree* db_tree_;
  beam_db_file_.GetObject("BeamData", db_tree_);


  std::map<std::string, std::map<uint64_t, IFBeamDataPoint> >*
    beam_data = NULL;
  TBranch* beam_branch = beam_tree->GetBranch("beam_data");
  beam_branch->SetAddress(&beam_data);

  int beam_branch_entries = beam_branch->GetEntries();

  // Build an index for the beam branch to avoid lengthy searches later.
  // Keys are entry numbers, values are start and end times for each
  // entry (in ms since the Unix epoch).
  std::cout << "Building beam database index\n";

  std::map<int, std::pair<uint64_t, uint64_t> >
    beam_index;
  for (int l = 0; l < beam_branch_entries; ++l) {
    beam_branch->GetEntry(l);

    // TODO: remove hard-coded device name
    const std::map<uint64_t, IFBeamDataPoint>& pot_map
      = beam_data->at("E:TOR875");

    std::cout << "Read new beam entry " << l << " of "
      << beam_branch_entries;

    uint64_t starting_ms = pot_map.begin()->first;
    std::string begin_time_string = make_time_string(starting_ms);
    uint64_t ending_ms = (--pot_map.end())->first;
    std::string end_time_string = make_time_string(ending_ms);

    std::cout << " (" << begin_time_string << " to "
      << end_time_string << ")\n";

    beam_index[l] = std::pair<uint64_t, uint64_t>(
      starting_ms, ending_ms);
  }


}

// TRGPOT
#include <ctime>
#include <map>

#include "TBranch.h"
#include "TFile.h"
#include "TTree.h"

#include "BeamStatus.hh"
#include "Event.hh"
#include "IFBeamDataPoint.hh"

namespace {
  constexpr uint64_t FIVE_SECONDS = 5000ull; // ms
  
  // Used to convert from seconds and nanoseconds to milliseconds below
  constexpr uint64_t THOUSAND = 1000ull;
  constexpr uint64_t MILLION = 1000000ull;
}

std::string make_time_string(uint64_t ms_since_epoch) {
  time_t s_since_epoch = ms_since_epoch / THOUSAND;
  std::string time_string = asctime(gmtime(&s_since_epoch));
  time_string.pop_back(); // remove the trailing \n from the time string
  return time_string;
}

void trgpot(TChain* ch, const std::string& output_filename)
{
  Trigger* trig = NULL;
  ch->SetBranchAddress("Trig", &trig);

  std::map<int, std::pair<unsigned long long, unsigned long long> >*
    beam_index = nullptr;
  beam_file.GetObject("BeamIndex", beam_index);

  // Use this copy since /annie/app is mounted for grid jobs but
  // /annie/data is not
  TString beam_data_filename = "/annie/app/users/gardiner/sorted_phase1_pot_maps.root";

  TFile beam_file(beam_data_filename, "read");
  TTree* beam_tree;
  beam_file.GetObject("BeamData", beam_tree);
  std::map<std::string, std::map<uint64_t, IFBeamDataPoint> >*
    beam_data = NULL;
  TBranch* beam_branch = beam_tree->GetBranch("beam_data");
  beam_branch->SetAddress(&beam_data);

  int beam_branch_entries = beam_branch->GetEntries();

  // Build an index for the beam branch to avoid lengthy searches later.
  // Keys are entry numbers, values are start and end times for each
  // entry (in ms since the Unix epoch).
  std::cout << "Building beam database index\n";

  std::map<int, std::pair<uint64_t, uint64_t> >
    beam_index;
  for (int l = 0; l < beam_branch_entries; ++l) {
    beam_branch->GetEntry(l);

    // TODO: remove hard-coded device name
    const std::map<uint64_t, IFBeamDataPoint>& pot_map
      = beam_data->at("E:TOR875");

    std::cout << "Read new beam entry " << l << " of "
      << beam_branch_entries;

    uint64_t starting_ms = pot_map.begin()->first;
    std::string begin_time_string = make_time_string(starting_ms);
    uint64_t ending_ms = (--pot_map.end())->first;
    std::string end_time_string = make_time_string(ending_ms);

    std::cout << " (" << begin_time_string << " to "
      << end_time_string << ")\n";

    beam_index[l] = std::pair<uint64_t, uint64_t>(
      starting_ms, ending_ms);
  }

  TFile out_file(output_filename.c_str(), "recreate");
  TTree* out_tree = new TTree("pot_tree", "Protons on target data");

  BeamStatus beam_status;
  BeamStatus* bs_ptr = &beam_status;
  out_tree->Branch("beam_status", "BeamStatus", &bs_ptr);

  int chain_entry = 0;
  out_tree->Branch("chain_entry", &chain_entry, "chain_entry/I");

  int trigger_num = 0;
  out_tree->Branch("trigger_num", &trigger_num, "trigger_num/I");

  int trigger_time_sec = 0;
  out_tree->Branch("trigger_time_sec", &trigger_time_sec, "trigger_time_sec/I");

  int beam_entry = -1;

  bool need_new_beam_data = true;

  int num_entries = ch->GetEntries();

  // Use our signal handler function to handle SIGINT signals (e.g., the
  // user pressing ctrl+C)
  std::signal(SIGINT, signal_handler);

  for (int i = 0; i < num_entries; ++i) {
    if (interrupted) break;
    ch->GetEntry(i);
    std::cout << "Retrieved trigger entry " << i << " of "
      << num_entries << " ( run " << trig->RunNumber << ", subrun = "
      << trig->SubrunNumber << ", file = " << trig->FileNumber << ")\n";

    trigger_time_sec = trig->TriggerTimeSec[0];
    chain_entry = i;
    trigger_num = trig->GetTrigNo();

    // Use the first card's trigger time to get the milliseconds since the Unix
    // epoch for the trigger corresponding to the current event
    uint64_t ms_since_epoch = (trigger_time_sec * THOUSAND)
      + (trig->TriggerTimeNSec[0] / MILLION);

    std::cout << "Finding beam status information for "
      << make_time_string(ms_since_epoch) << '\n';

    // If we don't have any times stored in the beam data that would come close
    // to matching this trigger, then ask the beam database for a new data set.
    bool looped_once = false;
    bool found_ok = true;
    int old_beam_entry = beam_entry;
    do {
      if (need_new_beam_data) {

	// The files in the chain will not necessarily be in time order, so
	// loop back to the beginning of the beam index if we reach the
	// end without finding the correct time
        ++beam_entry;
        if (beam_entry >= beam_branch_entries) {

          if (looped_once) {
            std::cerr << "ERROR: looped twice without finding a suitable beam"
              << " map entry for " << ms_since_epoch << " ms since the Unix"
              << " epoch (" << make_time_string(ms_since_epoch) << ")\n";
            found_ok = false;
            break;
          }

          looped_once = true;
          beam_entry = 0;
        }
      }
      const auto& ms_range_pair = beam_index.at(beam_entry);
      uint64_t start_ms = ms_range_pair.first;
      uint64_t end_ms = ms_range_pair.second;
      need_new_beam_data = (ms_since_epoch < start_ms)
        || (ms_since_epoch + FIVE_SECONDS > end_ms);
    } while (need_new_beam_data);

    // Find the POT value for the current event
    try {

      if (!found_ok) throw std::runtime_error("Unable to find"
        " a suitable entry in the beam database");

      if (old_beam_entry != beam_entry) {
        beam_branch->GetEntry(beam_entry);
        std::cout << "Loaded beam database entry " << beam_entry << '\n';
      }

      // TODO: remove hard-coded device name here
      // Get protons-on-target (POT) information from the parsed data
      const std::map<uint64_t, IFBeamDataPoint>& pot_map
        = beam_data->at("E:TOR875");

      // Find the POT entry with the closest time to that requested by the
      // user, and use it to create the BeamStatus object that will be returned
      std::map<uint64_t, IFBeamDataPoint>::const_iterator
        low = pot_map.lower_bound(ms_since_epoch);

      if (low == pot_map.cend()) {

        std::cerr << "WARNING: IF beam database did not have any information"
          << " for " << ms_since_epoch << " ms after the Unix epoch ("
          << make_time_string(ms_since_epoch) << ")\n";

        beam_status = BeamStatus();
      }

      else if (low == pot_map.cbegin()) {
        beam_status = BeamStatus(low->first, low->second.value);
      }

      // We're between two time values, so we need to figure out which is
      // closest to the value requested by the user
      else {

        std::map<uint64_t, IFBeamDataPoint>::const_iterator
          prev = low;
        --prev;

        if ((ms_since_epoch - prev->first) < (low->first - ms_since_epoch)) {
          beam_status = BeamStatus(prev->first, prev->second.value);
        }
        else {
          beam_status = BeamStatus(low->first, low->second.value);
        }
      }

    }

    catch (const std::exception& e) {
      std::cerr << "WARNING: problem encountered while querying IF beam"
        " database:\n  " << e.what() << '\n';

      // Use a default-constructed BeamStatus object since there was a problem.
      // The fOk member will be set to false by default, which will flag the
      // object as problematic.
      beam_status = BeamStatus();
    }

    out_tree->Fill();
  }

  out_file.cd();
  out_tree->Write();

  beam_file.Close();
  out_file.Close();
}
