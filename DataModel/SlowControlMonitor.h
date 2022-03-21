#ifndef SlowControlMonitor_H
#define SlowControlMonitor_H

#include "zmq.hpp"
#include <SerialisableObject.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>

using namespace std;

class SlowControlMonitor : public SerialisableObject{

 friend class boost::serialization::access;

 public:

  SlowControlMonitor();

  //Comms
  bool Send_Mon(zmq::socket_t* sock);
  bool Receive_Mon(zmq::socket_t* sock);
  bool Send_Config(zmq::socket_t* sock);
  bool Receive_Config(zmq::socket_t* sock);
  bool RelayControl(zmq::socket_t* sock);
  int recieveFlag = 0;
  std::string ident_string;
 
  //Version number
  unsigned int VersionNumber = 0x0004;
 
  //RHT
  float humidity_mon=-444;
  float temperature_mon=-444;
  float temperature_thermistor=-444; 
  
  //HV
  int HV_mon=-1;
  bool HV_state_set;
  float HV_volts=-1;
  float HV_return_mon = -1;

  //LV
  int LV_mon=-1;
  bool LV_state_set;
  float v33=-444;
  float v25=-444;
  float v12=-444;

  //Saltbridge
  float saltbridge = -1;

  //Emergency variables
  float LIMIT_temperature_low = 0;
  float LIMIT_humidity_low = 0;
  float LIMIT_temperature_high = 0;
  float LIMIT_humidity_high = 0;  
  float LIMIT_Thermistor_temperature_low = 0;
  float LIMIT_Thermistor_temperature_high = 0; 
  int FLAG_temperature = 0;
  int FLAG_humidity = 0;
  int FLAG_temperature_Thermistor = 0;
  int FLAG_saltbridge = 0; 

  //relay
  bool relayCh1;
  bool relayCh2;
  bool relayCh3;
  bool relayCh1_mon;
  bool relayCh2_mon;
  bool relayCh3_mon;

  //Triggerboard
  float TrigVref;
  float Trig1_threshold;
  float Trig1_mon=-1;
  float Trig0_threshold;
  float Trig0_mon=-1;
 
  //Light level
  float light;

  //errors
  vector<unsigned int> errorcodes;
  
  bool SetDefaults();
  bool Print();

 private:
 
 template <class Archive> void serialize(Archive& ar, const unsigned int version){

  ar & recieveFlag;
  ar & humidity_mon;
  ar & temperature_mon;
  ar & temperature_thermistor;
  ar & HV_mon;
  ar & HV_state_set;
  ar & HV_volts;
  ar & LV_mon;
  ar & LV_state_set;
  ar & v33;
  ar & v25;
  ar & v12;
  ar & LIMIT_temperature_low;
  ar & LIMIT_humidity_low;
  ar & LIMIT_temperature_high;
  ar & LIMIT_humidity_high;  
  ar & LIMIT_Thermistor_temperature_low;
  ar & LIMIT_Thermistor_temperature_high; 
  ar & FLAG_temperature;
  ar & FLAG_humidity;
  ar & FLAG_temperature_Thermistor;
  ar & FLAG_saltbridge;
  ar & Trig1_threshold;
  ar & Trig1_mon;
  ar & Trig0_threshold;
  ar & Trig0_mon;
  ar & TrigVref;
  ar & relayCh1;
  ar & relayCh2;
  ar & relayCh3;
  ar & relayCh1_mon;
  ar & relayCh2_mon;
  ar & relayCh3_mon;
  ar & light;
  ar & saltbridge;
  ar & errorcodes;
 }
 
};

#endif
