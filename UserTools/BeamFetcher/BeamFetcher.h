// Tool to download beam information from the Intensity Frontier
// beam database and save it in a BoostStore file for later retrieval
// by the BeamChecker tool
//
// Steven Gardiner <sjgardiner@ucdavis.edu>
#pragma once

// standard library includes
#include <iostream>
#include <string>

// ToolAnalysis includes
#include "Tool.h"

class BeamFetcher: public Tool {

  public:

    BeamFetcher();
    bool Initialise(std::string configfile, DataModel& data);
    bool Execute();
    bool Finalise();

  protected:

    /// @brief The verbosity to use when printing logging messages
    /// @details A larger value corresponds to more verbose output
    int verbosity_;

    /// @brief Transient BoostStore used to save downloaded information
    /// from the Intensity Frontier beam database to disk
    BoostStore beam_db_store_;

    /// @brief Helper function that downloads and processes the beam
    /// information
    bool fetch_beam_data(uint64_t start_ms_since_epoch,
      uint64_t end_ms_since_epoch, uint64_t chunk_step_ms);

    /// @brief Name of the output file in which the beam database information
    /// will be saved
    std::string db_filename_;
};
