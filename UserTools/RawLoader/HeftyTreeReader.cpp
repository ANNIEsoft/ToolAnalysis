// standard library includes
#include <stdexcept>
#include "HeftyTreeReader.h"

annie::HeftyTreeReader::HeftyTreeReader(const std::string& file_name)
  : HeftyTreeReader(std::vector<std::string>( { file_name } ))
{
}

annie::HeftyTreeReader::HeftyTreeReader(
  const std::vector<std::string>& file_names) : hefty_db_chain_("heftydb")
{
  for (const auto& file_name : file_names) {
    hefty_db_chain_.Add( file_name.c_str() );
  }

  set_branch_addresses();
}

annie::HeftyTreeReader::HeftyTreeReader(TChain* inputChain) : hefty_db_chain_("heftydb")
{
  hefty_db_chain_.Add(inputChain);
  set_branch_addresses();
}

void annie::HeftyTreeReader::set_branch_addresses() {
  hefty_db_chain_.SetBranchAddress("SequenceID", &br_SequenceID_);
  hefty_db_chain_.SetBranchAddress("Time", br_Time_.data());
  hefty_db_chain_.SetBranchAddress("Label", br_Label_.data());
  hefty_db_chain_.SetBranchAddress("TSinceBeam", br_TSinceBeam_.data());
  hefty_db_chain_.SetBranchAddress("More", br_More_.data());
}

std::unique_ptr<HeftyInfo> annie::HeftyTreeReader::next() {
  return load_next_entry(false);
}

std::unique_ptr<HeftyInfo> annie::HeftyTreeReader::previous() {
  return load_next_entry(true);
}

std::unique_ptr<HeftyInfo> annie::HeftyTreeReader::load_next_entry(
  bool reverse)
{
  int step = 1;
  if (reverse) {
    step = -1;
    if (current_hefty_db_entry_ <= 0) {
      return nullptr;
    }
    else {
      --current_hefty_db_entry_;
    }
  }

  // TChain::LoadTree returns the entry number that should be used with
  // the current TTree object, which (together with the TBranch objects
  // that it owns) doesn't know about the other TTrees in the TChain.
  // If the return value is negative, there was an I/O error, or we've
  // attempted to read past the end of the TChain.
  current_hefty_db_entry_ += step;
  int local_entry = hefty_db_chain_.LoadTree(current_hefty_db_entry_);
  if (local_entry < 0) {
    // If we've reached the end of the TChain (or encountered an I/O error)
    // return a nullptr.
    return nullptr;
    // TODO: consider throwing an exception here instead
  }

  hefty_db_chain_.GetEntry(current_hefty_db_entry_);

  // TODO: Switch to using std::make_unique<HeftyInfo>();
  // when our Docker image has C++14 support
  auto hefty_info = std::unique_ptr<HeftyInfo>(new HeftyInfo(br_SequenceID_,
    br_Time_, br_Label_, br_TSinceBeam_, br_More_));

  // Remember the SequenceID of the last entry to be successfully loaded
  last_sequence_id_ = hefty_info->sequence_id();

  return hefty_info;
}
