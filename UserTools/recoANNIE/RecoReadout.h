// Object representing a reconstructed DAQ readout
//
// Steven Gardiner <sjgardiner@ucdavis.edu>
#ifndef RECOREADOUT_H
#define RECOREADOUT_H

// standard library includes
#include <map>
#include <vector>

// reco-annie includes
#include "Constants.h"
#include "RecoPulse.h"

#include "ANNIEconstants.h"

namespace annie {

  class RecoReadout {

    public:

      RecoReadout(int SequenceID = BOGUS_INT);

      void add_pulse(int card_number, int channel_number,
        int minibuffer_number, const annie::RecoPulse& pulse);

      void add_pulses(int card_number, int channel_number,
        int minibuffer_number, const std::vector<annie::RecoPulse>& pulses);

      const std::vector<annie::RecoPulse>& get_pulses(int card_number,
        int channel_number, int minibuffer_number) const;

      const std::map<int, std::map<int, std::map<int,
        std::vector<annie::RecoPulse> > > >& pulses() const { return pulses_; }

      // Compute the "tank charge" in a given minibuffer within a time
      // window with endpoints given in ns relative to the start of the
      // minibuffer. Also load num_unique_pmts with the number of unique
      // water PMTs that recorded hits in the given time window.
      double tank_charge(int minibuffer_number, size_t start_time,
        size_t end_time, int& num_unique_water_pmts) const;

      inline int sequence_id() const { return sequence_id_; }

    protected:

      // @brief Integer identifier for this readout that is unique within a run
      int sequence_id_;

      /// @brief Reconstructed pulses on each channel
      /// @details The keys (from outer to inner) are (card index, channel
      /// index, minibuffer index). The values are vectors of reconstructed
      /// pulse objects.
      std::map<int, std::map<int, std::map<int,
        std::vector<annie::RecoPulse> > > > pulses_;
  };

}

#endif
