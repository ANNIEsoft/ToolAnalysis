#include "MonitorReceive.h"

MonitorReceive::MonitorReceive():Tool(){}


bool MonitorReceive::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////


  MonitorReceiver= new zmq::socket_t(*m_data->context, ZMQ_SUB);
  MonitorReceiver->connect("ipc://test");
  MonitorReceiver->setsockopt(ZMQ_SUBSCRIBE, "", 0);




  return true;
}


bool MonitorReceive::Execute(){

  boost::uuids::uuid m_UUID=boost::uuids::random_generator()();
   long msg_id=0;

 std::vector<Store*> RemoteServices;

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

     for(int i=0;i<RemoteServices.size();i++){
       delete RemoteServices.at(i);
       RemoteServices.at(i)=0;
     }
     RemoteServices.clear();


     for(int i=0;i<size;i++){

       Store *service = new Store;

       zmq::message_t servicem;
       Ireceive.recv(&servicem);

       std::istringstream ss(static_cast<char*>(servicem.data()));
       service->JsonParser(ss.str());

       RemoteServices.push_back(service);

     }




  return true;
}


bool MonitorReceive::Finalise(){

  return true;
}
