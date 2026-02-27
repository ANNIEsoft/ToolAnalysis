// Class that reads ANNIE non-post-processed raw data files
// and creates RawReadout objects representing each readout
// from the phase I DAQ.
//
// Steven Gardiner <sjgardiner@ucdavis.edu>
#ifndef RAWREADER_H
#define RAWREADER_H

// standard library includes
#include <memory>

// ROOT includes
#include "TBranch.h"
#include "TChain.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"

// reco-annie includes
#include "RawReadout.h"

namespace annie {

  class RawReader {

    public:

      // Because we are using a TChain internally, the file name(s) passed to
      // the constructors may contain wildcards.
      RawReader(const std::string& file_name);
      RawReader(const std::vector<std::string>& file_names);

      // Retrieve the next annie::RawReadout object from the input file(s)
      std::unique_ptr<RawReadout> next();
      std::unique_ptr<RawReadout> previous();

      inline TFile* get_current_file() const {
        return trig_data_chain_.GetFile();
      }

      // Set the flag that determines whether a mismatch between the SequenceID
      // value recorded in the TrigData TTree and the SequenceID value recorded
      // in the PMTData TTree should result in a thrown std::runtime_error
      // or simply a warning message printed to std::cerr.
      void set_throw_on_trig_pmt_sequenceID_mismatch(bool should_I_throw);

      // Attempt to retrieve the readout with the given SequenceID from the
      // input file(s)
      //std::unique_ptr<RawReadout> get_sequence_id(int SequenceID);

    protected:

      void set_branch_addresses();

      // Helper function for the next() and previous() methods
      std::unique_ptr<RawReadout> load_next_entry(bool reverse);

      TChain pmt_data_chain_;
      TChain trig_data_chain_;

      // index of the current PMTData TChain entry
      long long current_pmt_data_entry_ = 0;

      // index of the current TrigData TChain entry
      long long current_trig_data_entry_ = 0;

      /// @brief SequenceID value for the last raw readout that was
      /// successfully loaded from the input file(s)
      long long last_sequence_id_ = -1;

      // Variables used to read from each branch of the PMTData TChain
      unsigned long long br_LastSync_;
      int br_SequenceID_;
      int br_StartTimeSec_;
      int br_StartTimeNSec_;
      unsigned long long br_StartCount_;
      int br_TriggerNumber_;
      int br_CardID_;
      int br_Channels_;
      int br_BufferSize_;
      int br_FullBufferSize_;
      int br_EventSize_;
      std::vector<unsigned short> br_Data_; // [FullBufferSize]
      std::vector<unsigned long long> br_TriggerCounts_; // [TriggerNumber]
      std::vector<unsigned int> br_Rates_; // [Channels]

      // Variables used to read from each branch of the TrigData TChain
      int br_FirmwareVersion_;
      int br_TrigData_SequenceID_;
      int br_TrigData_EventSize_;
      int br_TriggerSize_;
      int br_FIFOOverflow_;
      int br_DriverOverflow_;
      std::vector<unsigned short> br_EventIDs_; // [EventSize]
      std::vector<unsigned long long> br_EventTimes_; // [EventSize]
      std::vector<unsigned int> br_TriggerMasks_; // [TriggerSize]
      std::vector<unsigned int> br_TriggerCounters_; // [TriggerSize]

      // A flag that determines whether a mismatch between the SequenceID
      // value recorded in the TrigData TTree and the SequenceID value recorded
      // in the PMTData TTree should result in a thrown std::runtime_error
      // (true) or simply a warning message printed to std::cerr (false).
      bool throw_on_trig_pmt_sequenceID_mismatch_ = false;
  };
}

#endif
