// standard library includes
#include <limits>

// ToolAnalysis includes
#include "ADCPulse.h"
#include "ANNIEconstants.h"
#include "BeamStatus.h"
#include "BoostStore.h"
#include "ChannelKey.h"
#include "MinibufferLabel.h"
#include "PhaseITreeMaker.h"

constexpr int UNKNOWN_NCV_POSITION = 0;

constexpr uint32_t NCV_PMT1_ID =  6u;
constexpr uint32_t NCV_PMT2_ID = 49u;

// Excluded ADC channel IDs
//  6 = NCV PMT #1 (card 4, channel 1)
// 19 = neutron calibration source trigger input (card 8, channel 2)
// 37 = cosmic trigger input (card 14, channel 0)
// 49 = NCV PMT #2 (card 18, channel 0)
// 61 = (card 21, channel 0)
// 62 = (card 21, channel 1)
// 63 = (card 21, channel 2)
// 64 = (card 21, channel 3)
constexpr std::array<uint32_t, 56> water_tank_pmt_IDs = {
  1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 21, 22, 23,
  24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 38, 39, 40, 41, 42, 43,
  44, 45, 46, 47, 48, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60
};

int PhaseITreeMaker::get_NCV_position(uint32_t run_number) const {
  if (run_number >= 635u && run_number < 704u) return 1;
  if (run_number >= 704u && run_number < 802u) return 2;
  if (run_number >= 802u && run_number < 808u) return 3;
  if (run_number >= 808u && run_number < 813u) return 4;
  if (run_number == 813u) return 5;
  if (run_number == 814u) return 6;
  if (run_number >= 815u && run_number < 825u) return 7;
  if (run_number >= 825u && run_number < 883u) return 8;
  else return UNKNOWN_NCV_POSITION;
}

PhaseITreeMaker::PhaseITreeMaker() : Tool() {}

bool PhaseITreeMaker::Initialise(std::string config_filename, DataModel& data)
{
  // Load configuration file variables
  if ( !config_filename.empty() ) m_variables.Initialise(config_filename);

  // Assign a transient data pointer
  m_data = &data;

  m_variables.Get("verbose", verbosity_);

  get_object_from_store("AfterpulsingCutTime", afterpulsing_veto_time_,
    m_variables);

  get_object_from_store("TankChargeWindowLength", tank_charge_window_length_,
    m_variables);

  get_object_from_store("MaxUniqueWaterPMTs", max_unique_water_pmts_,
    m_variables);

  get_object_from_store("MaxTankCharge", max_tank_charge_, m_variables);

  get_object_from_store("NCVCoincidenceTolerance", ncv_coincidence_tolerance_,
    m_variables);

  std::string output_filename;
  get_object_from_store("OutputFile", output_filename, m_variables);

  output_tfile_ = std::unique_ptr<TFile>(
    new TFile(output_filename.c_str(), "recreate"));

  output_tree_ = new TTree("phaseI", "ANNIE Phase I Analysis Tree");
  output_tree_->Branch("run", &run_number_, "run/i");
  output_tree_->Branch("subrun", &subrun_number_, "subrun/i");
  output_tree_->Branch("event", &event_number_, "event/i");
  output_tree_->Branch("ncv_position", &ncv_position_, "ncv_position/I");
  output_tree_->Branch("event_time_ns", &event_time_ns_, "event_time_ns/l");
  output_tree_->Branch("label", &event_label_, "label/b");

  return true;
}

