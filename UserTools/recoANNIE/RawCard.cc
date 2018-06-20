// standard library includes
#include <limits>

// reco-annie includes
#include "RawCard.h"

// ToolAnalysis includes
#include "ANNIEconstants.h"

namespace {
  // The cards have clock frequencies of 125 MHz, so they take
  // samples every 8 ns.
  constexpr unsigned long long CLOCK_TICK = 8; // ns
}

annie::RawCard::RawCard(int CardID, unsigned long long LastSync,
  int StartTimeSec, int StartTimeNSec, unsigned long long StartCount,
  int Channels, int BufferSize, int MiniBufferSize,
  const std::vector<unsigned short>& Data,
  const std::vector<unsigned long long>& TriggerCounts,
  const std::vector<unsigned int>& Rates) : card_id_(CardID),
  last_sync_(LastSync), start_time_sec_(StartTimeSec),
  start_time_nsec_(StartTimeNSec), start_count_(StartCount),
  trigger_counts_(TriggerCounts)
{
  if (Channels != static_cast<int>(Data.size()) / BufferSize) throw
    std::runtime_error("Mismatch between number of channels and"
    " channel buffer size in annie::RawCard::RawCard()");

  if ( TriggerCounts.size() != static_cast<size_t>(BufferSize
    / MiniBufferSize) )
  {
    throw std::runtime_error("Mismatch between number of minibuffers and"
    " minibuffer size in annie::RawCard::RawCard()");
  }

  for (int c = 0; c < Channels; ++c) add_channel(c, Data, BufferSize,
    Rates.at(c));
}

void annie::RawCard::add_channel(int channel_number,
  const std::vector<unsigned short>& full_buffer_data,
  int channel_buffer_size, unsigned int rate, bool overwrite_ok)
{
  auto iter = channels_.find(channel_number);
  if ( iter != channels_.end() ) {
    if (!overwrite_ok) throw std::runtime_error("RawChannel overwrite"
      " attempted in annie::RawCard::add_channel()");
    else channels_.erase(iter);
  }

  size_t start_index = channel_number*channel_buffer_size;
  size_t end_index = (channel_number + 1)*channel_buffer_size;

  if ( full_buffer_data.size() < end_index ) {
    throw std::runtime_error("Missing data for channel "
      + std::to_string(channel_number) + " encountered in annie::RawCard"
      "::add_channel()");
  }

  // The channel data are stored out of order (half at the beginning and
  // half midway through) so get iterators to both starting locations
  auto channel_begin = full_buffer_data.cbegin() + start_index;
  auto channel_halfway = channel_begin + channel_buffer_size / 2;
  channels_.emplace( std::make_pair(channel_number,
    annie::RawChannel(channel_number, channel_begin, channel_halfway,
    rate, trigger_counts_.size())) );
}

// Compute the nanoseconds since the Unix epoch for the trigger (based on
// the timestamps from this card) corresponding to the given minibuffer
unsigned long long annie::RawCard::trigger_time(size_t minibuffer_index) const
{
  // Start by expressing the start time in nanoseconds. It is stored as a
  // number of seconds and a remainder in nanoseconds.
  unsigned long long time = (static_cast<unsigned long long>(start_time_sec_)
    * BILLION) + static_cast<unsigned long long>(start_time_nsec_);

  // If the last sync value exceeds the start count, then we need to add
  // an offset. Both of these quantities are measured in card clock ticks,
  // so convert the difference to nanoseconds.
  if (last_sync_ > start_count_) {
    time += CLOCK_TICK * (last_sync_ - start_count_);
  }

  // Round the result so far to the nearest second
  time = BILLION * ((time + (BILLION / 2)) / BILLION);

  // Unset the most significant bit of the trigger count. To do this, we notice
  // that long long and unsigned long long have the same size, but long long is
  // signed (and therefore uses the most significant bit as the sign bit).
  unsigned long long masked_trigger_count = trigger_counts_.at(
    minibuffer_index) & ~(1ull << std::numeric_limits<long long>::digits);

  // We need to adjust the time by an offset given by the difference in the
  // trigger count and last sync values (converted to nanoseconds). This offset
  // can be positive or negative, so we'll compute it using a signed integer
  // and then apply it using an unsigned one.
  long long offset = CLOCK_TICK * (masked_trigger_count - last_sync_);

  if (offset < 0) time -= static_cast<unsigned long long>(-offset);
  else time += offset;

  return time;
}
