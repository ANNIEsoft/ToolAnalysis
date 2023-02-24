#include <PsecData.h>

PsecData::PsecData()
{
	VersionNumber = 0x0003;
	LAPPD_ID = 0;
    SetDefaults();
}

PsecData::PsecData(unsigned int id)
{
	VersionNumber = 0x0003;
	LAPPD_ID = id;
    SetDefaults();
}

PsecData::~PsecData()
{}

bool PsecData::Send(zmq::socket_t* sock)
{
	int S_RawWaveform = RawWaveform.size();
	int S_AccInfoFrame = AccInfoFrame.size();
	int S_errorcodes = errorcodes.size();
	int S_BoardIndex = BoardIndex.size();

	std::cout<<"S1"<<std::endl;
	
	zmq::message_t msgV(sizeof VersionNumber);
	std::memcpy(msgV.data(), &VersionNumber, sizeof VersionNumber);

	zmq::message_t msgID(sizeof LAPPD_ID);
	std::memcpy(msgID.data(), &LAPPD_ID, sizeof LAPPD_ID);

	zmq::message_t msgTime(Timestamp.length()+1);
	snprintf((char*) msgTime.data(), Timestamp.length()+1, "%s", Timestamp.c_str());	

	zmq::message_t msgSB(sizeof S_BoardIndex);
	std::memcpy(msgSB.data(), &S_BoardIndex, sizeof S_BoardIndex);
	zmq::message_t msgB(sizeof(int) * S_BoardIndex);
	std::memcpy(msgB.data(), BoardIndex.data(), sizeof(int) * S_BoardIndex);

	zmq::message_t msgSA(sizeof S_AccInfoFrame);
	std::memcpy(msgSA.data(), &S_AccInfoFrame, sizeof S_AccInfoFrame);
	zmq::message_t msgA(sizeof(unsigned short) * S_AccInfoFrame);
	std::memcpy(msgA.data(), AccInfoFrame.data(), sizeof(unsigned short) * S_AccInfoFrame);

	zmq::message_t msgSW(sizeof S_RawWaveform);
	std::memcpy(msgSW.data(), &S_RawWaveform, sizeof S_RawWaveform);		
	zmq::message_t msgW(sizeof(unsigned short) * S_RawWaveform);
	std::memcpy(msgW.data(), RawWaveform.data(), sizeof(unsigned short) * S_RawWaveform);

	zmq::message_t msgSE(sizeof S_errorcodes);
	std::memcpy(msgSE.data(), &S_errorcodes, sizeof S_errorcodes);
	zmq::message_t msgE(sizeof(unsigned int) * S_errorcodes);
	std::memcpy(msgE.data(), errorcodes.data(), sizeof(unsigned int) * S_errorcodes);

	zmq::message_t msgF(sizeof FailedReadCounter);
	std::memcpy(msgF.data(), &FailedReadCounter, sizeof FailedReadCounter);
	std::cout<<"S2"<<std::endl;
	sock->send(msgV,ZMQ_SNDMORE);
	std::cout<<"S3"<<std::endl;
	sock->send(msgID,ZMQ_SNDMORE);
	std::cout<<"S4"<<std::endl;
	sock->send(msgTime,ZMQ_SNDMORE);
	std::cout<<"S5"<<std::endl;
	sock->send(msgSB,ZMQ_SNDMORE);
	std::cout<<"S6"<<std::endl;
	if(S_BoardIndex>0){sock->send(msgB,ZMQ_SNDMORE);}
	std::cout<<"S7"<<std::endl;
	sock->send(msgSA,ZMQ_SNDMORE);
	std::cout<<"S8"<<std::endl;
	if(S_AccInfoFrame>0){sock->send(msgA,ZMQ_SNDMORE);}
	std::cout<<"S9"<<std::endl;
	sock->send(msgSW,ZMQ_SNDMORE);
	std::cout<<"S10"<<std::endl;
	if(S_RawWaveform>0){sock->send(msgW,ZMQ_SNDMORE);}
	std::cout<<"S11"<<std::endl;
	sock->send(msgSE,ZMQ_SNDMORE);
	std::cout<<"S12"<<std::endl;
	if(S_errorcodes>0){sock->send(msgE,ZMQ_SNDMORE);}
	std::cout<<"S13"<<std::endl;
	sock->send(msgF);
	std::cout<<"S14"<<std::endl;

	return true;
}


