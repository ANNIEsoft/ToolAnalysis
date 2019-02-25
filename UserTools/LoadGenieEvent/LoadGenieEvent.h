#ifndef LoadGenieEvent_H
#define LoadGenieEvent_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "genieinfo_struct.cpp"

class LoadGenieEvent: public Tool {


 public:

  LoadGenieEvent();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:
  




};


#endif
