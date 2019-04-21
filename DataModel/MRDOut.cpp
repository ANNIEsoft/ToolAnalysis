#include "MRDOut.h"

void datacleanup (void *data, void *hint) {
  //  free (data);
}

MRDOut::MRDOut(){

}

bool MRDOut::Send(zmq::socket_t *socket){

  zmq::message_t ms1(&OutN,sizeof OutN, datacleanup);
  zmq::message_t ms2(&Trigger,sizeof Trigger, datacleanup);
  zmq::message_t ms3(&TimeStamp,sizeof TimeStamp, datacleanup);
  zmq::message_t ms4(&Value[0],sizeof(unsigned int) * Value.size(), datacleanup);
  zmq::message_t ms5(&Slot[0],sizeof(unsigned int) * Slot.size(), datacleanup);
  zmq::message_t ms6(&Channel[0],sizeof(unsigned int) * Channel.size(), datacleanup);
  zmq::message_t ms7(&Crate[0],sizeof(unsigned int) * Crate.size(), datacleanup);
  //  zmq::message_t ms8(&buffersize,sizeof buffersize, datacleanup);
 
  socket->send(ms1,ZMQ_SNDMORE);
  socket->send(ms2,ZMQ_SNDMORE);
  socket->send(ms3,ZMQ_SNDMORE);
  socket->send(ms4,ZMQ_SNDMORE);
  socket->send(ms5,ZMQ_SNDMORE);
  socket->send(ms6,ZMQ_SNDMORE);                              
  socket->send(ms7);
  //socket->send(ms8,ZMQ_SNDMORE);
 
  return true;

}

bool MRDOut::Receive(zmq::socket_t *socket){

  zmq::message_t message;
  //  std::cout<<"d1"<<std::endl;
  if(socket->recv(&message)){
    // std::cout<<"d2"<<std::endl;
    OutN=*(reinterpret_cast<unsigned int*>(message.data()));
    if(socket->recv(&message)){
      //std::cout<<"d3"<<std::endl;

      Trigger=*(reinterpret_cast<unsigned int*>(message.data()));
      if(socket->recv(&message)){
	//std::cout<<"d4"<<std::endl;

	TimeStamp=*(reinterpret_cast<long*>(message.data()));
	if(socket->recv(&message)){ 
	  //std::cout<<"d5"<<std::endl;
	  Value.clear();
	  Value.resize(message.size() / sizeof(unsigned int));
	  memcpy(&Value[0], message.data(),message.size());
	  
	  if(socket->recv(&message)){
	    //std::cout<<"d6"<<std::endl;

	    Slot.clear();
	    Slot.resize(message.size() / sizeof(unsigned int));
	    memcpy(&Slot[0], message.data(),message.size());
	    
	    if(socket->recv(&message)){
	      //std::cout<<"d7"<<std::endl;
	      
	      Channel.clear();
	      Channel.resize(message.size() / sizeof(unsigned int));
	      memcpy(&Channel[0], message.data(),message.size());
	      
	      if(socket->recv(&message)){
		//std::cout<<"d8"<<std::endl;
		
		Crate.clear();
		Crate.resize(message.size() / sizeof(unsigned int));
		memcpy(&Crate[0], message.data(),message.size());
	      
	      }	
	      else return false;
	    }
	    else return false;
	  }
	  else return false;
	}
	else return false;
      }
      else return false;
    }
    else return false;
  }
  else return false;

return true; 
}


bool MRDOut::Print(){

  return true;

}
