#ifndef LAPPDParseScope_H
#define LAPPDParseScope_H

#include <string>
#include <iostream>

#include "Tool.h"

class LAPPDParseScope: public Tool {


 public:

  LAPPDParseScope();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
