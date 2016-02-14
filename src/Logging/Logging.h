#ifndef LOGGING_H
#define LOGGING_H

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <ctime>

#include <zmq.hpp>

#include <pthread.h>
#include <time.h>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/date_time/posix_time/posix_time.hpp>

#include "Store.h"
 

struct Logging_thread_args{

  Logging_thread_args( zmq::context_t *incontext,  boost::uuids::uuid inUUID, std::string inlogservice, int inlogport){
    context=incontext;
    logservice=inlogservice;
    UUID=inUUID;
    logport=inlogport;
  }

  zmq::context_t *context;
  boost::uuids::uuid UUID;
  std::string remoteservice;
  std::string logservice;
  int logport;
};



class Logging: public std::ostream {

  class MyStreamBuf: public std::stringbuf
    {

      std::ostream&   output;

    public:
      MyStreamBuf(std::ostream& str ,zmq::context_t *context,  boost::uuids::uuid UUID, std::string service, std::string mode, std::string localpath="", std::string logservice="", int logport=0);

      virtual int sync ( );

      int m_messagelevel;
      int m_verbose;

      /*
      {
	output << "[blah]" << str();
	str("");
	output.flush();
	return 0;
      }
      */
      ~MyStreamBuf();

    private:

      zmq::context_t *m_context;
      zmq::socket_t *LogSender;
      pthread_t thread;

      std::string m_service;
      std::string m_mode;

      std::ofstream file;
      std::streambuf *psbuf, *backup;

      static  void *RemoteThread(void* arg);
     
    }; 

 public:
  MyStreamBuf buffer;

 Logging(std::ostream& str,zmq::context_t *context,  boost::uuids::uuid UUID, std::string service, std::string mode, std::string localpath="", std::string logservice="", int logport=0):std::ostream(&buffer),buffer(str, context, UUID, service, mode, localpath, logservice, logport){};

  
  //  void Log(std::string message, int messagelevel=1, int verbose=1);
  //  void Log(std::ostringstream& ost, int messagelevel=1, int verbose=1);

  template <typename T>  void Log(T message, int messagelevel=1, int verbose=1){
    std::stringstream tmp;
    tmp<<message;
    buffer.m_messagelevel=messagelevel;
    buffer.m_verbose=verbose;
    std::cout<<tmp.str()<<std::endl;
    buffer.m_messagelevel=1;
    buffer.m_verbose=1;
  } 
  /* 
  template<typename T>Logging& operator<<(T obj){ 
    std::stringstream tmp;
    tmp<<obj;
    Log(tmp.str());

    return *this;
  }
 
  template<typename T> std::ostringstream&  operator<<(T obj){
     
     oss<<obj;
     Log(oss);
     return oss;
     
     
  } 

*/
  
 private:
  

 //  static  void *LocalThread(void* arg);
 // static  void *RemoteThread(void* arg);

  //  zmq::context_t *m_context;
  // zmq::socket_t *LogSender;
  //pthread_t thread;
  
  // std::string m_mode;

  // std::ostringstream oss;
  //std::ostringstream messagebuffer; 
};

/*
Logging& endl(Logging& L){
  L << "\n";
  return L;
}


Box operator+(const Box& b)
std::string operator<<endl(){

  return "\n";
}
*/

#endif
