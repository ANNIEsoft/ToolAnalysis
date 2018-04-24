#ifndef LAPPDParseScope_H
#define LAPPDParseScope_H

#include <string>
#include <iostream>
#include <TRandom3.h>
#include "Tool.h"

class LAPPDParseScope: public Tool {


 public:

  LAPPDParseScope();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

   ifstream isin;
   int NChannel;
   int WavDimSize;
   int TrigChannel;
   int iter;

};


#endif
