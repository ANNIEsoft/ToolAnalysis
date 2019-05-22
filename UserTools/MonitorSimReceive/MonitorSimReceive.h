#ifndef MonitorSimReceive_H
#define MonitorSimReceive_H

#include <string>
#include <iostream>
#include <stdlib.h>     
#include <time.h>   

#include "Tool.h"

class MonitorSimReceive: public Tool {


 public:

  MonitorSimReceive();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  std::string MRDDataPath, MRDDataPathSingle;
  std::string MRD_path_to_file;
  std::string mode;
  BoostStore* MRDData;
  BoostStore* MRDData2;

  std::vector<std::string> vec_filename;
  int i_loop;


};


#endif
