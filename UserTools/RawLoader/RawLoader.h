#pragma once

// standard library includes
#include <iostream>
#include <memory>
#include <string>

// ToolAnalysis includes
#include "Tool.h"

// recoANNIE includes
#include "annie/RawReader.hh"

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
