#ifndef LAPPDParseACC_H
#define LAPPDParseACC_H

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>

#include "Tool.h"



class LAPPDParseACC: public Tool {


 public:

  LAPPDParseACC();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


  bool FindChannelKeyFromACDCFile(int acdc_board, int acdc_ch, unsigned long& key, map<unsigned long, Channel>* lappd_channels_from_geom);
  void GetAllLAPPDChannels(Geometry* geom, map<unsigned long, Channel>* all_lappd_channels);

 private:
   ifstream dfs, mfs;
   string meta_header;
   int _event_no;
   streampos data_stream_pos;




};


#endif
