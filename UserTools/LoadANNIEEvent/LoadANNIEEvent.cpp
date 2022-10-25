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
  offset_evnum = 0;
  run_on_filtered=0;

  m_variables.Get("verbose", verbosity_);
  m_variables.Get("EventOffset", offset_evnum);
  m_variables.Get("GlobalEvNr",global_evnr);
  m_variables.Get("RunOnFiltered",run_on_filtered);

  global_ev = offset_evnum;

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

  m_data->CStore.Set("UserEvent",false);

  current_entry_ += offset_evnum;
  if (offset_evnum != 0)  {
    m_data->CStore.Set("UserEvent",true);
    m_data->CStore.Set("LoadEvNr",offset_evnum);
  }


  return true;
}


bool LoadANNIEEvent::Execute() {

  int stop_the_loop = -1;
  m_data->vars.Get("StopLoop", stop_the_loop);
  if ( stop_the_loop == 1 ) return false;

  if (input_filenames_.size()==0) m_data->vars.Set("StopLoop",1);

  if (need_new_file_) {
    need_new_file_=false;

    // Delete the old ANNIEEvent Store if there is one
    if ( m_data->Stores.count("ANNIEEvent") ) {
      auto* annie_event = m_data->Stores.at("ANNIEEvent");
      if (annie_event) delete annie_event;
    }
/*
    if (m_data->Stores.count("ANNIEEvent")){
    m_data->Stores["ANNIEEvent"]->Close();
    }*/

    // Create a new ANNIEEvent Store
    m_data->Stores["ANNIEEvent"] = new BoostStore(false,
      BOOST_STORE_MULTIEVENT_FORMAT);

    // Load it from the new input file
    std::string input_filename = input_filenames_.at(current_file_);
    std::cout <<"Reading in current file "<<current_file_<<std::endl;
    m_data->Stores["ANNIEEvent"]->Initialise(input_filename);
    m_data->Stores["ANNIEEvent"]->Header->Get("TotalEntries",
      total_entries_in_file_);
    if (current_file_==0) {
      global_events.push_back(total_entries_in_file_);
      global_events_start.push_back(0);
    }
    else {
      global_events.push_back(global_events.at(current_file_-1)+total_entries_in_file_);
      global_events_start.push_back(global_events.at(current_file_-1));
    }
  }

   bool user_event=false;
   m_data->CStore.Get("UserEvent",user_event);
   if (user_event){
     m_data->CStore.Set("UserEvent",false);
     int user_evnum;
     m_data->CStore.Get("LoadEvNr",user_evnum);
     if (!global_evnr){
       if (user_evnum < total_entries_in_file_ && user_evnum >=0) current_entry_ = user_evnum;
     } else {
       if (user_evnum >= global_events_start.at(current_file_) && user_evnum < global_events.at(current_file_)){
         current_entry_ = user_evnum-global_events_start.at(current_file_);
         global_ev = user_evnum;
       } else {
         while (current_file_ < input_filenames_.size()){
           ++current_file_;
           if ( current_file_ >= input_filenames_.size() ) {
             m_data->vars.Set("StopLoop", 1);
           }
           else {
             current_entry_ = 0u;
             if ( m_data->Stores.count("ANNIEEvent") ) {
               auto* annie_event = m_data->Stores.at("ANNIEEvent");
               if (annie_event) delete annie_event;
               m_data->Stores["ANNIEEvent"] = new BoostStore(false,
                 BOOST_STORE_MULTIEVENT_FORMAT);
               std::string input_filename = input_filenames_.at(current_file_);
               std::cout <<"Reading in current file "<<current_file_<<std::endl;
               m_data->Stores["ANNIEEvent"]->Initialise(input_filename);
               m_data->Stores["ANNIEEvent"]->Header->Get("TotalEntries",
                 total_entries_in_file_);
               global_events.push_back(global_events.at(current_file_-1)+total_entries_in_file_);
               global_events_start.push_back(global_events.at(current_file_-1));
               if (user_evnum >= global_events_start.at(current_file_) && user_evnum < global_events.at(current_file_)){
                 current_entry_ = user_evnum-global_events_start.at(current_file_);
                 global_ev = user_evnum;
                 break;
               }
             }
           }
         }
       }
     }
   }

  Log("ANNIEEvent store has "+std::to_string(total_entries_in_file_)+" entries",v_debug,verbosity_);
  Log("Loading entry " + std::to_string(current_entry_) + " from the"
    " ANNIEEvent input file \"" + input_filenames_.at(current_file_)
    + '\"', 1, verbosity_);
 
  if (current_entry_ != offset_evnum) m_data->Stores["ANNIEEvent"]->Delete();	//ensures that we can access pointers without problems

  m_data->Stores["ANNIEEvent"]->GetEntry(current_entry_);  
  if (!run_on_filtered) m_data->Stores["ANNIEEvent"]->Set("LocalEventNumber",current_entry_);
  ++current_entry_;
 
  if (global_evnr && !run_on_filtered) m_data->Stores["ANNIEEvent"]->Set("EventNumber",global_ev);
  global_ev++; 



  if ( current_entry_ >= total_entries_in_file_ ) {
    ++current_file_;
    if ( current_file_ >= input_filenames_.size() ) {
      m_data->vars.Set("StopLoop", 1);
    }
    else {
      current_entry_ = 0u;
      need_new_file_ = true;
    }
  }


  return true;
}


bool LoadANNIEEvent::Finalise() {
  return true;
}
