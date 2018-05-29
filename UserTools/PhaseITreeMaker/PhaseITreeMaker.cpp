// standard library includes
#include <limits>

// ToolAnalysis includes
#include "ADCPulse.h"
#include "ANNIEconstants.h"
#include "BeamStatus.h"
#include "BoostStore.h"
#include "ChannelKey.h"
#include "HeftyInfo.h"
#include "MinibufferLabel.h"
#include "PhaseITreeMaker.h"
#include "TimeClass.h"

constexpr int UNKNOWN_NCV_POSITION = 0;

constexpr uint32_t NCV_PMT1_ID =  6u;
constexpr uint32_t NCV_PMT2_ID = 49u;

// Excluded ADC channel IDs
//  6 = NCV PMT #1 (card 4, channel 1)
// 19 = neutron calibration source trigger input (card 8, channel 2)
// 37 = cosmic trigger input (card 14, channel 0)
// 49 = NCV PMT #2 (card 18, channel 0)
// 61 = summed signals from front veto (card 21, channel 0)
// 62 = summed signals from MRD 2 (card 21, channel 1)
// 63 = RWM (card 21, channel 2)
// 64 = summed signals from MRD 3 (card 21, channel 3)
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
  output_tree_->Branch("event_time_ns", &event_time_ns_, "event_time_ns/L");
  output_tree_->Branch("label", &event_label_, "label/b");
  output_tree_->Branch("hefty_mode", &hefty_mode_, "hefty_mode/O");
  // Used only for Hefty mode. Stores the trigger mask for the minibuffer
  // in which the event took place. For non-Hefty data, this branch will
  // always be zero.
  output_tree_->Branch("hefty_trigger_mask", &hefty_trigger_mask_,
    "hefty_trigger_mask/I");
  // Information about the two pulses in each NCV coincidence event
  output_tree_->Branch("amplitude_ncv1", &amplitude_ncv1_, "amplitude_ncv1/D");
  output_tree_->Branch("amplitude_ncv2", &amplitude_ncv2_, "amplitude_ncv2/D");
  output_tree_->Branch("charge_ncv1", &charge_ncv1_, "charge_ncv1/D");
  output_tree_->Branch("charge_ncv2", &charge_ncv2_, "charge_ncv2/D");
  output_tree_->Branch("raw_amplitude_ncv1", &raw_amplitude_ncv1_,
    "raw_amplitude_ncv1/s");
  output_tree_->Branch("raw_amplitude_ncv2", &raw_amplitude_ncv2_,
    "raw_amplitude_ncv2/s");

  // Create the branches for the output pulse TTree
  output_pulse_tree_ = new TTree("pulse_tree", "Hefty window pulse tree");
  output_pulse_tree_->Branch("run", &run_number_, "run/i");
  output_pulse_tree_->Branch("subrun", &subrun_number_, "subrun/i");
  output_pulse_tree_->Branch("event", &event_number_, "event/i");
  output_pulse_tree_->Branch("spill", &spill_number_, "spill/i");
  output_pulse_tree_->Branch("minibuffer_number", &minibuffer_number_,
    "minibuffer_number/i");
  output_pulse_tree_->Branch("ncv_position", &ncv_position_, "ncv_position/I");
  output_pulse_tree_->Branch("pulse_start_time_ns", &pulse_start_time_ns_,
    "pulse_start_time_ns/L");
  output_pulse_tree_->Branch("label", &event_label_, "label/b");
  output_pulse_tree_->Branch("hefty_mode", &hefty_mode_, "hefty_mode/O");
  // Used only for Hefty mode. Stores the trigger mask for the minibuffer
  // in which the event took place. For non-Hefty data, this branch will
  // always be zero.
  output_pulse_tree_->Branch("hefty_trigger_mask", &hefty_trigger_mask_,
    "hefty_trigger_mask/I");
  // Information about the two pulses in each NCV coincidence event
  output_pulse_tree_->Branch("pulse_amplitude", &pulse_amplitude_,
    "pulse_amplitude/D");
  output_pulse_tree_->Branch("pulse_charge", &pulse_charge_, "pulse_charge/D");
  output_pulse_tree_->Branch("pulse_pmt_id", &pulse_pmt_id_, "pulse_pmt_id/I");
  output_pulse_tree_->Branch("pulse_raw_amplitude", &pulse_raw_amplitude_,
    "pulse_raw_amplitude/s");

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

  // Decide whether we're using Hefty vs. non-Hefty data by checking whether
  // the HeftyInfo object is present
  hefty_mode_ = annie_event->Has("HeftyInfo");

  // One of these objects will be used to get minibuffer timestamps
  // depending on whether we're using Hefty mode or non-Hefty mode.
  HeftyInfo hefty_info; // used for Hefty mode only
  std::vector<TimeClass> mb_timestamps; // used for non-Hefty mode only
  size_t num_minibuffers = 0u;

  if (hefty_mode_) {
    get_object_from_store("HeftyInfo", hefty_info, *annie_event);
    num_minibuffers = hefty_info.num_minibuffers();

    if ( num_minibuffers == 0u ) {
      Log("Error: The PhaseITreeMaker tool found an empty HeftyInfo entry", 0,
        verbosity_);
      return false;
    }

    // Exclude beam spills (or source triggers) near the end of a full
    // multi-minibuffer readout that included extra self-triggers in the Hefty
    // window that could not be recorded. This is indicated in the heftydb
    // TTree via More[39] == 1 and in the HeftyInfo object by more() == true.
    if ( hefty_info.more() ) {
      // Find the first beam or source minibuffer counting backward from the
      // end of the full readout
      size_t mb = num_minibuffers - 1u;
      for (; mb > 0u; --mb) {
        MinibufferLabel label = mb_labels.at(mb);
        if ( label == MinibufferLabel::Beam
          || label == MinibufferLabel::Source ) break;
      }
      // Exclude the minibuffers from the incomplete beam or source trigger's
      // Hefty window by setting a new value of num_minibuffers. This will
      // prematurely end the loop over minibuffers below.
      num_minibuffers = mb;
    }
  }
  else {
    // non-Hefty data
    get_object_from_store("MinibufferTimestamps", mb_timestamps, *annie_event);
    check_that_not_empty("MinibufferTimestamps", mb_timestamps);
    num_minibuffers = mb_timestamps.size();
    // Trigger masks are not saved in the tree for non-Hefty mode
    hefty_trigger_mask_ = 0;
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

  // Load the reconstructed ADC hits
  std::map<ChannelKey, std::vector< std::vector<ADCPulse> > > adc_hits;

  get_object_from_store("RecoADCHits", adc_hits, *annie_event);
  check_that_not_empty("RecoADCHits", adc_hits);

  int old_spill_number = spill_number_;

  for (const auto& pair : adc_hits) {

    const auto& channel_key = pair.first;
    const auto& minibuffer_pulses = pair.second;

    // Use the same spill number for each channel by resetting it during
    // each channel loop iteration. The final loop will increment it in a
    // without resetting it for the next event.
    spill_number_ = old_spill_number;

    pulse_pmt_id_ = channel_key.GetDetectorElementIndex();

    // Flag that vetos minibuffers because the last beam minibuffer failed
    // the quality cuts.
    bool beam_veto_active = false;

    for (size_t mb = 0; mb < num_minibuffers; ++mb) {

      minibuffer_number_ = mb;

      // Determine the correct label for the events in this minibuffer
      MinibufferLabel event_mb_label = mb_labels.at(mb);
      event_label_ = static_cast<uint8_t>( event_mb_label );

      // If this is Hefty mode data, save the trigger mask for the
      // current minibuffer. This is distinct from the MinibufferLabel
      // assigned to the event_label_ variable above. The trigger
      // mask branch isn't used for non-Hefty data.
      if ( hefty_mode_ ) hefty_trigger_mask_ = hefty_info.label(mb);
      else hefty_trigger_mask_ = 0;

      // BEAM QUALITY CUT
      // Skip beam minibuffers with bad or missing beam status information
      // TODO: consider printing a warning message here
      const auto& beam_status = beam_statuses.at(mb);
      const auto& beam_condition = beam_status.condition();
      if (beam_condition == BeamCondition::Missing
        || beam_condition == BeamCondition::Bad)
      {
        // Skip all beam and Hefty window minibuffers until a good-quality beam
        // spill is found again
        beam_veto_active = true;
      }
      if ( beam_veto_active && beam_condition == BeamCondition::Ok ) {
        // We've found a new beam minibuffer that passed the quality check,
        // so disable the beam quality veto
        beam_veto_active = false;
      }
      if (beam_veto_active && (event_mb_label == MinibufferLabel::Hefty
          || event_mb_label == MinibufferLabel::Beam))
      {
        // Bad beam minibuffers and Hefty window minibuffers belonging to the
        // bad beam spill need to be skipped. Since other minibuffers (e.g.,
        // cosmic trigger minibuffers) are not part of the beam "macroevent,"
        // they may still be processed normally.
        continue;
      }

      if (beam_condition == BeamCondition::Ok) ++spill_number_;

      for (const ADCPulse& pulse : minibuffer_pulses.at(mb) ) {
        // For non-Hefty mode, the neutron capture candidate event time
        // is simply its timestamp relative to the start of the single
        // minibuffer.
        pulse_start_time_ns_ = pulse.start_time().GetNs();
        pulse_amplitude_ = pulse.amplitude();
        pulse_charge_ = pulse.charge();
        pulse_raw_amplitude_ = pulse.raw_amplitude();

        // For Hefty mode, the pulse time within the current minibuffer
        // needs to be added to the TSinceBeam value for Hefty window
        // minibuffers (labeled by the RawLoader tool with
        // MinibufferLabel::Hefty). This should be zero for all other
        // minibuffers (it defaults to that value in the heftydb TTree).
        //
        // NOTE: event times for minibuffer labels other than "Beam", "Source",
        // and "Hefty" are calculated relative to the start of the minibuffer,
        // not the beam, source trigger, etc. Only make timing plots using
        // those 3 labels unless you're interested in single-minibuffer timing!
        if ( hefty_mode_ && event_mb_label == MinibufferLabel::Hefty) {
          // The name "TSinceBeam" is used in the heftydb tree for the time
          // since a beam *or* a source trigger, since the two won't be used
          // simultaneously.
          pulse_start_time_ns_ += hefty_info.t_since_beam(mb);
        }

        // Record pulses only for minibuffers where TSinceBeam has been
        // calculated
        if ( !hefty_mode_ || (event_mb_label == MinibufferLabel::Hefty
          || event_mb_label == MinibufferLabel::Beam
          || event_mb_label == MinibufferLabel::Source) )
        {
          output_pulse_tree_->Fill();

          Log("Found pulse on channel " + std::to_string(pulse_pmt_id_)
            + " in run " + std::to_string(run_number_) + " subrun "
            + std::to_string(subrun_number_) + " event "
            + std::to_string(event_number_) + " in minibuffer "
            + std::to_string(minibuffer_number_) + " at "
            + std::to_string(pulse_start_time_ns_) + " ns", 3, verbosity_);
        }
      }
    }
  }

  // This variable stores the absolute time (non-Hefty mode: time within the
  // single minibuffer, Hefty mode: time within the current minibuffer + ns
  // since the Unix epoch of the current minibuffer) of the last NCV PMT #1
  // pulse that was found, whether or not it passed the cuts to mark it as a
  // neutron capture candidate. This time is used to apply an afterpulsing
  // veto. For Phase I, the afterpulsing veto was very conservative (10 us), so
  // it could apply to multiple minibuffers. The "old time" is reset to the
  // lowest possible double value with each new DAQ readout since a single
  // Hefty window will not be recorded across multiple readouts.
  int64_t old_time = std::numeric_limits<int64_t>::lowest(); // ns

  // TODO: add check that NCV PMT #1 and NCV PMT #2 have the same number
  // of minibuffers in their pulse vectors

  // Find NCV coincidence events
  const std::vector< std::vector<ADCPulse> >& ncv_pmt1_pulses
    = adc_hits.at( ChannelKey(subdetector::ADC, NCV_PMT1_ID) );

  // Flag that vetos minibuffers because the last beam minibuffer failed
  // the quality cuts.
  bool beam_veto_active = false;

  for (size_t mb = 0; mb < num_minibuffers; ++mb) {

    minibuffer_number_ = mb;

    // Determine the correct label for the events in this minibuffer
    MinibufferLabel event_mb_label = mb_labels.at(mb);
    event_label_ = static_cast<uint8_t>( event_mb_label );

    // If this is Hefty mode data, save the trigger mask for the
    // current minibuffer. This is distinct from the MinibufferLabel
    // assigned to the event_label_ variable above. The trigger
    // mask branch isn't used for non-Hefty data.
    if ( hefty_mode_ ) hefty_trigger_mask_ = hefty_info.label(mb);
    else hefty_trigger_mask_ = 0;

    // BEAM QUALITY CUT
    // Skip beam minibuffers with bad or missing beam status information
    // TODO: consider printing a warning message here
    const auto& beam_status = beam_statuses.at(mb);
    const auto& beam_condition = beam_status.condition();
    if (beam_condition == BeamCondition::Missing
      || beam_condition == BeamCondition::Bad)
    {
      // Skip all beam and Hefty window minibuffers until a good-quality beam
      // spill is found again
      beam_veto_active = true;
    }
    if ( beam_veto_active && beam_condition == BeamCondition::Ok ) {
      // We've found a new beam minibuffer that passed the quality check,
      // so disable the beam quality veto
      beam_veto_active = false;
    }
    if (beam_veto_active && (event_mb_label == MinibufferLabel::Hefty
        || event_mb_label == MinibufferLabel::Beam))
    {
      // Bad beam minibuffers and Hefty window minibuffers belonging to the
      // bad beam spill need to be skipped. Since other minibuffers (e.g.,
      // cosmic trigger minibuffers) are not part of the beam "macroevent,"
      // they may still be processed normally.
      continue;
    }

    // Increment beam POT count, etc. based on the characteristics of the
    // current minibuffer
    if (beam_condition == BeamCondition::Ok) {
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

    for (const ADCPulse& pulse : ncv_pmt1_pulses.at(mb) ) {
      int64_t pulse_time = pulse.start_time().GetNs();

      // The afterpulsing veto (which uses the value of pulse_time assigned
      // above) can span over multiple Hefty minibuffers, so use the
      // absolute time (pulse time within the current minibuffer
      // plus ns since the Unix epoch for the start of the current minibuffer)
      // while checking for afterpulsing in Hefty mode.
      if ( hefty_mode_ ) {
        int64_t mb_start_ns_since_epoch = hefty_info.time(mb);
        pulse_time += mb_start_ns_since_epoch;
      }

      Log("Found NCV PMT #1 pulse at "
        + std::to_string( pulse.start_time().GetNs() ) + " ns after the"
        " start of the current minibuffer", 3, verbosity_);
      if ( approve_event(pulse_time, old_time, pulse, adc_hits, mb) ) {
        // For non-Hefty mode, the neutron capture candidate event time
        // is simply its timestamp relative to the start of the single
        // minibuffer.
        event_time_ns_ = pulse.start_time().GetNs();

        // For Hefty mode, the event time within the current minibuffer
        // needs to be added to the TSinceBeam value for Hefty window
        // minibuffers (labeled by the RawLoader tool with
        // MinibufferLabel::Hefty). This should be zero for all other
        // minibuffers (it defaults to that value in the heftydb TTree).
        //
        // NOTE: event times for minibuffer labels other than "Beam", "Source",
        // and "Hefty" are calculated relative to the start of the minibuffer,
        // not the beam, source trigger, etc. Only make timing plots using
        // those 3 labels unless you're interested in single-minibuffer timing!
        if ( hefty_mode_ && event_mb_label == MinibufferLabel::Hefty) {
          // The name "TSinceBeam" is used in the heftydb tree for the time
          // since a beam *or* a source trigger, since the two won't be used
          // simultaneously.
          event_time_ns_ += hefty_info.t_since_beam(mb);
        }
        output_tree_->Fill();
        Log("Found NCV event in run " + std::to_string(run_number_)
          + " subrun " + std::to_string(subrun_number_) + " event "
          + std::to_string(event_number_) + " in minibuffer "
          + std::to_string(mb) + " at " + std::to_string(event_time_ns_)
          + " ns", 2, verbosity_);
      }

      // Apply the afterpulsing veto using the time for the last
      // NCV PMT #1 pulse, regardless of whether it passed the neutron
      // candidate cuts or not.
      old_time = pulse_time;
    }
  }

  return true;
}


bool PhaseITreeMaker::Finalise() {
  output_tree_->Write();
  output_pulse_tree_->Write();

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
bool PhaseITreeMaker::approve_event(int64_t event_time, int64_t old_time,
  const ADCPulse& first_ncv1_pulse,
  const std::map<ChannelKey, std::vector< std::vector<ADCPulse> > >& adc_hits,
  int minibuffer_index)
{
  // Afterpulsing cut (uses the absolute event time relative to the last beam
  // spill since the cut can extend across multiple minibuffers). The other
  // checks are for small time periods (typically 40 ns for phase I) so they
  // can use the time relative to the start of the current minibuffer (the
  // pulse object's "start time")
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
  // Minibuffers are short enough (80 us maximum for non-Hefty data, smaller
  // for Hefty mode data) that, even though the TimeClass has an underlying
  // type of uint64_t, an int64_t shouldn't overflow here.
  int64_t ncv1_time = first_ncv1_pulse.start_time().GetNs();
  bool found_coincidence = false;
  for ( const auto& pulse : ncv_pmt2_pulses ) {
    int64_t ncv2_time = pulse.start_time().GetNs();
    Log("Found NCV PMT #2 pulse at "
      + std::to_string(ncv2_time) + " ns after the start of the current"
      " minibuffer", 3, verbosity_);
    if ( std::abs( ncv1_time - ncv2_time ) <= ncv_coincidence_tolerance_ ) {
      found_coincidence = true;

      // Store information about the coincident pulses in this NCV
      // event to the appropriate branch variables for the output TTree
      amplitude_ncv1_ = first_ncv1_pulse.amplitude();
      charge_ncv1_ = first_ncv1_pulse.charge();
      raw_amplitude_ncv1_ = first_ncv1_pulse.raw_amplitude();

      amplitude_ncv2_ = pulse.amplitude();
      charge_ncv2_ = pulse.charge();
      raw_amplitude_ncv2_ = pulse.raw_amplitude();

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
  uint64_t start_time, uint64_t end_time, int& num_unique_water_pmts)
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

  Log("Tank charge = " + std::to_string(tank_charge) + " nC", 4, verbosity_);
  Log("Unique PMTs = " + std::to_string(num_unique_water_pmts), 4, verbosity_);
  return tank_charge;
}
