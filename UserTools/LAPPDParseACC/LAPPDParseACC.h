#ifndef LAPPDParseACC_H
#define LAPPDParseACC_H

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>

#include "Tool.h"

#define NUM_CELLS 256
#define NUM_CHS 30

class LAPPDParseACC: public Tool {


 public:

  LAPPDParseACC();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:
   ifstream dfs, mfs;
   vector<int> boards;
   int event;
   string meta_header;




};


#endif
