#include "BeamStatus.h"

BeamStatus::BeamStatus() : time_( TimeClass(0ull) ), pot_(0.),
  condition_(BeamCondition::Missing)
{
  serialise = true;
}

BeamStatus::BeamStatus(TimeClass time, double POT,
  BeamCondition condition) : time_(time), pot_(POT), condition_(condition)
{
  serialise = true;
}

void BeamStatus::clear()
{
  time_ = TimeClass(0ull);
  pot_ = 0.;
  condition_ = BeamCondition::Missing;
  data_.clear();
  cuts_.clear();
}

void BeamStatus::add_measurement(const std::string& device_name,
  uint64_t ms_since_epoch, const BeamDataPoint& bdp)
{
  data_[device_name] = std::pair<uint64_t, BeamDataPoint>(ms_since_epoch, bdp);
}

void BeamStatus::add_measurement(const std::string& device_name,
  uint64_t ms_since_epoch, double value, const std::string& unit)
{
  add_measurement(device_name, ms_since_epoch, BeamDataPoint(value, unit));
}
