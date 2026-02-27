/* vim:set noexpandtab tabstop=4 wrap */
#ifndef LAPPDPULSECLASS_H
#define LAPPDPULSECLASS_H

#include<Hit.h>
#include<SerialisableObject.h>

using namespace std;

class LAPPDPulse : public Hit{

	friend class boost::serialization::access;

	public:
  LAPPDPulse() : Hit(), ChannelID(0), Peak(0), LowRange(0), HiRange(0) {serialise=true;}
  LAPPDPulse(int tubeid, int channelid, double thetime, double charge, double peak, double low, double hi) : Hit(tubeid,thetime,charge), ChannelID(channelid), Peak(peak), LowRange(low), HiRange(hi) {serialise=true;}

	inline int GetChannelID(){return ChannelID;}
	inline double GetPeak(){return Peak;}
	inline double GetLowRange(){return LowRange;}
	inline double GetHiRange(){return HiRange;}
	inline void SetChannelID(int channelid){ChannelID=channelid;}
	inline void SetPeak(double peak){Peak=peak;}
	inline void SetRange(double low, double hi){LowRange=low; HiRange=hi;}
	inline void SetHalfHeightTime(double half){halfHeightTime=half;}
	inline double GetHalfHeightTime(){return halfHeightTime;}
	inline void SetHalfEndTime(double half){halfEndTime=half;}
	inline double GetHalfEndTime(){return halfEndTime;}
	inline void SetBaseline(double base){baseline=base;}
	inline double GetBaseline(){return baseline;}

	bool Print() {
		cout<<"TubeId : "<<TubeId<<endl;
		cout<<"ChannelID : "<<ChannelID<<endl;
		cout<<"Time : "<<Time<<endl;
		cout<<"Charge : "<<Charge<<endl;
		cout<<"HalfHeightTime : "<<halfHeightTime<<endl;
		cout<<"Baseline : "<<baseline<<endl;
		return true;
	}

	protected:
	double ChannelID;
	double Peak;
	double LowRange;
	double HiRange;
	double halfHeightTime;
	double halfEndTime;
	double baseline;



	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & TubeId;
			ar & ChannelID;
			ar & Time;
			ar & Charge;
			ar & Peak;
			ar & LowRange;
			ar & HiRange;
			ar & halfHeightTime;
			ar & halfEndTime;
			ar & baseline;
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
	RecoHit(double thetime, double thecharge) : Time(thetime), Charge(thecharge){};

	inline double GetCharge(){return Charge;}
	inline void SetCharge(double chg){Charge=chg;}

	private:
	double Charge;
};
*/

#endif
