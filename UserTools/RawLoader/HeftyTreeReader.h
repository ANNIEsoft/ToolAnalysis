// Class that reads Phase I Hefty mode timing information from the output ROOT
// files produced by Jonathan Eisch's annietools package
//
// Steven Gardiner <sjgardiner@ucdavis.edu>
//#pragma once
#ifndef HEFTYTREETEADER_H
#define HEFTYTREETEADER_H
// standard library includes
#include <memory>

// ROOT includes
#include "TBranch.h"
#include "TChain.h"
#include "TFile.h"
#include "TTree.h"

// ToolAnalysis includes
#include "HeftyInfo.h"

namespace annie {

  class HeftyTreeReader {

    public:

      // Because we are using a TChain internally, the file name(s) passed to
      // the constructors may contain wildcards.
      HeftyTreeReader(const std::string& file_name);
      HeftyTreeReader(const std::vector<std::string>& file_names);
      HeftyTreeReader(TChain* inputChain);

      // Retrieve the next annie::HeftyInfo object from the input file(s)
      std::unique_ptr<HeftyInfo> next();
      std::unique_ptr<HeftyInfo> previous();

    protected:

      void set_branch_addresses();

      // Helper function for the next() and previous() methods
      std::unique_ptr<HeftyInfo> load_next_entry(bool reverse);

      TChain hefty_db_chain_;

      // index of the current PMTData TChain entry
      long long current_hefty_db_entry_ = -1;

      /// @brief SequenceID value for the last Hefty DB entry that was
      /// successfully loaded from the input file(s)
      long long last_sequence_id_ = -1;

      /// @brief The number of minibuffers to assume for Hefty mode
      static constexpr unsigned int NUMBER_OF_MINIBUFFERS = 40u;

      // Variables used to read from each branch of the PMTData TChain
      int br_SequenceID_;

      std::vector<unsigned long long> br_Time_
        = std::vector<unsigned long long>(NUMBER_OF_MINIBUFFERS);

      std::vector<int> br_Label_
        = std::vector<int>(NUMBER_OF_MINIBUFFERS);

      std::vector<long long> br_TSinceBeam_
        = std::vector<long long>(NUMBER_OF_MINIBUFFERS);

      std::vector<int> br_More_
        = std::vector<int>(NUMBER_OF_MINIBUFFERS);
  };
}

#endif
