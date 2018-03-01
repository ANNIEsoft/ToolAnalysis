// recoANNIE includes
#include "RawTrigData.h"

annie::RawTrigData::RawTrigData(int FirmwareVersion, int FIFOOverflow,
  int DriverOverflow, const std::vector<unsigned short>& EventIDs,
  const std::vector<unsigned long long>& EventTimes,
  const std::vector<unsigned int>& TriggerMasks,
  const std::vector<unsigned int>& TriggerCounters)
  : firmware_version_(FirmwareVersion), fifo_overflow_(FIFOOverflow),
  driver_overflow_(DriverOverflow), event_IDs_(EventIDs),
  event_times_(EventTimes), trigger_masks_(TriggerMasks),
  trigger_counters_(TriggerCounters)
{
}
