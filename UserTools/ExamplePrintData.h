#ifndef ExamplePrintData_H
#define ExamplePrintData_H

#include <string>
#include <iostream>

#include "Tool.h"

class ExamplePrintData: public Tool {


 public:

  ExamplePrintData();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  int a;
  double b;
  std::string c;
  int debug;


};


#endif
