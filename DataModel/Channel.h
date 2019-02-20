/* vim:set noexpandtab tabstop=4 wrap */
#ifndef CHANNEL_H
#define CHANNEL_H

#include <SerialisableObject.h>

enum class channelstatus : uint8_t { OFF, ON, UNSTABLE };

class Channel : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
	
	Channel() : ChannelID(-1), channelrelposition(Position(-1,-1,-1)), stripside(2), stripnum(-1), signal_crate(-1), signal_card(-1), signal_channel(-1), level2_crate(-1), level2_card(-1), level2_channel(-1), hv_crate(-1), hv_card(-1), hv_channel(-1), status(channelstatus::OFF)
	 {serialise=true;}
	Channel(unsigned long chnID, Position chanpos, int strp, int strpnm, unsigned int sig_crt, unsigned int sig_crd, unsigned int sig_chn, unsigned int lv2_crt, unsigned int lv2_crd, unsigned int lv2_ch, unsigned int hv_crt, unsigned int hv_crd, unsigned int hv_chn, channelstatus chanstat) : ChannelID(chnID), channelrelposition(chanpos), stripside(strp), stripnum(strpnm), signal_crate(sig_crt), signal_card(sig_crd), signal_channel(sig_chn), level2_crate(lv2_crt), level2_card(lv2_crd), hv_crate(hv_crt), hv_card(hv_crd), hv_channel(hv_chn), status(chanstat) {serialise=true;}
	
	unsigned long GetChannelID(){return ChannelID;}
	Position GetChannelPosition(){return channelrelposition;}
	int GetStripSide(){return stripside;}
	int GetStripNum(){return stripnum;}
	unsigned int GetSignalCrate(){return signal_crate;}
	unsigned int GetSignalCard(){return signal_card;}
	unsigned int GetSignalChannel(){return signal_channel;}
	unsigned int GetLevel2Crate(){return level2_crate;}
	unsigned int GetLevel2Card(){return level2_card;}
	unsigned int GetLevel2Channel(){return level2_channel;}
	unsigned int GetHvCrate(){return hv_crate;}
	unsigned int GetHvCard(){return hv_card;}
	unsigned int GetHvChannel(){return hv_channel;}
	channelstatus GetStatus(){return status;}
	
	void SetChannelID(unsigned long channelIDin){ChannelID=channelIDin;}
	void SetRelPos(Position pos){ channelrelposition=pos;}
	void SetStripSide(int stripsidein){stripside=stripsidein;}
	void SetStripNum(int stripnumin){stripnum=stripnumin;}
	void SetSignalCrate(unsigned int cratein){signal_crate=cratein;}
	void SetSignalCard(unsigned int cardin){signal_card=cardin;}
	void SetSignalChannel(unsigned int chanin){signal_channel=chanin;}
	void SetLevel2Crate(unsigned int cratein){level2_crate=cratein;}
	void SetLevel2Card(unsigned int cardin){level2_card=cardin;}
	void SetLevel2Channel(unsigned int chanin){level2_channel=chanin;}
	void SetHvCrate(unsigned int cratein){hv_crate=cratein;}
	void SetHvCard(unsigned int cardin){hv_card=cardin;}
	void SetHvChannel(unsigned int chanin){hv_channel=chanin;}
	void SetStatus(channelstatus stat){status=stat;}
	
	bool Print(){
		std::cout<<"ChannelID          : "<<ChannelID<<std::endl;
		std::cout<<"ChannelRelPosition : "; channelrelposition.Print();
		std::cout<<"StripSide          : "<<stripside<<std::endl;
		std::cout<<"StripNum           : "<<stripnum<<std::endl;
		std::cout<<"Signal Crate       : "<<signal_crate<<std::endl;
		std::cout<<"Signal Card        : "<<signal_card<<std::endl;
		std::cout<<"Signal Channel     : "<<signal_channel<<std::endl;
		std::cout<<"Level2 Crate       : "<<level2_crate<<std::endl;
		std::cout<<"Level2 Card        : "<<level2_card<<std::endl;
		std::cout<<"Level2 Channel     : "<<level2_channel<<std::endl;
		std::cout<<"HV Crate           : "<<hv_crate<<std::endl;
		std::cout<<"HV Card            : "<<hv_card<<std::endl;
		std::cout<<"HV Channel         : "<<hv_channel<<std::endl;
		std::cout<<"Status             : "; PrintStatus(status);
		return true;
	}
	bool PrintStatus(channelstatus status){
		switch(status){
			case (channelstatus::OFF): std::cout<<"OFF"<<std::endl; break;
			case (channelstatus::ON): std::cout<<"ON"<<std::endl; break;
			case (channelstatus::UNSTABLE) : std::cout<<"UNSTABLE"<<std::endl; break;
		}
		return true;
	}
	
	private:
	unsigned long ChannelID;   // unique ChannelKey
	Position channelrelposition;
	int stripside, stripnum;
	unsigned int signal_crate, signal_card, signal_channel, level2_crate, level2_card, level2_channel,
	hv_crate, hv_card, hv_channel;
	channelstatus status;
	template<class Archive> void serialize(Archive & ar, const unsigned int	version){
		if(serialise){
			ar & ChannelID;
			ar & channelrelposition;
			ar & stripside;
			ar & stripnum;
			ar & signal_crate;
			ar & signal_card;
			ar & signal_channel;
			ar & level2_crate;
			ar & level2_card;
			ar & level2_channel;
			ar & hv_crate;
			ar & hv_card;
			ar & hv_channel;
			ar & status;
		}
	}
	
};

#endif
