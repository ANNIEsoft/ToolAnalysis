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

  BeamDataPoint() : value(0.), unit("") {}
  BeamDataPoint(double Value, const std::string& Unit)
    : value(Value), unit(Unit) {}
  double value;
  std::string unit;

  template<class Archive> void serialize(Archive & ar,
    const unsigned int version)
  {
    ar & value;
    ar & unit;
  }

  virtual bool Print() override {
    std::cout << "Value : " << value << '\n';
    std::cout << "Unit : " << unit << '\n';
    return true;
  }

};
