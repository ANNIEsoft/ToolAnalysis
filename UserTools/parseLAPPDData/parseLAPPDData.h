#ifndef parseLAPPDData_H
#define parseLAPPDData_H

#include <string>
#include <iostream>
#include <bitset>

#include "Tool.h"

#include "PsecData.h"

#define NUM_CH 30
#define NUM_SAMP 256
#define NUM_PSEC 5
#define NUM_VECTOR_DATA 7795
#define NUM_VECTOR_PPS 16



/**
 * \class parseLAPPDData
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class parseLAPPDData: public Tool {


 public:

  parseLAPPDData(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  int getParsedData(vector<unsigned short> buffer, int ch_start);
  int getParsedMeta(vector<unsigned short> buffer, int BoardId);

 private:

  vector<unsigned short> Raw_Buffer;
  vector<unsigned short> Parse_Buffer;
  vector<int> BoardId_Buffer;
  map<unsigned long, vector<Waveform<double>>> LAPPDWaveforms;

  int verbosity;
  int Nboards;

  int channel_count=0;
  map<int, std::vector<unsigned short>> data;
  std::vector<unsigned short> meta;
  std::vector<unsigned short> pps;
  boost::posix_time::ptime first;

  double CLOCK_to_SEC = 3.125e-9;       //320MHz clock -> 1/320MHz = 3.125ns
  double CLOCK_to_NSEC = 3.125;       //320MHz clock -> 1/320MHz = 3.125ns


};


#endif
