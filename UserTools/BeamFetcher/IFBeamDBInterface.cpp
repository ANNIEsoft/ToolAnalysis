// standard library includes
#include <algorithm>
#include <array>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <sstream>

// ToolAnalysis includes
#include "ANNIEconstants.h"
#include "IFBeamDBInterface.h"

IFBeamDBInterface::IFBeamDBInterface()
{
  fCurl = curl_easy_init();
  if (!fCurl) throw std::runtime_error("IFBeamDBInterface failed to"
    " initialize libcurl");
}

IFBeamDBInterface::~IFBeamDBInterface()
{
  curl_easy_cleanup(fCurl);
}

////////////////////////////////////////////////////////////////////////////////
const IFBeamDBInterface& IFBeamDBInterface::Instance() {

  // Create the IFBeamDBInterface using a static variable. This ensures that
  // the singleton instance is only created once.
  static std::unique_ptr<IFBeamDBInterface> the_instance(
    new IFBeamDBInterface());

  // Return a reference to the singleton instance
  return *the_instance;
}

////////////////////////////////////////////////////////////////////////////////
std::map<uint64_t, std::map<std::string, BeamDataPoint> >
  IFBeamDBInterface::QueryBeamDBSingleSpan(std::string device, uint64_t t0, uint64_t t1)
  const
{
  std::stringstream url_stream;
  url_stream << "http://ifb-data.fnal.gov:8089/ifbeam/data/data?e=e,1d&v=";
  url_stream << device;
  url_stream << std::fixed << std::setprecision(3) << (t0 - 1)/1000.;
  url_stream << "&t1=" << (t1 + 1)/1000.;
  url_stream << "&f=csv";

  std::string response;
  int code = RunQuery(url_stream, response);

  if (code != CURLE_OK) throw std::runtime_error("Error accessing"
    " IF beam database. Please check your internet connection.");

  auto beam_data = ParseDBResponseSingleSpan(response);

  return beam_data;
}

////////////////////////////////////////////////////////////////////////////////
std::map<uint64_t, std::map<std::string, BeamDataPoint> >
  IFBeamDBInterface::QueryBeamDBBundleSpan(std::string bundle, uint64_t t0, uint64_t t1)
  const
{
  std::stringstream url_stream;
  url_stream << "http://ifb-data.fnal.gov:8089/ifbeam/data/data?e=e,1d&b=";
  url_stream << bundle;
  url_stream << "&t0="; 
  url_stream << std::fixed << std::setprecision(3) << (t0 - 1)/1000.;
  url_stream << "&t1=" << (t1 + 1)/1000.;
  url_stream << "&f=csv";

  std::string response;
  int code = RunQuery(url_stream, response);

  if (code != CURLE_OK) throw std::runtime_error("Error accessing"
    " IF beam database. Please check your internet connection.");

  auto beam_data = ParseDBResponseBundleSpan(response);

  return beam_data;
}

////////////////////////////////////////////////////////////////////////////////
std::map<uint64_t, std::map<std::string, BeamDataPoint> >
  IFBeamDBInterface::QueryBeamDBSingle(std::string device, uint64_t time) const
{
  std::stringstream url_stream;
  url_stream << "http://ifb-data.fnal.gov:8089/ifbeam/data/data?e=e,1d&v=";
  url_stream << device;
  url_stream << "&t=" << std::fixed << std::setprecision(3) << time/1000.;
  url_stream << "&f=xml";

  std::string response;
  int code = RunQuery(url_stream, response);

  if (code != CURLE_OK) throw std::runtime_error("Error accessing"
    " IF beam database. Please check your internet connection.");

  auto beam_data = ParseDBResponseSingle(response);

  return beam_data;
}

////////////////////////////////////////////////////////////////////////////////
std::map<uint64_t, std::map<std::string, BeamDataPoint> >
  IFBeamDBInterface::QueryBeamDBBundle(std::string bundle, uint64_t time) const
{
  std::stringstream url_stream;
  url_stream << "http://ifb-data.fnal.gov:8089/ifbeam/data/data?e=e,1d&b=";
  url_stream << bundle;
  url_stream << "&t=" << std::fixed << std::setprecision(3) << time/1000.;
  url_stream << "&f=csv";

  std::string response;
  int code = RunQuery(url_stream, response);

  if (code != CURLE_OK) throw std::runtime_error("Error accessing"
    " IF beam database. Please check your internet connection.");

  auto beam_data = ParseDBResponseBundle(response);

  return beam_data;
}

