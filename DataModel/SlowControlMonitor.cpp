#include <SlowControlMonitor.h>

SlowControlMonitor::SlowControlMonitor(){}

bool SlowControlMonitor::Send_Mon(zmq::socket_t* sock){
	std::string tmp="LAPPDMonData";
	zmq::message_t msg0(tmp.length()+1);
	snprintf((char*) msg0.data(), tmp.length()+1, "%s", tmp.c_str());

  int S_errorcodes = errorcodes.size();

	zmq::message_t msg1(sizeof humidity_mon);
	std::memcpy(msg1.data(), &humidity_mon, sizeof humidity_mon);
	zmq::message_t msg2(sizeof temperature_mon);
	std::memcpy(msg2.data(), &temperature_mon, sizeof temperature_mon);
	zmq::message_t msg3(sizeof HV_mon);
	std::memcpy(msg3.data(), &HV_mon, sizeof HV_mon);
	zmq::message_t msg4(sizeof LV_mon);
	std::memcpy(msg4.data(), &LV_mon, sizeof LV_mon);
	zmq::message_t msg5(sizeof FLAG_temperature);
	std::memcpy(msg5.data(), &FLAG_temperature, sizeof FLAG_temperature);
	zmq::message_t msg6(sizeof FLAG_humidity);
	std::memcpy(msg6.data(), &FLAG_humidity, sizeof FLAG_humidity);
	zmq::message_t msg7(sizeof relayCh1_mon);
	std::memcpy(msg7.data(), &relayCh1_mon, sizeof relayCh1_mon);
	zmq::message_t msg8(sizeof relayCh2_mon);
	std::memcpy(msg8.data(), &relayCh2_mon, sizeof relayCh2_mon);
	zmq::message_t msg9(sizeof relayCh3_mon);
	std::memcpy(msg9.data(), &relayCh3_mon, sizeof relayCh3_mon);
	zmq::message_t msg10(sizeof Trig0_mon);
	std::memcpy(msg10.data(), &Trig0_mon, sizeof Trig0_mon);
	zmq::message_t msg11(sizeof Trig1_mon);
	std::memcpy(msg11.data(), &Trig1_mon, sizeof Trig1_mon);
	zmq::message_t msg12(sizeof v33);
	std::memcpy(msg12.data(), &v33, sizeof v33);
	zmq::message_t msg13(sizeof v25);
	std::memcpy(msg13.data(), &v25, sizeof v25);
	zmq::message_t msg14(sizeof v12);
	std::memcpy(msg14.data(), &v12, sizeof v12);
	zmq::message_t msg15(sizeof light);
	std::memcpy(msg15.data(), &light, sizeof light);

  zmq::message_t msg16(sizeof S_errorcodes);
  std::memcpy(msg16.data(), &S_errorcodes, sizeof S_errorcodes);
  zmq::message_t msg17(sizeof(unsigned int) * S_errorcodes);
  std::memcpy(msg17.data(), errorcodes.data(), sizeof(unsigned int) * S_errorcodes);

	sock->send(msg0,ZMQ_SNDMORE);
	sock->send(msg1,ZMQ_SNDMORE);
	sock->send(msg2,ZMQ_SNDMORE);
	sock->send(msg3,ZMQ_SNDMORE);
	sock->send(msg4,ZMQ_SNDMORE);
	sock->send(msg5,ZMQ_SNDMORE);
	sock->send(msg6,ZMQ_SNDMORE);
	sock->send(msg7,ZMQ_SNDMORE);
	sock->send(msg8,ZMQ_SNDMORE);
	sock->send(msg9,ZMQ_SNDMORE);
	sock->send(msg10,ZMQ_SNDMORE);
	sock->send(msg11,ZMQ_SNDMORE);
	sock->send(msg12,ZMQ_SNDMORE);
	sock->send(msg13,ZMQ_SNDMORE);
	sock->send(msg14,ZMQ_SNDMORE);
	sock->send(msg15,ZMQ_SNDMORE);
  sock->send(msg16,ZMQ_SNDMORE);
  sock->send(msg17);
	
  return true;
}

