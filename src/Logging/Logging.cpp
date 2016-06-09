#include "Logging.h"

Logging::MyStreamBuf::~MyStreamBuf(){

  if(m_mode=="Local")file.close();

  if(m_mode=="Remote"){

    zmq::message_t Send(256);
    snprintf ((char *) Send.data(), 256 , "%s" ,"Quit");
    LogSender->send(Send);
    delete LogSender;
    LogSender=0;
    delete args;
    args=0;
  }


}

Logging::MyStreamBuf::MyStreamBuf (std::ostream& str ,zmq::context_t *context,  boost::uuids::uuid UUID, std::string service, std::string mode, std::string localpath, std::string logservice, int logport):output(str){

  m_context =context;
  m_mode=mode;
  m_messagelevel=1;
  m_service = service;
  m_verbose=1;

  if(m_mode=="Local"){

    file.open(localpath.c_str());
    psbuf = file.rdbuf();
    output.rdbuf(psbuf);
    
  }

  if(m_mode=="Remote"){
 
    args=new Logging_thread_args(m_context, UUID , logservice, logport);

    pthread_create (&thread, NULL, Logging::MyStreamBuf::RemoteThread, args); // make pushthread with two socets one going out one comming in and buffer socket

    LogSender = new zmq::socket_t(*context, ZMQ_PUSH);
    LogSender->connect("inproc://LogSender");
  }



}


int Logging::MyStreamBuf::sync ( )
{
  if(m_messagelevel <= m_verbose){


    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];

    time (&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer,80,"%d-%m-%Y %I:%M:%S",timeinfo);
    std::string t(buffer);

  
    if (m_mode=="Remote"){
      
      
      std::stringstream tmp;
      
      tmp << "{"<<m_service<<":"<<t<<"} ["<<m_messagelevel<<"]: "<< str();
      str("");
      std::string line=tmp.str();
      zmq::message_t Send(line.length()+1);
      snprintf ((char *) Send.data(), line.length()+1 , "%s" ,line.c_str());
      LogSender->send(Send);
    }
    
    else if(m_mode=="Local"){
      output<< "{"<<m_service<<":"<<t<<"} ["<<m_messagelevel<<"]: " << str();
      str("");
      output.flush();
      
    }
    else if(m_mode=="Interactive"){
      output<< "["<<m_messagelevel<<"]: " << str();
      str("");
      output.flush(); 
    }
  }
  else{
    str("");
  }
  
  return 0;
}

bool Logging::MyStreamBuf::ChangeOutFile(std::string localpath){

  file.close();
  file.open(localpath.c_str());
  psbuf = file.rdbuf();
  output.rdbuf(psbuf);
  
}

//Logging(std::ostream& str,zmq::context_t *context,  boost::uuids::uuid UUID, std::string service, std::string mode, std::string localpath, std::string logservice, int logport):std::ostream(&buffer),buffer(str, context,  UUID, service, mode, localpath, logservice, logport){
//}
//  m_context =context;
  // m_mode=mode;
  
  //Logging_thread_args args(m_context, localpath);
  
  //  if(m_mode=="Local") pthread_create (&thread, NULL, Logging::LocalThread, &args);; //openfile in thread
  //if(m_mode=="Remote") pthread_create (&thread, NULL, Logging::RemoteThread, &args);; // make pushthread with two socets one going out one comming in and buffer socket
  
  /*
    if(m_mode!="Interactive"){
    LogSender = new zmq::socket_t(*context, ZMQ_PUSH);
    LogSender->connect("inproc://LogSender");
    }
  */
//}

