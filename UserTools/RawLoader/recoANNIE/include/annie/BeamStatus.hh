#pragma once
// Class to store beam status information retrieved from Fermilab's Intensity
// Frontier beam database
//
// Steven Gardiner (sjgardiner@ucdavis.edu)

namespace annie {
  class BeamStatus {

    private:

      /// @brief Milliseconds since the Unix epoch
      unsigned long long time_;

      // TODO: maybe adjust things here since there are two measurements
      // of POT for the BNB

      /// @brief Protons on target
      double pot_; // protons on target

      /// @brief True if the data can be trusted, false if a database query
      /// failed
      bool ok_;

    public:

      BeamStatus();
      BeamStatus(unsigned long long time, double POT, bool ok = true);

      void clear();

      inline unsigned long long time() const { return time_; }
      inline double pot() const { return pot_; }
      inline bool ok() const { return ok_; }

      inline void set_time(unsigned long long time) { time_ = time; }
      inline void set_pot(double POT) { pot_ = POT; }
      inline void set_ok(bool ok) { ok_ = ok; }
  };
}
