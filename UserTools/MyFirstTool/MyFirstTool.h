#ifndef MyFirstTool_H
#define MyFirstTool_H

#include <string>
#include <iostream>

#include "Tool.h"

class MyFirstTool: public Tool {


 public:

  MyFirstTool();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
