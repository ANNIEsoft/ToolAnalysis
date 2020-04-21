#ifndef MCARDDATA_H
#define MCCARDDATA_H

#include <vector>
#include "ANNIEconstants.h"
#include "TROOT.h"
#include <limits>  

namespace{
//  constexpr int BOGUS_INT = std::numeric_limits<int>::max();
  //std::numeric_limits<double>::quiet_NaN()
  constexpr uint64_t BOGUS_UINT64 = std::numeric_limits<uint64_t>::max();
  constexpr uint32_t BOGUS_UINT32 = std::numeric_limits<uint32_t>::max();
  constexpr uint16_t BOGUS_UINT16 = std::numeric_limits<uint16_t>::max();
}

class MCCardData{
  public:
  uint64_t LastSync;                    // ??? clock ticks of last GPS sync pulse FIXME
  int SequenceID;                       // readout id. incremented once every 40 triggers
  int StartTimeSec;                     // unix epoch timestamp at StartCount. FIXME corresponding to when?
  int StartTimeNSec;                    // above, ns part FIXME
  uint64_t StartCount;                  // ADC counts from powerup when ... FIXME
  //uint64_t* triggerCounts;            // --
  std::vector<ULong64_t> TriggerCounts; // ADC ticks of the start of each minibuffer in the Full buffer? FIXME
  //uint32_t* Rates;                    // --
  std::vector<UInt_t> Rates;            // Avg pulse rate? units? size is number of channels.
  int CardID;                           // card position in VME crate
  int TriggerNumber;                    // number of minibuffers. 40 for Hefty mode. 
  int Channels;                         // num channels on card
  int BufferSize;                       // datapoints per channel in a full buffer.
  int Eventsize;                        // samples per minibuffer per channel divided by 4.
  int FullBufferSize;                   // Channels * BufferSize. corresponds to 80us.
  //uint16_t* Data;                     // Concatenated data from all channels on card. Size FullBufferSize.
  std::vector<uint16_t> Data;
  // With a fullbuffer of 160k and 4 channels, there's 40k samples per channel, 
  // and with 40 minibuffers per fullbuffer, that's 1k samples per minibuffer per channel.
  // At 2ns per sample that's 2000ns = 2us per minibuffer.
  
  MCCardData(){ /*Reset();*/ }            // cannot reset in constructor: need to set e.g. TriggerNumber first
  void Reset();
  
};

#endif
