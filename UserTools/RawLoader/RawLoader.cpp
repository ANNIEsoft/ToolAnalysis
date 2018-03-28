// ToolAnalysis includes
#include "ChannelKey.h"
#include "MinibufferLabel.h"
#include "RawLoader.h"
#include "Waveform.h"

// recoANNIE includes
#include "RawCard.h"
#include "RawChannel.h"
#include "RawReadout.h"

// Boost includes
#include "boost/bimap.hpp"

// standard library includes
#include <map>
#include <string>
#include <vector>

namespace {

  constexpr int BOOST_STORE_MULTIEVENT_FORMAT = 2;

  template <typename L, typename R> boost::bimap<L, R>
    makeBimap(std::initializer_list<
      typename boost::bimap<L, R>::value_type> list)
  {
    return boost::bimap<L, R>(list.begin(), list.end());
  }

  // Phase I definitions for the PMT IDs are hard-coded here.
  //
  // TODO: use the Geometry class from the header (or something else)
  // to get these matchups instead
  const auto pmt_ID_and_card_channel_bimap = makeBimap<int,
    std::pair<int, int> >
  ({
    {1, {3, 0}}, // PMTID 1 corresponds to VME card 3, channel 0
    {2, {3, 1}},
    {3, {3, 2}},
    {4, {3, 3}},
    {5, {4, 0}},
    {6, {4, 1}},
    {7, {4, 2}},
    {8, {4, 3}},
    {9, {5, 0}},
    {10, {5, 1}},
    {11, {5, 2}},
    {12, {5, 3}},
    {13, {6, 0}},
    {14, {6, 1}},
    {15, {6, 2}},
    {16, {6, 3}},
    {17, {8, 0}},
    {18, {8, 1}},
    {19, {8, 2}},
    {20, {8, 3}},
    {21, {9, 0}},
    {22, {9, 1}},
    {23, {9, 2}},
    {24, {9, 3}},
    {25, {10, 0}},
    {26, {10, 1}},
    {27, {10, 2}},
    {28, {10, 3}},
    {29, {11, 0}},
    {30, {11, 1}},
    {31, {11, 2}},
    {32, {11, 3}},
    {33, {13, 0}},
    {34, {13, 1}},
    {35, {13, 2}},
    {36, {13, 3}},
    {37, {14, 0}},
    {38, {14, 1}},
    {39, {14, 2}},
    {40, {14, 3}},
    {41, {15, 0}},
    {42, {15, 1}},
    {43, {15, 2}},
    {44, {15, 3}},
    {45, {16, 0}},
    {46, {16, 1}},
    {47, {16, 2}},
    {48, {16, 3}},
    {49, {18, 0}},
    {50, {18, 1}},
    {51, {18, 2}},
    {52, {18, 3}},
    {53, {19, 0}},
    {54, {19, 1}},
    {55, {19, 2}},
    {56, {19, 3}},
    {57, {20, 0}},
    {58, {20, 1}},
    {59, {20, 2}},
    {60, {20, 3}},
    {61, {21, 0}}, // Non-standard Phase I PMTIDs begin here
    {62, {21, 1}},
    {63, {21, 2}},
    {64, {21, 3}},
  });

}

RawLoader::RawLoader() : Tool()
{}

bool RawLoader::Initialise(const std::string config_file, DataModel& data)
{
  // Load configuration file variables
  if ( !config_file.empty() ) m_variables.Initialise(config_file);

  // Assign transient data pointer
  m_data = &data;

  // TODO: allow for multiple input files in the same run
  std::string input_file_name;
  m_variables.Get("InputFile", input_file_name);

  int verbosity;
  m_variables.Get("verbose", verbosity);

  Log("Opening input file " + input_file_name, 1, verbosity);
  // TODO: Switch to using
  // m_reader = std::make_unique<annie::RawReader>(input_file_name);
  // when our Docker image has C++14 support
  m_reader = std::unique_ptr<annie::RawReader>(
    new annie::RawReader(input_file_name));

  m_data->Stores["ANNIEEvent"] = new BoostStore(false,
    BOOST_STORE_MULTIEVENT_FORMAT);

  return true;
}

