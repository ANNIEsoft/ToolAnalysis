#ifndef LAPPDnnls_H
#define LAPPDnnls_H

#include <string>
#include <iostream>

#include "Tool.h"

class LAPPDnnls: public Tool {


 public:

  LAPPDnnls();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
