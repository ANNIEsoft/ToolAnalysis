#ifndef ExampleloadStore_H
#define ExampleloadStore_H

#include <string>
#include <iostream>
#include <sstream>

#include "Tool.h"

class ExampleloadStore: public Tool {


 public:

  ExampleloadStore();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  unsigned long currententry;
  int verbose;
  std::string inputfile;
  unsigned long NumEvents;

  std::stringstream logmessage;

};


#endif
