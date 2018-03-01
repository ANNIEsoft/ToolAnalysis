#include "RecoPulse.h"

annie::RecoPulse::RecoPulse(size_t start_time, size_t peak_time,
  double baseline, double sigma_baseline, unsigned long area,
  unsigned short raw_amplitude, double calibrated_amplitude,
  double charge) : start_time_(start_time), peak_time_(peak_time),
  baseline_(baseline), sigma_baseline_(sigma_baseline), raw_area_(area),
  raw_amplitude_(raw_amplitude), calibrated_amplitude_(calibrated_amplitude),
  charge_(charge)
{
}