int IFBeamDBInterface::RunQuery(const std::stringstream &url_stream, std::string &response_string)
  const
{
  if (!fCurl) {
    throw std::runtime_error("IFBeamDBInterface::RunQuery() called"
      " without inititalizing libcurl");
    return -1;
  }

  //std::cout << "IFBeamDBInterface: sending the following query: " << url_stream.str() << std::endl;
  
  curl_easy_setopt(fCurl, CURLOPT_URL, url_stream.str().c_str());

  response_string.clear();

  curl_easy_setopt(fCurl, CURLOPT_WRITEFUNCTION,
    static_cast<size_t(*)(char*, size_t, size_t, std::string*)>(
      [](char* ptr, size_t size,
        size_t num_members, std::string* data) -> size_t
      {
        data->append(ptr, size * num_members);
        return size * num_members;
      }
    )
  );

  curl_easy_setopt(fCurl, CURLOPT_WRITEDATA, &response_string);

  int code = curl_easy_perform(fCurl);

  // Check the HTTP response code from the IF beam database server. If
  // it's not 200, then throw an exception (something went wrong).
  // For more information about the possible HTTP status codes, see
  // this Wikipedia article: http://tinyurl.com/8yqvhwf
  long http_response_code;
  constexpr long HTTP_OK = 200;
  curl_easy_getinfo(fCurl, CURLINFO_RESPONSE_CODE, &http_response_code);
  if (http_response_code != HTTP_OK) {
    throw std::runtime_error("HTTP error (code "
      + std::to_string(http_response_code) + ") encountered while querying"
      " the IF beam database");
  }

  return code;
}

// Here's some documentation for some of the parameters stored in the beam
// database. It's taken from the MicroBooNE operations wiki:
// http://tinyurl.com/z3c4mxs
//
// The status page shows the present reading of beamline instrumentation. All
// of this data is being stored to IF beam Database. The "IF Beam DB
// dashboard":http://dbweb4.fnal.gov:8080/ifbeam/app/BNBDash/index provides
// another view of beam data. Some of it is redundant to the status page, but
// it verifies that the data is being stored in the database. At present the
// page shows following devices:
//   * TOR860, TOR875 - two toroids in BNB measuring beam intensity. TOR860 is
//     at the beginning of the beamline, and TOR875 is at the end.
//   * THCURR - horn current
//   * HWTOUT - horn water temperature coming out of the horn.
//   * BTJT2 - target temperature
//   * HP875, VP875 - beam horizontal and vertical positions at the 875
//     location, about 4.5 m upstream of the target center.
//   * HPTG1, VPTG1 - beam horizontal and vertical positions immediately
//     (about 2.5 m) upstream of the target center.
//   * HPTG2, VPTG2 - beam horizontal and vertical positions more immediately
//     (about 1.5 m) upstream of the target center.
//   * Because there are no optics between H/VP875 and H/VPTG2, the movements
//     on these monitors should scale with the difference in distances.
//   * BTJT2 - target air cooling temperature. Four RTD measure the return
//     temperature of the cooling air. This is the one closest to the target.
//   * BTH2T2 - target air cooling temperature. This is the temperature of the
//     air going into the horn.

