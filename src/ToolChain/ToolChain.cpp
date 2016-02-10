#include "ToolChain.h"

ToolChain::ToolChain(std::string configfile){
 
  Store config;
  config.Initialise(configfile);
  config.Get("verbose",m_verbose);
  config.Get("error_level",m_errorlevel);
  config.Get("remote_port",m_remoteport);
  config.Get("log_mode",m_log_mode);
  config.Get("log_local_path",m_log_local_path);
  config.Get("service_discovery_address",m_multicastaddress);
  config.Get("service_discovery_port",m_multicastport);
  config.Get("service_name",m_service);
  config.Get("log_service",m_log_service);
  config.Get("log_port",m_log_port);

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

ToolChain::ToolChain(int verbose, int errorlevel, std::string logmode, std::string log_service, int log_port){

  m_verbose=verbose;
  m_errorlevel=errorlevel;
  m_log_mode = logmode;
  m_service="test";
  m_log_service=log_service;
  m_log_port=log_port;

  Init();

  
}

void ToolChain::Init(){

  context=new zmq::context_t(5);
  m_data.context=context;

 
  bcout=std::cout.rdbuf();
  out=new  std::ostream(bcout);

  m_data.Log= new Logging(*out, context, m_UUID, m_service, m_log_mode, m_log_local_path, m_log_service, m_log_port);

  std::cout.rdbuf(&(m_data.Log->buffer));

  m_UUID = boost::uuids::random_generator()();
  /*
    if(m_verbose){ 
    *(m_data.Log)<<"UUID = "<<m_UUID<<std::endl;
    *(m_data.Log)<<"********************************************************"<<std::endl;
    *(m_data.Log)<<"**** Tool chain created ****"<<std::endl;
    *(m_data.Log)<<"********************************************************"<<std::endl;
    }
  */
  logmessage<<"UUID = "<<m_UUID<<std::endl<<"********************************************************"<<std::endl<<"**** Tool chain created ****"<<std::endl<<"********************************************************"<<std::endl;
    m_data.Log->Log(logmessage.str(),1,m_verbose);
    logmessage.str("");

  execounter=0;
  Initialised=false;
  Finalised=true;
  paused=false;

  msg_id=0;
}



void ToolChain::Add(std::string name,Tool *tool,std::string configfile){
  if(tool!=0){
    // if(m_verbose)*(m_data.Log)<<"Adding Tool=\""<<name<<"\" tool chain"<<std::endl;
    logmessage<<"Adding Tool=\""<<name<<"\" tool chain";
    m_data.Log->Log(logmessage.str(),1,m_verbose);
    logmessage.str("");

    m_tools.push_back(tool);
    m_toolnames.push_back(name);
    m_configfiles.push_back(configfile);

    //    if(m_verbose)*(m_data.Log)<<"Tool=\""<<name<<"\" added successfully"<<std::endl<<std::endl; 
    logmessage<<"Tool=\""<<name<<"\" added successfully"<<std::endl;
    m_data.Log->Log(logmessage.str(),1,m_verbose);
    logmessage.str("");
  

}
}



int ToolChain::Initialise(){

  bool result=0;

  if (Finalised){
    logmessage<<"********************************************************"<<std::endl<<"**** Initialising tools in toolchain ****"<<std::endl<<"********************************************************"<<std::endl;
    m_data.Log->Log(logmessage.str(),1,m_verbose);
    logmessage.str("");

    /*if(m_verbose){
      *(m_data.Log)<<"********************************************************"<<std::endl;
      *(m_data.Log)<<"**** Initialising tools in toolchain ****"<<std::endl;
      *(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
      }*/
    
    for(int i=0 ; i<m_tools.size();i++){  
      
      logmessage<<"Initialising "<<m_toolnames.at(i);
      m_data.Log->Log(logmessage.str(),2,m_verbose);
      logmessage.str("");

      //if(m_verbose) *(m_data.Log)<<"Initialising "<<m_toolnames.at(i)<<std::endl;
      
      try{    
	if(m_tools.at(i)->Initialise(m_configfiles.at(i), m_data)){
	  //  if(m_verbose)*(m_data.Log)<<m_toolnames.at(i)<<" initialised successfully"<<std::endl<<std::endl;
	  logmessage<<"Initialising "<<m_toolnames.at(i);
	  m_data.Log->Log( logmessage.str(),2,m_verbose);
	  logmessage.str("");
	}
	else{
	  //*(m_data.Log)<<"WARNING !!!!! "<<m_toolnames.at(i)<<" Failed to initialise (exit error code)"<<std::endl<<std::endl;
	  logmessage<<"WARNING !!!!! "<<m_toolnames.at(i)<<" Failed to \
initialise (exit error code)"<<std::endl;
          m_data.Log->Log( logmessage.str(),0,m_verbose);
          logmessage.str("");
	  result=1;
	  if(m_errorlevel>1) exit(1);
	}
	
      }
      
      catch(...){
	//*(m_data.Log)<<"WARNING !!!!! "<<m_toolnames.at(i)<<" Failed to initialise (uncaught error)"<<std::endl<<std::endl;
	logmessage<<"WARNING !!!!! "<<m_toolnames.at(i)<<" Failed to in\
itialise (uncaught error)"<<std::endl;
	m_data.Log->Log( logmessage.str(),0,m_verbose);
	logmessage.str("");
	result=2;
	if(m_errorlevel>0) exit(1);
      }
      
    }
    
    //   if(m_verbose){*(m_data.Log)<<"**** Tool chain initilised ****"<<std::endl;;
    // *(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
    //  }
    
    logmessage<<std::endl<<"**** Tool chain initilised ****"<<std::endl<<"********************************************************"<<std::endl;
    m_data.Log->Log( logmessage.str(),1,m_verbose);
    logmessage.str("");

    execounter=0;
    Initialised=true;
    Finalised=false;
  }
  else {
    //*(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
    // *(m_data.Log)<<" ERROR: ToolChain Cannot Be Initialised as already running. Finalise old chain first";
    //*(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
    logmessage<<"********************************************************"<<std::endl<<std::endl<<" ERROR: ToolChain Cannot Be Initialised as already running. Finalise old chain first"<<"********************************************************"<<std::endl;
    m_data.Log->Log( logmessage.str(),0,m_verbose);
    logmessage.str("");


    result=-1;
  }
  
  return result;
}



int ToolChain::Execute(int repeates){
 
  int result =0;
  
  if(Initialised){

    if(interactive || Inline){
    logmessage<<"********************************************************"<<std::endl<<"**** Executing toolchain "<<repeates<<" times ****"<<std::endl<<"********************************************************"<<std::endl;
    m_data.Log->Log( logmessage.str(),1,m_verbose);
    logmessage.str("");
    }

    for(int i=0;i<repeates;i++){
      /*
	if(m_verbose){
	*(m_data.Log)<<"********************************************************"<<std::endl;
	*(m_data.Log)<<"**** Executing tools in toolchain ****"<<std::endl;
	*(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
	}
      */
      logmessage<<"********************************************************"<<std::endl<<"**** Executing tools in toolchain ****"<<std::endl<<"********************************************************"<<std::endl;
      m_data.Log->Log( logmessage.str(),3,m_verbose);
      logmessage.str("");
      
      for(int i=0 ; i<m_tools.size();i++){
	
	//if(m_verbose)    *(m_data.Log)<<"Executing "<<m_toolnames.at(i)<<std::endl;
	logmessage<<"Executing "<<m_toolnames.at(i);
	m_data.Log->Log( logmessage.str(),4,m_verbose);
	logmessage.str("");	
	
	try{
	  if(m_tools.at(i)->Execute()){
	    //    if(m_verbose)*(m_data.Log)<<m_toolnames.at(i)<<" executed  successfully"<<std::endl<<std::endl;
	    logmessage<<m_toolnames.at(i)<<" executed  successfully"<<std::endl;
	    m_data.Log->Log( logmessage.str(),4,m_verbose);
	    logmessage.str("");
	    
	  }
	  else{
	    //*(m_data.Log)<<"WARNING !!!!!! "<<m_toolnames.at(i)<<" Failed to execute (error code)"<<std::endl<<std::endl;
	    logmessage<<"WARNING !!!!!! "<<m_toolnames.at(i)<<" Failed to execute (error code)"<<std::endl;
            m_data.Log->Log( logmessage.str(),0,m_verbose);
            logmessage.str("");
	    
	    
	    result=1;
	    if(m_errorlevel>1)exit(1);
	  }  
	}
	
	catch(...){
	  // *(m_data.Log)<<"WARNING !!!!!! "<<m_toolnames.at(i)<<" Failed to execute (uncaught error)"<<std::endl<<std::endl;
	  logmessage<<"WARNING !!!!!! "<<m_toolnames.at(i)<<" Failed t\
o execute (uncaught error)"<<std::endl;
	  m_data.Log->Log( logmessage.str(),0,m_verbose);
	  logmessage.str("");
	  
	  result=2;
	  if(m_errorlevel>0)exit(1);
	}
	
      } 
      /*      if(m_verbose){
       *(m_data.Log)<<"**** Tool chain executed ****"<<std::endl;
       *(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
       }
      */
      logmessage<<"**** Tool chain executed ****"<<std::endl<<"********************************************************"<<std::endl;
      m_data.Log->Log( logmessage.str(),3,m_verbose);
      logmessage.str("");
    }
    
    execounter++;
    if(interactive || Inline){
      logmessage<<"********************************************************"<<std::endl<<"**** Executed toolchain "<<repeates<<" times ****"<<std::endl<<"********************************************************"<<std::endl;
      m_data.Log->Log( logmessage.str(),1,m_verbose);
      logmessage.str("");
    }
  }
  
  else {
    /*
    *(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
    *(m_data.Log)<<" ERROR: ToolChain Cannot Be Executed As Has Not Been Initialised yet."<<std::endl;
    *(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
    */

    logmessage<<"********************************************************"<<std::endl<<std::endl<<" ERROR: ToolChain Cannot Be Executed As Has Not Been Initialised yet."<<std::endl<<"********************************************************"<<std::endl;
    m_data.Log->Log( logmessage.str(),0,m_verbose);
    logmessage.str("");
    result=-1;
  }

  return result;
}



int ToolChain::Finalise(){
 
  int result=0;

  if(Initialised){
    /*
    if(m_verbose){
      *(m_data.Log)<<"********************************************************"<<std::endl;
      *(m_data.Log)<<"**** Finalising tools in toolchain ****"<<std::endl;
      *(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
    }  
    */
    logmessage<<"********************************************************"<<std::endl<<"**** Finalising tools in toolchain ****"<<std::endl<<"********************************************************"<<std::endl;
    m_data.Log->Log( logmessage.str(),1,m_verbose);
    logmessage.str("");

    for(int i=0 ; i<m_tools.size();i++){
      
      //if(m_verbose)*(m_data.Log)<<"Finalising "<<m_toolnames.at(i)<<std::endl;
      logmessage<<"Finalising "<<m_toolnames.at(i);
      m_data.Log->Log( logmessage.str(),2,m_verbose);
      logmessage.str("");
      
      try{
	if(m_tools.at(i)->Finalise()){
	  //  if(m_verbose)*(m_data.Log)<<m_toolnames.at(i)<<" Finalised successfully"<<std::endl<<std::endl;
	  logmessage<<m_toolnames.at(i)<<" Finalised successfully"<<std::endl;
	  m_data.Log->Log( logmessage.str(),2,m_verbose);
	  logmessage.str("");

	}
	else{
	  //  *(m_data.Log)<<"WARNING !!!!!!! "<<m_toolnames.at(i)<<" Finalised successfully (error code)"<<std::endl<<std::endl;;
	  logmessage<<"WARNING !!!!!!! "<<m_toolnames.at(i)<<" Finalised successfully (error code)"<<std::endl;
	  m_data.Log->Log( logmessage.str(),0,m_verbose);
	  logmessage.str("");
	  
	  result=1;
	  if(m_errorlevel>1)exit(1);
	}  
      }
      
      catch(...){
	//*(m_data.Log)<<"WARNING !!!!!!! "<<m_toolnames.at(i)<<" Finalised successfully (uncaught error)"<<std::endl<<std::endl;
	logmessage<<"WARNING !!!!!!! "<<m_toolnames.at(i)<<" Finalised successfully (uncaught error)"<<std::endl;
	m_data.Log->Log( logmessage.str(),0,m_verbose);
	logmessage.str("");
	
	result=2;
	if(m_errorlevel>0)exit(1);
      }
      
    }
    /*
      if(m_verbose){
      *(m_data.Log)<<"**** Tool chain Finalised ****"<<std::endl;
      *(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
      }
    */
    logmessage<<"**** Toolchain Finalised ****"<<std::endl<<"********************************************************"<<std::endl;
    m_data.Log->Log( logmessage.str(),1,m_verbose);
    logmessage.str("");
    
    execounter=0;
    Initialised=false;
    Finalised=true;
    paused=false;
  }
  
  else {
    /*
    *(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
    *(m_data.Log)<<" ERROR: ToolChain Cannot Be Finalised As Has Not Been Initialised yet."<<std::endl;
    *(m_data.Log)<<"********************************************************"<<std::endl<<std::endl;
    */    

    logmessage<<"********************************************************"<<std::endl<<std::endl<<" ERROR: ToolChain Cannot Be Finalised As Has Not Been Initialised yet."<<std::endl<<"********************************************************"<<std::endl;
    m_data.Log->Log( logmessage.str(),0,m_verbose);
    logmessage.str("");

    result=-1;
  }



  return result;
}


void ToolChain::Interactive(){
  //  m_verbose=false;  
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
      
      // std::cout<<ExecuteCommand(command)<<std::endl<<std::endl;
      logmessage<<ExecuteCommand(command);
      printf("%s \n\n",logmessage.str().c_str());
      //m_data.Log->Log( logmessage.str(),0,m_verbose);
      logmessage.str("");

      command="";
      //std::cout<<"Please type command : Start, Pause, Unpause, Stop, Quit (Initialise, Execute, Finalise)"<<std::endl;
      //std::cout<<">";

      logmessage<<"Please type command : Start, Pause, Unpause, Stop, Status, Quit, ?, (Initialise, Execute, Finalise)";
      //   m_data.Log->Log( logmessage.str(),0,m_verbose);
      printf("%s \n %s",logmessage.str().c_str(),">");
      logmessage.str("");
      
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
    returnmsg<<"command not recognised please try again";
  }

  if(exeloop) Execute();
  return returnmsg.str();
}




