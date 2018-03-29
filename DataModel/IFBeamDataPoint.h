#pragma once
// Container used to store IF beam database information in a ROOT file
// for easy offline retrieval
//
// Steven Gardiner <sjgardiner@ucdavis.edu>

// standard library includes
#include <map>
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
