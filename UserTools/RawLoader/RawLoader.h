// RawLoader tool
//
// Loads objects in the ANNIEEvent store with data from an event in an
// ANNIE Phase I raw data file. Uses code borrowed from the previous recoANNIE
// framework.
//
// Steven Gardiner <sjgardiner@ucdavis.edu>
//#pragma once

#ifndef RAWLOADER_H
#define RAWLOADER_H

// standard library includes
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>

// ToolAnalysis includes
#include "Tool.h"
#include "HeftyInfo.h"
#include "HeftyTreeReader.h"
// ToolAnalysis includes
#include "ANNIEconstants.h"
#include "MinibufferLabel.h"
#include "Waveform.h"

// recoANNIE includes                                                                                                                                                                                   #include "RawCard.h"
#include "RawChannel.h"
#include "RawReadout.h"
#include "RawReader.h"

// Boost includes
#include "boost/bimap.hpp"

class RawLoader : public Tool {

 public:

  RawLoader();
  bool Initialise(const std::string config_file, DataModel& data) override;
  bool Execute() override;
  bool Finalise() override;
  void FillBimap(std::string bimapfile);

 protected:

  // Helper object used to load the raw data from the ROOT file
  std::unique_ptr<annie::RawReader> m_reader;

  // Helper object used to load the Hefty mode timing data from a ROOT file
  std::unique_ptr<annie::HeftyTreeReader> m_hefty_tree_reader;
  
  // bimap between PMT id and DAQ card + channel, read from file specified in config (for now)
  boost::bimap<int, std::pair<int, int> > pmt_ID_and_card_channel_bimap;

  // Flag indicating whether we're processing Hefty mode data (true) or not
  // (false)
  bool m_using_hefty_mode;
};

#endif