void ToolChain::Remote(int portnum, std::string SD_address, int SD_port){

  m_remoteport=portnum;
  m_multicastport=SD_port;
  m_multicastaddress=SD_address;
  //m_verbose=false;
  exeloop=false;

  ServiceDiscovery *SD=new ServiceDiscovery(true,true, m_remoteport, m_multicastaddress.c_str(),m_multicastport,context,m_UUID,m_service);

  
  std::stringstream tcp;
  tcp<<"tcp://*:"<<m_remoteport;

  zmq::socket_t Ireceiver (*context, ZMQ_REP);
  int a=120000;
  Ireceiver.setsockopt(ZMQ_RCVTIMEO, a);
  Ireceiver.setsockopt(ZMQ_SNDTIMEO, a);
  
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

  //  std::cout<<"Please type command : Start, Pause, Unpause, Stop, Quit (Initialise, Execute, Finalise)"<<std::endl;
  // std::cout<<">";

  printf("%s \n %s","Please type command : Start, Pause, Unpause, Stop, Status, Quit, ?, (Initialise, Execute, Finalise)",">");
  /* logmessage<<"Please type command : Start, Pause, Unpause, Stop, Quit (Initialise, Execute, Finalise)"<<std::endl<<">";
  m_data.Log->Log( logmessage.str(),0,m_verbose);
  logmessage.str("");
  */

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


/*
bool ToolChain::Log(std::string message, int messagelevel, bool verbose){

  if(verbose){

    if (m_logging=="Interactive" && verboselevel <= m_verbose)std::cout<<"\
["<<verboselevel<<"] "<<message<<std::endl;

  }


}


static  void *LogThread(void* arg){



}
*/

ToolChain::~ToolChain(){

  delete SD;
  SD=0;
  delete context;
  context=0;
  std::cout.rdbuf(bcout);
  delete out;
  out=0;
  delete m_data.Log;
  m_data.Log=0;
}
