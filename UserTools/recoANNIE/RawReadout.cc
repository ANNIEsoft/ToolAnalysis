// reco-annie includes
#include "RawReadout.h"

void annie::RawReadout::add_card(int CardID, unsigned long long LastSync,
  int StartTimeSec, int StartTimeNSec, unsigned long long StartCount,
  int Channels, int BufferSize, int MiniBufferSize,
  const std::vector<unsigned short>& FullBufferData,
  const std::vector<unsigned long long>& TriggerCounts,
  const std::vector<unsigned int>& Rates, bool overwrite_ok)
{
  auto iter = cards_.find(CardID);
  if ( iter != cards_.end() ) {
    if (!overwrite_ok) throw std::runtime_error("RawCard overwrite"
      " attempted in annie::RawReadout::add_card()");
    else cards_.erase(iter);
  }

  cards_.emplace( std::make_pair(CardID,
    annie::RawCard(CardID, LastSync, StartTimeSec,
    StartTimeNSec, StartCount, Channels, BufferSize, MiniBufferSize,
    FullBufferData, TriggerCounts, Rates)) );
}