bool PsecData::Receive(zmq::socket_t* sock)
{
	zmq::message_t msg;
	int tmp_size=0;

	sock->recv(&msg);
	unsigned int tempVersionNumber;
	tempVersionNumber=*(reinterpret_cast<unsigned int*>(msg.data())); 
	/*if(tempVersionNumber != VersionNumber)
	{
		return false;
	}
	*/

	//ID
	sock->recv(&msg);
	LAPPD_ID=*(reinterpret_cast<unsigned int*>(msg.data())); 

	//Timestamp
	sock->recv(&msg);
	std::stringstream iss(static_cast<char*>(msg.data()));
	iss >> Timestamp;   

	//Boards
	sock->recv(&msg);
	tmp_size=0;
	tmp_size=*(reinterpret_cast<int*>(msg.data()));
	if(tmp_size>0)
	{
		sock->recv(&msg);
		BoardIndex.resize(msg.size()/sizeof(int));
		std::memcpy(&BoardIndex[0], msg.data(), msg.size());
	}

	//ACC
	sock->recv(&msg);
	tmp_size=0;
	tmp_size=*(reinterpret_cast<int*>(msg.data()));
	if(tmp_size>0)
	{
		sock->recv(&msg);
		AccInfoFrame.resize(msg.size()/sizeof(unsigned short));
		std::memcpy(&AccInfoFrame[0], msg.data(), msg.size());
	}

	//Waveforms
	sock->recv(&msg);
	tmp_size=0;
	tmp_size=*(reinterpret_cast<int*>(msg.data()));
	if(tmp_size>0)
	{
		sock->recv(&msg);
		RawWaveform.resize(msg.size()/sizeof(unsigned short));
		std::memcpy(&RawWaveform[0], msg.data(), msg.size());
	}

	//Errors
	sock->recv(&msg);
	tmp_size=0;
	tmp_size=*(reinterpret_cast<int*>(msg.data()));
	if(tmp_size>0)
	{
		sock->recv(&msg);
		errorcodes.resize(msg.size()/sizeof(unsigned int));
		std::memcpy(&errorcodes[0], msg.data(), msg.size());
	}
	
	sock->recv(&msg);
	FailedReadCounter=*(reinterpret_cast<int*>(msg.data()));

	return true;
}

