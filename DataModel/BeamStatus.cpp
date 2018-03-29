#include "BeamStatus.h"

BeamStatus::BeamStatus() : time_( TimeClass(0ull) ), pot_(0.),
  condition_(BeamCondition::Missing) {}

BeamStatus::BeamStatus(TimeClass time, double POT,
  BeamCondition condition) : time_(time), pot_(POT), condition_(condition)
{}

void BeamStatus::clear()
{
  time_ = TimeClass(0ull);
  pot_ = 0.;
  condition_ = BeamCondition::Missing;
}
