#include "ADCPulse.h"

// Note that the value of start_time is duplicated between the member variables
// Time and start_time.
// TODO: think about changing this
// TODO: consider adding the pulse end time as a data member for this class
ADCPulse::ADCPulse(int TubeId, double start_time, double peak_time,
  double baseline, double sigma_baseline, unsigned long area,
  unsigned short raw_amplitude, double calibrated_amplitude,
  double charge) : Hit(TubeId, start_time, charge),
  start_time_(start_time), peak_time_(peak_time),
  baseline_(baseline), sigma_baseline_(sigma_baseline), raw_area_(area),
  raw_amplitude_(raw_amplitude), calibrated_amplitude_(calibrated_amplitude)
{
}