bool SlowControlMonitor::Receive_Mon(zmq::socket_t* sock){

  zmq::message_t msg;
	
  //Ident message string
  sock->recv(&msg);
  ident_string=*(reinterpret_cast<char*>(msg.data()));
  
  //Temperature/Humidity
  sock->recv(&msg);   
  humidity_mon=*(reinterpret_cast<float*>(msg.data()));
  sock->recv(&msg);  
  temperature_mon=*(reinterpret_cast<float*>(msg.data()));

  //HV/LV
  sock->recv(&msg);   
  HV_mon=*(reinterpret_cast<int*>(msg.data()));
  sock->recv(&msg);  
  LV_mon=*(reinterpret_cast<int*>(msg.data()));

  //Emergency
  sock->recv(&msg);
  FLAG_temperature=*(reinterpret_cast<int*>(msg.data())); 
  sock->recv(&msg);
  FLAG_humidity=*(reinterpret_cast<int*>(msg.data())); 

  //Relay
  sock->recv(&msg);
  relayCh1_mon=*(reinterpret_cast<bool*>(msg.data())); 
  sock->recv(&msg);
  relayCh2_mon=*(reinterpret_cast<bool*>(msg.data()));  
  sock->recv(&msg);
  relayCh3_mon=*(reinterpret_cast<bool*>(msg.data())); 
	
  //Triggerboard
  sock->recv(&msg);   
  Trig0_mon=*(reinterpret_cast<float*>(msg.data()));
  sock->recv(&msg);   
  Trig1_mon=*(reinterpret_cast<float*>(msg.data())); 
	
  //Addioions
sock->recv(&msg);   
  v33=*(reinterpret_cast<float*>(msg.data()));	
  sock->recv(&msg);   
  v25=*(reinterpret_cast<float*>(msg.data()));
  sock->recv(&msg);   
  v12=*(reinterpret_cast<float*>(msg.data()));
  sock->recv(&msg);   
  light=*(reinterpret_cast<float*>(msg.data()));

  sock->recv(&msg);
  int tmp_size=0;
  tmp_size=*(reinterpret_cast<int*>(msg.data()));
  if(tmp_size>0)

 {
    sock->recv(&msg);
    errorcodes.resize(msg.size()/sizeof(unsigned int));
    std::memcpy(&errorcodes[0], msg.data(), msg.size());
  }
	
  return true;
}

bool SlowControlMonitor::Send_Config(zmq::socket_t* sock){

	zmq::message_t msg1(sizeof recieveFlag);
	std::memcpy(msg1.data(), &recieveFlag, sizeof recieveFlag);
	zmq::message_t msg2(sizeof HV_state_set);
	std::memcpy(msg2.data(), &HV_state_set, sizeof HV_state_set);
	zmq::message_t msg3(sizeof HV_volts);
	std::memcpy(msg3.data(), &HV_volts, sizeof HV_volts);
	zmq::message_t msg4(sizeof LV_state_set);
	std::memcpy(msg4.data(), &LV_state_set, sizeof LV_state_set);
	zmq::message_t msg5(sizeof LIMIT_temperature_low);
	std::memcpy(msg5.data(), &LIMIT_temperature_low, sizeof LIMIT_temperature_low);
	zmq::message_t msg6(sizeof LIMIT_temperature_high);
	std::memcpy(msg6.data(), &LIMIT_temperature_high, sizeof LIMIT_temperature_high);
	zmq::message_t msg7(sizeof LIMIT_humidity_low);
	std::memcpy(msg7.data(), &LIMIT_humidity_low, sizeof LIMIT_humidity_low);
	zmq::message_t msg8(sizeof LIMIT_humidity_high);
	std::memcpy(msg8.data(), &LIMIT_humidity_high, sizeof LIMIT_humidity_high);
	zmq::message_t msg9(sizeof Trig0_threshold);
	std::memcpy(msg9.data(), &Trig0_threshold, sizeof Trig0_threshold);
	zmq::message_t msg10(sizeof Trig1_threshold);
	std::memcpy(msg10.data(), &Trig1_threshold, sizeof Trig1_threshold);
	zmq::message_t msg11(sizeof TrigVref);
	std::memcpy(msg11.data(), &TrigVref, sizeof TrigVref);
	zmq::message_t msg12(sizeof relayCh1);
	std::memcpy(msg12.data(), &relayCh1, sizeof relayCh1);
	zmq::message_t msg13(sizeof relayCh2);
	std::memcpy(msg13.data(), &relayCh2, sizeof relayCh2);
	zmq::message_t msg14(sizeof relayCh3);
	std::memcpy(msg14.data(), &relayCh3, sizeof relayCh3);

  sock->send(msg1,ZMQ_SNDMORE);
  sock->send(msg2,ZMQ_SNDMORE);
  sock->send(msg3,ZMQ_SNDMORE);
  sock->send(msg4,ZMQ_SNDMORE);
  sock->send(msg5,ZMQ_SNDMORE);
  sock->send(msg6,ZMQ_SNDMORE);
  sock->send(msg7,ZMQ_SNDMORE);
  sock->send(msg8,ZMQ_SNDMORE);
  sock->send(msg9,ZMQ_SNDMORE);
  sock->send(msg10,ZMQ_SNDMORE);
  sock->send(msg11,ZMQ_SNDMORE);
  sock->send(msg12,ZMQ_SNDMORE);
  sock->send(msg13,ZMQ_SNDMORE);
  sock->send(msg14);

  return true;
}

