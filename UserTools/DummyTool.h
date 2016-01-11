#ifndef DummyTool_H
#define DummyTool_H

#include <string>
#include <iostream>

#include "Tool.h"

class DummyTool: public Tool {


 public:

  DummyTool();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
