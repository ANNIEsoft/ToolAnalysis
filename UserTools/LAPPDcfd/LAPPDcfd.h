#ifndef LAPPDcfd_H
#define LAPPDcfd_H

#include <string>
#include <iostream>

#include "Tool.h"

class LAPPDcfd: public Tool {


 public:

  LAPPDcfd();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  double CFD_Discriminator1(std::vector<double>* trace, double amp);
  double CFD_Discriminator2(std::vector<double>* trace, double amp);


 private:





};


#endif
