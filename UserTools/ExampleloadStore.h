#ifndef ExampleloadStore_H
#define ExampleloadStore_H

#include <string>
#include <iostream>

#include "Tool.h"

class ExampleloadStore: public Tool {


 public:

  ExampleloadStore();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  long currententry;
  int verbose;
  std::string inputfile;
  long NumEvents;


};


#endif
