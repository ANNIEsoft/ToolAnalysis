#ifndef LAPPDFindPeak_H
#define LAPPDFindPeak_H

#include <string>
#include <iostream>
#include "ANNIEalgorithms.h"

#include "Tool.h"

class LAPPDFindPeak: public Tool {


 public:

  LAPPDFindPeak();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
