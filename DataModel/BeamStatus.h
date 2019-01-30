// Class to store beam status information retrieved from Fermilab's Intensity
// Frontier beam database
//
// Steven Gardiner <sjgardiner@ucdavis.edu>
#ifndef BEAMSTATUS_H
#define BEAMSTATUS_H

// standard library includes
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>

// ToolAnalysis includes
#include "BeamDataPoint.h"
#include "SerialisableObject.h"
#include "TimeClass.h"

// TODO: Revisit these options
enum class BeamCondition { NonBeamMinibuffer, Ok, Missing, Bad };

// Operator for printing a description of a BeamCondition value to a
// std::ostream
inline std::ostream& operator<<(std::ostream& out, const BeamCondition& bc)
{
  switch (bc) {
    case BeamCondition::NonBeamMinibuffer:
      out << "Not Beam";
      break;
    case BeamCondition::Ok:
      out << "Ok";
      break;
    case BeamCondition::Bad:
      out << "Bad";
      break;
    default:
      out << "Missing";
      break;
  }
  return out;
}

inline std::string make_beam_condition_string(const BeamCondition& bc) {
  std::stringstream temp_string_stream;
  temp_string_stream << bc;
  return temp_string_stream.str();
}

class BeamStatus : public SerialisableObject {

  friend class boost::serialization::access;

  public:

    BeamStatus();
    BeamStatus(TimeClass time, double POT,
      BeamCondition condition = BeamCondition::Missing);

    void clear();

    inline TimeClass time() const { return time_; }
    inline double pot() const { return pot_; }
    inline BeamCondition condition() const { return condition_; }
    inline const std::map<std::string, std::pair<uint64_t, BeamDataPoint> >&
      data() const { return data_; }
    inline const std::map<std::string, bool>& cuts() const { return cuts_; }

    // Good beam minibuffers for the analysis should use is_beam() == true
    // and ok() == true. Good non-beam minibuffers for the analysis
    // should use is_beam() == false and ok() == true.
    inline bool is_beam() const
      { return condition_ != BeamCondition::NonBeamMinibuffer; }
    inline bool is_missing() const
      { return condition_ != BeamCondition::Missing; }
    inline bool is_bad() const
      { return condition_ != BeamCondition::Bad; }
    inline bool ok() const
      { return condition_ != BeamCondition::Bad
          && condition_ != BeamCondition::Missing; }

    // Beam quality cut checks
    inline bool passed_cut(const std::string& cut_name) const {
      return cuts_.at(cut_name);
    }
    inline bool passed_all_cuts() const {
      // Returns true if all of the entries in the cuts_ map have true
      // boolean values, or false otherwise.
      return std::all_of(cuts_.cbegin(), cuts_.cend(),
        [](const std::pair<std::string, bool>& p) -> bool { return p.second; });
    }

    inline void set_time(TimeClass time) { time_ = time; }
    inline void set_pot(double POT) { pot_ = POT; }
    inline void set_condition(BeamCondition bc) { condition_ = bc; }

    // Adds a new monitoring device measurement to the data map
    void add_measurement(const std::string& device_name,
      uint64_t ms_since_epoch, const BeamDataPoint& bdp);

    void add_measurement(const std::string& device_name,
      uint64_t ms_since_epoch, double value, const std::string& unit);

    // Adds a new cut result to the cuts map (passed == true means that the
    // beam spill survived the quality cut)
    // TODO: Note that this will overwrite an existing cut with the same
    // name. Think more carefully about whether this is the optimal behavior.
    inline void add_cut(const std::string& cut_name, bool passed) {
      cuts_[cut_name] = passed;
    }

    inline bool Print() {
      std::cout << "Timestamp : " << time_ << '\n';
      std::cout << "Beam Intensity [ETOR875] : " << pot_ << " POT\n";
      std::cout << "Condition : " << condition_ << '\n';
      for (auto& outer_pair : data_) {
        std::cout << "Monitoring Data for device : " << outer_pair.first
          << '\n';
        std::cout << "Measurement timestamp : " << outer_pair.second.first
          << " ms since epoch\n";
        outer_pair.second.second.Print();
      }
      for (auto& pair : cuts_) {
        if (pair.second) std::cout << "PASSED ";
        else std::cout << "FAILED ";
        std::cout << " the " << pair.first << " beam quality cut\n";
      }
      return true;
    }

  protected:

    /// @brief The timestamp from the beam database used to assign a POT
    /// value to the current minibuffer (ns since the Unix epoch)
    /// @details Note that the beam database itself only records data with
    /// millisecond precision
    TimeClass time_;

    // TODO: maybe adjust things here since there are two measurements
    // of POT for the BNB (with E:TOR875 being the furthest downstream)

    /// @brief Protons on target
    double pot_; // protons on target

    /// @brief Enum class describing whether the data can be trusted.
    /// @details Minibuffers arising from something other than beam
    /// triggers should all be marked as "NonBeamMinibuffer".
    /// Beam minibuffers should be marked as "Ok", "Missing" (a query
    /// to the beam database failed), or "Bad" (beam information was retrieved
    /// successfully, but the current beam spill should be ignored in the
    /// analysis). Reasons for marking a beam minibuffer as "Bad" include
    /// a very low POT value (suggesting that the beam monitor E:TOR875
    /// was off at the time), a low peak horn current, etc.
    BeamCondition condition_;

    // Map storing data from the beam monitoring instruments for the spill
    // of interest. Keys are device names, values are pairs where the
    // first element is a timestamp for the measurement (in ms since the Unix
    // epoch) and the second element is a BeamDataPoint object.
    //
    // ** Known device names (from the Intensity Frontier beam database) **
    // E:1DCNT = Accumulating count of the number of "$1D" triggers (a "$1D"
    //           signal indicates that the Booster is preparing to send a beam
    //           pulse to ANNIE).
    //
    // E:HI860, E:VI860, E:HP860, E:VP860 = beam intensity (I) or position (P)
    //                                      monitor reading at position 860.
    //                                      Horizontal (H) or vertical (V)
    //                                      position monitors are read
    //                                      separately.
    //
    // E:HP875, E:VP875 = similar monitors at position 875
    //
    // E:HITG1, E:VITG1, E:VPTG1, E:HPTG1 = similar monitors at position TG1
    //
    // E:HPTG2, E:HITG2, E:VITG2, E:VPTG2 = similar monitors at position TG2
    //
    // E:THCURR = Horn current
    // E:TOR860 = POT as measured by toroid at position 860
    //            (further away from target than 875)
    //
    // E:TOR875 = POT as measured by toroid at position 875
    std::map<std::string, std::pair<uint64_t, BeamDataPoint> > data_;

    // Map storing the results of each of the beam quality cut checks.
    // Keys are cut names, values are boolean variables telling whether
    // the spill of interest passed (true) or failed (false) the beam
    // quality cut. For good beam spills to be used for analysis, all of
    // the entries in this map should have a true boolean value.
    std::map<std::string, bool> cuts_;

    template<class Archive> void serialize(Archive & ar,
      const unsigned int version)
    {
      if (!serialise) return;
      ar & time_;
      ar & pot_;
      ar & condition_;
      ar & data_;
      ar & cuts_;
    }
};
#endif