////////////////////////////////////////////////////////////////////////////////
std::map<uint64_t, std::map<std::string, BeamDataPoint> >
IFBeamDBInterface::ParseDBResponseSingleSpan(const std::string& response) const
{
  // Create an empty map to store the parsed data.
  std::map<uint64_t, std::map<std::string, BeamDataPoint> > retMap;

  // Use a stringstream to parse the data
  std::istringstream response_stream(response);

  // Temporary storage for the comma-separated fields in the database response
  // string
  std::string line, junk, data_type, time_string, unit, val_string;
  uint64_t timestamp;

  // Holder for the earliest TS in a chunk
  // We will roll this forward when a TS 60 ms later comes in
  // (the max rate of $1D is 15 Hz or 66 ms)  
  uint64_t earlyTS = 0;

  // Skip the first line (which gives textual column headers)
  std::getline(response_stream, line, '\n');

  while (std::getline(response_stream, line, '\n')) {
    std::stringstream ss(line);

    std::getline(ss, junk, ',');
    std::getline(ss, data_type, ',');
    std::getline(ss, time_string, ',');
    std::getline(ss, unit, ',');
    std::getline(ss, val_string);

    timestamp = std::stoull(time_string);
    if (timestamp - earlyTS > 60) earlyTS = timestamp;
    
    retMap[earlyTS][data_type] = BeamDataPoint(std::stod(val_string),
					       unit,
					       timestamp);
  }
  
  return retMap;
}

////////////////////////////////////////////////////////////////////////////////
std::map<uint64_t, std::map<std::string, BeamDataPoint> >
IFBeamDBInterface::ParseDBResponseBundleSpan(const std::string& response) const
{
  // Create an empty map to store the parsed data.
  std::map<uint64_t, std::map<std::string, BeamDataPoint> > retMap;

  // Use a stringstream to parse the data
  std::istringstream response_stream(response);

  // Temporary storage for the comma-separated fields in the database response
  // string
  std::string line, junk, data_type, time_string, unit, val_string;
  uint64_t timestamp;

  // Holder for the earliest TS in a chunk
  // We will roll this forward when a TS 60 ms later comes in
  // (the max rate of $1D is 15 Hz or 66 ms)  
  uint64_t earlyTS = 0;


  // Skip the first line (which gives textual column headers)
  std::getline(response_stream, line, '\n');

  while (std::getline(response_stream, line, '\n')) {
    std::stringstream ss(line);

    std::getline(ss, time_string, ',');
    std::getline(ss, data_type, ',');
    std::getline(ss, unit, ',');
    std::getline(ss, val_string);

    timestamp = std::stoull(time_string);
    if (timestamp - earlyTS > 60) earlyTS = timestamp;
    
    retMap[earlyTS][data_type] = BeamDataPoint(std::stod(val_string),
					       unit,
					       timestamp);
  }
  
  return retMap;
}

////////////////////////////////////////////////////////////////////////////////
std::map<uint64_t, std::map<std::string, BeamDataPoint> >
IFBeamDBInterface::ParseDBResponseSingle(const std::string& response) const
{
  std::map<uint64_t, std::map<std::string, BeamDataPoint> > retMap;
 
  // Use a stringstream to parse the data
  std::istringstream response_stream(response);

  // Temporary storage for the comma-separated fields in the database response
  // string
  std::string line, junk, data_type, time_string, unit, val_string;
  uint64_t timestamp;

  // This is a bit less necessary here,
  // but it keeps all the points in the same timestamp map element
  // Holder for the earliest TS in a chunk
  // We will roll this forward when a TS 60 ms later comes in
  // (the max rate of $1D is 15 Hz or 66 ms)  
  uint64_t earlyTS = 0;

  // Skip the first line (which gives textual column headers)
  std::getline(response_stream, line, '\n');

  // The second line has all the info, but also a load of crap
  std::getline(response_stream, line, '\n');
  std::stringstream ss(line);
  std::getline(ss, junk, '"');
  std::getline(ss, data_type, '"');
  std::getline(ss, junk, '"'); std::getline(ss, junk, '"'); std::getline(ss, junk, '"');
  std::getline(ss, unit, '"');
  std::getline(ss, junk, '"'); std::getline(ss, junk, '"'); std::getline(ss, junk, '"');
  std::getline(ss, time_string, '"');
  std::getline(ss, junk, '>');
  std::getline(ss, val_string, '\n');
  // we can ignore the remaining lines and finish up

  timestamp = std::stoull(time_string);
  if (timestamp - earlyTS > 60) earlyTS = timestamp;
    
  retMap[earlyTS][data_type] = BeamDataPoint(std::stod(val_string),
					     unit,
					     timestamp);
  
  return retMap;
}

