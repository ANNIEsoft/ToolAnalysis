#ifndef Logger_H
#define Logger_H

#include <string>
#include <iostream>

#include "Tool.h"

class Logger: public Tool {


 public:

  Logger();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  int m_log_port;
  zmq::socket_t *LogReceiver; 

};


#endif
