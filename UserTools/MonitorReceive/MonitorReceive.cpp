#include "MonitorReceive.h"

MonitorReceive::MonitorReceive():Tool(){}


bool MonitorReceive::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////


  MonitorReceiver= new zmq::socket_t(*m_data->context, ZMQ_SUB);
  MonitorReceiver->setsockopt(ZMQ_SUBSCRIBE, "", 0);

  items[0]={MonitorReceiver, 0, ZMQ_POLLIN, 0};

  UpdateMonitorSources();

  last= boost::posix_time::ptime(boost::posix_time::second_clock::local_time());
  period =boost::posix_time::time_duration(0,0,10,0);

  m_data->Stores["CCData"]=new BoostStore(false,0);


  return true;
}


bool MonitorReceive::Execute(){


  boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());
  boost::posix_time::time_duration duration(current - last);
  if(duration>period){
    last=current;
    UpdateMonitorSources();
  }

  zmq::poll(&items[0], 1, 0);

  if ((items [0].revents & ZMQ_POLLIN)) {

    zmq::message_t command;
    MonitorReceiver->recv(&command);
    
    std::istringstream tmp(static_cast<char*>(command.data()));
    m_data->CStore.Set("State",tmp.str());
    
    if (tmp.str()=="MRDSingle"){

      MRDOut data;
      data.Receive(MonitorReceiver);
      m_data->Stores["CCData"]->Set("Single",data);      

    }

    else if(tmp.str()=="DataFile"){


      //do stuff
      
    }
    
  }
  else{
    std::string State="Wait";
    m_data->CStore.Set("State",State);

  }
  

  return true;
}


bool MonitorReceive::Finalise(){

  delete MonitorReceiver;
  MonitorReceiver=0;

  for ( std::map<std::string,Store*>::iterator it=connections.begin(); it!=connections.end(); ++it){
    delete it->second;
    it->second=0;
  }

  connections.clear();

  m_data->CStore.Remove("State");

  m_data->Stores["CCData"]->Remove("FileData");
  m_data->Stores["CCData"]->Remove("Single");
  m_data->Stores.clear();

  return true;
}


void MonitorReceive::UpdateMonitorSources(){


  boost::uuids::uuid m_UUID=boost::uuids::random_generator()();
  long msg_id=0;
  
  zmq::socket_t Ireceive (*m_data->context, ZMQ_DEALER);
  Ireceive.connect("inproc://ServiceDiscovery");
  
  
  zmq::message_t send(4);
  snprintf ((char *) send.data(), 4 , "%s" ,"All") ;
  
  
  Ireceive.send(send);
  
  
  zmq::message_t receive;
  Ireceive.recv(&receive);
  std::istringstream iss(static_cast<char*>(receive.data()));
  
  int size;
  iss>>size;
  
  for(int i=0;i<size;i++){
    
    Store *service = new Store;
    
    zmq::message_t servicem;
    Ireceive.recv(&servicem);
    
    std::istringstream ss(static_cast<char*>(servicem.data()));
    service->JsonParser(ss.str());
    
    std::string type;
    std::string uuid;
    service->Get("msg_value",type);
    service->Get("uuid",uuid);    
    
    if(type == "MonitorData" && connections.count(uuid)==0){
      connections[uuid]=service;
      std::string ip;
      std::string port;
      service->Get("ip",ip);
      service->Get("remote_port",port);
      std::string tmp="tcp://"+ ip +":"+port;
      MonitorReceiver->connect(tmp.c_str());
    }  
    
    
    
  }
  
}
