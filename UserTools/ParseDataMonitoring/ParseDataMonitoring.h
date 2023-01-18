#ifndef ParseDataMonitoring_H
#define ParseDataMonitoring_H

#include <string>
#include <iostream>
#include <map>
#include <vector>

#include "Tool.h"

#include "PsecData.h"

#define NUM_CH 30
#define NUM_SAMP 256
#define NUM_PSEC 5

using namespace std;
/**
 *  * \class ParseDataMonitoring
 *   *
 *    * This is a balnk template for a Tool used by the script to generate a new custom tool. Please fill out the descripton and author information.
 *    *
 *    * $Author: M.Breisch, small adaptions for Monitoring by M.Nieslony$
 *    * $Date: 2021/10/19 $
 *    */
class ParseDataMonitoring: public Tool {


 public:

  ParseDataMonitoring(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resorces. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Executre function used to perform Tool perpose. 
  bool Finalise(); ///< Finalise funciton used to clean up resorces.

  int getParsedData(vector<unsigned short> buffer, int ch_start);
  int getParsedMeta(vector<unsigned short> buffer, int BoardId);
  bool ReadPedestals(int boardNo);

 private:

  vector<unsigned short> Raw_Buffer;
  vector<unsigned short> Parse_Buffer;
  vector<int> BoardId_Buffer;
  map<unsigned long, vector<Waveform<double>>> LAPPDWaveforms;

  int verbosity;
  int max_entries;
  ifstream PedFile;
  string PedFileName, PedFileNameTXT;
  int DoPedSubtract;
  std::map<unsigned long, vector<int>> *PedestalValues;    
  std::map<int, int> map_global_id;
  int Nboards;
  std::string GlobalBoardConfig;

  int NUM_VECTOR_DATA = 7795;
  int NUM_VECTOR_PPS = 16;

  int channel_count=0;
  map<int, std::vector<unsigned short>> data;
  std::vector<unsigned short> meta;
  std::vector<unsigned short> pps;
  boost::posix_time::ptime first;

  BoostStore *LAPPDData = nullptr;
  BoostStore *TempData = nullptr;

};


#endif
