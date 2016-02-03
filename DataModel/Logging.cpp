#include "Logging.h"


Logging::Logging(zmq::context_t *context, std::string mode, std::string localpath){

  m_context =context;
  m_mode=mode;

  Logging_thread_args args(m_context, localpath);
 
  if(m_mode=="Local") pthread_create (&thread, NULL, Logging::LocalThread, &args);; //openfile in thread
  if(m_mode=="Remote") pthread_create (&thread, NULL, Logging::RemoteThread, &args);; // make pushthread with two socets one going out one comming in and buffer socket

  LogSender = new zmq::socket_t(*context, ZMQ_PUSH);
  LogSender->connect("inproc://LogSender");

  
}

bool Logging::Log(std::string message, int messagelevel, int verbose){

  if(verbose&& messagelevel <= verbose){

    if (m_mode=="Interactive" )std::cout<<"["<<messagelevel<<"] "<<message<<std::endl;
    if (m_mode=="Local" || m_mode=="Remote"){ //send to file writer thread
 
      zmq::message_t Send(256);
      snprintf ((char *) Send.data(), 256 , "%s" ,message.c_str()) ;
      LogSender->send(Send);


    }
 

  }
}



 void* Logging::LocalThread(void* arg){

   Logging_thread_args* args= static_cast<Logging_thread_args*>(arg);
   zmq::context_t * context = args->context;
   std::string localpath=args->localpath;


   zmq::socket_t LogReceiver(*context, ZMQ_PULL);
   LogReceiver.bind("inproc://LogSender");
  
   std::ofstream logfile (localpath.c_str());
    
   bool running =true;

   while(running){


     zmq::message_t Receive;
     LogReceiver.recv (&Receive);
     std::istringstream ss(static_cast<char*>(Receive.data()));


     if (logfile.is_open())
       {
	 logfile << ss.str()<<std::endl;;
      
       }
     
     if(ss.str()=="Quit")running=false;
   }

   logfile.close();   

 }



void* Logging::RemoteThread(void* arg){

  Logging_thread_args* args= static_cast<Logging_thread_args*>(arg);
  zmq::context_t * context = args->context;
  std::string localpath=args->localpath;


  zmq::socket_t LogReceiver(*context, ZMQ_PULL);
  LogReceiver.bind("inproc://LogSender");

  std::vector<Store*> RemoteServices;
  zmq::socket_t Ireceive (*context, ZMQ_DEALER);
  Ireceive.connect("inproc://ServiceDiscovery");


  bool running =true;

  while(running){

    zmq::message_t send(256);
    snprintf ((char *) send.data(), 256 , "%s" ,"All NULL") ;
    
    Ireceive.send(send);
    
    zmq::message_t receive;
    Ireceive.recv(&receive);
    std::istringstream iss(static_cast<char*>(receive.data()));
    
    int size;
    iss>>size;
    
    RemoteServices.clear();
    

    for(int i=0;i<size;i++){
      
      Store *service = new Store;
      
      zmq::message_t servicem;
      Ireceive.recv(&servicem);
      
      std::istringstream ss(static_cast<char*>(servicem.data()));
      service->JsonPaser(ss.str());
      
      std::string servicetype;
      service->Get("msg_value",servicetype);
      if(servicetype=="Remote Log")  RemoteServices.push_back(service);
      else delete service  ;
      
    }
    if(RemoteServices.size()>0){    
    
      zmq::message_t Receive;
      LogReceiver.recv (&Receive);
      std::istringstream ss(static_cast<char*>(Receive.data()));

      
      
      for(int i=0;i<RemoteServices.size();i++){
	
	std::string ip;
	int remoteport=24010;
      
	  //*(it->second)>> output;
	  ip=*((*(RemoteServices.at(i)))["ip"]);
	

	zmq::socket_t RemoteSend (*context, ZMQ_PUSH);
	std::stringstream tmp;
	tmp<<"tcp://"<<ip<<":"<<remoteport;
	 RemoteSend.connect(tmp.str().c_str());
	 //add time out

	 //probably poll and buffer

	//zmq::message_t send(256);
	//	snprintf ((char *) send.data(), 256 , "%s" ,"All NULL") ;

	 RemoteSend.send(Receive);

	
      }
      
      
      
    
    
   
    
    
    

    
    
    
      if(ss.str()=="Quit")running=false;
    }

  }
}

