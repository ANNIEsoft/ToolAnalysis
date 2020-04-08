#include "TriggerData.h"

void bencleanup2 (void *data, void *hint) { 
}

TriggerData::~TriggerData(){

}

void  TriggerData::Send(zmq::socket_t *socket, int flag){
  
  //  std::cout<<"d0"<<std::endl;
  zmq::message_t ms1(&FirmwareVersion,sizeof FirmwareVersion, bencleanup2);
  // std::cout<<"d0.1"<<std::endl;  
zmq::message_t ms2(&SequenceID,sizeof SequenceID, bencleanup2);
//std::cout<<"d0.2"<<std::endl;

 zmq::message_t ms3(&EventSize,sizeof EventSize, bencleanup2);
  //std::cout<<"d0.3"<<std::endl;

  zmq::message_t ms4(&TimeStampSize,sizeof TimeStampSize, bencleanup2); 
  //zmq::message_t ms4(&TriggerSize,sizeof TriggerSize, bencleanup2);
  //std::cout<<"d0.4"<<std::endl;

  zmq::message_t ms5(&FIFOOverflow,sizeof FIFOOverflow, bencleanup2);
  zmq::message_t ms6(&DriverOverflow,sizeof DriverOverflow, bencleanup2);
  //std::cout<<"triggercounts snd size check = "<<triggerCounts.size()<<" card = "<< CardID<<std::endl;
  //std::cout<<"data.size = "<<Data.size()<<std::endl;
  //std::cout<<"d0.5"<<std::endl;

  //zmq::message_t ms6(&(triggerCounts.at(0)), sizeof(uint64_t)*triggerCounts.size(), bencleanup2);
  //std::cout<<"d0.6"<<std::endl;

  // zmq::message_t ms7(&(Rates.at(0)), sizeof(uint32_t)*Rates.size(), bencleanup2); 
  //std::cout<<"d0.7"<<std::endl;
  
  //  zmq::message_t ms1(&CardID,sizeof CardID, bencleanup2);
  //std::cout<<"d0.8"<<std::endl;

  // std::cout<<"CardID = "<<CardID<<std::endl;
  //zmq::message_t ms9(&channels,sizeof channels, bencleanup2);
  //std::cout<<"d0.9"<<std::endl;

  //  zmq::message_t ms10(&buffersize,sizeof buffersize, bencleanup2);
  //std::cout<<"d0.10"<<std::endl;

  // std::cout<<"buffersize = "<<buffersize<<std::endl;
  //std::cout<<"fullbuffsize = "<<fullbuffsize<<std::endl;
  //zmq::message_t ms11(&eventsize,sizeof eventsize, bencleanup2);
  //std::cout<<"d0.11"<<std::endl;

  //zmq::message_t ms3(&FirmwareVersion,sizeof FirmwareVersion, bencleanup2); 
  zmq::message_t ms7(&(EventIDs.at(0)), sizeof(uint16_t)*EventIDs.size(), bencleanup2);
  zmq::message_t ms8(&(EventTimes.at(0)), sizeof(uint64_t)*EventTimes.size(), bencleanup2);
  //zmq::message_t ms9(&(TriggerMasks.at(0)), sizeof(uint32_t)*TriggerMasks.size(), bencleanup2);

  //zmq::message_t ms10(&(TriggerCounters.at(0)), sizeof(uint32_t)*TriggerCounters.size(), bencleanup2);
  zmq::message_t ms10(&(TimeStampData.at(0)), sizeof(uint32_t)*TimeStampData.size(), bencleanup2);   
  //  std::cout<<"d0.12"<<std::endl;

  //  std::cout<<"data.size = "<<Data.size()<<std::endl;
  // std::cout<<"data.size2 = "<<Data.size()*sizeof(uint16_t)<<std::endl;
  //std::cout<<"d1"<<std::endl;  
/*  for (int i=0;i<Data.size();i++){

if(Data.at(i)==0)    std::cout<<" data.at("<<i<<")="<<Data.at(i);
  }
  std::cout<<std::endl;
  */
  //zmq::message_t ms8(&fullbuffsize,sizeof fullbuffsize, bencleanup2);

  socket->send(ms1,ZMQ_SNDMORE);
  socket->send(ms2,ZMQ_SNDMORE);
  socket->send(ms3,ZMQ_SNDMORE);
  socket->send(ms4,ZMQ_SNDMORE);
  socket->send(ms5,ZMQ_SNDMORE);
  socket->send(ms6,ZMQ_SNDMORE);                              
  socket->send(ms7,ZMQ_SNDMORE);
  socket->send(ms8,ZMQ_SNDMORE);
  //socket->send(ms9,ZMQ_SNDMORE);
  socket->send(ms10,flag);
  // socket->send(ms11,ZMQ_SNDMORE);
  // socket->send(ms12);
  // std::cout<<"d2"<<std::endl;
  //std::cout<<" ben 1"<<std::endl;
  //for(int i=0;i<channels;i++){
  //zmq::message_t ms(&PMTID[0],&PMTID[channels]);
  // zmq::message_t ms(PMTID,sizeof(int)*channels,bencleanup2);
  //zmq::message_t ms(&PMTID[i],sizeof PMTID[i], bencleanup2);
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
  //    zmq::message_t mss(Data, sizeof(uint16_t)*fullbuffsize, bencleanup2); 

    //std::cout<<" ben 4"<<std::endl;
    
      //zmq::message_t ms(&Data[i],sizeof Data[i], bencleanup2);
  // socket->send(mss);
    //std::cout<<" ben 5"<<std::endl;

    //}

  //socket->send(ms6);


}

