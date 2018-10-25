#ifndef SaveRecoEvent_H
#define SaveRecoEvent_H

#include <string>
#include <iostream>

#include "Tool.h"

class SaveRecoEvent: public Tool {


 public:

  SaveRecoEvent();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:
  std::string path;




};


#endif
