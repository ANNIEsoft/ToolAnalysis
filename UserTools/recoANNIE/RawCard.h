// Class that represents a full readout of raw data from all channels
// that are monitored by a single DAQ VME card.
//
// Steven Gardiner <sjgardiner@ucdavis.edu>
#ifndef RAWCARD_H
#define RAWCARD_H

// standard library includes
#include <map>

// reco-annie includes
#include "RawChannel.h"

namespace annie {

  class RawCard {

    public:

      RawCard() {}

      RawCard(int CardID, unsigned long long LastSync, int StartTimeSec,
        int StartTimeNSec, unsigned long long StartCount, int Channels,
        int BufferSize, int MiniBufferSize,
        const std::vector<unsigned short>& Data,
        const std::vector<unsigned long long>& TriggerCounts,
        const std::vector<unsigned int>& Rates);

      inline unsigned int card_id() const { return card_id_; }

      inline const std::map<int, annie::RawChannel>& channels() const
        { return channels_; }

      inline const annie::RawChannel& channel(int index) const
        { return channels_.at(index); }

      inline unsigned long long last_sync() const { return last_sync_; }

      inline int start_time_sec() const { return start_time_sec_; }
      inline int start_time_nsec() const { return start_time_nsec_; }

      inline unsigned long long start_count() const { return start_count_; }

      /// @brief Compute the time (in nanoseconds since the Unix epoch) for the
      /// trigger corresponding to the given minibuffer using the timestamps
      /// from this card
      unsigned long long trigger_time(size_t minibuffer_index) const;

      /// @brief Get the number of minibuffers stored for each channel owned
      /// by this card
      inline size_t num_minibuffers() const { return trigger_counts_.size(); }

    protected:

      void add_channel(int channel_number,
        const std::vector<unsigned short>& full_buffer_data,
        int channel_buffer_size, unsigned int rate, bool overwrite_ok = false);

      /// @brief The index of this VME card
      unsigned card_id_;

      unsigned long long last_sync_;
      int start_time_sec_;
      int start_time_nsec_;
      unsigned long long start_count_;
      std::vector<unsigned long long> trigger_counts_;

      /// @brief Raw data for each of the channels read out by this card
      /// @details Keys are channel IDs, values are RawChannel objects
      /// that store the associated data from the PMTData tree.
      std::map<int, annie::RawChannel> channels_;
  };
}

#endif