bool PhaseITreeMaker::Execute() {

  // Get a pointer to the ANNIEEvent Store
  auto* annie_event = m_data->Stores["ANNIEEvent"];

  if (!annie_event) {
    Log("Error: The PhaseITreeMaker tool could not find the ANNIEEvent Store",
      0, verbosity_);
    return false;
  }

  // Load the labels describing the type of data stored in each minibuffer
  std::vector<MinibufferLabel> mb_labels;

  get_object_from_store("MinibufferLabels", mb_labels, *annie_event);
  check_that_not_empty("MinibufferLabels", mb_labels);

  bool hefty_mode = mb_labels.size() > 1u;
  if (hefty_mode) {
    Log("Error: Hefty mode data handling has not yet been implemented in the"
      " PhaseITreeMaker tool", 0, verbosity_);
    return false;
  }

  // Load the beam status objects for each minibuffer
  std::vector<BeamStatus> beam_statuses;
  get_object_from_store("BeamStatuses", beam_statuses, *annie_event);
  check_that_not_empty("BeamStatuses", beam_statuses);

  // Load run, subrun, and event numbers
  get_object_from_store("RunNumber", run_number_, *annie_event);
  get_object_from_store("SubRunNumber", subrun_number_, *annie_event);
  get_object_from_store("EventNumber", event_number_, *annie_event);

  // Determine the NCV position based on the run number
  ncv_position_ = get_NCV_position(run_number_);

  // Create a new object to store the POT, etc. information for this NCV
  // position if one doesn't exist yet
  if ( !ncv_position_info_.count(ncv_position_) ) {
    ncv_position_info_[ncv_position_] = NCVPositionInfo();
  }

  // Get a reference to the position information object. We will use this
  // reference to update it as we analyze the current ANNIEEvent
  auto& pos_info = ncv_position_info_.at(ncv_position_);

  //// Non-Hefty mode data
  //MinibufferLabel nonhefty_mb_label = mb_labels.front();
  //if (nonhefty_mb_label == MinibufferLabel::Beam)

  // Load the reconstructed ADC hits
  std::map<ChannelKey, std::vector< std::vector<ADCPulse> > > adc_hits;

  get_object_from_store("RecoADCHits", adc_hits, *annie_event);
  check_that_not_empty("RecoADCHits", adc_hits);

  // Find NCV coincidence events
  const std::vector< std::vector<ADCPulse> >& ncv_pmt1_pulses
    = adc_hits.at( ChannelKey(subdetector::ADC, NCV_PMT1_ID) );

  // TODO: add check that NCV PMT #1 and NCV PMT #2 have the right number
  // of minibuffers in their pulse vectors
  size_t num_minibuffers = mb_labels.size();
  for (size_t mb = 0; mb < num_minibuffers; ++mb) {

    // Determine the correct label for the events in this minibuffer
    // TODO: revise for Hefty mode
    MinibufferLabel event_mb_label = mb_labels.at(mb);
    event_label_ = static_cast<uint8_t>( event_mb_label );

    // Skip beam minibuffers with bad or missing beam status information
    // TODO: consider printing a warning message here
    const auto& beam_status = beam_statuses.at(mb);
    const auto& beam_condition = beam_status.condition();
    if (beam_condition == BeamCondition::Missing
      || beam_condition == BeamCondition::Bad) continue;
    else if (beam_condition == BeamCondition::Ok) {
      ++pos_info.num_beam_spills;
      pos_info.total_POT += beam_status.pot();
    }

    if ( event_mb_label == MinibufferLabel::Source ) {
      ++pos_info.num_source_triggers;
    }
    else if ( event_mb_label == MinibufferLabel::Cosmic ) {
      ++pos_info.num_cosmic_triggers;
    }
    else if ( event_mb_label == MinibufferLabel::Soft ) {
      ++pos_info.num_soft_triggers;
    }

    double old_time = std::numeric_limits<double>::lowest(); // ns
    for (const ADCPulse& pulse : ncv_pmt1_pulses.at(mb) ) {
      double event_time = static_cast<double>( pulse.start_time().GetNs() );
      Log("Found NCV PMT #1 pulse with event_time = "
        + std::to_string( pulse.start_time().GetNs() ) + " ns", 3, verbosity_);
      if ( approve_event(event_time, old_time, pulse, adc_hits, mb) ) {
        event_time_ns_ = pulse.start_time().GetNs();
        output_tree_->Fill();
        Log("Found NCV event in run " + std::to_string(run_number_)
          + " subrun " + std::to_string(subrun_number_) + " event "
          + std::to_string(event_number_) + " in minibuffer "
          + std::to_string(mb) + " at " + std::to_string(event_time_ns_)
          + " ns", 2, verbosity_);
      }

      // Apply the afterpulsing veto using the event time for the last
      // NCV PMT #1 pulse, regardless of whether it passed the neutron
      // candidate cuts or not.
      old_time = event_time;
    }
  }

  return true;
}


bool PhaseITreeMaker::Finalise() {
  output_tree_->Write();

  TTree* beam_tree = new TTree("ncv_pos_info",
    "Information about each NCV position");

  bool made_branches = false;
  for (auto& pair : ncv_position_info_) {
    int ncv_position = pair.first;

    auto& info = pair.second;

    if ( !made_branches ) {
      beam_tree->Branch("ncv_position", &ncv_position, "ncv_position/I");
      beam_tree->Branch("total_pot", &(info.total_POT), "total_pot/D");
      beam_tree->Branch("num_beam_spills", &(info.num_beam_spills),
        "num_beam_spills/l");
      beam_tree->Branch("num_source_triggers", &(info.num_source_triggers),
        "num_source_triggers/l");
      beam_tree->Branch("num_cosmic_triggers", &(info.num_cosmic_triggers),
        "num_cosmic_triggers/l");
      beam_tree->Branch("num_soft_triggers", &(info.num_soft_triggers),
        "num_soft_triggers/l");

      made_branches = true;
    }
    else {
      beam_tree->SetBranchAddress("ncv_position", &ncv_position);
      beam_tree->SetBranchAddress("total_pot", &(info.total_POT));
      beam_tree->SetBranchAddress("num_beam_spills", &(info.num_beam_spills));
      beam_tree->SetBranchAddress("num_source_triggers",
        &(info.num_source_triggers));
      beam_tree->SetBranchAddress("num_cosmic_triggers",
        &(info.num_cosmic_triggers));
      beam_tree->SetBranchAddress("num_soft_triggers",
        &(info.num_soft_triggers));
    }

    beam_tree->Fill();
  }

  beam_tree->Write();

  output_tfile_->Close();
  return true;
}

