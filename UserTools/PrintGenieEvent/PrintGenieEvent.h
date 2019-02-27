#ifndef PrintGenieEvent_H
#define PrintGenieEvent_H

#include <string>
#include <iostream>

#include "Tool.h"
//#include "genieinfo_struct_noroot.cpp"

class PrintGenieEvent: public Tool {


 public:

  PrintGenieEvent();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
