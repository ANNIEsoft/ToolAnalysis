#ifndef ExampleOverTool_H
#define ExampleOverTool_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "ToolChain.h"

class ToolChain;

class ExampleOverTool: public Tool {


 public:

  ExampleOverTool();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  ToolChain* Sub;

};


#endif
