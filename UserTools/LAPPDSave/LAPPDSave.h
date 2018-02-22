#ifndef LAPPDSave_H
#define LAPPDSave_H

#include <string>
#include <iostream>

#include "Tool.h"

class LAPPDSave: public Tool {


 public:

  LAPPDSave();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:
     std::string path;




};


#endif
