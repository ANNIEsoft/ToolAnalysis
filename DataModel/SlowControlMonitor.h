#ifndef SlowControlMonitor_H
#define SlowControlMonitor_H

#include "zmq.hpp"
#include <SerialisableObject.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>

using namespace std;

class SlowControlMonitor : public SerialisableObject{

 friend class boost::serialization::access;

 public:

    SlowControlMonitor();
    SlowControlMonitor(unsigned int id);
    ~SlowControlMonitor();

    //Comms
    bool Send_Mon(zmq::socket_t* sock);
    bool Receive_Mon(zmq::socket_t* sock);
    bool Send_Config(zmq::socket_t* sock);
    bool Receive_Config(zmq::socket_t* sock);
    bool RelayControl(zmq::socket_t* sock);
    int recieveFlag;

    std::string ident_string;

    //Version number
    unsigned int VersionNumber;

    //LAPPD ID
    unsigned int LAPPD_ID;

    //Timestamp
    std::string timeSinceEpochMilliseconds;

    //RHT
    float humidity_mon;
    float temperature_mon;
    float temperature_thermistor; 

    //HV
    bool HV_state_set; //Default chosen 
    float HV_volts; //Default chosen 
    int HV_mon;
    float HV_return_mon;

    //LV
    bool LV_state_set; //Default chosen 
    int LV_mon;
    float v33;
    float v25;
    float v12;

    //Saltbridge
    float saltbridge;

    //Emergency variables
    int FLAG_temperature;
    int FLAG_humidity;
    int FLAG_temperature_Thermistor;
    int FLAG_saltbridge;

    float LIMIT_temperature_low; //Default chosen 
    float LIMIT_temperature_high; //Default chosen 
    float LIMIT_humidity_low; //Default chosen 
    float LIMIT_humidity_high; //Default chosen 
    float LIMIT_Thermistor_temperature_low; //Default chosen 
    float LIMIT_Thermistor_temperature_high; //Default chosen 
    float LIMIT_saltbridge_low; //Default chosen 
    float LIMIT_saltbridge_high; //Default chosen 

    //relay
    bool SumRelays;
    bool relayCh1; //Default chosen 
    bool relayCh2; //Default chosen 
    bool relayCh3; //Default chosen 
    bool relayCh1_mon;
    bool relayCh2_mon;
    bool relayCh3_mon;

    //Triggerboard
    float TrigVref; //Default chosen 
    float Trig0_threshold; //Default chosen 
    float Trig0_mon;
    float Trig1_threshold; //Default chosen 
    float Trig1_mon;


    //Light level
    float light;

    //errors
    vector<unsigned int> errorcodes;

    bool SetDefaultValues();
    bool SetDefaultSettings();
    bool Print();

 private:
 
 template <class Archive> void serialize(Archive& ar, const unsigned int version){
    ar & VersionNumber;
    ar & LAPPD_ID;
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
