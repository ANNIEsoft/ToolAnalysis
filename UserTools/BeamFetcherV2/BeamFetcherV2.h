// Tool to download beam information from the Intensity Frontier
// beam database and save it to the CStore. Modified from the original BeamFetcher
//
// Steven Gardiner <sjgardiner@ucdavis.edu>
// Andrew Sutton <asutton@fnal.gov>

#pragma once

// standard library includes
#include <iostream>
#include <string>

// Boost includes
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

// ToolAnalysis includes
#include "Tool.h"

// ROOT includes
#include "TFile.h"
#include "TTree.h"


class BeamFetcherV2: public Tool {

  public:
    BeamFetcherV2();
    bool Initialise(std::string configfile, DataModel& data);
    bool Execute();
    bool Finalise();

  protected:
    bool FetchFromTrigger();    
    bool SaveToFile();

    void SetupROOTFile();
    void WriteTrees();
    void SaveROOTFile();


    // Holder for the retrieved data and the stuff we'll save
    std::map<uint64_t, std::map<std::string, BeamDataPoint> > fBeamData;
    std::map<uint64_t, std::map<std::string, BeamDataPoint> > fBeamDataToSave;

    // For saving out to a file
    std::map<int, std::pair<uint64_t, uint64_t> > fBeamDBIdx;

    // Holder for the devices we're going to look up
    std::vector<std::string> fDevices;

    // For ROOT file
    TFile *fOutFile;
    TTree *fOutTree;
    uint64_t fTimestamp;
    double fValues[100];
    std::map<std::string, int> fDevIdx; // map from device string to idx in fValues

    
    // Configuration variables
    int verbosity;
    bool fIsBundle;
    bool fSaveROOT;
    std::string fDevicesFile;
    std::string fOutFileName;
    uint64_t fChunkStepMSec;

    // Verbosity things
    int v_error   = 0;
    int v_warning = 1;
    int v_message = 2;
    int v_debug   = 3;
    int vv_debug  = 4;
    std::string logmessage;
};
