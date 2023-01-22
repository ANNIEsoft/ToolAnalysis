#ifndef LAPPDDataDecoder_H
#define LAPPDDataDecoder_H

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
 * \class LAPPDDataDecoder
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class LAPPDDataDecoder: public Tool {


 public:

  LAPPDDataDecoder(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  int getParsedMeta(std::vector<unsigned short> buffer, int BoardId);
  int LoopThroughPPSData();
  int LoopThroughMetaData();

 private:

  std::string InputFile;
  int EntriesPerExecute;
  int PMTDEntryNum = 0;
  int FileNum = 0;
  int CurrentRunNum;
  int CurrentSubrunNum;
  std::string CurrentFile = "NONE";


  double CLOCK_to_SEC = 3.125e-9;       //320MHz clock -> 1/320MHz = 3.125ns
  double CLOCK_to_NSEC = 3.125;

  vector<unsigned short> Raw_Buffer;
  vector<unsigned short> Parse_Buffer;
  vector<int> BoardId_Buffer;
  std::vector<unsigned short> meta;
  std::vector<unsigned short> pps;
  int channel_count=0;

  bool LAPPDDataBuilt;
  int LAPPDDEntryNum;

  PsecData *Ldata = nullptr;
  
  std::vector<uint64_t> *LAPPDPPS;
  std::vector<uint64_t> *LAPPDTimestamps;
  std::vector<uint64_t>* LAPPDBeamgateTimestamps;
  std::vector<PsecData>* LAPPDPulses;

  int verbosity;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  int vv_debug=4;
  std::string logmessage;

};


#endif