////////////////////////////////////////////////////////////////////////////////
std::map<uint64_t, std::map<std::string, BeamDataPoint> >
IFBeamDBInterface::ParseDBResponseBundle(const std::string& response) const
{
  std::map<uint64_t, std::map<std::string, BeamDataPoint> > retMap;
  
  // Use a stringstream to parse the data
  std::istringstream response_stream(response);

  // Temporary storage for the comma-separated fields in the database response
  // string
  std::string line, junk, data_type, time_string, unit, val_string;
  uint64_t timestamp;

  // This is a bit less necessary here,
  // but it keeps all the points in the same timestamp map element
  // Holder for the earliest TS in a chunk
  // We will roll this forward when a TS 60 ms later comes in
  // (the max rate of $1D is 15 Hz or 66 ms)  
  uint64_t earlyTS = 0;

  // Skip the first line (which gives textual column headers)
  std::getline(response_stream, line, '\n');

  while (std::getline(response_stream, line, '\n')) {
    std::stringstream ss(line);

    std::getline(ss, data_type, ',');
    std::getline(ss, junk, ',');
    std::getline(ss, time_string, ',');
    std::getline(ss, unit, ',');
    std::getline(ss, val_string);

    timestamp = std::stoull(time_string);
    if (timestamp - earlyTS > 60) earlyTS = timestamp;
    
    retMap[earlyTS][data_type] = BeamDataPoint(std::stod(val_string),
					       unit,
					       timestamp);
  }
  
  return retMap;
}


// ////////////////////////////////////////////////////////////////////////////////
// BeamStatus IFBeamDBInterface::GetBeamStatus(uint64_t time) const
// {
//   constexpr uint64_t TIME_OFFSET = 5000ull; // ms

//   // The exact time given by the user might not appear in the database, so
//   // look for entries on the interval [time - TIME_OFFSET, time + TIME_OFFSET]
//   uint64_t t0 = time - TIME_OFFSET;
//   uint64_t t1 = time + TIME_OFFSET;

//   // Ask the beam database for information about the time window of interest.
//   // If there are any problems, then return a bogus BeamStatus object.
//   try {

//     auto beam_data = QueryBeamDB(t0, t1);

//     // TODO: remove hard-coded device name here
//     // Get protons-on-target (POT) information from the parsed data
//     const auto& pot_map = beam_data.at("E:TOR875");

//     // Find the POT entry with the closest time to that requested by the
//     // user, and use it to create the BeamStatus object that will be returned
//     const auto low = pot_map.lower_bound(time);

//     if (low == pot_map.cend()) {
//       std::cerr << "WARNING: IF beam database did not have any information"
//         " for " << time << " ms after the Unix epoch\n";

//       return BeamStatus(TimeClass(time * MILLION), 0., BeamCondition::Missing);
//     }

//     else if (low == pot_map.cbegin()) {
//       // TODO: add checks for BeamCondition::Bad here?
//       return BeamStatus(TimeClass( low->first * MILLION ), low->second.value,
//         BeamCondition::Ok);
//     }

//     // We're between two time values, so we need to figure out which is closest
//     // to the value requested by the user
//     else {

//       auto prev = low;
//       --prev;

//       if ((time - prev->first) < (low->first - time)) {
//         // TODO: add checks for BeamCondition::Bad here?
//         return BeamStatus(TimeClass( prev->first * MILLION ),
//           prev->second.value, BeamCondition::Ok);
//       }
//       else {
//         // TODO: add checks for BeamCondition::Bad here?
//         return BeamStatus(TimeClass( low->first * MILLION ), low->second.value,
//           BeamCondition::Ok);
//       }
//     }

//   }

//   catch (const std::exception& e) {
//     std::cerr << "WARNING: problem encountered while querying IF beam"
//       " database:\n  " << e.what() << '\n';

//     // Return a default-constructed BeamStatus object since there was a
//     // problem. The fOk member will be set to false by default, which will
//     // flag the object as problematic.
//     return BeamStatus(TimeClass( time ), 0., BeamCondition::Missing);
//   }

// }
