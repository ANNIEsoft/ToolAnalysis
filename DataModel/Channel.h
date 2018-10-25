/* vim:set noexpandtab tabstop=4 wrap */
#ifndef CHANNELCLASS_H
#define CHANNELCLASS_H

#include<SerialisableObject.h>
#include "Position.h"

using namespace std;


class Channel : public SerialisableObject{

	friend class boost::serialization::access;

	public:
		Channel() : ChannelKey(), relx(), rely(), relz(), card(), crate(), elecid(), stripside(), rotorientation() {serialise=true;}
		Channel(unsigned long chnky, float xpos, float ypos, float zpos ,unsigned int crd, unsigned int crt, unsigned int elcid, int strp, int rotation) : ChannelKey(chnky), relx(xpos), rely(ypos), relz(zpos), card(crd), crate(crt), elecid(elcid), stripside(strp), rotorientation(rotation) {serialise=true;}

unsigned long GetChannelKey(){return ChannelKey;}
float GetRelXPos(){return relx;} //relative to position of detector 
float GetRelYPos(){return rely;}
float GetRelZPos(){return relz;}
int GetStripSide(){return stripside;}
unsigned int GetCard(){return card;}
unsigned int GetCrate(){return crate;}
unsigned int GetElecID(){return elecid;}
int GetRotOrientation(){return rotorientation;}

void SetChannelKey(unsigned long channelkeyin){ChannelKey=channelkeyin;}
void SetRelPos(int xin,int yin,int zin){ relx=xin; rely=yin; relz=zin;}
void SetStripSide(int stripsidein){stripside=stripsidein;}
void SetCard(unsigned int cardin){card=cardin;}
void SetCrate(unsigned int cratein){crate=cratein;}
void SetElecID(unsigned int elecidin){elecid=elecidin;}
void SetRotOrientation(int rotorin){rotorientation=rotorin;}


	private:
		unsigned long ChannelKey;
		float relx, rely, relz;
		int stripside; //-1 for left, 1 for right
		unsigned int card, crate, elecid;
		int rotorientation;

	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & ChannelKey;
			ar & relx;
			ar & rely;
			ar & relz;
			ar & stripside;
			ar & card;
			ar & crate;
			ar & elecid;
			ar & rotorientation;
		}
	}
};

#endif
