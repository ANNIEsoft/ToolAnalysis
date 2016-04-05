#ifndef SERVICEDISCOVERY_H
#define SERVICEDISCOVERY_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/date_time/posix_time/posix_time.hpp>

#include "zmq.hpp"

/*
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/reader.h"
#include "rapidjson/stringbuffer.h"
*/

#include "Store.h"

struct thread_args{
  
  thread_args(boost::uuids::uuid inUUID, zmq::context_t *incontext,std::string inmulticastaddress, int inmulticastport, std::string inservice, int inremoteport, int  inpubsec=5, int inkicksec=5){
    UUID=inUUID;
    context=incontext;
    multicastaddress=inmulticastaddress;
    multicastport=inmulticastport;
    service=inservice;
    remoteport=inremoteport;
    pubsec=inpubsec;
    kicksec=inkicksec;
  }
  
  boost::uuids::uuid UUID;
  zmq::context_t *context;
  std::string multicastaddress;
  int multicastport;
  std::string service;
  int remoteport;
  int pubsec;
  int kicksec;
};



class ServiceDiscovery{
    
 public:
  
  ServiceDiscovery(bool Send, bool Receive, int remoteport, std::string address, int multicastport, zmq::context_t * incontext, boost::uuids::uuid UUID, std::string service, int pubsec=5, int kicksec=60);
  ServiceDiscovery( std::string address, int multicastport, zmq::context_t * incontext, int kicksec=60);  
  ~ServiceDiscovery();
  
  
 private:
  
  static  void *MulticastPublishThread(void* arg);
  static  void *MulticastListenThread(void* arg);

  boost::uuids::uuid m_UUID;
  zmq::context_t *context;
  pthread_t thread[2];
  thread_args *args;

  int m_multicastport;
  std::string m_multicastaddress;
  std::string m_service;
  int m_remoteport;

  bool m_send;
  bool m_receive;


  
};

#endif
