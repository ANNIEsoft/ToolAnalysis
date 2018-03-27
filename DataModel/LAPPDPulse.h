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
  LAPPDPulse() : Hit(), ChannelID(0), Tpsec(0), Peak(0), LowRange(0), HiRange(0) {serialise=true;}
  LAPPDPulse(int tubeid, int channelid, TimeClass thetime, double charge, double tpsec, double peak, double low, double hi) : Hit(tubeid,thetime,charge), ChannelID(channelid), Tpsec(tpsec), Peak(peak), LowRange(low), HiRange(hi){serialise=true;}

	inline int GetChannelID(){return ChannelID;}
	inline double GetTpsec(){return Tpsec;}
	inline double GetPeak(){return Peak;}
	inline double GetLowRange(){return LowRange;}
	inline double GetHiRange(){return HiRange;}
	inline void SetChannelID(int channelid){ChannelID=channelid;}
	inline void SetTpsec(double tpsec){Tpsec=tpsec;}
	inline void SetPeak(double peak){Peak=peak;}
	inline void SetRange(double low, double hi){LowRange=low; HiRange=hi;}

	bool Print() {
		cout<<"TubeId : "<<TubeId<<endl;
		cout<<"ChannelID : "<<ChannelID<<endl;
		cout<<"Time : "; Time.Print();
		cout<<"Charge : "<<Charge<<endl;
		return true;
	}

	protected:
	int TubeId;
	double ChannelID;
	double Tpsec;
	double Peak;
	double LowRange;
	double HiRange;


	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & TubeId;
			ar & ChannelID;
			ar & Time;
			ar & Charge;
			ar & Tpsec;
			ar & Peak;
			ar & LowRange;
			ar & HiRange;
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
