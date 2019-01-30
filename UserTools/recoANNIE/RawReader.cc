// standard library includes
#include <stdexcept>

// reco-annie includes
#include "Constants.h"
#include "RawReader.h"

// anonymous namespace for definitions local to this source file
namespace {
  // Converts the value from the Eventsize branch to the minibuffer size (in
  // samples)
  constexpr int EVENT_SIZE_TO_MINIBUFFER_SIZE = 4;
}

annie::RawReader::RawReader(const std::string& file_name)
  : RawReader(std::vector<std::string>( { file_name } ))
{
}

annie::RawReader::RawReader(const std::vector<std::string>& file_names)
  : pmt_data_chain_("PMTData"), trig_data_chain_("TrigData"),
  current_pmt_data_entry_(0), current_trig_data_entry_(-1)
{
  for (const auto& file_name : file_names) {
    pmt_data_chain_.Add( file_name.c_str() );
    trig_data_chain_.Add( file_name.c_str() );
  }

  set_branch_addresses();
}

void annie::RawReader::set_branch_addresses() {
  // Set PMTData branch addresses
  pmt_data_chain_.SetBranchAddress("LastSync", &br_LastSync_);
  pmt_data_chain_.SetBranchAddress("SequenceID", &br_SequenceID_);
  pmt_data_chain_.SetBranchAddress("StartTimeSec", &br_StartTimeSec_);
  pmt_data_chain_.SetBranchAddress("StartTimeNSec", &br_StartTimeNSec_);
  pmt_data_chain_.SetBranchAddress("StartCount", &br_StartCount_);
  pmt_data_chain_.SetBranchAddress("TriggerNumber", &br_TriggerNumber_);
  pmt_data_chain_.SetBranchAddress("CardID", &br_CardID_);
  pmt_data_chain_.SetBranchAddress("Channels", &br_Channels_);
  pmt_data_chain_.SetBranchAddress("BufferSize", &br_BufferSize_);
  pmt_data_chain_.SetBranchAddress("FullBufferSize", &br_FullBufferSize_);
  pmt_data_chain_.SetBranchAddress("Eventsize", &br_EventSize_);

  // Set TrigData branch addresses
  trig_data_chain_.SetBranchAddress("FirmwareVersion", &br_FirmwareVersion_);
  trig_data_chain_.SetBranchAddress("SequenceID", &br_TrigData_SequenceID_);
  trig_data_chain_.SetBranchAddress("EventSize", &br_TrigData_EventSize_);
  trig_data_chain_.SetBranchAddress("TriggerSize", &br_TriggerSize_);
  trig_data_chain_.SetBranchAddress("FIFOOverflow", &br_FIFOOverflow_);
  // Typo exists in the branch name definition in the DAQ software
  trig_data_chain_.SetBranchAddress("DriverOverfow", &br_DriverOverflow_);
}

std::unique_ptr<annie::RawReadout> annie::RawReader::next() {
  return load_next_entry(false);
}

std::unique_ptr<annie::RawReadout> annie::RawReader::previous() {
  return load_next_entry(true);
}

