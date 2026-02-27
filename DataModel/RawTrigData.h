// Class that collects the contents of the raw TrigData tree from a single
// DAQ readout
//
// Steven Gardiner <sjgardiner@ucdavis.edu>

#ifndef RAWTRIGDATA_H
#define RAWTRIGDATA_H

#include <vector>

namespace annie {

  class RawTrigData {

    public:

      RawTrigData() {}

      RawTrigData(int FirmwareVersion, int FIFOOverflow, int DriverOverflow,
        const std::vector<unsigned short>& EventIDs,
        const std::vector<unsigned long long>& EventTimes,
        const std::vector<unsigned int>& TriggerMasks,
        const std::vector<unsigned int>& TriggerCounters);

      inline int firmware_version() const { return firmware_version_; }
      inline int fifo_overflow() const { return fifo_overflow_; }
      inline int driver_overflow() const { return driver_overflow_; }

      inline const std::vector<unsigned short>& event_IDs() const
        { return event_IDs_; }

      inline const std::vector<unsigned long long>& event_times() const
        { return event_times_; }

      inline const std::vector<unsigned int>& trigger_masks() const
        { return trigger_masks_; }

      inline const std::vector<unsigned int>& trigger_counters() const
        { return trigger_counters_; }

    protected:

      int firmware_version_ = 0;
      int fifo_overflow_ = 0;
      int driver_overflow_ = 0;

      std::vector<unsigned short> event_IDs_;
      std::vector<unsigned long long> event_times_;
      std::vector<unsigned int> trigger_masks_;
      std::vector<unsigned int> trigger_counters_;
  };
}

#endif
