// ToolAnalysis includes
//#include "ANNIEconstants.h"
//#include "MinibufferLabel.h"
#include "RawLoader.h"
//#include "Waveform.h"

// recoANNIE includes
//#include "RawCard.h"
//#include "RawChannel.h"
//#include "RawReadout.h"

// Boost includes
//#include "boost/bimap.hpp"

// standard library includes
//#include <map>
//#include <memory>
//#include <string>
//#include <vector>

namespace {

  // Parts of the trigger mask to check when assigning minibuffer labels
  // for Hefty mode data
  constexpr int HEFTY_BEAM_TRIGGER_MASK = 0x1 << 4;
  constexpr int HEFTY_SOURCE_TRIGGER_MASK = 0x1 << 20;

  constexpr int HEFTY_SOFT_TRIGGER_MASK = 0x1 << 15;
  constexpr int HEFTY_PERIODIC_TRIGGER_MASK = 0x1 << 21;
  constexpr int HEFTY_MINRATE_TRIGGER_MASK = 0x1 << 25;

  constexpr int HEFTY_WINDOW_TRIGGER_MASK = 0x1 << 24;

  // Specific cosmic triggers
  constexpr int HEFTY_COSMIC_NCV_VERTICAL_TRIGGER_MASK = 0x1 << 26;
  constexpr int HEFTY_COSMIC_NCV_TO_MRD_DIAGONAL_TRIGGER_MASK = 0x1 << 27;
  constexpr int HEFTY_COSMIC_WATER_VERTICAL_TRIGGER_MASK = 0x1 << 28;
  constexpr int HEFTY_COSMIC_MRD_TO_NCV_DIAGONAL_TRIGGER_MASK = 0x1 << 29;

  // Generic cosmic trigger (OR of the specific ones)
  constexpr int HEFTY_COSMIC_TRIGGER_MASK =
    HEFTY_COSMIC_NCV_VERTICAL_TRIGGER_MASK |
    HEFTY_COSMIC_NCV_TO_MRD_DIAGONAL_TRIGGER_MASK |
    HEFTY_COSMIC_WATER_VERTICAL_TRIGGER_MASK |
    HEFTY_COSMIC_MRD_TO_NCV_DIAGONAL_TRIGGER_MASK;

  // LED trigger
  constexpr int HEFTY_LED_TRIGGER_MASK = 0x1 << 30;
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
  m_data->CStore.Set("InputFile",input_file_name); // for other tools

  int verbosity;
  m_variables.Get("verbose", verbosity);
  // TODO: use the Geometry class from the header (or something else) instead
  std::string bimapfile;
  int get_ok = m_variables.Get("PMTtoCardChannelFile",bimapfile);
  if(!get_ok) bimapfile = "mapPMTtoDAQCardChannel_phase1";  // provide defaults for people who haven't specified
  FillBimap(bimapfile);                                     // FIXME ^ probably a bad idea ^

  Log("Opening input file " + input_file_name, 1, verbosity);
  // TODO: Switch to using
  // m_reader = std::make_unique<annie::RawReader>(input_file_name);
  // when our Docker image has C++14 support
  m_reader = std::unique_ptr<annie::RawReader>(
    new annie::RawReader(input_file_name));

  // Assume that the data were taken in Hefty mode if the HeftyTimingFile
  // keyword is present in the configuration file
  // TODO: consider using a different method to determine this
  // TODO: switch to using Has() when it's added to the regular Store class
  // (not just BoostStore)
  //m_using_hefty_mode = m_variables.Has("HeftyTimingFile");
  std::string dummy_str;
  m_using_hefty_mode = m_variables.Get("HeftyTimingFile", dummy_str);
  m_data->CStore.Set("UsingHeftyMode",m_using_hefty_mode); // for other tools
  if ( m_using_hefty_mode ) {

    std::string hefty_timing_filename;
    bool got_timing_file = m_variables.Get("HeftyTimingFile",
      hefty_timing_filename);
    m_data->CStore.Set("HeftyTimingFile",hefty_timing_filename); // for other tools

    if ( !got_timing_file ) {
      Log("ERROR: Failed to retrieve Hefty timing file name", 0, verbosity);
      return false;
    }

    Log("Opening Hefty timing input file " + hefty_timing_filename, 1,
      verbosity);

    m_hefty_tree_reader = std::unique_ptr<annie::HeftyTreeReader>(
      new annie::HeftyTreeReader(hefty_timing_filename));
  }

