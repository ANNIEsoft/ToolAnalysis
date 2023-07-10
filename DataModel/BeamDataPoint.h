#pragma once
// Container used to store IF beam database information in a Store
// for easy offline retrieval
//
// Steven Gardiner <sjgardiner@ucdavis.edu>

// standard library includes
#include <iostream>
#include <map>
#include <string>

// ToolAnalysis includes
#include "SerialisableObject.h"

/// @brief Container to hold values from Intensity Frontier beam database
/// queries, together with their associated units
struct BeamDataPoint : public SerialisableObject {

  friend class boost::serialization::access;

  BeamDataPoint() : value(0.), unit(""), time(0) {}
  BeamDataPoint(double Value, const std::string& Unit, uint64_t time=0)
    : value(Value), unit(Unit), time(0) {}
  double value;
  std::string unit;
  uint64_t time;

  template<class Archive> void serialize(Archive & ar,
    const unsigned int version)
  {
    ar & value;
    ar & unit;
    ar & time;
  }

  virtual bool Print() override {
    std::cout << "Value : " << value << '\n';
    std::cout << "Unit  : " << unit  << '\n';
    std::cout << "Time  : " << time  << '\n';
    return true;
  }

};
