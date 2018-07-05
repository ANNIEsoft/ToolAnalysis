// Tool used to assign POT values (and save other beam-related information)
// to beam minibuffers in the ANNIEEvent
//
// Steven Gardiner <sjgardiner@ucdavis.edu>
#pragma once

// standard library includes
#include <map>
#include <memory>
#include <string>

// ToolAnalysis includes
#include "BeamStatus.h"
#include "Tool.h"
#include "MinibufferLabel.h"

class BeamChecker: public Tool {

  public:

    BeamChecker();
    bool Initialise(std::string configfile, DataModel& data);
    bool Execute();
    bool Finalise();

  protected:

    /// @brief Helper function that opens the beam database data file and
    /// loads the BeamDB BoostStore
    bool initialise_beam_db();

    BeamStatus get_beam_status(uint64_t ns_since_epoch,
      MinibufferLabel mb_label);

    /// @brief Transient BoostStore used to read previously-saved information
    /// from the beam database
    BoostStore beam_db_store_;

    /// @brief Map that enables quick searches of the beam database BoostStore
    /// @details Keys are entry numbers in the BeamData TTree, values are (start
    /// time, end time) pairs giving the range of times (in ms since the Unix
    /// epoch) recorded for the E:TOR875 device (used to determine POT values)
    /// in each entry
    std::map<int, std::pair<unsigned long long, unsigned long long> >
      beam_db_index_;

    /// @brief BeamStatus objects with a value of POT less than or equal to this
    /// value will automatically be marked as "Bad"
    double bad_pot_max_;

    /// @brief The verbosity to use when printing logging messages
    /// @details A larger value corresponds to more verbose output
    int verbosity_;

    /// @brief The timestamp (ms since the Unix epoch) of the earliest POT
    /// information available in the current beam database file
    unsigned long long start_ms_since_epoch_;

    /// @brief The timestamp (ms since the Unix epoch) of the latest POT
    /// information available in the current beam database file
    unsigned long long end_ms_since_epoch_;
};
