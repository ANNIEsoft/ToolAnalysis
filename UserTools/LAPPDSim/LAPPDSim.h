#ifndef LAPPDSim_H
#define LAPPDSim_H

#include <string>
#include <iostream>

#include "Tool.h"

class LAPPDSim: public Tool {


 public:

  LAPPDSim();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
