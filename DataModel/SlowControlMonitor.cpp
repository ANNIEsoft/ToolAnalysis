#include <SlowControlMonitor.h>

SlowControlMonitor::SlowControlMonitor()
{
    VersionNumber = 0x0007;
	recieveFlag = 1;
	LAPPD_ID = 0;
	SetDefaultSettings();
	SetDefaultValues();
}


SlowControlMonitor::SlowControlMonitor(unsigned int id)
{
    VersionNumber = 0x0007;
	recieveFlag = 1;
	LAPPD_ID = id;
	SetDefaultSettings();
	SetDefaultValues();
}

SlowControlMonitor::~SlowControlMonitor()
{}

bool SlowControlMonitor::Send_Mon(zmq::socket_t* sock){
	std::string tmp="LAPPDMon";
	zmq::message_t msg0(tmp.length()+1);
	snprintf((char*) msg0.data(), tmp.length()+1, "%s", tmp.c_str());

  	int S_errorcodes = errorcodes.size();
	
	//Version send
	zmq::message_t msgV(sizeof VersionNumber);
	std::memcpy(msgV.data(), &VersionNumber, sizeof VersionNumber);	

	//LAPPD_ID
	zmq::message_t msgID(sizeof LAPPD_ID);
	std::memcpy(msgID.data(), &LAPPD_ID, sizeof LAPPD_ID);	

	//Timestamp
	zmq::message_t msgTime(timeSinceEpochMilliseconds.length()+1);
	snprintf((char*) msgTime.data(), timeSinceEpochMilliseconds.length()+1, "%s", timeSinceEpochMilliseconds.c_str());	

	//Sensor send
	zmq::message_t msgHum(sizeof humidity_mon);
	std::memcpy(msgHum.data(), &humidity_mon, sizeof humidity_mon);
	zmq::message_t msgTemp1(sizeof temperature_mon);
	std::memcpy(msgTemp1.data(), &temperature_mon, sizeof temperature_mon);
	zmq::message_t msgTemp2(sizeof temperature_thermistor);
	std::memcpy(msgTemp2.data(), &temperature_thermistor, sizeof temperature_thermistor);	
	
	//HV send
	zmq::message_t msgHVstate(sizeof HV_mon);
	std::memcpy(msgHVstate.data(), &HV_mon, sizeof HV_mon);
	zmq::message_t msgHV(sizeof HV_return_mon);
	std::memcpy(msgHV.data(), &HV_return_mon, sizeof HV_return_mon);
	
	//LV send
	zmq::message_t msgLVstate(sizeof LV_mon);
	std::memcpy(msgLVstate.data(), &LV_mon, sizeof LV_mon);
	
	//FLAG for emergency send
	zmq::message_t msgFT(sizeof FLAG_temperature);
	std::memcpy(msgFT.data(), &FLAG_temperature, sizeof FLAG_temperature);
	zmq::message_t msgFH(sizeof FLAG_humidity);
	std::memcpy(msgFH.data(), &FLAG_humidity, sizeof FLAG_humidity);
	zmq::message_t msgFTT(sizeof FLAG_temperature_Thermistor );
	std::memcpy(msgFTT.data(), &FLAG_temperature_Thermistor , sizeof FLAG_temperature_Thermistor );
	zmq::message_t msgFS(sizeof FLAG_saltbridge );
	std::memcpy(msgFS.data(), &FLAG_saltbridge , sizeof FLAG_saltbridge );

	//Relay send
	zmq::message_t msgR1(sizeof relayCh1_mon);
	std::memcpy(msgR1.data(), &relayCh1_mon, sizeof relayCh1_mon);
	zmq::message_t msgR2(sizeof relayCh2_mon);
	std::memcpy(msgR2.data(), &relayCh2_mon, sizeof relayCh2_mon);
	zmq::message_t msgR3(sizeof relayCh3_mon);
	std::memcpy(msgR3.data(), &relayCh3_mon, sizeof relayCh3_mon);
	
	//Trigger threshold send
	zmq::message_t msgTrig0(sizeof Trig0_mon);
	std::memcpy(msgTrig0.data(), &Trig0_mon, sizeof Trig0_mon);
	zmq::message_t msgTrig1(sizeof Trig1_mon);
	std::memcpy(msgTrig1.data(), &Trig1_mon, sizeof Trig1_mon);
	
	//VV send
	zmq::message_t msgV33(sizeof v33);
	std::memcpy(msgV33.data(), &v33, sizeof v33);
	zmq::message_t msgV25(sizeof v25);
	std::memcpy(msgV25.data(), &v25, sizeof v25);
	zmq::message_t msgV12(sizeof v12);
	std::memcpy(msgV12.data(), &v12, sizeof v12);
	
	//Photosensor send
	zmq::message_t msgP(sizeof light);
	std::memcpy(msgP.data(), &light, sizeof light);
	
	//Saltbridge send
	zmq::message_t msgSalt(sizeof saltbridge );
	std::memcpy(msgSalt.data(), &saltbridge , sizeof saltbridge );	

	//Error send
	zmq::message_t msgES(sizeof S_errorcodes);
	std::memcpy(msgES.data(), &S_errorcodes, sizeof S_errorcodes);
	zmq::message_t msgE(sizeof(unsigned int) * S_errorcodes);
	std::memcpy(msgE.data(), errorcodes.data(), sizeof(unsigned int) * S_errorcodes);

	sock->send(msg0,ZMQ_SNDMORE);
	sock->send(msgV,ZMQ_SNDMORE);
	sock->send(msgID,ZMQ_SNDMORE);
	sock->send(msgTime,ZMQ_SNDMORE);
	sock->send(msgHum,ZMQ_SNDMORE);
	sock->send(msgTemp1,ZMQ_SNDMORE);
	sock->send(msgTemp2,ZMQ_SNDMORE);
	sock->send(msgHVstate,ZMQ_SNDMORE);
	sock->send(msgHV,ZMQ_SNDMORE);
	sock->send(msgLVstate,ZMQ_SNDMORE);
	sock->send(msgFT,ZMQ_SNDMORE);
	sock->send(msgFH,ZMQ_SNDMORE);
	sock->send(msgFTT,ZMQ_SNDMORE);
	sock->send(msgFS,ZMQ_SNDMORE);
	sock->send(msgR1,ZMQ_SNDMORE);
	sock->send(msgR2,ZMQ_SNDMORE);
	sock->send(msgR3,ZMQ_SNDMORE);
	sock->send(msgTrig0,ZMQ_SNDMORE);
	sock->send(msgTrig1,ZMQ_SNDMORE);
	sock->send(msgV33,ZMQ_SNDMORE);
	sock->send(msgV25,ZMQ_SNDMORE);
	sock->send(msgV12,ZMQ_SNDMORE);
	sock->send(msgP,ZMQ_SNDMORE);
	sock->send(msgSalt,ZMQ_SNDMORE);
	sock->send(msgES,ZMQ_SNDMORE);
	//sock->send(msgE);
	
	sock->send(msgE,ZMQ_SNDMORE);
	Send_Config(sock);
	
  	return true;
}

