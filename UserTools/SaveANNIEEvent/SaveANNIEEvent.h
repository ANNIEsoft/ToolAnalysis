#ifndef SaveANNIEEvent_H
#define SaveANNIEEvent_H

#include <string>
#include <iostream>

#include "Tool.h"

class SaveANNIEEvent: public Tool {


 public:

  SaveANNIEEvent();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:
  std::string path;




};


#endif
