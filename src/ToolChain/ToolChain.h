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
#include <boost/progress.hpp>

#include "Tool.h"
#include "DataModel.h"
#include "Logging.h"
#include "zmq.hpp"
#include "Factory.h"
#include "Store.h"
#include "ServiceDiscovery.h"

class ToolChain{
  
 public:
  ToolChain(std::string configfile);
  ToolChain(int verbose=1, int errorlevel=0, std::string service="test", std::string logmode="Interactive",  std::string log_service="", int log_port=0, int pub_sec=5, int kick_sec=60); 
  //verbosity: true= print out status messages , false= print only error messages;
  //errorlevels: 0= do not exit; error 1= exit if unhandeled error ; exit 2= exit on handeled and unhandeled errors; 
  ~ToolChain(); 
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
  /*
  template <type T> bool Log(T message,int verboselevel=1){
    if(m_verbose>0){
      
      if (m_logging=="Interactive" && verboselevel <= m_verbose)std::cout<<"["<<verboselevel<<"] "<<message<<std::endl;
      
    }
  }
  
  
  static  void *LogThread(void* arg);
  */

  // bool Log(std::string message, int messagelevel=1,bool verbose=true);

  ServiceDiscovery *SD;

  //Tools configs and data
  std::vector<Tool*> m_tools;
  std::vector<std::string> m_toolnames;
  std::vector<std::string> m_configfiles;
  DataModel m_data;
  std::streambuf  *bcout;
  std::ostream *out;
  
  //conf variables
  boost::uuids::uuid m_UUID;
  int m_verbose;
  int m_errorlevel;
  std::string m_log_mode;
  std::string m_log_local_path;
  std::string m_log_service;
  int m_log_port;
  std::string m_service;
  bool interactive;
  bool remote;
  int Inline;
  bool m_recover;
    
  //status variables
  bool exeloop;
  unsigned long execounter;
  bool Initialised;
  bool Finalised;
  bool paused;
  std::stringstream logmessage;  

  //socket coms and threading variables
  pthread_t thread[2];
  zmq::context_t *context;
  int m_remoteport;
  int m_multicastport;
  std::string m_multicastaddress;
  long msg_id;
  int m_pub_sec;
  int m_kick_sec;
};

#endif
