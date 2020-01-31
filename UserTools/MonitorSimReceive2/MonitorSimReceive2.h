#ifndef MonitorSimReceive2_H
#define MonitorSimReceive2_H

#include <string>
#include <iostream>
#include <stdlib.h>     
#include <time.h>   

#include "Tool.h"

class MonitorSimReceive2: public Tool {


 public:

  MonitorSimReceive2();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  std::string MRDDataPath;
  std::string mode;
  std::string outpath; 
  int verbosity;
 
  BoostStore* MRDData;
  BoostStore* MRDData2;
  BoostStore* PMTData;

};


#endif
