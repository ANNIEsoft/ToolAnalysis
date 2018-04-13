// standard library includes
#include <fstream>

// ToolAnalysis includes
#include "LoadANNIEEvent.h"

LoadANNIEEvent::LoadANNIEEvent():Tool() {}

bool LoadANNIEEvent::Initialise(std::string config_filename, DataModel &data) {

  // Load settings from the configuration file
  if ( !config_filename.empty() ) m_variables.Initialise(config_filename);

  // Assign transient data pointer
  m_data= &data;

  m_variables.Get("verbose", verbosity_);

  std::string input_list_filename;
  bool got_input_file_list = m_variables.Get("FileForListOfInputs",
    input_list_filename);

  if ( !got_input_file_list ) {
    Log("Error: Missing input list file in the configuration for the"
      " LoadANNIEEvent tool", 0, verbosity_);
    return false;
  }

  std::ifstream list_file(input_list_filename);
  if ( !list_file.good() ) {
    Log("Error: Could not open the input list file for the LoadANNIEEvent tool",
      0, verbosity_);
    return false;
  }

  std::string temp_str;
  while ( list_file >> temp_str ) input_filenames_.push_back( temp_str );

  current_entry_ = 0u;
  current_file_ = 0u;
  need_new_file_ = true;
 
  return true;
}


bool LoadANNIEEvent::Execute() {

  int stop_the_loop = -1;
  m_data->vars.Get("StopLoop", stop_the_loop);
  if ( stop_the_loop == 1 ) return false;

  if (need_new_file_) {

    // Delete the old ANNIEEvent Store if there is one
    if ( m_data->Stores.count("ANNIEEvent") ) {
      auto* annie_event = m_data->Stores.at("ANNIEEvent");
      if (annie_event) delete annie_event;
    }

    // Create a new ANNIEEvent Store
    m_data->Stores["ANNIEEvent"] = new BoostStore(false,
      BOOST_STORE_MULTIEVENT_FORMAT);

    // Load it from the new input file
    std::string input_filename = input_filenames_.at(current_file_);
    m_data->Stores["ANNIEEvent"]->Initialise(input_filename);
    m_data->Stores["ANNIEEvent"]->Header->Get("TotalEntries",
      total_entries_in_file_);
  }


  Log("Loading entry " + std::to_string(current_entry_) + " from the"
    " ANNIEEvent input file \"" + input_filenames_.at(current_file_)
    + '\"', 1, verbosity_);
 
  m_data->Stores["ANNIEEvent"]->GetEntry(current_entry_);  
  ++current_entry_;
  
  if ( current_entry_ >= total_entries_in_file_ ) {
    if ( current_file_ >= input_filenames_.size() ) {
      m_data->vars.Set("StopLoop", 1);
    }
    else {
      ++current_file_;
      current_entry_ = 0u;
      need_new_file_ = true;
    }
  }

  return true;
}


bool LoadANNIEEvent::Finalise() {
  return true;
}
