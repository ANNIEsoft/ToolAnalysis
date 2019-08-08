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


 private:
   ifstream dfs, mfs;
   vector<int> boards;
   string meta_header;
   int n_cells;
   int n_chs;




};


#endif
