#ifndef GetLAPPDEvents_H
#define GetLAPPDEvents_H

#include <string>
#include <iostream>
#include <fstream>

#include "Tool.h"


/**
 * \class GetLAPPDEvents
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class GetLAPPDEvents: public Tool {


 public:

  GetLAPPDEvents(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

  bool InitializeNewFile();

 private:

  std::vector<std::string> OrganizedFileList;
  std::string CurrentFile = "NONE";
  std::string InputFile;
  std::string OutputFile;
  std::string OutputFileLAPPD;

  std::vector<std::string> OrganizeRunParts(std::string InputFile); //Parses all run files in InputFile and returns a vector of file paths organized by part

  int FileNum = 0;
  bool FileCompleted = false;

  std::ofstream out_file;
  std::ofstream out_file_lappd;

  BoostStore *RawData = nullptr;
  BoostStore *LAPPDData = nullptr;
  BoostStore *PMTData = nullptr;
  BoostStore *MRDData = nullptr;
  BoostStore *TrigData = nullptr;

  int verbosity;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  int vv_debug=4;
  std::string logmessage;
};


#endif
