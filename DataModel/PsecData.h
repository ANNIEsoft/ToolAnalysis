#ifndef PSECDATA_H
#define PSECDATA_H

#include "zmq.hpp"
#include <SerialisableObject.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <queue>

using namespace std;

class PsecData{

    friend class boost::serialization::access;

 public:

    PsecData();
    PsecData(unsigned int id);
    ~PsecData();

    bool Send(zmq::socket_t* sock);
    bool Receive(zmq::socket_t* sock);
    bool Receive(std::queue<zmq::message_t> &message_queue);

    //General data
    unsigned int VersionNumber;
    unsigned int LAPPD_ID;
    string Timestamp;

    //Received data from the ACC class
    vector<unsigned short> ReceiveData;

    //To send data form ACC
    vector<int> BoardIndex;
    vector<unsigned short> AccInfoFrame;
    vector<unsigned short> RawWaveform;
    vector<unsigned int> errorcodes;
    int FailedReadCounter;

    //Run control for ACC
    int readRetval;

    bool Print();
    bool SetDefaults();

 private:

 template <class Archive> void serialize(Archive& ar, const unsigned int version){

    ar & VersionNumber;
    ar & LAPPD_ID;
    ar & Timestamp;
    ar & BoardIndex;
    ar & AccInfoFrame;
    ar & RawWaveform;
    ar & errorcodes;
    ar & FailedReadCounter;

 }

 
};

#endif
