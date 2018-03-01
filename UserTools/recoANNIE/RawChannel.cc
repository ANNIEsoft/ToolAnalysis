// standard library includes
#include <stdexcept>

// reco-annie includes
#include "RawChannel.h"

// The raw channel data are stored out of order (half at the beginning and half
// midway through the channel buffer) so use an iterator to the start and an
// iterator at the halfway point to put the samples in order.
annie::RawChannel::RawChannel(int ChannelNumber,
  const std::vector<unsigned short>::const_iterator data_begin,
  const std::vector<unsigned short>::const_iterator data_halfway,
  unsigned int Rate, size_t MiniBufferCount) : channel_id_(ChannelNumber),
  rate_(Rate)
{
  size_t half_minibuffer_length = std::distance(data_begin, data_halfway)
    / MiniBufferCount;

  for (size_t mb = 0; mb < MiniBufferCount; ++mb) {
    data_.emplace_back(); // Create a new empty minibuffer

    // Get the starting and ending indices (within the current
    // channel's subbuffer) for the current minibuffer
    size_t start_sample = mb * half_minibuffer_length;
    size_t end_sample = (mb + 1) * half_minibuffer_length;

    for (size_t s = start_sample; s < end_sample; s += 2) {
      data_.back().push_back( *(data_begin + s) );
      data_.back().push_back( *(data_begin + s + 1) );
      data_.back().push_back( *(data_halfway + s) );
      data_.back().push_back( *(data_halfway + s + 1) );
    }
  }
}

const std::vector<unsigned short>& annie::RawChannel::minibuffer_data(
  size_t mb_index) const
{
  if (mb_index >= data_.size()) throw std::runtime_error("MiniBuffer index"
    " out-of-range in annie::RawChannel::minibuffer_data()");

  return data_.at(mb_index);
}
