// RawLoader tool
//
// Loads objects in the ANNIEEvent store with data from an event in an
// ANNIE Phase I raw data file. Uses code borrowed from the previous recoANNIE
// framework.
//
// Steven Gardiner <sjgardiner@ucdavis.edu>
#pragma once

// standard library includes
#include <iostream>
#include <memory>
#include <string>

// ToolAnalysis includes
#include "Tool.h"

// recoANNIE includes
#include "RawReader.h"

class RawLoader : public Tool {

 public:

  RawLoader();
  bool Initialise(const std::string config_file, DataModel& data) override;
  bool Execute() override;
  bool Finalise() override;

 protected:

  // Helper object used to load the raw data from the ROOT file
  std::unique_ptr<annie::RawReader> m_reader;
};
