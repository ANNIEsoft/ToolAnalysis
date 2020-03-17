#include "CardData.h"

void bencleanup (void *data, void *hint) {
  //  free (data);
}

CardData::~CardData(){

}

void  CardData::Send(zmq::socket_t *socket, int flag){
  
  //  std::cout<<"d0"<<std::endl;
  //zmq::message_t ms1(&LastSync,sizeof LastSync, bencleanup);
  // std::cout<<"d0.1"<<std::endl;  
zmq::message_t ms2(&SequenceID,sizeof SequenceID, bencleanup);
//std::cout<<"d0.2"<<std::endl;

//  zmq::message_t ms3(&StartTimeSec,sizeof StartTimeSec, bencleanup);
  //std::cout<<"d0.3"<<std::endl;

//  zmq::message_t ms4(&StartTimeNSec,sizeof StartTimeNSec, bencleanup);
  //std::cout<<"d0.4"<<std::endl;

  //zmq::message_t ms5(&StartCount,sizeof StartCount, bencleanup);
  //std::cout<<"triggercounts snd size check = "<<triggerCounts.size()<<" card = "<< CardID<<std::endl;
  //std::cout<<"data.size = "<<Data.size()<<std::endl;
  //std::cout<<"d0.5"<<std::endl;

  //zmq::message_t ms6(&(triggerCounts.at(0)), sizeof(uint64_t)*triggerCounts.size(), bencleanup);
  //std::cout<<"d0.6"<<std::endl;

  // zmq::message_t ms7(&(Rates.at(0)), sizeof(uint32_t)*Rates.size(), bencleanup); 
  //std::cout<<"d0.7"<<std::endl;
  
  zmq::message_t ms1(&CardID,sizeof CardID, bencleanup);
  //std::cout<<"d0.8"<<std::endl;

  // std::cout<<"CardID = "<<CardID<<std::endl;
  //zmq::message_t ms9(&channels,sizeof channels, bencleanup);
  //std::cout<<"d0.9"<<std::endl;

  //  zmq::message_t ms10(&buffersize,sizeof buffersize, bencleanup);
  //std::cout<<"d0.10"<<std::endl;

  // std::cout<<"buffersize = "<<buffersize<<std::endl;
  //std::cout<<"fullbuffsize = "<<fullbuffsize<<std::endl;
  //zmq::message_t ms11(&eventsize,sizeof eventsize, bencleanup);
  //std::cout<<"d0.11"<<std::endl;

  zmq::message_t ms3(&FirmwareVersion,sizeof FirmwareVersion, bencleanup);        

  zmq::message_t ms4(&(Data.at(0)), sizeof(uint32_t)*Data.size(), bencleanup);
  //  std::cout<<"d0.12"<<std::endl;

  //  std::cout<<"data.size = "<<Data.size()<<std::endl;
  // std::cout<<"data.size2 = "<<Data.size()*sizeof(uint16_t)<<std::endl;
  //std::cout<<"d1"<<std::endl;  
/*  for (int i=0;i<Data.size();i++){

if(Data.at(i)==0)    std::cout<<" data.at("<<i<<")="<<Data.at(i);
  }
  std::cout<<std::endl;
  */
  //zmq::message_t ms8(&fullbuffsize,sizeof fullbuffsize, bencleanup);

  socket->send(ms1,ZMQ_SNDMORE);
  socket->send(ms2,ZMQ_SNDMORE);
  socket->send(ms3,ZMQ_SNDMORE);
  socket->send(ms4,flag);
  //  socket->send(ms5,ZMQ_SNDMORE);
  //socket->send(ms6,ZMQ_SNDMORE);                              
  //socket->send(ms7,ZMQ_SNDMORE);
  //socket->send(ms8,ZMQ_SNDMORE);
  //socket->send(ms9,ZMQ_SNDMORE);
  // socket->send(ms10,ZMQ_SNDMORE);
  // socket->send(ms11,ZMQ_SNDMORE);
  // socket->send(ms12);
  // std::cout<<"d2"<<std::endl;
  //std::cout<<" ben 1"<<std::endl;
  //for(int i=0;i<channels;i++){
  //zmq::message_t ms(&PMTID[0],&PMTID[channels]);
  // zmq::message_t ms(PMTID,sizeof(int)*channels,bencleanup);
  //zmq::message_t ms(&PMTID[i],sizeof PMTID[i], bencleanup);
    // std::cout<<" ben 2"<<std::endl;

    //socket->send(ms);
    //  }
    //std::cout<<" ben 3 "<< (sizeof(Data)/sizeof(uint16_t)) <<std::endl;

    //for(int i=0;i<fullbuffsize;i++){
    // Data[i];
    // }
    //std::cout<<"sizeof *Data "<<sizeof *Data <<std::endl;

    //std::cout<<"&Data[0] "<<&Data[0]<<std::endl;
    //std::cout<<"&Data[fullbuffsize] "<<&Data[fullbuffsize]<<std::endl;
     
    //std::vector<uint16_t> v(Data, Data + sizeof Data / sizeof Data[0]);
    //zmq::message_t mss(v.begin(),v.end());
  //    zmq::message_t mss(Data, sizeof(uint16_t)*fullbuffsize, bencleanup); 

    //std::cout<<" ben 4"<<std::endl;
    
      //zmq::message_t ms(&Data[i],sizeof Data[i], bencleanup);
  // socket->send(mss);
    //std::cout<<" ben 5"<<std::endl;

    //}

  //socket->send(ms6);


}

bool CardData::Receive(zmq::socket_t *socket){
  /*
  int more;                                                                   
  size_t more_size = sizeof (more);                                           
  // Receive->getsockopt(ZMQ_RCVMORE, &more, &more_size);                     
                                                                                
  zmq::message_t message;                                                     
  //        std::cout<<"about to get message"<<std::endl;                     
                                                                                
  while (true){                                
    IF(Receive->recv(&message)){   
  */  
   
  zmq::message_t message;

  socket->recv(&message);
  CardID=*(reinterpret_cast<int*>(message.data()));
  socket->recv(&message);
  SequenceID=*(reinterpret_cast<int*>(message.data()));
  socket->recv(&message);
  FirmwareVersion=*(reinterpret_cast<int*>(message.data()));
  socket->recv(&message);
  Data.resize(message.size()/sizeof(uint32_t));
  std::memcpy(&Data[0], message.data(), message.size());
    
      /*
      Receive->getsockopt(ZMQ_RCVMORE, &more, &more_size);                    
          
}
                                                        
    if(more==0) break;
      */

  return true;

}
