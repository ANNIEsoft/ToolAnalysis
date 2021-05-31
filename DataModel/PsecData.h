#ifndef PSECDATA_H
#define PSECDATA_H

#include "zmq.hpp"
#include <SerialisableObject.h>
#include <iostream>
#include <vector>
#include <map>

using namespace std;

class PsecData{

 friend class boost::serialization::access;

 public:

  PsecData();

  bool Send(zmq::socket_t* sock);
  bool Receive(zmq::socket_t* sock);
 
  //Timing vector for speedtests

  vector<string> timevec;
 
  //Received data from the ACC class

  map<int, vector<unsigned short>> ReceiveData;
  //map<int, vector<unsigned short>> map_acdcIF;

  vector<unsigned int> errorcodes;

  //To send data 
  int BoardIndex;
  unsigned int VersionNumber = 0x0001;
  vector<unsigned short> AccInfoFrame;
  vector<unsigned short> RawWaveform;
  //vector<unsigned short> AcdcInfoFrame;
  int FailedReadCounter=0;

  int readRetval;

  bool Print();

 private:

 template <class Archive> void serialize(Archive& ar, const unsigned int version){

  ar & VersionNumber;
  ar & BoardIndex;
  ar & RawWaveform;
  ar & AccInfoFrame;
  //ar & AcdcInfoFrame;
  ar & FailedReadCounter;
  ar & errorcodes;
 }

 
};

#endif