  if(m_data->Stores.count("ANNIEEvent")==0){
    m_data->Stores["ANNIEEvent"] = new BoostStore(false,
      BOOST_STORE_MULTIEVENT_FORMAT);
  }

  return true;
}

bool RawLoader::Execute() {

  int verbosity;
  m_variables.Get("verbose", verbosity);

  // Load the next raw data readout from the input file
  auto raw_readout_unique = m_reader->next();
  annie::RawReadout* raw_readout = raw_readout_unique.release(); // so we can put it in the ANNIEEvent
  m_data->Stores.at("ANNIEEvent")->Set("RawReadout",raw_readout,false);  // for e.g. PlotWaveforms Tool

  std::unique_ptr<HeftyInfo> hefty_info = nullptr;
  if (m_using_hefty_mode) hefty_info = m_hefty_tree_reader->next();

  // TODO: can we make this a bool?
  // If we've reached the end of the input file, set the stop
  // loop flag and don't do anything else.
  if ( !raw_readout || (m_using_hefty_mode && !hefty_info) )
  {
    m_data->vars.Set("StopLoop", 1);
    return true;
  }

  if ( m_using_hefty_mode && (raw_readout->sequence_id()
    != hefty_info->sequence_id()) )
  {
    Log("ERROR: SequenceID mismatch between raw data and hefty db ROOT files",
      0, verbosity);
    return false;
  }

  static int readout_counter = -1;
  ++readout_counter;

  // Otherwise, place the raw data into the ANNIEEvent in the Store
  auto* annie_event = m_data->Stores["ANNIEEvent"];
  uint32_t event_number = raw_readout->sequence_id();
  annie_event->Set("EventNumber", event_number);

  // Build the ChannelKey -> (raw) Waveform map from the annie::RawReadout
  // object
  std::map<unsigned long, std::vector<Waveform<unsigned short> > >
    raw_waveform_map;

  size_t num_minibuffers = 0;

  for ( const auto& card_pair : raw_readout->cards() ) {
    const auto& card = card_pair.second;

    for ( const auto& channel_pair : card.channels() ) {
      const auto& channel = channel_pair.second;

      int pmt_id = pmt_ID_and_card_channel_bimap.right.at(
        std::pair<int, int>(card.card_id(), channel.channel_id()) );

      unsigned long ck = pmt_id; // bypassing the Geometry::ConsumeNextFreeChannelKey !!!
      // Note: in this case pmt_id's MUST be unique!
      std::vector<Waveform<unsigned short> > raw_waveforms;

      num_minibuffers = channel.num_minibuffers();

      // TODO: add a check that the number of minibuffers is the same for
      // each channel

      for ( size_t mb = 0; mb < num_minibuffers; ++mb) {
        const auto& minibuffer_data = channel.minibuffer_data( mb );

        // The time in ns since the Unix epoch for the start of the current
        // minibuffer
        uint64_t ns_since_epoch = BOGUS_INT;

        if ( !m_using_hefty_mode ) {
          // Estimate the time in ns since the Unix epoch for the start of this
          // minibuffer using the standard recoANNIE technique for non-Hefty
          // data. This will be good enough for matching to the POT database
          // but not good enough for Hefty mode calculations.
          ns_since_epoch = card.trigger_time( mb );
        }
        else {
          // Use the Hefty mode timestamp calculated using annietools
          ns_since_epoch = hefty_info->time( mb );
        }

        // Create a new raw waveform object for the current minibuffer
        raw_waveforms.emplace_back( ns_since_epoch, minibuffer_data);
      }

      raw_waveform_map[ck] = raw_waveforms;
    }
  }

  annie_event->Set("RawADCData", raw_waveform_map);

  // Store the minibuffer timestamps to the Store if this is non-Hefty data
  // (allows us to get the timestamps without loading the full raw waveforms).
  // Don't both if this is Hefty mode data (they will appear in the HeftyInfo
  // object that we will save instead)
  if ( !m_using_hefty_mode ) {

    // Use the first channel (arbitrarily) to get non-Hefty minibuffer
    // timestamps
    std::vector<TimeClass> mb_timestamps;

    const auto& pair = *raw_waveform_map.cbegin();
    const auto& raw_waveforms = pair.second;

    for (const auto& rwf : raw_waveforms) {
      mb_timestamps.emplace_back( rwf.GetStartTime() );
    }

    // Store the minibuffer timestamps separately
    annie_event->Set("MinibufferTimestamps", mb_timestamps);
  }
  // Hefty mode
  else annie_event->Set("HeftyInfo", *hefty_info);

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
      annie_event->Set("SubRunNumber", subrun_number);
    }
  }

  // Determine the trigger labels to use for each minibuffer
  std::vector<MinibufferLabel> minibuffer_labels;

  if ( !m_using_hefty_mode ) {
    // If the raw data were not taken in Hefty mode, then we can assign
    // a label describing the type of data in the single minibuffer using
    // the run type (e.g., beam, cosmic, source)

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
      case 6: mb_label = MinibufferLabel::Hefty; break; // Hefty
      case 7: mb_label = MinibufferLabel::Hefty; break; // HeftySource
      default: break;
    }

    if (mb_label == MinibufferLabel::Hefty)
    {
      Log("WARNING: Hefty run type encountered when loading non-Hefty data",
        0, verbosity);
    }

    minibuffer_labels = std::vector<MinibufferLabel>(num_minibuffers, mb_label);
  }
  else {
    // Hefty mode minibuffer labels are created using trigger masks
    for (size_t mb = 0; mb < hefty_info->num_minibuffers(); ++mb) {

      MinibufferLabel mb_label = MinibufferLabel::Unknown;

      int mask = hefty_info->label(mb);
      if ( mask & HEFTY_BEAM_TRIGGER_MASK ) mb_label = MinibufferLabel::Beam;
      else if ( mask & HEFTY_SOURCE_TRIGGER_MASK )
        mb_label = MinibufferLabel::Source;

      // Label minibuffers within the Hefty self-trigger window as
      // "Hefty" minibuffers, even if they include a cosmic, soft, etc.
      // trigger
      else if ( mask & HEFTY_WINDOW_TRIGGER_MASK )
        mb_label = MinibufferLabel::Hefty;

      else if ( mask & HEFTY_LED_TRIGGER_MASK )
        mb_label = MinibufferLabel::LED;

      else if ( mask & HEFTY_COSMIC_TRIGGER_MASK )
        mb_label = MinibufferLabel::Cosmic;

      // Label minrate and periodic minibuffers as "soft" in addition to
      // true "soft" minibuffers
      // TODO: reconsider whether you should do this
      else if ( (mask & HEFTY_SOFT_TRIGGER_MASK)
        || (mask & HEFTY_PERIODIC_TRIGGER_MASK)
        || (mask & HEFTY_MINRATE_TRIGGER_MASK) )
        mb_label = MinibufferLabel::Soft;

      minibuffer_labels.push_back( mb_label );
    }
  }

  annie_event->Set("MinibufferLabels", minibuffer_labels);

  std::string event_description;
  if (!m_using_hefty_mode) {
    minibuffer_label_to_string( minibuffer_labels.back() );
  }
  else {
    // Hefty mode
    event_description = "Hefty";
  }

  Log("Loaded raw data for run " + std::to_string(run_number) + ", subrun "
    + std::to_string(subrun_number) + ", event " + std::to_string(event_number)
    + " (" + event_description + " data)", 2, verbosity);

  // TODO: store TDCData, CCData

  return true;
}


bool RawLoader::Finalise() {
  return true;
}

void RawLoader::FillBimap(std::string bimapfile){
  // boost::bimap<int, std::pair<int, int> > pmt_ID_and_card_channel_bimap;
  
  std::ifstream file(bimapfile.c_str());
  std::string line;
  if(file.is_open()){
    while (getline(file,line)){
      if (line.size()>0){
        if (line.at(0)=='#')continue;
        int pmtid;
        int card;
        int channel;
        std::stringstream stream(line);
        if(stream>>pmtid>>card>>channel){
          pmt_ID_and_card_channel_bimap.insert(
              boost::bimap<int, std::pair<int, int> >::value_type(pmtid, std::pair<int,int>(card,channel))
          );
        }
      }
    }
  }
  //cout<<"map contents";
  //for(auto anen = pmt_ID_and_card_channel_bimap.left.begin(); anen!=pmt_ID_and_card_channel_bimap.left.end(); anen++ ){ cout<<anen->first<<", ("<<anen->second.first<<", "<<anen->second.second<<")"<<endl; }
  file.close();
  
}