bool SlowControlMonitor::Receive_Config(zmq::socket_t* sock){

  zmq::message_t msg;
  
  //flag
  sock->recv(&msg);
  recieveFlag=*(reinterpret_cast<int*>(msg.data())); 

  //HV
  sock->recv(&msg);   
  HV_state_set=*(reinterpret_cast<bool*>(msg.data()));
  sock->recv(&msg);   
  HV_volts=*(reinterpret_cast<float*>(msg.data())); 

  //LV
  sock->recv(&msg);  
  LV_state_set=*(reinterpret_cast<bool*>(msg.data()));

  //Emergency
  sock->recv(&msg);
  LIMIT_temperature_low=*(reinterpret_cast<float*>(msg.data())); 
  sock->recv(&msg);
  LIMIT_temperature_high=*(reinterpret_cast<float*>(msg.data())); 
  sock->recv(&msg);
  LIMIT_humidity_low=*(reinterpret_cast<float*>(msg.data())); 
  sock->recv(&msg);
  LIMIT_humidity_high=*(reinterpret_cast<float*>(msg.data())); 

  //Triggerboard
  sock->recv(&msg);   
  Trig0_threshold=*(reinterpret_cast<float*>(msg.data()));
  sock->recv(&msg);   
  Trig1_threshold=*(reinterpret_cast<float*>(msg.data())); 
  sock->recv(&msg);   
  TrigVref=*(reinterpret_cast<float*>(msg.data()));

  //relay
  sock->recv(&msg);
  relayCh1=*(reinterpret_cast<bool*>(msg.data())); 
  sock->recv(&msg);
  relayCh2=*(reinterpret_cast<bool*>(msg.data()));  
  sock->recv(&msg);
  relayCh3=*(reinterpret_cast<bool*>(msg.data())); 
  
  return true;
}

bool SlowControlMonitor::SetDefaults(){
  HV_state_set = 0;
  HV_volts = 100;
  LV_state_set = 1;

  LIMIT_temperature_low=30;
  LIMIT_temperature_high=50;
  LIMIT_humidity_low=50;
  LIMIT_humidity_high=80; 
  Trig0_threshold=1.5;
  Trig1_threshold=1.5;
  TrigVref=2.981;

 //relay
  relayCh1=false; 
  relayCh2=false;
  relayCh3=true;
	
  return true;
}


bool SlowControlMonitor::Print(){

  std::cout << "humidity = " << humidity_mon << std::endl;
  std::cout << "temperature = " << temperature_mon << std::endl;
  std::cout << "HV state should be " << std::boolalpha << HV_state_set << " and is " << std::boolalpha << HV_mon << " and is set to " << HV_volts << " V" << std::endl;
  std::cout << "LV state should be " << std::boolalpha << LV_state_set << " and is " << std::boolalpha << LV_mon << std::endl;
  std::cout << "LV voltages are V(3.3)= " << v33 << "V, V(2.5)= " << v25 << "V, V(1.2)= " << v12 << "V" << std::endl;	
  std::cout << "Temperature warning flag is " << std::boolalpha << FLAG_temperature << std::endl;
  std::cout << "Humidity warning flag is " << std::boolalpha << FLAG_humidity << std::endl;
  std::cout << "Relay 1 is " << std::boolalpha << relayCh1_mon << std::endl;
  std::cout << "Relay 2 is " << std::boolalpha << relayCh2_mon << std::endl;
  std::cout << "Relay 3 is " << std::boolalpha << relayCh3_mon << std::endl;
  std::cout << "Threshold for DAC 0 is " << Trig0_mon << " V" << std::endl;
  std::cout << "Threshold for DAC 1 is " << Trig1_mon << " V" << std::endl;
  std::cout << "Photodiode return is " << light << std::endl;
	if(errorcodes.size()==1 && errorcodes[0]==0x00000000)
  {
    printf("No errorcodes found all good: 0x%08x\n", errorcodes[0]);
  }else
  {
    printf("Errorcodes found: %li\n", errorcodes.size());
  }
  return true;
}
