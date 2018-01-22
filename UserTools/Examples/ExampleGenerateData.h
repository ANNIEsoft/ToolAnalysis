#ifndef ExampleGenerateData_H
#define ExampleGenerateData_H

#include <string>
#include <iostream>

#include "Tool.h"

class ExampleGenerateData: public Tool {


 public:

  ExampleGenerateData();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  unsigned long NumEvents;
  int verbose;
  unsigned long currentevent;

};


#endif
