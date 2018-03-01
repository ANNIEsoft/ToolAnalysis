// Class that represents a full readout of raw data from a single channel of
// one of the DAQ VME cards.
//
// Steven Gardiner <sjgardiner@ucdavis.edu>
#ifndef RAWCHANNEL_H
#define RAWCHANNEL_H

// standard library includes
#include <vector>

namespace annie {

  class RawChannel {

    public:

      RawChannel() {}

      RawChannel(int ChannelNumber,
        const std::vector<unsigned short>::const_iterator data_begin,
        const std::vector<unsigned short>::const_iterator data_end,
        unsigned int Rate, size_t MiniBufferCount);

      unsigned int channel_id() const { return channel_id_; }
      void set_channel_id( unsigned int cn) { channel_id_ = cn; }

      unsigned int rate() const { return rate_; }
      void set_rate( unsigned int r) { rate_ = r; }

      const std::vector< std::vector<unsigned short> >& data()
        const { return data_; }

      size_t num_minibuffers() const { return data_.size(); }

      const std::vector<unsigned short>& minibuffer_data(size_t mb_index) const;

    protected:

      /// @brief The index of this channel in the full waveform buffer
      /// of its VME card
      unsigned channel_id_;

      /// @brief The rate for this channel
      unsigned rate_;

      /// @brief Raw ADC counts from the full readout for this channel
      /// split into minibuffers
      /// @details The outer index refers to the minibuffer, the inner
      /// index refers to the sample
      std::vector< std::vector<unsigned short> > data_;
  };
}

#endif