bool SlowControlMonitor::Receive_Mon(zmq::socket_t* sock){

	zmq::message_t msg;

	//Ident message string
	//	sock->recv(&msg);
	//	ident_string=*(reinterpret_cast<char*>(msg.data()));

	//Version 
	unsigned int tVersion;
	sock->recv(&msg);
	tVersion=*(reinterpret_cast<unsigned int*>(msg.data()));
	if(VersionNumber != tVersion)
	{
		std::cout << "Wrong version number! Please check immediately!" << std::endl;
		//return false;
	}

	//LAPPD_ID
	sock->recv(&msg);   
	LAPPD_ID=*(reinterpret_cast<unsigned int*>(msg.data()));

	//Timestamp
	sock->recv(&msg);   
	std::stringstream iss(static_cast<char*>(msg.data()));
	iss >> timeSinceEpochMilliseconds;   
	//timeSinceEpochMilliseconds=*(reinterpret_cast<char*>(msg.data()));

	//Temperature/Humidity
	sock->recv(&msg);   
	humidity_mon=*(reinterpret_cast<float*>(msg.data()));
	sock->recv(&msg);  
	temperature_mon=*(reinterpret_cast<float*>(msg.data()));
	sock->recv(&msg);  
	temperature_thermistor=*(reinterpret_cast<float*>(msg.data()));

	//HV
	sock->recv(&msg);   
	HV_mon=*(reinterpret_cast<int*>(msg.data())); 
	sock->recv(&msg);   
	HV_return_mon=*(reinterpret_cast<float*>(msg.data()));
	
	//LV
	sock->recv(&msg);  
	LV_mon=*(reinterpret_cast<int*>(msg.data()));

	//Emergency
	sock->recv(&msg);
	FLAG_temperature=*(reinterpret_cast<int*>(msg.data())); 
	sock->recv(&msg);
	FLAG_humidity=*(reinterpret_cast<int*>(msg.data())); 
	sock->recv(&msg);
	FLAG_temperature_Thermistor=*(reinterpret_cast<int*>(msg.data())); 
	sock->recv(&msg);
	FLAG_saltbridge=*(reinterpret_cast<int*>(msg.data())); 

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

	//VV
	sock->recv(&msg);   
	v33=*(reinterpret_cast<float*>(msg.data()));	
	sock->recv(&msg);   
	v25=*(reinterpret_cast<float*>(msg.data()));
	sock->recv(&msg);   
	v12=*(reinterpret_cast<float*>(msg.data()));
	
	//Light
	sock->recv(&msg);   
	light=*(reinterpret_cast<float*>(msg.data()));
	
	//Saltbridge
	sock->recv(&msg);   
	saltbridge=*(reinterpret_cast<float*>(msg.data()));
	
	//Error
	sock->recv(&msg);
	int tmp_size=0;
	tmp_size=*(reinterpret_cast<int*>(msg.data()));
	if(tmp_size>0)
	{
		sock->recv(&msg);
		errorcodes.resize(msg.size()/sizeof(unsigned int));
		std::memcpy(&errorcodes[0], msg.data(), msg.size());
	}

	Receive_Config(sock);
	
	return true;
}

