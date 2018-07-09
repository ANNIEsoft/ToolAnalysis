#ifndef ExampleSaveStore_H
#define ExampleSaveStore_H

#include <string>
#include <iostream>

#include "Tool.h"

class ExampleSaveStore: public Tool {


 public:

  ExampleSaveStore();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  std::string outfile;
  int verbose;


};


#endif
