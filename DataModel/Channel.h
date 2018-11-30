#ifndef CHANNEL_H
#define CHANNEL_H

#include <SerialisableObject.h>

class Channel : public SerialisableObject{

  friend class boost::serialization::access;

 public:

  Channel() : ChannelID(), channelrelposition(), card(), crate(), elecid(), stripside(), status() {serialise=true;}
  Channel(unsigned long chnID, Position chanpos, unsigned int crd, unsigned int crt, unsigned int elcid, int strp, int chanstat) : ChannelID(chnID), channelrelposition(chanpos), card(crd), crate(crt), elecid(elcid), stripside(strp), status(chanstat)  {serialise=true;}


  unsigned long GetChannelID(){return ChannelID;}
  Position GetChannelPosition(){return channelrelposition;}
  int GetStripSide(){return stripside;}
  unsigned int GetCard(){return card;}
  unsigned int GetCrate(){return crate;}
  unsigned int GetElecID(){return elecid;}
  int GetStatus(){return status;}


  void SetChannelID(unsigned long channelIDin){ChannelID=channelIDin;}
  void SetRelPos(Position pos){ channelrelposition=pos;}
  void SetStripSide(int stripsidein){stripside=stripsidein;}
  void SetCard(unsigned int cardin){card=cardin;}
  void SetCrate(unsigned int cratein){crate=cratein;}
  void SetElecID(unsigned int elecidin){elecid=elecidin;}
  void SetStatus(int stat){status=stat;}

bool Print() {



return true;
}

 private:

   unsigned long ChannelID;
   int stripside;
   unsigned int card, crate, elecid;
   int status;
   Position channelrelposition;
  template<class Archive> void serialize(Archive & ar, const unsigned int  version){


    if(serialise){
      ar & ChannelID;
      ar & channelrelposition;
      ar & stripside;
      ar & card;
      ar & crate;
      ar & elecid;
      ar & status;
    }
  }


};

#endif
