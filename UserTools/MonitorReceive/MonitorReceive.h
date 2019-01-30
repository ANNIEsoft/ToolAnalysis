#ifndef MonitorReceive_H
#define MonitorReceive_H

#include <string>
#include <iostream>

#include "Tool.h"

class MonitorReceive: public Tool {


 public:

  MonitorReceive();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

zmq::socket_t* MonitorReceiver;





};


#endif
