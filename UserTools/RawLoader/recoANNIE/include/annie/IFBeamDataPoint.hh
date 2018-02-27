#pragma once

// standard library includes
#include <string>

/// @brief Container to hold values from IF beam database queries, together
/// with their associated units
// TODO: consider making this a member struct of the IFBeamDBInterface class
// TODO: move this into the annie namespace
struct IFBeamDataPoint {
  IFBeamDataPoint() : value(0.), unit("") {}
  IFBeamDataPoint(double Value, const std::string& Unit)
    : value(Value), unit(Unit) {}
  double value;
  std::string unit;
};
