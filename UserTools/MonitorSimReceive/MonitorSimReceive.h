#ifndef MonitorSimReceive_H
#define MonitorSimReceive_H

#include <string>
#include <iostream>
#include <stdlib.h>     
#include <time.h>   

#include "MRDOut.h"
#include "Tool.h"

class MonitorSimReceive: public Tool {


 public:

  MonitorSimReceive();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  std::string file_list;
  std::string mode;
  std::string outpath;
  int verbosity;
  
  BoostStore *indata = nullptr;
  BoostStore* MRDData = nullptr;
  BoostStore* PMTData = nullptr;
  BoostStore* TrigData = nullptr;

  std::vector<std::string> vec_filename;
  int i_loop;


};


#endif
