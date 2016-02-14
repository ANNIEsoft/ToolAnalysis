#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "ServiceDiscovery.h"
#include "zmq.hpp"

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/date_time/posix_time/posix_time.hpp>

int main(int argc, char** argv){

  //  if (argc!=3) return 1;

   boost::uuids::uuid m_UUID=boost::uuids::random_generator()();
   long msg_id=0;
   
   
   zmq::context_t context(3);

  //std::string address(argv[1]);
  // std::stringstream tmp (argv[2]);
  
  std::vector<Store*> RemoteServices;

  std::string address("239.192.1.1");
  std::stringstream tmp ("5000");

  int port=5000;
  //  tmp>>port;

  ServiceDiscovery SD(address,port,&context,12);

  bool running=true;

  zmq::socket_t Ireceive (context, ZMQ_DEALER);
  Ireceive.connect("inproc://ServiceDiscovery");

  while(running){

    std::cout<< " Please type \"List\" to find services. To send a command to a service type \"Command\" then ther service number followed by the command e.g. ( Command 2 Status). Use command \"?\" to list commands for that service. Type \"Quit\" to end"<<std::endl<<std::endl;
    

    std::string line;
    getline(std::cin, line);
      
    std::stringstream input(line);
    

    //std::stringstream finput(input);

    if (input.str()=="List"){

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
     }
     RemoteServices.clear();


     for(int i=0;i<size;i++){

       Store *service = new Store;

       zmq::message_t servicem;
       Ireceive.recv(&servicem);

       std::istringstream ss(static_cast<char*>(servicem.data()));
       service->JsonPaser(ss.str());

       RemoteServices.push_back(service);

     }


      zmq::message_t tmp;
      Ireceive.recv(&tmp);
 
      std::cout<<std::endl<<"-----------------------------------------------------------------------------------------------"<<std::endl;
      std::cout<<" [Service number]    IP  ,   Service name  ,  Service status"<<std::endl;
      std::cout<<"-----------------------------------------------------------------------------------------------"<<std::endl<<std::endl;;

      for(int i=0;i<RemoteServices.size();i++){

	std::string ip;
	std::string service;
	std::string status;
	
	//*(it->second)>> output;
	ip=*((*(RemoteServices.at(i)))["ip"]);
	service=*((*(RemoteServices.at(i)))["msg_value"]);
	status=*((*(RemoteServices.at(i)))["status"]);

	std::cout<<"["<<i<<"]  "<<ip<<" , "<<service<<" , "<<status<<std::endl;
    
      }

      std::cout<<"-----------------------------------------------------------------------------------------------"<<std::endl<<std::endl;;


    }




    else if(input.str()=="Quit")running=false;
  
    else{

      std::string Command;
      int ServiceNum;

      std::string Send;
      std::string var1;

      input>>Command>>ServiceNum>>Send>>var1;

      if(Command=="Command"){

	if(ServiceNum<RemoteServices.size()){

	zmq::socket_t ServiceSend (context, ZMQ_REQ);
	int a=120000;
	ServiceSend.setsockopt(ZMQ_RCVTIMEO, a);
	ServiceSend.setsockopt(ZMQ_SNDTIMEO, a);

	std::stringstream connection;
	connection<<"tcp://"<<*((*(RemoteServices.at(ServiceNum)))["ip"])<<":"<<*((*(RemoteServices.at(ServiceNum)))["remote_port"]);
	ServiceSend.connect(connection.str().c_str());


	zmq::message_t send(256);


	boost::posix_time::ptime t = boost::posix_time::microsec_clock::universal_time();
	std::stringstream isot;
	isot<<boost::posix_time::to_iso_extended_string(t) << "Z";

	msg_id++;
	Store bb;

	bb.Set("uuid",m_UUID);
	bb.Set("msg_id",msg_id);
	*bb["msg_time"]=isot.str();
	*bb["msg_type"]="Command";
	bb.Set("msg_value",Send);
	bb.Set("var1",var1);
	
	
	std::string tmp="";
	bb>>tmp;
	snprintf ((char *) send.data(), 256 , "%s" ,tmp.c_str()) ;
	
	ServiceSend.send(send);
	

	
	/////////////////////////////////
	if(Send=="File"){

	  std::stringstream connection;
	  connection<<"tcp://"<<*((*(RemoteServices.at(ServiceNum)))["ip"])<<":"<<24001;

	   
	  zmq::socket_t ftp (context, ZMQ_PUSH);
	  int a=120000;
	  ftp.setsockopt(ZMQ_RCVTIMEO, a);
	  ftp.setsockopt(ZMQ_SNDTIMEO, a);
	  ftp.connect(connection.str().c_str());
	  
	  // zmq::message_t file(256);
	  std::string line;
	  std::ifstream myfile (var1.c_str());
	  if (myfile.is_open())
	    {
	      while ( getline (myfile,line) )
		{
		  //line+="\n";
		  zmq::message_t file(line.length()+1);
		  //std::cout<<"debug = "<<line.c_str()<<std::endl;
		  // memcpy(file.data(), line.c_str(), line.length());
		  snprintf ((char *) file.data(), line.length()+1 , "%s" ,line.c_str()) ;
		  //std::cout<<"sending part"<<std::endl;
		  ftp.send(file,ZMQ_SNDMORE);
		  //ftp.send(file);
		  //std::cout<<"sendt part ="<<line.c_str()<<std::endl;
		}
	      myfile.close();
	    }
	  
	  zmq::message_t end(256);
	  std::string tmp2="";
	  snprintf ((char *) end.data(), 256 , "%s" ,tmp2.c_str()) ;
	  //std::cout<<"sending end"<<std::endl;
	  ftp.send(end,0);
	  //std::cout<<"sent end"<<std::endl;
	  
	  
	  
	  
	  ///////
	}


        zmq::message_t receive;
        ServiceSend.recv(&receive);
        std::istringstream iss(static_cast<char*>(receive.data()));

        std::string answer;
        answer=iss.str();

        Store rr;
        rr.JsonPaser(answer);
        if(*rr["msg_type"]=="Command Reply") std::cout<<std::endl<<*rr["msg_value"]<<std::endl<<std::endl;

	
	}
	
	else std::cout<< "Service number out of range"<<std::endl<<std::endl;
      }


    else std::cout<<"Error not a valid command"<<std::endl<<std::endl;

    }
  }

  return 0;

}