bool TriggerData::Receive(zmq::socket_t *socket){
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
  FirmwareVersion=*(reinterpret_cast<int*>(message.data()));
  socket->recv(&message);
  SequenceID=*(reinterpret_cast<int*>(message.data()));
  socket->recv(&message);
  EventSize=*(reinterpret_cast<int*>(message.data()));
  socket->recv(&message);
  TimeStampSize=*(reinterpret_cast<int*>(message.data()));  
  //TriggerSize=*(reinterpret_cast<int*>(message.data()));
  socket->recv(&message);
  FIFOOverflow=*(reinterpret_cast<int*>(message.data()));
  socket->recv(&message);
  DriverOverflow=*(reinterpret_cast<int*>(message.data()));
  //std::cout<<"triggercounts rec size check = "<<message.size()<<std::endl;
 
   zmq::message_t message2;  
  socket->recv(&message2);
   int tmp=*(reinterpret_cast<int*>(message2.data()));
   if(tmp>0){  
      socket->recv(&message); 
     EventIDs.resize(message.size()/sizeof(uint16_t)); 
      std::memcpy(&EventIDs[0], message.data(), message.size());   
     }  
  
   zmq::message_t message3;
  socket->recv(&message3);  
   tmp=*(reinterpret_cast<int*>(message3.data())); 
  if(tmp>0){  
    socket->recv(&message); 
    EventTimes.resize(message.size()/sizeof(uint64_t));  
     std::memcpy(&EventTimes[0], message.data(), message.size());
    } 
  
  zmq::message_t message5; 
  socket->recv(&message5);      
  tmp=*(reinterpret_cast<int*>(message5.data())); 
  if(tmp>0){
    socket->recv(&message);
     TimeStampData.resize(message.size()/sizeof(uint32_t));
    std::memcpy(&TimeStampData[0], message.data(), message.size());
  }
  
  
     
     
  /*
  socket->recv(&message);
  EventIDs.resize(message.size()/sizeof(uint16_t));
  std::memcpy(&EventIDs[0], message.data(), message.size());
  socket->recv(&message);
  EventTimes.resize(message.size()/sizeof(uint64_t));
  std::memcpy(&EventTimes[0], message.data(), message.size());
  socket->recv(&message);
  TriggerMasks.resize(message.size()/sizeof(uint32_t));
  std::memcpy(&TriggerMasks[0], message.data(), message.size());
  socket->recv(&message);
  TriggerCounters.resize(message.size()/sizeof(uint32_t));
  std::memcpy(&TriggerCounters[0], message.data(), message.size());
  /*
  CardID=*(reinterpret_cast<int*>(message.data()));
  std::cout<<"CardID = "<<CardID<<std::endl;
  socket->recv(&message);
  channels=*(reinterpret_cast<int*>(message.data()));
  socket->recv(&message);
  buffersize=*(reinterpret_cast<int*>(message.data()));
  socket->recv(&message);
  eventsize=*(reinterpret_cast<int*>(message.data()));
  socket->recv(&message);
  Data.resize(message.size());
  fullbuffsize=message.size();
  std::memcpy(&Data[0], message.data(), message.size());
  */
      /*
      Receive->getsockopt(ZMQ_RCVMORE, &more, &more_size);                    
          
}
                                                        
    if(more==0) break;
      */

  return true;
}
