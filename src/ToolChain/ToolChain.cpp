#include "ToolChain.h"
#include "ServiceDiscovery.cpp"

ToolChain::ToolChain(std::string configfile){
 
  Store config;
  config.Initialise(configfile);
  config.Get("verbose",m_verbose);
  config.Get("error_level",m_errorlevel);
  config.Get("remote_port",m_remoteport);
  config.Get("service_discovery_address",m_multicastaddress);
  config.Get("service_discovery_port",m_multicastport);
  config.Get("service_name",m_service);

  Init();

  std::string toolsfile="";
  config.Get("Tools_File",toolsfile);
  
  if(toolsfile!=""){
    std::ifstream file(toolsfile.c_str());
    std::string line;
    if(file.is_open()){
      
      while (getline(file,line)){
	if (line.size()>0){
	  if (line.at(0)=='#')continue;
	  std::string name;
	  std::string tool;
	  std::string conf;
	  std::stringstream stream(line);
	  
	  if(stream>>name>>tool>>conf) Add(name,Factory(tool),conf);
	 
	}
	
      }
    }
    
    file.close();
    
  }

  Inline=0;
  interactive=false;
  remote=false;
  config.Get("Inline",Inline);
  config.Get("Interactive",interactive);
  config.Get("Remote",remote);
 
  if(Inline>0){
    ServiceDiscovery *SD=new ServiceDiscovery(false, true, m_remoteport, m_multicastaddress.c_str(),m_multicastport,context,m_UUID,m_service);
    Initialise();
    Execute(Inline);
    Finalise();
  }
  else if(interactive){
    ServiceDiscovery *SD=new ServiceDiscovery(false, true, m_remoteport, m_multicastaddress.c_str(),m_multicastport,context,m_UUID,m_service);
    Interactive();
  }
  else if(remote)Remote(m_remoteport, m_multicastaddress, m_multicastport);
  
}

ToolChain::ToolChain(bool verbose,int errorlevel){

  m_verbose=verbose;
  m_errorlevel=errorlevel;
  m_service="test";
  Init();
  
}

void ToolChain::Init(){

  context=new zmq::context_t(5);
  m_data.context=context;
  
  m_UUID = boost::uuids::random_generator()();

  if(m_verbose){ 
   std::cout<<"UUID = "<<m_UUID<<std::endl;
    std::cout<<"********************************************************"<<std::endl;
    std::cout<<"**** Tool chain created ****"<<std::endl;
    std::cout<<"********************************************************"<<std::endl<<std::endl;
    }
  
  execounter=0;
  Initialised=false;
  Finalised=true;
  paused=false;

  msg_id=0;
}



void ToolChain::Add(std::string name,Tool *tool,std::string configfile){
  if(tool!=0){
    if(m_verbose)std::cout<<"Adding Tool=\""<<name<<"\" tool chain"<<std::endl;
    m_tools.push_back(tool);
    m_toolnames.push_back(name);
    m_configfiles.push_back(configfile);
    if(m_verbose)std::cout<<"Tool=\""<<name<<"\" added successfully"<<std::endl<<std::endl; 
  }
}



int ToolChain::Initialise(){

  bool result=0;

  if (Finalised){
    if(m_verbose){
      std::cout<<"********************************************************"<<std::endl;
      std::cout<<"**** Initialising tools in toolchain ****"<<std::endl;
      std::cout<<"********************************************************"<<std::endl<<std::endl;
    }
    
    for(int i=0 ; i<m_tools.size();i++){  
      
      if(m_verbose) std::cout<<"Initialising "<<m_toolnames.at(i)<<std::endl;
      
      try{    
	if(m_tools.at(i)->Initialise(m_configfiles.at(i), m_data)){
	  if(m_verbose)std::cout<<m_toolnames.at(i)<<" initialised successfully"<<std::endl<<std::endl;
	}
	else{
	  std::cout<<"WARNING !!!!! "<<m_toolnames.at(i)<<" Failed to initialise (exit error code)"<<std::endl<<std::endl;
	  result=1;
	  if(m_errorlevel>1) exit(1);
	}
	
      }
      
      catch(...){
	std::cout<<"WARNING !!!!! "<<m_toolnames.at(i)<<" Failed to initialise (uncaught error)"<<std::endl<<std::endl;
	result=2;
	if(m_errorlevel>0) exit(1);
      }
      
    }
    
    if(m_verbose){std::cout<<"**** Tool chain initilised ****"<<std::endl;
      std::cout<<"********************************************************"<<std::endl<<std::endl;
    }
    
    execounter=0;
    Initialised=true;
    Finalised=false;
  }
  else {
    std::cout<<"********************************************************"<<std::endl<<std::endl;
    std::cout<<" ERROR: ToolChain Cannot Be Initialised as already running. Finalise old chain first"<<std::endl;
    std::cout<<"********************************************************"<<std::endl<<std::endl;
    result=-1;
  }
  
  return result;
}



