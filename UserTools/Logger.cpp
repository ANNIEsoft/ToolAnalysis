#include "Logger.h"

Logger::Logger():Tool(){}


bool Logger::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  m_variables.Get("log_port",m_log_port);
  
  LogReceiver = new zmq::socket_t(*m_data->context, ZMQ_PULL);
  
  //int a=12000;
  //LogReceiver->setsockopt(ZMQ_RCVTIMEO, a);
  
  std::stringstream tmp;
  tmp<<"tcp://*:"<<m_log_port;
  
  //  printf(" %s \n",tmp.str().c_str());
  LogReceiver->bind(tmp.str().c_str());
  //LogReceiver->setsockopt(ZMQ_SUBSCRIBE, "", 0);
  
  return true;
}


bool Logger::Execute(){


  zmq::message_t Rmessage;
  if(  LogReceiver->recv (&Rmessage,ZMQ_NOBLOCK)){
  
    std::istringstream ss(static_cast<char*>(Rmessage.data()));
    
    Store bb;
    
    bb.JsonPaser(ss.str());
    
    std::cout<<*(bb["msg_value"])<<std::flush;
  }
  
  return true;
}


bool Logger::Finalise(){

  std::stringstream tmp;
  tmp<<"tcp://*:"<<m_log_port;
  LogReceiver->disconnect(tmp.str().c_str());
  delete LogReceiver;
  LogReceiver=0;

  return true;
}
