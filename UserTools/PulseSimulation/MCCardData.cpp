#include "MCCardData.h"

void MCCardData::Reset(){
  SequenceID = BOGUS_INT;
  CardID = BOGUS_INT;
  LastSync = BOGUS_UINT64;
  StartTimeSec = BOGUS_INT;
  StartTimeNSec = BOGUS_INT;
  StartCount = BOGUS_UINT64;
  TriggerCounts.assign(TriggerNumber,BOGUS_UINT64);
  Rates.assign(Channels,BOGUS_UINT32);
  Data.assign(FullBufferSize,0); //BOGUS_UINT16); // makes it awkward to view partially filled buffers
}
