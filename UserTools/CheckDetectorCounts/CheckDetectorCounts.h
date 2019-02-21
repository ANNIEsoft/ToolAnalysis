#ifndef CheckDetectorCounts_H
#define CheckDetectorCounts_H

#include <string>
#include <iostream>

#include "Tool.h"

class CheckDetectorCounts: public Tool {


 public:

  CheckDetectorCounts();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