bool SlowControlMonitor::Send_Config(zmq::socket_t* sock){

	//Version send
	zmq::message_t msgV(sizeof VersionNumber);
	std::memcpy(msgV.data(), &VersionNumber, sizeof VersionNumber);		
	
	//Receive flag working as a run mode flag
	zmq::message_t msgR(sizeof recieveFlag);
	std::memcpy(msgR.data(), &recieveFlag, sizeof recieveFlag);
	
	//HV
	zmq::message_t msgHVstate(sizeof HV_state_set);
	std::memcpy(msgHVstate.data(), &HV_state_set, sizeof HV_state_set);
	zmq::message_t msgHV(sizeof HV_volts);
	std::memcpy(msgHV.data(), &HV_volts, sizeof HV_volts);
	
	//LV
	zmq::message_t msgLV(sizeof LV_state_set);
	std::memcpy(msgLV.data(), &LV_state_set, sizeof LV_state_set);
	
	//Limits for emergency trigger
	zmq::message_t msgLTL(sizeof LIMIT_temperature_low);
	std::memcpy(msgLTL.data(), &LIMIT_temperature_low, sizeof LIMIT_temperature_low);
	zmq::message_t msgLTH(sizeof LIMIT_temperature_high);
	std::memcpy(msgLTH.data(), &LIMIT_temperature_high, sizeof LIMIT_temperature_high);
	zmq::message_t msgLHL(sizeof LIMIT_humidity_low);
	std::memcpy(msgLHL.data(), &LIMIT_humidity_low, sizeof LIMIT_humidity_low);
	zmq::message_t msgLHH(sizeof LIMIT_humidity_high);
	std::memcpy(msgLHH.data(), &LIMIT_humidity_high, sizeof LIMIT_humidity_high);
	zmq::message_t msgLTTL(sizeof LIMIT_Thermistor_temperature_low);
	std::memcpy(msgLTTL.data(), &LIMIT_Thermistor_temperature_low , sizeof LIMIT_Thermistor_temperature_low);
	zmq::message_t msgLTTH(sizeof LIMIT_Thermistor_temperature_high);
	std::memcpy(msgLTTH.data(), &LIMIT_Thermistor_temperature_high, sizeof LIMIT_Thermistor_temperature_high);
	zmq::message_t msgLSL(sizeof LIMIT_saltbridge_low);
	std::memcpy(msgLSL.data(), &LIMIT_saltbridge_low , sizeof LIMIT_saltbridge_low);
	zmq::message_t msgLSH(sizeof LIMIT_saltbridge_high);
	std::memcpy(msgLSH.data(), &LIMIT_saltbridge_high, sizeof LIMIT_saltbridge_high);
	
	//Triggerboard
	zmq::message_t msgTrig0(sizeof Trig0_threshold);
	std::memcpy(msgTrig0.data(), &Trig0_threshold, sizeof Trig0_threshold);
	zmq::message_t msgTrig1(sizeof Trig1_threshold);
	std::memcpy(msgTrig1.data(), &Trig1_threshold, sizeof Trig1_threshold);
	zmq::message_t msgTrigRef(sizeof TrigVref);
	std::memcpy(msgTrigRef.data(), &TrigVref, sizeof TrigVref);
	
	//Relays
	zmq::message_t msgR1(sizeof relayCh1);
	std::memcpy(msgR1.data(), &relayCh1, sizeof relayCh1);
	zmq::message_t msgR2(sizeof relayCh2);
	std::memcpy(msgR2.data(), &relayCh2, sizeof relayCh2);
	zmq::message_t msgR3(sizeof relayCh3);
	std::memcpy(msgR3.data(), &relayCh3, sizeof relayCh3);

	sock->send(msgV,ZMQ_SNDMORE);
	sock->send(msgR,ZMQ_SNDMORE);
	sock->send(msgHVstate,ZMQ_SNDMORE);
	sock->send(msgHV,ZMQ_SNDMORE);
	sock->send(msgLV,ZMQ_SNDMORE);
	sock->send(msgLTL,ZMQ_SNDMORE);
	sock->send(msgLTH,ZMQ_SNDMORE);
	sock->send(msgLHL,ZMQ_SNDMORE);
	sock->send(msgLHH,ZMQ_SNDMORE);
	sock->send(msgLTTL,ZMQ_SNDMORE);
	sock->send(msgLTTH,ZMQ_SNDMORE);
	sock->send(msgLSL,ZMQ_SNDMORE);
	sock->send(msgLSH,ZMQ_SNDMORE);
	sock->send(msgTrig0,ZMQ_SNDMORE);
	sock->send(msgTrig1,ZMQ_SNDMORE);
	sock->send(msgTrigRef,ZMQ_SNDMORE);
	sock->send(msgR1,ZMQ_SNDMORE);
	sock->send(msgR2,ZMQ_SNDMORE);
	sock->send(msgR3);

	return true;
}