bool PsecData::Receive(std::queue<zmq::message_t> &message_queue)
{
  //printf("w1 size=%d\n",message_queue.size());
  if(message_queue.size() <8){
    //printf("w1\n");
    return false;
  }
  
  int tmp_size=0;
  
  unsigned int tempVersionNumber;
  tempVersionNumber=*(reinterpret_cast<unsigned int*>(message_queue.front().data())); 
  message_queue.pop();
  //  if(tempVersionNumber != VersionNumber) return false;
  //printf("tempVersionNumber=%d\n",tempVersionNumber);  
  
  //ID
  LAPPD_ID=*(reinterpret_cast<unsigned int*>(message_queue.front().data())); 
  message_queue.pop();         
  
  //printf("LAPPD_ID=%d\n",LAPPD_ID);

  //Timestamp
  std::stringstream iss(static_cast<char*>(message_queue.front().data()));
  iss >> Timestamp;   
  message_queue.pop();
  
  //printf("Timestamp=%s\n",Timestamp.c_str());

  //Boards
  tmp_size=0;
  tmp_size=*(reinterpret_cast<int*>(message_queue.front().data()));
  message_queue.pop();   

  //printf("tmp_size=%d\n",tmp_size);
  //printf("w2 size=%d\n",message_queue.size());  

  if(tmp_size>0 &&  message_queue.size()>=5){
    BoardIndex.resize(message_queue.front().size()/sizeof(int));
    std::memcpy(&BoardIndex[0], message_queue.front().data(), message_queue.front().size());
    message_queue.pop();
  }
  else if(tmp_size!=0){
    //printf("w2\n");
    return false;
  }

  //ACC
  tmp_size=0;
  tmp_size=*(reinterpret_cast<int*>(message_queue.front().data()));
  message_queue.pop();

  //printf("tmp_size=%d\n",tmp_size);
  //printf("w3 size=%d\n",message_queue.size());

  if(tmp_size>0 &&  message_queue.size()>=4){
    AccInfoFrame.resize(message_queue.front().size()/sizeof(unsigned short));
    std::memcpy(&AccInfoFrame[0], message_queue.front().data(), message_queue.front().size());
    message_queue.pop();	
  }
  else if(tmp_size!=0){
    //printf("w3\n");
    return false;
  }

  //Waveforms
  tmp_size=0;
  tmp_size=*(reinterpret_cast<int*>(message_queue.front().data()));
  message_queue.pop();

  //printf("tmp_size=%d\n",tmp_size);  
  //printf("w4 size=%d\n",message_queue.size());

  if(tmp_size>0  &&  message_queue.size()>=3){
    RawWaveform.resize(message_queue.front().size()/sizeof(unsigned short));
    std::memcpy(&RawWaveform[0], message_queue.front().data(), message_queue.front().size());
    message_queue.pop();
  }
  else if(tmp_size!=0){
    //printf("w4\n");
    return false;
  }

  //Errors
  tmp_size=0;
  tmp_size=*(reinterpret_cast<int*>(message_queue.front().data()));
  message_queue.pop();
  
  //printf("tmp_size=%d\n",tmp_size);
  //printf("w5 size=%d\n",message_queue.size());

  if(tmp_size>0  &&  message_queue.size()>=2){
    errorcodes.resize(message_queue.front().size()/sizeof(unsigned int));
    std::memcpy(&errorcodes[0], message_queue.front().data(), message_queue.front().size());
    message_queue.pop();
  }
  else if(tmp_size!=0) {
    //printf("w5\n");
    return false;
  }

  FailedReadCounter=*(reinterpret_cast<int*>(message_queue.front().data()));
  message_queue.pop();    
  //printf("FailedReadCounter=%d\n",FailedReadCounter);  

  return true;
}


bool PsecData::SetDefaults()
{
	Timestamp="";
	FailedReadCounter=0;
	readRetval=0;
    
	return true;
}


bool PsecData::Print(){
	std::cout << "----------------------Sent data---------------------------" << std::endl;
	printf("Version number: 0x%04x\n", VersionNumber);
	printf("LAPPD ID: %i\n", LAPPD_ID);
	for(int i=0; i<BoardIndex.size(); i++)
	{
		printf("Board number: %i\n", BoardIndex[i]);
	}
	printf("Failed read attempts: %i\n", FailedReadCounter);
	printf("Waveform size: %li\n", RawWaveform.size());
	printf("ACC Infoframe size: %li\n", AccInfoFrame.size());
	if(errorcodes.size()==1 && errorcodes[0]==0x00000000)
	{
		printf("No errorcodes found all good: 0x%08x\n", errorcodes[0]);
	}else
	{
		printf("Errorcodes found: %li\n", errorcodes.size());
		for(unsigned int k=0; k<errorcodes.size(); k++)
		{
			printf("Errorcode: 0x%08x\n", errorcodes[k]);
	
		}
	}
	std::cout << "----------------------------------------------------------" << std::endl;
	
	return true;
}
