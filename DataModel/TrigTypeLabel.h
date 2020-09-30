// Enum class that represents the type of trigger
//#pragma once

#ifndef TRIGTYPELABEL_H
#define TRIGTYPELABEL_H

#include <fstream>
#include <sstream>
#include <string>

// TODO: update this to allow for all known trigger types
/* should be derived somehow from:
  --! Assign Timestamp input array
  TS_in(0)  <= bes_holdoff;			--! Booster Extraction Sync over fiber
  TS_in(1)  <= input_1f;			--! $1F Booster Extraction Sync
  TS_in(2)  <= input_1d;			--! $1D Prepare to BNB
  TS_in(3)  <= input_rwm;			--! RWM input
  TS_in(4)  <= Delay_beam_trigger1;		--! Delayed trigger to ADC system
  TS_in(5)  <= Beam_veto1;			--! Beam1 veto high
  TS_in(6)  <= NOT Beam_veto1;			--! Beam1 veto low
  TS_in(7)  <= Delay_beam_trigger2;		--! Delayed trigger to MRD system
  TS_in(8)  <= Beam_veto2;			--! Beam2 veto high
  TS_in(9)  <= NOT Beam_veto2;			--! Beam2 veto low
  TS_in(10) <= Delay_ext_trigger;		--! Delayed external trigger
  TS_in(11) <= Ext_veto;			--! External veto high
  TS_in(12) <= NOT Ext_veto;			--! External veto low
  TS_in(13) <= Beam_trigger;			--! Undelayed Beam Trigger
  TS_in(14) <= Ext_trigger;			--! Undelayed external trigger
  TS_in(15) <= SoftTrigger_out;			--! Software trigger
  TS_in(16) <= TimeLatch_in;			--! Timelatch (not a trigger, used to sync with CPU)
  TS_in(17) <= Window_true AND Window_nonzero AND
              (reg_sig.TMASKWINDOW(TMASK_SELF1) AND SelfTrigger1);
                          			--! Selftrigger back from ADC system AND Window_true
  TS_in(18) <= input_trigger;			--! External Trigger Input
  TS_in(19) <= input_calsource;			--! Calibration Source Trigger Input
  TS_in(20) <= Cal_Trigger;			--! Undelayed Calibration Trigger
  TS_in(21) <= Periodic_trigger;		--! Constant period Trigger
  TS_in(22) <= Window_true;			--! Window open
  TS_in(23) <= NOT Window_true;			--! Window close
  TS_in(24) <= Window_trigger;			--! Window trigger
  TS_in(25) <= MinRate_trigger;			--! Minimum Rate Trigger
  TS_in(29 downto 26) <= CosmicTriggers;	--! Cosmic-ray muon trigger ouputs (Inputs would be too high rate)
  TS_in(30) <= LED_start;			--! Start of LED timer
  TS_in(31) <= input_sync;			--! GPS 1PPS Sync signal
  TS_in(32) <= LED_adc_trigger;			--! Trigger to ADC after LED delay.
  TS_in(33) <= input_8f;			--! TCLK $8F 1PPS Signal
  TS_in(34) <= reg_sig.TMASK1(TMASK_SELF1) AND
                SelfTrigger1;			--! SelfTrigger if used to trigger the ADCs
  TS_in(35) <= MRD_CR_trigger;			--! MRD Cosmic ray trigger
  TS_in(36) <= BE_status.activity_window_start;
  TS_in(37) <= BE_status.activity_window_end;
  TS_in(38) <= BE_status.self_trigger_in_window;
  TS_in(39) <= BE_status.minbias_start;
  TS_in(40) <= BE_status.extend_start;
  TS_in(63 downto 41) <= (others => '0');
*/

enum class TrigTypeEnum : uint8_t { Beam=5, LED=33, AmBe=35, MrdCosmic = 36 };

// Operator for printing a description of the minibuffer label to a
// std::ostream
inline std::ostream& operator<<(std::ostream& out, const TrigTypeEnum& mbl)
{
  switch (mbl) {
    case TrigTypeEnum::Beam:
      out << "Beam";
      break;
    case TrigTypeEnum::LED:
      out << "LED";
      break;
    case TrigTypeEnum::AmBe:
      out << "AmBe";
      break;
    case TrigTypeEnum::MrdCosmic:
      out << "MrdCosmic";
      break;
    default:
      out << "Unknown";
      break;
  }
  return out;
}

inline std::string trigtype_to_string(const TrigTypeEnum& mbl){
  std::stringstream temp_stream;
  temp_stream << mbl;
  return temp_stream.str();
}

#endif
