#ifndef LOGGING_H
#define LOGGING_H

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include <zmq.hpp>

struct Logging_thread_args{

  Logging_thread_args( zmq::context_t *incontext, std::string inlocalpath){
    context=incontext;
    localpath=inlocalpath;
  }

  zmq::context_t *context;
  std::string localpath;

};



class Logging {

 public:

  Logging(zmq::context_t *context, std::string mode, std::string localpath="");
  bool Log(std::string message, int messagelevel=1, int verbose=true);
  //templated version
  //<<version

 private:

  static  void *LocalThread(void* arg);
  static  void *RemoteThread(void* arg);

  zmq::context_t *m_context;
  zmq::socket_t *LogSender;
  pthread_t thread;
  
  std::string m_mode;
 
};


#endif
