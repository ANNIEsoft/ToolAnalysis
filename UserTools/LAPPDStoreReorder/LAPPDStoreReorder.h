#ifndef LAPPDStoreReorder_H
#define LAPPDStoreReorder_H

#include <string>
#include <iostream>

#include "Tool.h"
#include <bitset>
#include "Geometry.h"


using namespace std;
/**
* \class LAPPDStoreReorder
*
* Do LAPPD data read in from BoostStore
*
* $Author:  $
* $Date:  $
* Contact: 
*/
class LAPPDStoreReorder: public Tool {


 public:

  LAPPDStoreReorder(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.

void CleanDataObjects();
bool DoReorder();
bool ConstructTimestampsFromMeta();
 private:

  //**************************** This tool, control variables ***************************************************
  // (only used in this tool, every thing that is not an data object)
  // Variables that you get from the config file
  int LAPPDReorderVerbosityLevel;
  int NUM_VECTOR_METADATA;
  int delayoffset;
  int GlobalShift;
  bool LoadLAPPDMap;
  // Variables that you need in the tool

  //**************************** LAPPD tool chain, control variables ***************************************************
  //(Will be shared in multiple LAPPD tools to show the state of the tool chain in each loop)
  // Variables that you get from the config file
  int LAPPDchannelOffset;
  string InputWavLabel;
  string OutputWavLabel;

  // Variables that you need in the tool
  bool LAPPDana;

  //**************************** LAPPD tool chain, data variables ***************************************************
  // (Will be used in multiple LAPPD tools)
  // everything you get or set to Store, which means it may be used in other tools or it's from other tools
  std::map<unsigned long,vector<Waveform<double>>> reordereddata;
  std::map<unsigned long,vector<Waveform<double>>> lappddata;
  vector<unsigned short> acdcmetadata;
  vector<int> NReadBoards;
  vector<int> ACDCReadedLAPPDID;
  vector<unsigned int> tcounters;

  //**************************** This tool, data variables ***************************************************
  // (only used in this tool, every thing that is an data object)
  // data variables don't need to be cleared in each loop
    Geometry* _geom;

};


#endif
