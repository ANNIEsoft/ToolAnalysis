#pragma once
// Singleton class used by the BeamFetcherV2 tool to communicate with the
// Fermilab Intensity Frontier beam database
// Modified from the original version in the BeamFetcher tool
// (see http://ifb-data.fnal.gov:8100/ifbeam/data)
//
// Steven Gardiner (sjgardiner@ucdavis.edu)
// Andrew Sutton (asutton@fnal.gov)

// standard library includes
#include <map>
#include <string>

// libcurl includes
#include <curl/curl.h>

// ToolAnalysis includes
#include "BeamDataPoint.h"
#include "BeamStatus.h"
/// @brief Singleton used to interact with the Intensity Frontier beam database
/// at Fermilab
class IFBeamDBInterfaceV2 {

  public:

    /// @brief Clean up libcurl stuff as the IFBeamDBInterfaceV2 object is
    /// destroyed
    ~IFBeamDBInterfaceV2();

    /// @brief Deleted copy constructor
    IFBeamDBInterfaceV2(const IFBeamDBInterfaceV2&) = delete;

    /// @brief Deleted move constructor
    IFBeamDBInterfaceV2(IFBeamDBInterfaceV2&&) = delete;

    /// @brief Deleted copy assignment operator
    IFBeamDBInterfaceV2& operator=(const IFBeamDBInterfaceV2&) = delete;

    /// @brief Deleted move assignment operator
    IFBeamDBInterfaceV2& operator=(IFBeamDBInterfaceV2&&) = delete;

    /// @brief Get a const reference to the singleton instance of the
    /// IFBeamDBInterfaceV2
    static const IFBeamDBInterfaceV2& Instance();

    /// @brief Get information about the Booster Neutrino Beam (BNB) state from
    /// the database for the time interval [t0, t1]
    /// @param t0 Starting timestamp (milliseconds since the Unix epoch)
    /// @param t1 Starting timestamp (milliseconds since the Unix epoch)
    /// @return A nested map containing the parsed data. Keys of the outer map
    /// are device names, values are inner maps. Keys of the inner map are
    /// timestamps (milliseconds since the Unix epoch), values are
    /// BeamDataPoint structs that hold a numerical value and a unit string.
    /// @note An easy way to get the current milliseconds since the Unix epoch
    /// is to use the terminal utility date like this: @verbatim date +%s%3N
    /// @endverbatim
    /// I found this handy trick here: http://unix.stackexchange.com/a/123764
    std::map<uint64_t, std::map<std::string, BeamDataPoint> >
      QueryBeamDBSingleSpan(std::string device, uint64_t t0, uint64_t t1) const;

    std::map<uint64_t, std::map<std::string, BeamDataPoint> >
      QueryBeamDBBundleSpan(std::string bundle, uint64_t t0, uint64_t t1) const;

    std::map<uint64_t, std::map<std::string, BeamDataPoint> >
      QueryBeamDBSingle(std::string device, uint64_t time) const;

    std::map<uint64_t, std::map<std::string, BeamDataPoint> >
      QueryBeamDBBundle(std::string bundle, uint64_t time) const;

    int RunQuery(const std::stringstream &url_stream, std::string &response_string) const;

    /// @brief The list of devices required to save in the root tree
    std::map<std::string, std::string> requiredDevices;
    
  protected:
    /// @brief Create the singleton IFBeamDBInterfaceV2 object
    IFBeamDBInterfaceV2();

    std::map<uint64_t, std::map<std::string, BeamDataPoint> >
      ParseDBResponseSingleSpan(const std::string& response) const;

    std::map<uint64_t, std::map<std::string, BeamDataPoint> >
      ParseDBResponseBundleSpan(const std::string& response) const;

    std::map<uint64_t, std::map<std::string, BeamDataPoint> >
      ParseDBResponseSingle(const std::string& response) const;

    std::map<uint64_t, std::map<std::string, BeamDataPoint> >
      ParseDBResponseBundle(const std::string& response) const;

    /// @brief Pointer used to interact with libcurl
    CURL* fCurl = nullptr;
};