std::unique_ptr<annie::RawReadout> annie::RawReader::load_next_entry(
  bool reverse)
{
  int step = 1;
  if (reverse) {
    step = -1;
    if (current_pmt_data_entry_ <= 0 || current_trig_data_entry_ <= 0) {
      return nullptr;
    }
    else {
      --current_pmt_data_entry_;
    }
  }

  // TODO: Switch to using std::make_unique<annie::RawReadout>();
  // when our Docker image has C++14 support
  auto raw_readout = std::unique_ptr<annie::RawReadout>(new annie::RawReadout);

  int first_sequence_id = BOGUS_INT;
  bool loaded_first_card = false;

  ///// Load data from the PMTData tree /////
  // Loop indefinitely until the SequenceID changes (we've finished
  // loading a full DAQ readout) or we run out of TChain entries.
  while (true) {
    // TChain::LoadTree returns the entry number that should be used with
    // the current TTree object, which (together with the TBranch objects
    // that it owns) doesn't know about the other TTrees in the TChain.
    // If the return value is negative, there was an I/O error, or we've
    // attempted to read past the end of the TChain.
    int local_entry = pmt_data_chain_.LoadTree(current_pmt_data_entry_);
    if (local_entry < 0) {
      // If we've reached the end of the TChain (or encountered an I/O error)
      // without loading data from any of the VME cards, return a nullptr.
      if (!loaded_first_card) return nullptr;
      // If we've loaded at least one card, exit the loop, which will allow
      // this function to return the completed RawReadout object (which was
      // possibly truncated by an unexpected end-of-file)
      else break;
    }

    // Load all of the branches except for the variable-length arrays, which
    // we handle separately below using the sizes obtained from this call
    // to TChain::GetEntry().
    pmt_data_chain_.GetEntry(current_pmt_data_entry_);

    // Continue iterating over the tree until we find a readout other
    // than the one that was last loaded
    if (br_SequenceID_ == last_sequence_id_) {
      current_pmt_data_entry_ += step;
      continue;
    }

    // Check that the variable-length array sizes are nonnegative. If one
    // of them is negative, complain.
    if (br_FullBufferSize_ < 0) throw std::runtime_error("Negative"
      " FullBufferSize value encountered in annie::RawReader::next()");
    else if (br_TriggerNumber_ < 0) throw std::runtime_error("Negative"
      " TriggerNumber value encountered in annie::RawReader::next()");
    else if (br_Channels_ < 0) throw std::runtime_error("Negative"
      " Channels value encountered in annie::RawReader::next()");

    // Check the variable-length array sizes and adjust the vector dimensions
    // as needed before loading the corresponding branches.
    size_t fbs_temp = static_cast<size_t>(br_FullBufferSize_);
    if ( br_Data_.size() != fbs_temp) br_Data_.resize(fbs_temp);

    size_t tn_temp = static_cast<size_t>(br_TriggerNumber_);
    if ( br_TriggerCounts_.size() != tn_temp) br_TriggerCounts_.resize(tn_temp);

    size_t cs_temp = static_cast<size_t>(br_Channels_);
    if ( br_Rates_.size() != cs_temp) br_Rates_.resize(cs_temp);

    // Load the variable-length arrays from the current entry. The C++ standard
    // guarantees that std::vector elements are stored contiguously in memory
    // (something that is not true of std::deque elements), so we can use
    // a pointer to the first element of each vector as each branch address.
    TTree* temp_tree = pmt_data_chain_.GetTree();
    temp_tree->SetBranchAddress("Data", br_Data_.data());
    temp_tree->SetBranchAddress("TriggerCounts", br_TriggerCounts_.data());
    temp_tree->SetBranchAddress("Rates", br_Rates_.data());

    // Memory corruption can occur if we call GetEntry with a zero-length
    // branch enabled. We've already resized the vectors to zero above,
    // so just disable their branches for the zero-length case here as needed.
    if (fbs_temp == 0) temp_tree->SetBranchStatus("Data", false);
    else temp_tree->SetBranchStatus("Data", true);

    if (tn_temp == 0) temp_tree->SetBranchStatus("TriggerCounts", false);
    else temp_tree->SetBranchStatus("TriggerCounts", true);

    if (cs_temp == 0) temp_tree->SetBranchStatus("Rates", false);
    else temp_tree->SetBranchStatus("Rates", true);

    temp_tree->GetEntry(local_entry);

    // If this is the first card to be loaded, store its SequenceID for
    // reference.
    if (!loaded_first_card) {
      first_sequence_id = br_SequenceID_;
      loaded_first_card = true;
      raw_readout->set_sequence_id(first_sequence_id);
    }
    // When we encounter a new SequenceID value, we've finished loading a full
    // readout and can exit the loop.
    else if (first_sequence_id != br_SequenceID_) break;

    // Add the current card to the incomplete RawReadout object
    raw_readout->add_card(br_CardID_, br_LastSync_, br_StartTimeSec_,
      br_StartTimeNSec_, br_StartCount_, br_Channels_,
      br_BufferSize_, br_EventSize_ * EVENT_SIZE_TO_MINIBUFFER_SIZE,
      br_Data_, br_TriggerCounts_, br_Rates_);

    // Move on to the next TChain entry
    current_pmt_data_entry_ += step;
  }

  // TODO: Re-activate reading from the TrigData tree when you can properly
  // deal with ROOT's memory leak when setting the branch addresses for the
  // variable-length arrays below.

  /////// Load data from the TrigData tree /////
  /////
  //// TChain::LoadTree returns the entry number that should be used with
  //// the current TTree object, which (together with the TBranch objects
  //// that it owns) doesn't know about the other TTrees in the TChain.
  //// If the return value is negative, there was an I/O error, or we've
  //// attempted to read past the end of the TChain.
  //current_trig_data_entry_ += step;
  //int local_entry = trig_data_chain_.LoadTree(current_trig_data_entry_);
  //if (local_entry < 0) {
  //  // If we've reached the end of the TChain (or encountered an I/O error)
  //  // return a nullptr.
  //  return nullptr;
  //  // TODO: consider throwing an exception here instead
  //}

  //// Load all of the branches except for the variable-length arrays, which
  //// we handle separately below using the sizes obtained from this call
  //// to TChain::GetEntry().
  //trig_data_chain_.GetEntry(current_trig_data_entry_);

  //// Check that the variable-length array sizes are nonnegative. If one
  //// of them is negative, complain.
  //if (br_TrigData_EventSize_ < 0) throw std::runtime_error("Negative"
  //  " EventSize value encountered in annie::RawReader::next()"
  //  " while reading from the TrigData TTree");
  //if (br_TriggerSize_ < 0) throw std::runtime_error("Negative"
  //  " TriggerSize value encountered in annie::RawReader::next()"
  //  " while reading from the TrigData TTree");

  //// Check the variable-length array sizes and adjust the vector dimensions
  //// as needed before loading the corresponding branches.
  //size_t es_temp = static_cast<size_t>(br_TrigData_EventSize_);
  //if ( br_EventIDs_.size() != es_temp) br_EventIDs_.resize(es_temp);
  //if ( br_EventTimes_.size() != es_temp) br_EventTimes_.resize(es_temp);

  //size_t ts_temp = static_cast<size_t>(br_TriggerSize_);
  //if ( br_TriggerMasks_.size() != ts_temp) br_TriggerMasks_.resize(ts_temp);
  //if ( br_TriggerCounters_.size() != ts_temp)
  //  br_TriggerCounters_.resize(ts_temp);

  //// Load the variable-length arrays from the current entry. The C++ standard
  //// guarantees that std::vector elements are stored contiguously in memory
  //// (something that is not true of std::deque elements), so we can use
  //// a pointer to the first element of each vector as each branch address.
  //TTree* temp_tree = trig_data_chain_.GetTree();

  ///// BEGIN FATAL ROOT MEMORY LEAK

  //temp_tree->SetBranchAddress("EventIDs", br_EventIDs_.data());
  //temp_tree->SetBranchAddress("EventTimes", br_EventTimes_.data());
  //temp_tree->SetBranchAddress("TriggerMasks", br_TriggerMasks_.data());
  //temp_tree->SetBranchAddress("TriggerCounters", br_TriggerCounters_.data());

  ///// BEGIN FATAL ROOT MEMORY LEAK

  //// Memory corruption can occur if we call GetEntry with a zero-length
  //// branch enabled. We've already resized the vectors to zero above,
  //// so just disable their branches for the zero-length case here as needed.
  //if (es_temp == 0) {
  //  temp_tree->SetBranchStatus("EventIDs", false);
  //  temp_tree->SetBranchStatus("EventTimes", false);
  //}
  //else {
  //  temp_tree->SetBranchStatus("EventIDs", true);
  //  temp_tree->SetBranchStatus("EventTimes", true);
  //}

  //if (ts_temp == 0) {
  //  temp_tree->SetBranchStatus("TriggerMasks", false);
  //  temp_tree->SetBranchStatus("TriggerCounters", false);
  //}
  //else {
  //  temp_tree->SetBranchStatus("TriggerMasks", true);
  //  temp_tree->SetBranchStatus("TriggerCounters", true);
  //}

  //temp_tree->GetEntry(local_entry);

  //// Add the TrigData information to the incomplete RawReadout object
  //raw_readout->set_trig_data( annie::RawTrigData(br_FirmwareVersion_,
  //  br_FIFOOverflow_, br_DriverOverflow_, br_EventIDs_, br_EventTimes_,
  //  br_TriggerMasks_, br_TriggerCounters_) );

  //// Check that the TrigData tree's SequenceID matches that of the PMTData tree
  //// (if not, then we're loading data from two mismatched DAQ readouts!)
  //if ( br_TrigData_SequenceID_ != raw_readout->sequence_id() ) {
  //  std::string warning_message = "Mismatched TrigData ("
  //    + std::to_string(br_TrigData_SequenceID_) + ") and PMTData ("
  //    + std::to_string( raw_readout->sequence_id() ) + ") SequenceID values";

  //  if (throw_on_trig_pmt_sequenceID_mismatch_) {
  //    throw std::runtime_error(warning_message);
  //  }
  //  else {
  //    std::cerr << '\n' << "WARNING: " << warning_message << '\n';
  //  }
  //}

  // Remember the SequenceID of the last raw readout to be successfully loaded
  last_sequence_id_ = raw_readout->sequence_id();

  // Move forward by one on the PMTData TChain if we loaded this readout in
  // reverse (this ensures that we begin loading the next readout from the same
  // place regardless of the preceding direction)
  if (reverse) ++current_pmt_data_entry_;

  // Load the JSON-format strings from the RunInformation TTree.
  // Load the run information tree from the current file
  TFile* temp_file = get_current_file();
  if (!temp_file) throw std::runtime_error("Failed to retrieve"
    " current TFile in RawReader::load_next_entry()");

  TTree* run_info_tree = nullptr;
  temp_file->GetObject("RunInformation", run_info_tree);
  if (!run_info_tree) throw std::runtime_error("Failed to retrieve"
    " RunInformation TTree in RawReader::load_next_entry()");

  // Temporary branch variables used to read the RunInformation tree
  std::string* info_title = nullptr;
  std::string* info_message = nullptr;

  run_info_tree->SetBranchAddress("InfoTitle", &info_title);
  run_info_tree->SetBranchAddress("InfoMessage", &info_message);

  std::map<std::string, std::string> run_info;

  // Load each entry from the RunInformation tree into the map
  for (int ri_entry = 0; ri_entry < run_info_tree->GetEntries(); ++ri_entry) {
    run_info_tree->GetEntry(ri_entry);
    run_info[*info_title] = *info_message;
  }

  raw_readout->set_run_information(run_info);

  return raw_readout;
}


void annie::RawReader::set_throw_on_trig_pmt_sequenceID_mismatch(
  bool should_I_throw)
{
  throw_on_trig_pmt_sequenceID_mismatch_ = should_I_throw;
}
