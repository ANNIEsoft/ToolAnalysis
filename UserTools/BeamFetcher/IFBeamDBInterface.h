#pragma once
// Singleton class used by the BeamFetcher tool to communicate with the
// Fermilab Intensity Frontier beam database
// (see http://ifb-data.fnal.gov:8100/ifbeam/data)
//
// Steven Gardiner (sjgardiner@ucdavis.edu)

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
class IFBeamDBInterface {

  public:

    /// @brief Clean up libcurl stuff as the IFBeamDBInterface object is
    /// destroyed
    ~IFBeamDBInterface();

    /// @brief Deleted copy constructor
    IFBeamDBInterface(const IFBeamDBInterface&) = delete;

    /// @brief Deleted move constructor
    IFBeamDBInterface(IFBeamDBInterface&&) = delete;

    /// @brief Deleted copy assignment operator
    IFBeamDBInterface& operator=(const IFBeamDBInterface&) = delete;

    /// @brief Deleted move assignment operator
    IFBeamDBInterface& operator=(IFBeamDBInterface&&) = delete;

    /// @brief Get a const reference to the singleton instance of the
    /// IFBeamDBInterface
    static const IFBeamDBInterface& Instance();

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
    std::map<std::string, std::map<uint64_t, BeamDataPoint> >
      QueryBeamDB(uint64_t t0, uint64_t t1) const;

    /// @brief Get information about the BNB state from the database for the
    /// time interval [t0, t1]
    /// @param t0 Starting timestamp (milliseconds since the Unix epoch)
    /// @param t1 Starting timestamp (milliseconds since the Unix epoch)
    /// @param[out] response_string A string that will be loaded with the
    /// csv-format data from the database
    /// @return The libcurl integer return code for the query (zero if
    /// everything worked ok, or nonzero if an error occurred)
    int QueryBeamDB(uint64_t t0, uint64_t t1,
      std::string& response_string) const;

    /// @brief Get information about the BNB state from the database as close
    /// as possible to a given time
    /// @param time Timestamp (milliseconds since the Unix epoch) to use when
    /// searching the database
    BeamStatus GetBeamStatus(uint64_t time) const;

  protected:

    /// @brief Create the singleton IFBeamDBInterface object
    IFBeamDBInterface();

    std::map<std::string, std::map<uint64_t, BeamDataPoint> >
      ParseDBResponse(const std::string& response) const;

    void PostprocessParsedResponse(std::map<std::string,
      std::map<uint64_t, BeamDataPoint> >& parsed_response) const;

    /// @brief Pointer used to interact with libcurl
    CURL* fCurl = nullptr;
};