int ToolChain::Execute(int repeates){
 
  int result =0;
  
  if(Initialised){
    for(int i=0;i<repeates;i++){
      
      if(m_verbose){
	std::cout<<"********************************************************"<<std::endl;
	std::cout<<"**** Executing tools in toolchain ****"<<std::endl;
	std::cout<<"********************************************************"<<std::endl<<std::endl;
      }
      
      for(int i=0 ; i<m_tools.size();i++){
	
	if(m_verbose)    std::cout<<"Executing "<<m_toolnames.at(i)<<std::endl;
	
	try{
	  if(m_tools.at(i)->Execute()){
	    if(m_verbose)std::cout<<m_toolnames.at(i)<<" executed  successfully"<<std::endl<<std::endl;
	  }
	  else{
	    std::cout<<"WARNING !!!!!! "<<m_toolnames.at(i)<<" Failed to execute (error code)"<<std::endl<<std::endl;
	    result=1;
	    if(m_errorlevel>1)exit(1);
	  }  
	}
	
	catch(...){
	  std::cout<<"WARNING !!!!!! "<<m_toolnames.at(i)<<" Failed to execute (uncaught error)"<<std::endl<<std::endl;
	  result=2;
	  if(m_errorlevel>0)exit(1);
	}
	
      } 
      if(m_verbose){
	std::cout<<"**** Tool chain executed ****"<<std::endl;
	std::cout<<"********************************************************"<<std::endl<<std::endl;
      }
    }
    
    execounter++;
  }
  
  else {
    std::cout<<"********************************************************"<<std::endl<<std::endl;
    std::cout<<" ERROR: ToolChain Cannot Be Executed As Has Not Been Initialised yet."<<std::endl;
    std::cout<<"********************************************************"<<std::endl<<std::endl;
    result=-1;
  }

  return result;
}



int ToolChain::Finalise(){
 
  int result=0;

  if(Initialised){
    if(m_verbose){
      std::cout<<"********************************************************"<<std::endl;
      std::cout<<"**** Finalising tools in toolchain ****"<<std::endl;
      std::cout<<"********************************************************"<<std::endl<<std::endl;
    }  
    
    for(int i=0 ; i<m_tools.size();i++){
      
      if(m_verbose)std::cout<<"Finalising "<<m_toolnames.at(i)<<std::endl;
    
      
      try{
	if(m_tools.at(i)->Finalise()){
	  if(m_verbose)std::cout<<m_toolnames.at(i)<<" Finalised successfully"<<std::endl<<std::endl;
	}
	else{
	  std::cout<<"WRNING !!!!!!! "<<m_toolnames.at(i)<<" Finalised successfully (error code)"<<std::endl<<std::endl;;
	  result=1;
	  if(m_errorlevel>1)exit(1);
	}  
      }
      
      catch(...){
	std::cout<<"WRNING !!!!!!! "<<m_toolnames.at(i)<<" Finalised successfully (uncaught error)"<<std::endl<<std::endl;
	result=2;
	if(m_errorlevel>0)exit(1);
      }
      
    }
    
  if(m_verbose){
    std::cout<<"**** Tool chain Finalised ****"<<std::endl;
    std::cout<<"********************************************************"<<std::endl<<std::endl;
  }
  
  execounter=0;
  Initialised=false;
  Finalised=true;
  paused=false;
  }
  
  else {
    std::cout<<"********************************************************"<<std::endl<<std::endl;
    std::cout<<" ERROR: ToolChain Cannot Be Finalised As Has Not Been Initialised yet."<<std::endl;
    std::cout<<"********************************************************"<<std::endl<<std::endl;
    result=-1;
  }
  
  return result;
}


void ToolChain::Interactive(){
  m_verbose=false;  
  exeloop=false;
  
  zmq::socket_t Ireceiver (*context, ZMQ_PAIR);
  Ireceiver.bind("inproc://control");
  
  pthread_create (&thread[0], NULL, ToolChain::InteractiveThread, context);
  
  while (true){
    
    zmq::message_t message;
    std::string command="";
    if(Ireceiver.recv (&message, ZMQ_NOBLOCK)){
      
      std::istringstream iss(static_cast<char*>(message.data()));
      iss >> command;
      
      std::cout<<ExecuteCommand(command)<<std::endl<<std::endl;
      command="";
      std::cout<<"Please type command : Start, Pause, Unpause, Stop, Quit (Initialise, Execute, Finalise)"<<std::endl;
      std::cout<<">";
      
    }
    
    ExecuteCommand(command);
  }  
  
  
}  



