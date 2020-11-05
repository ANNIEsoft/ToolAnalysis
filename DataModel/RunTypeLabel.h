// Enum class that represents the type of run
//#pragma once

#ifndef RUNTYPELABEL_H
#define RUNTYPELABEL_H

#include <fstream>
#include <sstream>
#include <string>

// TODO: update this to allow for all known run types
// this is currently (almost) exactly the same as MinibufferLabel, but runtypes for phase II are likely
// to change and/or be extended - e.g. we probably want a better label than "Source"
// (previously known as "Calibration" in MinibufferLabel) since we will have multiple calibration sources.
enum class RunTypeEnum : uint8_t { testing = 0u, LED, Soft, Beam, Cosmic, Source, Hefty, HeftySource };

// Operator for printing a description of the minibuffer label to a
// std::ostream
inline std::ostream& operator<<(std::ostream& out, const RunTypeEnum& mbl)
{
  switch (mbl) {
    case RunTypeEnum::LED:
      out << "LED";
      break;
    case RunTypeEnum::Soft:
      out << "Soft";
      break;
    case RunTypeEnum::Beam:
      out << "Beam";
      break;
    case RunTypeEnum::Cosmic:
      out << "Cosmic";
      break;
    case RunTypeEnum::Source:
      out << "Source";
      break;
    case RunTypeEnum::Hefty:
      out << "Hefty";
      break;
    case RunTypeEnum::HeftySource:
      out << "HeftySource";
      break;
    default:
      out << "Testing";
      break;
  }
  return out;
}

inline std::string runtype_to_string(const RunTypeEnum& mbl){
  std::stringstream temp_stream;
  temp_stream << mbl;
  return temp_stream.str();
}

#endif
