// Class to store beam status information retrieved from Fermilab's Intensity
// Frontier beam database
//
// Steven Gardiner (sjgardiner@ucdavis.edu)
#ifndef BEAMSTATUS_H
#define BEAMSTATUS_H

// standard library includes
#include <iostream>
#include <fstream>
#include <sstream>

// ToolAnalysis includes
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

    inline void set_time(TimeClass time) { time_ = time; }
    inline void set_pot(double POT) { pot_ = POT; }
    inline void set_condition(BeamCondition bc) { condition_ = bc; }

    inline bool Print() {
      std::cout << "Timestamp : " << time_ << '\n';
      std::cout << "Beam Intensity [ETOR875] : " << pot_ << " POT\n";
      std::cout << "Condition : " << condition_ << '\n';
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
    /// was off at the time, Hefty mode information suggesting that one or
    /// more self-trigger minibuffers might be missing after the current beam
    /// spill, etc.
    BeamCondition condition_;

    template<class Archive> void serialize(Archive & ar,
      const unsigned int version)
    {
      if (!serialise) return;
      ar & time_;
      ar & pot_;
      ar & condition_;
    }
};
#endif
