/* vim:set noexpandtab tabstop=4 wrap */
#ifndef LAPPDPULSECLASS_H
#define LAPPDPULSECLASS_H

#include<Hit.h>
#include<SerialisableObject.h>
#include "TimeClass.h"

using namespace std;

class LAPPDPulse : public Hit{

	friend class boost::serialization::access;

	public:
  LAPPDPulse() : TubeId(0), ChannelID(0), Time(TimeClass()), Charge(0){serialise=true;}
  LAPPDPulse(int tubeid, int channelid, TimeClass thetime, double charge) : TubeId(tubeid), ChannelID(channelid), Time(thetime), Charge(charge){serialise=true;}

	inline int GetChannelID(){return ChannelID;}
	inline void SetChannelID(int channelid){ChannelID=channelid;}
	inline void SetLocalPosition(std::vector<double> locpos){LocalPosition=locpos;}

	bool Print(){
		cout<<"TubeId : "<<TubeId<<endl;
		cout<<"ChannelID : "<<ChannelID<<endl;
		cout<<"Time : "; Time.Print();
		cout<<"Charge : "<<Charge<<endl;
		return true;
	}

	protected:
	int TubeId;
	int ChannelID
	TimeClass Time;
	double Charge;

	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & TubeId;
			ar & ChannelID
			ar & Time;
			ar & Charge;
		}
	}
};

/*  Derived classes, if there's a reason to have them. So far...not really

class TDCHit : public Hit {
	public:

	private:
}

class RecoHit : public Hit {
	public:
	RecoHit(TimeClass thetime, double thecharge) : Time(thetime), Charge(thecharge){};

	inline double GetCharge(){return Charge;}
	inline void SetCharge(double chg){Charge=chg;}

	private:
	double Charge;
};
*/

#endif
