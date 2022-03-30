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
  int recieveFlag = 1;
  
  std::string ident_string;
 
  //Version number
  unsigned int VersionNumber = 0x0006;
  
  //Timestamp
  unsigned long timeSinceEpochMilliseconds;
 
  //RHT
  float humidity_mon=-1;
  float temperature_mon=-1;
  float temperature_thermistor=-1; 
 
  //HV
  bool HV_state_set=false; //Default chosen 
  float HV_volts=0; //Default chosen 
  int HV_mon=-1;
  float HV_return_mon =-1;
 
  //LV
  bool LV_state_set=false; //Default chosen 
  int LV_mon=-1;
  float v33=-1;
  float v25=-1;
  float v12=-1;
 
  //Saltbridge
  float saltbridge = -1;

  //Emergency variables
  int FLAG_temperature = 0;
  int FLAG_humidity = 0;
  int FLAG_temperature_Thermistor = 0;
  int FLAG_saltbridge = 0;
  
  float LIMIT_temperature_low = 50; //Default chosen 
  float LIMIT_temperature_high = 58; //Default chosen 
  float LIMIT_humidity_low = 15; //Default chosen 
  float LIMIT_humidity_high = 20; //Default chosen 
  float LIMIT_Thermistor_temperature_low = 7000; //Default chosen 
  float LIMIT_Thermistor_temperature_high = 5800; //Default chosen 
  float LIMIT_saltbridge_low = 500000; //Default chosen 
  float LIMIT_saltbridge_high = 400000; //Default chosen 
 
  //relay
  bool SumRelays=false;
  bool relayCh1=false; //Default chosen 
  bool relayCh2=false; //Default chosen 
  bool relayCh3=false; //Default chosen 
  bool relayCh1_mon;
  bool relayCh2_mon;
  bool relayCh3_mon;

  //Triggerboard
  float TrigVref=2.981; //Default chosen 
  float Trig0_threshold=1.223; //Default chosen 
  float Trig0_mon=-1;
  float Trig1_threshold=1.23; //Default chosen 
  float Trig1_mon=-1;

 
  //Light level
  float light=-1;

  //errors
  vector<unsigned int> errorcodes;
  
  bool SetDefaults();
  bool Print();

 private:
 
 template <class Archive> void serialize(Archive& ar, const unsigned int version){
  ar & VersionNumber;
  ar & timeSinceEpochMilliseconds;
  ar & recieveFlag;
  ar & humidity_mon;
  ar & temperature_mon;
  ar & temperature_thermistor;
  ar & HV_mon;
  ar & HV_state_set;
  ar & HV_volts;
  ar & HV_return_mon;
  ar & LV_mon;
  ar & LV_state_set;
  ar & v33;
  ar & v25;
  ar & v12;
  ar & LIMIT_temperature_low;
  ar & LIMIT_temperature_high;
  ar & LIMIT_humidity_low;
  ar & LIMIT_humidity_high;  
  ar & LIMIT_Thermistor_temperature_low;
  ar & LIMIT_Thermistor_temperature_high;
  ar & LIMIT_saltbridge_low;
  ar & LIMIT_saltbridge_high;
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
