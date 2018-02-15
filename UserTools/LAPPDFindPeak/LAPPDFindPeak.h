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
  int FindPulses_TOT(std::vector<double> *theWav);
  int FindPulses_Thresh(std::vector<double> *theWav);


 private:





};


#endif