std::string ToolChain::ExecuteCommand(std::string command){
  std::stringstream returnmsg;
  
  if(command=="Initialise"){
    int ret=Initialise();
    if (ret==0)returnmsg<<"Initialising ToolChain";
    else returnmsg<<"Error Code "<<ret;
  }
  else if (command=="Execute"){
    int ret=Execute();
    if (ret==0)returnmsg<<"Executing ToolChain";
    else returnmsg<<"Error Code "<<ret;
  }
  else if (command=="Finalise"){
    exeloop=false;
     int ret=Finalise();
    if (ret==0)returnmsg<<"Finalising  ToolChain";
    else returnmsg<<"Error Code "<<ret;
  }
  else if (command=="Quit"){
    returnmsg<<"Quitting";
    if (interactive)exit(0);
}
  else if (command=="Start"){
    int ret=Initialise();
    exeloop=true;
    if (ret==0)returnmsg<<"Starting ToolChain";
    else returnmsg<<"Error Code "<<ret;
  }
  else if(command=="Pause"){
    exeloop=false;
    paused=true;
    returnmsg<<"Pausing ToolChain";
  }
  else if(command=="Unpause"){
    exeloop=true;
    paused=false;
    returnmsg<<"Unpausing ToolChain";
  }
  else if(command=="Stop") {
    exeloop=false;
    int ret=Finalise();
    if (ret==0)returnmsg<<"Stopping ToolChain";
    else returnmsg<<"Error Code "<<ret;
  }
  else if(command=="Status"){
    std::stringstream tmp;
    if(Finalised) tmp<<"Waiting to Initialise ToolChain";
    if(Initialised && execounter==0) tmp<<"Initialised waiting to Execute ToolChain";
    if(Initialised && execounter>0){
      if(paused)tmp<<"ToolChain execution pasued";
      else tmp<<"ToolChain running (loop counter="<<execounter<<")";
    }
    returnmsg<<tmp.str();
  }
 else if(command=="?")returnmsg<<" Available commands: Initialise, Execute, Finalise, Start, Stop, Pause, Unpause, Quit, Status, ?";
  else if(command!=""){
    returnmsg<<"command not recognised please try again"<<std::endl;
  }

  if(exeloop) Execute();
  return returnmsg.str();
}




void ToolChain::Remote(int portnum, std::string SD_address, int SD_port){

  m_remoteport=portnum;
  m_multicastport=SD_port;
  m_multicastaddress=SD_address;
  m_verbose=false;
  exeloop=false;

  ServiceDiscovery *SD=new ServiceDiscovery(true,true, m_remoteport, m_multicastaddress.c_str(),m_multicastport,context,m_UUID,m_service);

  
  std::stringstream tcp;
  tcp<<"tcp://*:"<<m_remoteport;

  zmq::socket_t Ireceiver (*context, ZMQ_REP);
  Ireceiver.bind(tcp.str().c_str());
  
  while (true){

    zmq::message_t message;
    std::string command="";
    if(Ireceiver.recv(&message, ZMQ_NOBLOCK)){
      
      std::istringstream iss(static_cast<char*>(message.data()));
      command=iss.str();

      Store rr;
      rr.JsonPaser(command);
      if(*rr["msg_type"]=="Command") command=*rr["msg_value"];
      else command="NULL";
	     
      zmq::message_t send(256);


       boost::posix_time::ptime t = boost::posix_time::microsec_clock::universal_time();
      std::stringstream isot;
      isot<<boost::posix_time::to_iso_extended_string(t) << "Z";

      msg_id++;
      Store bb;

      bb.Set("uuid",m_UUID);
      bb.Set("msg_id",msg_id);
      *bb["msg_time"]=isot.str();
      *bb["msg_type"]="Command Reply";
      bb.Set("msg_value",ExecuteCommand(command));
    

      std::string tmp="";
      bb>>tmp;
      snprintf ((char *) send.data(), 256 , "%s" ,tmp.c_str()) ;
      Ireceiver.send(send);
      
      // std::cout<<"received "<<command<<std::endl;
      //std::cout<<"sent "<<tmp<<std::endl;
      if(command=="Quit"){
	
	zmq::socket_t ServicePublisher (*context, ZMQ_PUSH);
	ServicePublisher.connect("inproc://ServicePublish");
	zmq::socket_t ServiceDiscovery (*context, ZMQ_DEALER);	
	ServiceDiscovery.connect("inproc://ServiceDiscovery");

	zmq::message_t Qsignal1(256);
	zmq::message_t Qsignal2(256);
	std::string tmp="Quit";
	snprintf ((char *) Qsignal1.data(), 256 , "%s" ,tmp.c_str()) ;
	snprintf ((char *) Qsignal2.data(), 256 , "%s" ,tmp.c_str()) ;
	ServicePublisher.send(Qsignal1);
	ServiceDiscovery.send(Qsignal2);
	exit(0);
      }

      command="";
    }
  
    ExecuteCommand(command);
   
  }  
  
  
}



void* ToolChain::InteractiveThread(void* arg){

  zmq::context_t * context = static_cast<zmq::context_t*>(arg);

  zmq::socket_t Isend (*context, ZMQ_PAIR);

  std::stringstream socketstr;
  Isend.connect("inproc://control");

  bool running=true;

  std::cout<<"Please type command : Start, Pause, Unpause, Stop, Quit (Initialise, Execute, Finalise)"<<std::endl;
  std::cout<<">";

  
  while (running){

    std::string tmp;
    std::cin>>tmp;
    zmq::message_t message(256);
    snprintf ((char *) message.data(), 256 , "%s" ,tmp.c_str()) ;
    Isend.send(message);

    if (tmp=="Quit")running=false;
  }

  return (NULL);

}
