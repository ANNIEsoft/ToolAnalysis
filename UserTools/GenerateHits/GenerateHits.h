#ifndef GenerateHits_H
#define GenerateHits_H

#include <string>
#include <iostream>
#include "LAPPDHit.h"

#include "Tool.h"

class GenerateHits: public Tool {


 public:

  GenerateHits();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  double fRand(double fMin, double fMax);


 private:





};


#endif
