#include "annie/BeamStatus.hh"

annie::BeamStatus::BeamStatus() : time_(0), pot_(0), ok_(false)
{}

annie::BeamStatus::BeamStatus(unsigned long long time, double POT, bool ok)
  : time_(time), pot_(POT), ok_(ok)
{}

void annie::BeamStatus::clear()
{
  time_ = 0;
  pot_ = 0.;
  ok_ = false;
}
