#ifndef LAPPDRawToACDC_H
#define LAPPDRawToACDC_H

#include <iostream>

#include "Tool.h"

class LAPPDRawToACDC: public Tool {


 public:

  LAPPDRawToACDC();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:
 	char outputfile[300];


};


#endif