bool SlowControlMonitor::Receive_Config(zmq::socket_t* sock){

	zmq::message_t msg;
	
	//Version 
	unsigned int tVersion;
	sock->recv(&msg);
	tVersion=*(reinterpret_cast<unsigned int*>(msg.data()));
	if(VersionNumber != tVersion)
	{
		std::cout << "Wrong version number! Please check immediately!" << std::endl;
		return false;
	}

	//Receive flag
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
	sock->recv(&msg);
	LIMIT_Thermistor_temperature_low =*(reinterpret_cast<float*>(msg.data())); 
	sock->recv(&msg);
	LIMIT_Thermistor_temperature_high =*(reinterpret_cast<float*>(msg.data())); 
	sock->recv(&msg);
	LIMIT_saltbridge_low =*(reinterpret_cast<float*>(msg.data())); 
	sock->recv(&msg);
	LIMIT_saltbridge_high =*(reinterpret_cast<float*>(msg.data())); 

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

bool SlowControlMonitor::SetDefaultValues(){
	//Timestamp
	timeSinceEpochMilliseconds="";

	//RHT
	humidity_mon=-1;
	temperature_mon=-1;
	temperature_thermistor=-1; 

	//HV
	HV_mon=-1;
	HV_return_mon =-1;

	//LV
	LV_mon=-1;
	v33=-1;
	v25=-1;
	v12=-1;

	//Saltbridge
	saltbridge = -1;

	//Emergency variables
	FLAG_temperature = 0;
	FLAG_humidity = 0;
	FLAG_temperature_Thermistor = 0;
	FLAG_saltbridge = 0;

	//relay
	relayCh1_mon;
	relayCh2_mon;
	relayCh3_mon;

	//Triggerboard
	Trig0_mon=-1;
	Trig1_mon=-1;

	//Light level
	light=-1;

	return true;
}

bool SlowControlMonitor::SetDefaultSettings(){
	//HV
	HV_state_set=false; //Default chosen 
	HV_volts=0; //Default chosen 

	//LV
	LV_state_set=false; //Default chosen 

	LIMIT_temperature_low = 50; //Default chosen 
	LIMIT_temperature_high = 58; //Default chosen 
	LIMIT_humidity_low = 15; //Default chosen 
	LIMIT_humidity_high = 20; //Default chosen 
	LIMIT_Thermistor_temperature_low = 7000; //Default chosen 
	LIMIT_Thermistor_temperature_high = 5800; //Default chosen 
	LIMIT_saltbridge_low = 500000; //Default chosen 
	LIMIT_saltbridge_high = 400000; //Default chosen 

	//relay
	SumRelays=false;
	relayCh1=false; //Default chosen 
	relayCh2=false; //Default chosen 
	relayCh3=false; //Default chosen 

	//Triggerboard
	TrigVref=2.981; //Default chosen 
	Trig0_threshold=1.223; //Default chosen 
	Trig1_threshold=1.23; //Default chosen 

	return true;
}


bool SlowControlMonitor::Print(){

	std::cout << "LAPPD ID is " << LAPPD_ID << std::endl;
	std::cout << "Timestamp ms since epoch = " << timeSinceEpochMilliseconds << std::endl;
	std::cout << "humidity = " << humidity_mon << std::endl;
	std::cout << "temperature = " << temperature_mon << std::endl;
	std::cout << "thermistor = " << temperature_thermistor << std::endl;
	std::cout << "HV state should be " << std::boolalpha << HV_state_set << " and is " << std::boolalpha << HV_mon << " and is set to " << HV_volts << " V" << std::endl;
	std::cout << "Returned HV is " << HV_return_mon << "V" << std::endl;
	std::cout << "LV state should be " << std::boolalpha << LV_state_set << " and is " << std::boolalpha << LV_mon << std::endl;
	std::cout << "LV voltages are V(3.3)= " << v33 << "V, V(2.5)= " << v25 << "V, V(1.2)= " << v12 << "V" << std::endl;	
	std::cout << "Temperature warning flag is " << std::boolalpha << FLAG_temperature << std::endl;
	std::cout << "Humidity warning flag is " << std::boolalpha << FLAG_humidity << std::endl;
	std::cout << "Temperature 2 warning flag is " << std::boolalpha << FLAG_temperature_Thermistor << std::endl;
	std::cout << "Saltbridge warning flag is " << std::boolalpha << FLAG_saltbridge << std::endl;
	std::cout << "Relay 1 is " << std::boolalpha << relayCh1_mon << std::endl;
	std::cout << "Relay 2 is " << std::boolalpha << relayCh2_mon << std::endl;
	std::cout << "Relay 3 is " << std::boolalpha << relayCh3_mon << std::endl;
	std::cout << "Threshold for DAC 0 is " << Trig0_mon << " V" << std::endl;
	std::cout << "Threshold for DAC 1 is " << Trig1_mon << " V" << std::endl;
	std::cout << "Photodiode return is " << light << std::endl;
	std::cout << "Saltbridge return is " << saltbridge << std::endl;
	if(errorcodes.size()==1 && errorcodes[0]==0x00000000)
	{
		printf("No errorcodes found all good: 0x%08x\n", errorcodes[0]);
	}else
	{
		printf("Errorcodes found: %li\n", errorcodes.size());
		for(unsigned int k: errorcodes)
		{
			printf("0x%08x\n",k);	
		}
	}
	
	return true;
}
