#ifndef MRDOUT_H
#define MRDOUT_H

#include <SerialisableObject.h>
#include <zmq.hpp>

class MRDOut  : public SerialisableObject{

  friend class boost::serialization::access;

public:
  
  MRDOut();
  
  unsigned int OutN, Trigger;
  std::vector<unsigned int> Value, Slot, Channel, Crate;
  std::vector<std::string> Type;
  //ULong64_t 
  long TimeStamp;

  bool  Send(zmq::socket_t *socket);
  bool Receive(zmq::socket_t *socket);

  bool Print();
  
 private:
  
  template <class Archive> void serialize(Archive& ar, const unsigned int version){
    
    ar & OutN;
    ar & Trigger;
    ar & Value;
    ar & Slot;
    ar & Channel;
    ar & Crate;
    ar & Type;
    ar & TimeStamp;
    
  }

  




};

#endif