/* Logging::Log(std::string message, int messagelevel, int verbose){
   
   if(verbose&& messagelevel <= verbose){
   
   if (m_mode=="Interactive" ){
   //std::cout<<"["<<messagelevel<<"] "<<message;//<<std::endl;
   //MyStreamBuf buff(std::cout);
   // buff<<message;      
   //buff<<message;
   }
   if (m_mode=="Local" || m_mode=="Remote"){ //send to file writer thread
   
   zmq::message_t Send(256);
   snprintf ((char *) Send.data(), 256 , "[%d] %s" ,messagelevel,message.c_str()) ;
   LogSender->send(Send);
   
   std::cout<<"sending = "<<message<<std::endl;
   }
   
   
   }
   }

void Logging::Log(std::ostringstream& message, int messagelevel, int verbose){

  if(verbose&& messagelevel <= verbose){

    if (m_mode=="Interactive" ){
      //std::ostringstream tmp;
      //tmp<<std::cout.rdbuf();
     
      if(messagebuffer.str()=="")std::cout<<"["<<messagelevel<<"] ";
      messagebuffer<<message.str();
      std::stringstream tmp;
      tmp<<messagebuffer.str();;
      std::istream is(tmp.rdbuf());
      std::vector<std::string> lines ;
      std::string line;

      while (std::getline(is, line,'\n')) lines.push_back(line);
      
      std::cout<<"tmp = "<<tmp.str()<<std::endl;
      std::cout<<"lines size = "<<lines.size()<<std::endl;
      for(int i=0;i<lines.size();i++){ 

	std::cout<<"i = "<<i<<" "<<lines.at(i)<<std::endl;
	if(i!=lines.size()-1)	std::cout<<"["<<messagelevel<<"] "<<lines.at(i)<<std::endl;
      }
      messagebuffer.str("");
      messagebuffer<<lines.at(lines.size()-1);
      
            /*
	MyStreamBuf buff(std::cout);
      buff.messagelevel=messagelevel;
      std::ostream out(&buff);     
      out<<message.str();//<< std::flush;
      //std::cout<<out;
      *//*
    }

    if (m_mode=="Local" || m_mode=="Remote"){ //send to file writer thread

      zmq::message_t Send(256);
      snprintf ((char *) Send.data(), 256 , "[%d] %s" ,messagelevel,message.str().c_str()) ;
      LogSender->send(Send);

      std::cout<<"sending = "<<message<<std::endl;
    }
    message.str("");

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
	 logfile << ss.str();//<<std::endl;
      
       }
     
     if(ss.str()=="Quit")running=false;
   }
   std::cout<<"imclosing"<<std::endl;
   logfile.close();   

 }

	*/

 void* Logging::MyStreamBuf::RemoteThread(void* arg){

   Logging_thread_args* args= static_cast<Logging_thread_args*>(arg);
   zmq::context_t * context = args->context;
   std::string logservice=args->logservice;
   boost::uuids::uuid UUID=args->UUID;
   int logport=args->logport;
  
   zmq::socket_t LogReceiver(*context, ZMQ_PULL);
   LogReceiver.bind("inproc://LogSender");
   
   std::vector<Store*> RemoteServices;
   zmq::socket_t Ireceive (*context, ZMQ_DEALER);
   Ireceive.connect("inproc://ServiceDiscovery");
   
   long msg_id=0;   
   bool running =true;
   
   while(running){
     
   
     zmq::message_t Receive;
     LogReceiver.recv (&Receive);
     std::istringstream ss(static_cast<char*>(Receive.data()));
 
     boost::posix_time::ptime t = boost::posix_time::microsec_clock::universal_time();
     std::stringstream isot;
     isot<<boost::posix_time::to_iso_extended_string(t) << "Z";
     
     msg_id++;
     
     Store outmessage;

     outmessage.Set("uuid",UUID);
     outmessage.Set("msg_id",msg_id);
     *outmessage["msg_time"]=isot.str();
     *outmessage["msg_type"]="Log";
     outmessage.Set("msg_value",ss.str());
     
     zmq::message_t send(256);
     snprintf ((char *) send.data(), 256 , "%s" ,"All NULL") ;
     
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
       service->JsonPaser(ss.str());
       
       std::string servicetype;
       service->Get("msg_value",servicetype);
       //printf("%s \n",servicetype.c_str());
       if(servicetype==logservice)  RemoteServices.push_back(service);
       else delete service  ;
       
     } 
     
     for(int i=0;i<RemoteServices.size();i++){
       
       std::string ip;
       //int logport=24010;
       
       //*(it->second)>> output;
       ip=*((*(RemoteServices.at(i)))["ip"]);
       
       
       zmq::socket_t RemoteSend (*context, ZMQ_PUSH);
       int a=12000;
       RemoteSend.setsockopt(ZMQ_SNDTIMEO, a);
       std::stringstream tmp;
       tmp<<"tcp://"<<ip<<":"<<logport;
       // printf("%s \n",tmp.str().c_str());
       RemoteSend.connect(tmp.str().c_str());
      
       //add time out
       
       //probably poll and buffer

       std::string rmessage="";
       outmessage>>rmessage;
       zmq::message_t rlogmessage(rmessage.length()+1);
       snprintf ((char *) rlogmessage.data(), rmessage.length()+1 , "%s" , rmessage.c_str()) ;
       // printf("%s \n",rmessage.c_str());       
       RemoteSend.send(rlogmessage);
    
     }

     if(ss.str()=="Quit")running=false;
   }

   for(int i=0;i<RemoteServices.size();i++){

     delete RemoteServices.at(i);
     RemoteServices.at(i)=0;
   }

   RemoteServices.clear();

 }