// Put all analysis cuts here (will be applied for both Hefty and non-Hefty
// modes in the same way)
bool PhaseITreeMaker::approve_event(double event_time, double old_time,
  const ADCPulse& first_ncv1_pulse,
  const std::map<ChannelKey, std::vector< std::vector<ADCPulse> > >& adc_hits,
  int minibuffer_index)
{
  // Afterpulsing cut
  if (event_time <= old_time + afterpulsing_veto_time_) {
    Log("Failed afterpulsing cut", 3, verbosity_);
    return false;
  }

  Log("Passed afterpulsing cut", 3, verbosity_);

  // Unique water PMT and tank charge cuts
  int num_unique_water_pmts = BOGUS_INT;
  uint64_t tc_start_time = first_ncv1_pulse.start_time().GetNs();
  double tank_charge = compute_tank_charge(minibuffer_index,
    adc_hits, tc_start_time, tc_start_time + tank_charge_window_length_,
    num_unique_water_pmts);

  if (num_unique_water_pmts > max_unique_water_pmts_) {
    Log("Failed unique water PMT cut", 3, verbosity_);
    return false;
  }
  Log("Passed unique water PMT cut", 3, verbosity_);

  if (tank_charge > max_tank_charge_) {
    Log("Failed tank charge cut", 3, verbosity_);
    return false;
  }
  Log("Passed tank charge cut", 3, verbosity_);

  // NCV coincidence cut
  const std::vector<ADCPulse>& ncv_pmt2_pulses = adc_hits.at(
    ChannelKey(subdetector::ADC, NCV_PMT2_ID) ).at(minibuffer_index);
  long long ncv1_time = first_ncv1_pulse.start_time().GetNs();
  bool found_coincidence = false;
  for ( const auto& pulse : ncv_pmt2_pulses ) {
    long long ncv2_time = pulse.start_time().GetNs();
    Log("Found NCV PMT #2 pulse with event_time = "
      + std::to_string(ncv2_time) + " ns", 3, verbosity_);
    if ( std::abs( ncv1_time - ncv2_time ) <= ncv_coincidence_tolerance_ ) {
      found_coincidence = true;
      break;
    }
  }

  if (!found_coincidence) {
    Log("Failed NCV coincidence cut", 3, verbosity_);
    return false;
  }

  Log("Passed NCV coincidence cut", 3, verbosity_);

  return true;
}

// Returns the integrated tank charge in a given time window.
// Also loads the integer num_unique_water_pmts with the number
// of hit water tank PMTs.
double PhaseITreeMaker::compute_tank_charge(size_t minibuffer_number,
  const std::map< ChannelKey, std::vector< std::vector<ADCPulse> > >& adc_hits,
  uint64_t start_time, uint64_t end_time, int& num_unique_water_pmts) const
{
  double tank_charge = 0.;
  num_unique_water_pmts = 0;

  for (const auto& channel_pair : adc_hits) {
    const ChannelKey& ck = channel_pair.first;

    // Only consider ADC channels that belong to water tank PMTs
    if (ck.GetSubDetectorType() != subdetector::ADC) continue;

    // Skip non-water-tank PMTs
    uint32_t pmt_id = ck.GetDetectorElementIndex();
    if ( std::find(std::begin(water_tank_pmt_IDs), std::end(water_tank_pmt_IDs),
       pmt_id) == std::end(water_tank_pmt_IDs) ) continue;

    // Get the vector of pulses for the current channel and minibuffer of
    // interest
    const auto& pulse_vec = channel_pair.second.at(minibuffer_number);

    bool found_pulse_in_time_window = false;

    for ( const auto& pulse : pulse_vec ) {
      size_t pulse_time = pulse.start_time().GetNs();
      if ( pulse_time >= start_time && pulse_time <= end_time) {

        // Increment the unique hit PMT counter if the current pulse is within
        // the given time window (and we hadn't already)
        if (!found_pulse_in_time_window) {
          ++num_unique_water_pmts;
          found_pulse_in_time_window = true;
        }
        tank_charge += pulse.charge();
      }
    }
  }

  return tank_charge;
}
