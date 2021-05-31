#include <PsecData.h>

PsecData::PsecData(){}

bool PsecData::Send(zmq::socket_t* sock){

	int S_RawWaveform = RawWaveform.size();
	int S_AccInfoFrame = AccInfoFrame.size();
	int S_errorcodes = errorcodes.size();
	//int S_AcdcInfoFrame = AcdcInfoFrame.size();

	zmq::message_t msg1(sizeof VersionNumber);
	std::memcpy(msg1.data(), &VersionNumber, sizeof VersionNumber);

	
	zmq::message_t msg2(sizeof BoardIndex);
	std::memcpy(msg2.data(), &BoardIndex,sizeof BoardIndex);
	
	zmq::message_t msg3(sizeof S_RawWaveform);
	std::memcpy(msg3.data(), &S_RawWaveform, sizeof S_RawWaveform);
	zmq::message_t msg4(sizeof(unsigned short) * S_RawWaveform);
	std::memcpy(msg4.data(), RawWaveform.data(), sizeof(unsigned short) * S_RawWaveform);
	
	zmq::message_t msg5(sizeof S_AccInfoFrame);
	std::memcpy(msg5.data(), &S_AccInfoFrame, sizeof S_AccInfoFrame);
	zmq::message_t msg6(sizeof(unsigned short) * S_AccInfoFrame);
	std::memcpy(msg6.data(), AccInfoFrame.data(), sizeof(unsigned short) * S_AccInfoFrame);
	
	//zmq::message_t msg7(sizeof S_AcdcInfoFrame);
	//std::memcpy(msg7.data(), &S_AcdcInfoFrame, sizeof S_AcdcInfoFrame);
	//zmq::message_t msg8(sizeof(unsigned short) * S_AcdcInfoFrame);
	//std::memcpy(msg8.data(), AcdcInfoFrame.data(), sizeof(unsigned short) * S_AcdcInfoFrame);

	zmq::message_t msg9(sizeof S_errorcodes);
	std::memcpy(msg9.data(), &S_errorcodes, sizeof S_errorcodes);
	zmq::message_t msg10(sizeof(unsigned int) * S_errorcodes);
	std::memcpy(msg10.data(), errorcodes.data(), sizeof(unsigned int) * S_errorcodes);
	
	zmq::message_t msg11(sizeof FailedReadCounter);
	std::memcpy(msg11.data(), &FailedReadCounter, sizeof FailedReadCounter);

	sock->send(msg1,ZMQ_SNDMORE);
	sock->send(msg2,ZMQ_SNDMORE);
	sock->send(msg3,ZMQ_SNDMORE);
	sock->send(msg4,ZMQ_SNDMORE);
	sock->send(msg5,ZMQ_SNDMORE);
	sock->send(msg6,ZMQ_SNDMORE);
	//sock->send(msg7,ZMQ_SNDMORE);
	//sock->send(msg8,ZMQ_SNDMORE);
	sock->send(msg9,ZMQ_SNDMORE);
	sock->send(msg10,ZMQ_SNDMORE);
	sock->send(msg11);
  	return true;
}


bool PsecData::Receive(zmq::socket_t* sock){

	zmq::message_t msg;

	sock->recv(&msg);
	unsigned int tempVersionNumber;
	tempVersionNumber=*(reinterpret_cast<unsigned int*>(msg.data())); 
	if(tempVersionNumber == VersionNumber)
	{
		sock->recv(&msg);
		BoardIndex=*(reinterpret_cast<int*>(msg.data()));

		sock->recv(&msg);
		int tmp_size=0;
		tmp_size=*(reinterpret_cast<int*>(msg.data()));
		if(tmp_size>0)
		{
			sock->recv(&msg);
			RawWaveform.resize(msg.size()/sizeof(unsigned short));
			std::memcpy(&RawWaveform[0], msg.data(), msg.size());
		}

		sock->recv(&msg);
		tmp_size=0;
		tmp_size=*(reinterpret_cast<int*>(msg.data()));
		if(tmp_size>0)
		{
			sock->recv(&msg);
			AccInfoFrame.resize(msg.size()/sizeof(unsigned short));
			std::memcpy(&AccInfoFrame[0], msg.data(), msg.size());
		}
		
		//sock->recv(&msg);
		//tmp_size=0;
		//tmp_size=*(reinterpret_cast<int*>(msg.data()));
		//if(tmp_size>0)
		//{
		//	sock->recv(&msg);
		//	AcdcInfoFrame.resize(msg.size()/sizeof(unsigned short));
		//	std::memcpy(&AcdcInfoFrame[0], msg.data(), msg.size());
		//}

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
	}else
	{
		return false;
	}
	
	return true;
}


bool PsecData::Print(){
	std::cout << "----------------------Sent data---------------------------" << std::endl;
	printf("Version number: 0x%04x\n", VersionNumber);
	printf("Board number: %i\n", BoardIndex);
	printf("Failed read attempts: %i\n", FailedReadCounter);
	printf("Waveform size: %li\n", RawWaveform.size());
	printf("ACC Infoframe size: %li\n", AccInfoFrame.size());
	//printf("ACDC Infoframe size: %li\n", AcdcInfoFrame.size());
	if(errorcodes.size()==1 && errorcodes[0]==0x00000000)
	{
		printf("No errorcodes found all good: 0x%08x\n", errorcodes[0]);
	}else
	{
		printf("Errorcodes found: %li\n", errorcodes.size());
		for(unsigned int k: errorcodes)
		{
			printf("Errorcode: 0x%08x\n", k);
	
		}
	}
	std::cout << "----------------------------------------------------------" << std::endl;
	
	return true;
}
