#include <zmq.hpp>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fstream>

#include "ServiceDiscovery.h"
#include "Store.h"

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/date_time/posix_time/posix_time.hpp>

void FStop(std::vector< pid_t > *pids){
    
  for(int i=0;i<pids->size();i++){
    //int kill(pid_t pid, int sig);
  
    if(0==kill(pids->at(i),0)){
      kill(pids->at(i), 15);
  
    }
    if(0==kill(pids->at(i),0)){
      kill(pids->at(i), 9);
    }
    

    if(!(0==kill(pids->at(i),0))){
      pids->erase(pids->begin() + i);

    }
    waitpid(-1, 0, WNOHANG);
  }
}

void Stop(std::vector< pid_t > *pids){





}

std::string Start(std::vector< pid_t > *pids, std::string configfile=""){
 
  std::string ret="";
  pid_t pid = fork(); /* Create a child process */
  switch (pid) {
  case -1: /* Error */
    ret="Uh-Oh! fork() failed.\n";
    exit(1);
  case 0: /* Child process */
    if (configfile=="")    execl("./main", "./main",  NULL); /* Execute the program */
    else execl("./main", "./main", configfile.c_str(), NULL);
    ret="Uh-Oh! execl() failed!"; /* execl doesn't return unless there's an error */
    //exit(1);
  default: /* Parent process */
    std::stringstream tmp;
    tmp<< "ToolChain Process created with pid " << pid << "\n";

    ret=tmp.str();
    pids->push_back(pid);
    return ret;
    //int status;
    //while (!WIFEXITED(status)) {
    // waitpid(pid, status, 0); /* Wait for the process to complete */
    
    //}
    
  }
  
}

int main(){


  boost::uuids::uuid m_UUID=boost::uuids::random_generator()();
  long msg_id=0;


  zmq::context_t *context=new zmq::context_t(3);

  std::string address("239.192.1.1");
  std::stringstream tmp ("5000");

  int port=5000;

  ServiceDiscovery *SD=new ServiceDiscovery(true, false, 24000, address, port,context, m_UUID, "Node Daemon");

  int a =120000;
  zmq::socket_t direct (*context, ZMQ_REP);
  direct.setsockopt(ZMQ_RCVTIMEO, a);
  direct.setsockopt(ZMQ_SNDTIMEO, a);
  direct.bind("tcp://*:24000");
  
  
  zmq::socket_t ftp (*context, ZMQ_PULL);
  ftp.setsockopt(ZMQ_RCVTIMEO, a);
  ftp.setsockopt(ZMQ_SNDTIMEO, a);
  ftp.bind("tcp://*:24001");
  
  bool run=true;
  std::vector< pid_t > *pids;
  pids= new std::vector< pid_t >;
  
  while (run){
    
    //std::cout<<"Starting"<<std::endl;
    zmq::message_t message;
    
    direct.recv(&message);

    std::istringstream iss(static_cast<char*>(message.data()));
    //std::cout<<"Received message: "<<iss.str()<<std::endl;

    Store bb;
    bb.JsonPaser(iss.str());
    
 
    std::string ret="Command not recognised (Use ? to find valid commands)";

    if(*(bb["msg_value"])=="KILL"){
      ret="Killing NodeDaemon";
    }

    if(*(bb["msg_value"])=="Reboot"){
      ret="Rebooting Node";
    }

       
    if(*(bb["msg_value"])=="Status"){
      ret="Online";
    }
    
    else if (*(bb["msg_value"])=="?"){
      ret="Available commands: Status, Stop, FStop, Start, Restart, File name, Build, KILL, Reboot ";

    }

    else if (*(bb["msg_value"])=="FStop"){
      ret="Forcing ToolChain to Stop";
      FStop(pids);      
    }    

    else if (*(bb["msg_value"])=="Stop"){
      ret="Stopping ToolChains";
      Stop(pids);
    }    
    else if(*(bb["msg_value"])=="Start"){
     
      ret=Start(pids, *(bb["var1"]));  
    }
    else if(*(bb["msg_value"])=="Restart"){
      ret="Restarting ToolChan";
      Stop(pids);
      FStop(pids);
      Start(pids); 
      
    }
    
    
    else if(*(bb["msg_value"])=="File"){
      ret="Receiving file";
      std::ofstream outfile ((*(bb["var1"])).c_str());
      if (outfile.is_open()){
	
	while (1) {
	  
	  zmq::message_t file;
	  
	  ftp.recv(&file);

	  int64_t more;
	  size_t size = sizeof(int64_t);
	  ftp.getsockopt(ZMQ_RCVMORE, &more, &size);
	  std::istringstream filess(static_cast<char*>(file.data()));
	  //  char *tmp=static_cast<char*>(file.data());
	  //	std::cout<<"buf before = "<<filess.rdbuf()<<std::endl;
	  //	filess>>tmp;	  
	  //std::string tmp2;
	  // filess>>tmp2;
	  //std::cout<<"received = "<<tmp<<std::endl;
	  outfile<<filess.rdbuf()<<"\n";
	  // std::cout<<"received part"<<std::endl;
	  //file.rebuild();
	  if (!more) break;
	  
	}
	  /*
	  zmq_msg_t file;
	  zmq_msg_init (&file);
	  std::cout<<"waiting for part"<<std::endl;
	  zmq_msg_recv (&file, ftp, 0);
	  std::cout<<"received part"<<std::endl;
	  //  Process the message frame
	  outfile << zmq_msg_data(&file);
	  zmq_msg_close (&file);
	  if (!zmq_msg_more (&file)) break;      //  Last message frame
	  */
	
	
	outfile.close();
      }
      
    }

    else if (*(bb["msg_value"])=="Build"){
      ret="Building ToolChain";
      system("make clean; make");
      
      
    }
   
   

    zmq::message_t send(256);


    boost::posix_time::ptime t = boost::posix_time::microsec_clock::universal_time();
    std::stringstream isot;
    isot<<boost::posix_time::to_iso_extended_string(t) << "Z";

    msg_id++;
    Store rr;

    rr.Set("uuid",m_UUID);
    rr.Set("msg_id",msg_id);
    *rr["msg_time"]=isot.str();
    *rr["msg_type"]="Command Reply";
    rr.Set("msg_value",ret);


    std::string tmp="";
    rr>>tmp;
    snprintf ((char *) send.data(), 256 , "%s" ,tmp.c_str()) ;

    //std::cout<<"Sending : "<<tmp<<std::endl;

    direct.send(send);

  
   if(*(bb["msg_value"])=="KILL"){
      
      Stop(pids);
      FStop(pids);
      
      run=false;

      //may need to signal service discovery stop
      
    }
    
    if(*(bb["msg_value"])=="Reboot"){
      Stop(pids);
      FStop(pids);
      system("reboot --force");
    }
  } 
    


  return 0;
  }
