/* vim:set noexpandtab tabstop=4 wrap */
#ifndef CHANNELCLASS_H
#define CHANNELCLASS_H

#include<SerialisableObject.h>
#include "Position.h"

using namespace std;


class Channel : public SerialisableObject{

	friend class boost::serialization::access;

	public:
		Channel() : ChannelKey(), RelPara(), RelPerp(), card(), crate(), elecid(), stripside(), rotorientation(), status() {serialise=true;}
		Channel(unsigned long chnky, float relpara, float relperp, unsigned int crd, unsigned int crt, unsigned int elcid, int strp, int rotation, int chanstat) : ChannelKey(chnky), RelPara(relpara), RelPerp(relperp), card(crd), crate(crt), elecid(elcid), stripside(strp), rotorientation(rotation), status(chanstat) {serialise=true;}

unsigned long GetChannelKey(){return ChannelKey;}
float GetRelPara(){return RelPara;} //relative to position of detector
float GetRelPerp(){return RelPerp;}
int GetStripSide(){return stripside;}
unsigned int GetCard(){return card;}
unsigned int GetCrate(){return crate;}
unsigned int GetElecID(){return elecid;}
int GetRotOrientation(){return rotorientation;}
int GetStatus(){return status;}
void SetChannelKey(unsigned long channelkeyin){ChannelKey=channelkeyin;}
void SetRelPos(int para,int perp){ RelPara=para; RelPerp=perp;}
void SetStripSide(int stripsidein){stripside=stripsidein;}
void SetCard(unsigned int cardin){card=cardin;}
void SetCrate(unsigned int cratein){crate=cratein;}
void SetElecID(unsigned int elecidin){elecid=elecidin;}
void SetRotOrientation(int rotorin){rotorientation=rotorin;}
void SetStatus(int stat){status=stat;}

	private:
		unsigned long ChannelKey;
		float RelPara, RelPerp;
		int stripside; //-1 for left, 1 for right
		unsigned int card, crate, elecid;
		int rotorientation;
		int status;
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & ChannelKey;
			ar & RelPara;
			ar & RelPerp;
			ar & stripside;
			ar & card;
			ar & crate;
			ar & elecid;
			ar & rotorientation;
			ar & status;
		}
	}
};

#endif
