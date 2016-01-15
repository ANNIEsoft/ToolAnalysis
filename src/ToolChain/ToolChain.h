#ifndef TOOLCHAIN_H
#define TOOLCHAIN_H

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <pthread.h>
#include <time.h> 


#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/date_time/posix_time/posix_time.hpp>

#include "Tool.h"
#include "DataModel.h"
#include "zmq.hpp"
#include "Factory.h"
#include "Store.h"

class ToolChain{
  
 public:
  ToolChain(std::string configfile);
  ToolChain(bool verbose=true, int errorlevel=0); 
  //verbosity: true= print out status messages , false= print only error messages;
  //errorlevels: 0= do not exit; error 1= exit if unhandeled error ; exit 2= exit on handeled and unhandeled errors; 
  
  void Add(std::string name,Tool *tool,std::string configfile="");
  int Initialise();
  int Execute(int repeates=1);
  int Finalise();

  void Interactive();
  void Remote(int portnum, std::string SD_address="239.192.1.1", int SD_port=5000);
   
private:

  void Init();

  static  void *InteractiveThread(void* arg);
  std::string ExecuteCommand(std::string connand);

  //Tools configs and data
  std::vector<Tool*> m_tools;
  std::vector<std::string> m_toolnames;
  std::vector<std::string> m_configfiles;
  DataModel m_data;
  
  //conf variables
  boost::uuids::uuid m_UUID;
  bool m_verbose;
  int m_errorlevel;
  std::string m_service;
  bool interactive;
  bool remote;
  int Inline;
  
  //status variables
  bool exeloop;
  long execounter;
  bool Initialised;
  bool Finalised;
  bool paused;
  

  //socket coms and threading variables
  pthread_t thread[2];
  zmq::context_t *context;
  int m_remoteport;
  int m_multicastport;
  std::string m_multicastaddress;
  long msg_id;
};

#endif