bool RawLoader::Execute() {

  int verbosity;
  m_variables.Get("verbose", verbosity);

  // Load the next raw data readout from the input file
  auto raw_readout = m_reader->next();

  // TODO: can we make this a bool?
  // If we've reached the end of the input file, set the stop
  // loop flag and don't do anything else.
  if (!raw_readout) {
    m_data->vars.Set("StopLoop", 1);
    return true;
  }

  static int readout_counter = -1;
  ++readout_counter;

  // Otherwise, place the raw data into the ANNIEEvent in the Store
  auto* annie_event = m_data->Stores["ANNIEEvent"];
  uint32_t event_number = raw_readout->sequence_id();
  annie_event->Set("EventNumber", event_number);

  // Build the ChannelKey -> (raw) Waveform map from the annie::RawReadout
  // object
  std::map<ChannelKey, std::vector<Waveform<unsigned short> > >
    raw_waveform_map;

  size_t num_minibuffers = 0;
  for ( const auto& card_pair : raw_readout->cards() ) {
    const auto& card = card_pair.second;

    for ( const auto& channel_pair : card.channels() ) {
      const auto& channel = channel_pair.second;

      int pmt_id = pmt_ID_and_card_channel_bimap.right.at(
        std::pair<int, int>(card.card_id(), channel.channel_id()) );

      ChannelKey ck(subdetector::ADC, pmt_id);
      std::vector<Waveform<unsigned short> > raw_waveforms;

      num_minibuffers = channel.num_minibuffers();
      // TODO: test for Hefty mode (num_minibuffers > 1) here

      for ( size_t mb = 0; mb < num_minibuffers; ++mb) {
        const auto& minibuffer_data = channel.minibuffer_data( mb );

        // Estimate the time in ns since the Unix epoch for the start of this
        // minibuffer using the standard recoANNIE technique for non-Hefty
        // data. This will be good enough for matching to the POT database
        // but not good enough for Hefty mode calculations.
        // TODO: switch to using Jonathan's timing scripts for Hefty mode data
        uint64_t ns_since_epoch = card.trigger_time( mb );

        // Create a new raw waveform object for the current minibuffer
        raw_waveforms.emplace_back( TimeClass(ns_since_epoch), minibuffer_data);
      }

      raw_waveform_map[ck] = raw_waveforms;
    }
  }

  annie_event->Set("RawADCData", raw_waveform_map);

  // Store RunInformation data as JSON strings
  //
  // TODO: consider putting this stuff in the header
  uint32_t run_number;
  uint32_t subrun_number;
  for (const auto& key_value_pair : raw_readout->run_information()) {
    std::string key = key_value_pair.first;
    std::string value = key_value_pair.second;
    annie_event->Set(key, value);

    // This information is redundant, but we'll extract the run and
    // subrun numbers and save them in the ANNIEEvent separately
    if (key == "PostgresVariables") {
      Store pv_temp;
      pv_temp.JsonParser(value);
      pv_temp.Get("RunNumber", run_number);
      pv_temp.Get("SubRunNumber", subrun_number);

      // Store the extracted numbers in the ANNIEEvent directly
      annie_event->Set("RunNumber", run_number);
      annie_event->Set("SubrunNumber", subrun_number);
    }
  }

  // Determine the trigger labels to use for each minibuffer
  std::vector<MinibufferLabel> minibuffer_labels;

  // If the raw data were not taken in Hefty mode, then we can assign
  // a label describing the type of data in the single minibuffer using
  // the run type (e.g., beam, cosmic, source)
  bool hefty_mode = num_minibuffers > 1;
  // TODO: enable a different method for Hefty mode data
  if (true/*!hefty_mode*/) {
    // Parse the RunType integer using the "InputVariables" JSON object
    // that we loaded earlier from the RunInformation TTree
    Store temp_store;
    std::string temp_string;
    annie_event->Get("InputVariables", temp_string);
    temp_store.JsonParser(temp_string);

    int run_type_code = BOGUS_INT;
    temp_store.Get("RunType", run_type_code);

    MinibufferLabel mb_label = MinibufferLabel::Unknown;
    switch (run_type_code) {
      case 1: mb_label = MinibufferLabel::LED; break;
      case 2: mb_label = MinibufferLabel::Soft; break;
      case 3: mb_label = MinibufferLabel::Beam; break;
      case 4: mb_label = MinibufferLabel::Cosmic; break;
      case 5: mb_label = MinibufferLabel::Source; break;
      case 6: mb_label = MinibufferLabel::Hefty; break;
      case 7: mb_label = MinibufferLabel::HeftySource; break;
      default: break;
    }

    if (mb_label == MinibufferLabel::Hefty
      || mb_label == MinibufferLabel::HeftySource)
    {
      Log("WARNING: Hefty run type encountered when loading non-Hefty data",
        0, verbosity);
    }

    minibuffer_labels = std::vector<MinibufferLabel>(num_minibuffers, mb_label);
  }
  // TODO: add code to label the minibuffers for Hefty mode

  annie_event->Set("MinibufferLabels", minibuffer_labels);

  Log("Loaded raw data for run " + std::to_string(run_number) + ", subrun "
    + std::to_string(subrun_number) + ", event "
    + std::to_string(event_number) + " ("
    // TODO: change this message when adapting to Hefty mode
    + minibuffer_label_to_string( minibuffer_labels.back() ) + " data)",
    2, verbosity);

  // TODO: store TDCData, CCData

  return true;
}


bool RawLoader::Finalise() {
  return true;
}
