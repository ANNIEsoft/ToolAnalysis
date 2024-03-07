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

  BeamDataPoint() : value(0.), unit(""), time(1) {}
  BeamDataPoint(double Value, const std::string& Unit, uint64_t time=1)
    : value(Value), unit(Unit), time(time) {
    std::cout << "BDP - value: " << value << ", unit: " << unit << ", time: " << time << std::endl;}
  double value;
  std::string unit;
  uint64_t time;
    
  template<class Archive> void serialize(Archive & ar,
    const unsigned int version)
  {
    ar & value;
    ar & unit;
    if (version > 0)
      ar & time;
  }

  virtual bool Print() override {
    std::cout << "Value : " << value << '\n';
    std::cout << "Unit  : " << unit  << '\n'
    std::cout << "Time  : " << time  << '\n';
    return true;
  }

};

// Need to increment the class version since we added time as a new variable
// the version number ensures backward compatibility when serializing 
BOOST_CLASS_VERSION(BeamDataPoint, 1)







