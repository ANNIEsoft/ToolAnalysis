#ifndef DataDecoder_H
#define DataDecoder_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "CardData.h"
#include "TriggerData.h"

/**
 * \class DataDecoder
 *
 This tool is used to decode the binary raw data and construct the ANNIEEvent
 boost stores.
 
 *
* $Author: Teal Pershing $
* $Date: 2019/05/28 10:44:00 $
* Contact: tjpershing@ucdavis.edu 
*/


struct DecodedFrame{
  int frameid;
  bool has_recordheader;
  std::vector<uint16_t> samples;
  ~DecodedFrame(){
  }
};

class DataDecoder: public Tool {


 public:

  DataDecoder(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
  std::vector<DecodedFrame> UnpackFrames(std::vector<uint32_t> bank);

 private:

  BoostStore* VMEData;
  //BoostStore* PMTData;
  //BoostStore* TriggerData;

  std::string InputFile;


  /// Let's start by filling maps based on the MTC counters
   
  /// \brief verbosity levels: if 'verbosity' < this level, the message type will be logged.
  int verbosity;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;

};


#endif
