#ifndef EnergyReco_H
#define EnergyReco_H

#include <string>
#include <iostream>

#include "Tool.h"

class EnergyReco: public Tool {


 public:

  EnergyReco();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
