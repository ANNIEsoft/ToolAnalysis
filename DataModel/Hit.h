/* vim:set noexpandtab tabstop=4 wrap */
#ifndef HITCLASS_H
#define HITCLASS_H

#include<SerialisableObject.h>
#include "TimeClass.h"

using namespace std;

class Hit : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
  Hit() : TubeId(0), Time(TimeClass()), Charge(0){serialise=true;}
  Hit(int tubeid, TimeClass thetime, double charge) : TubeId(tubeid), Time(thetime), Charge(charge){serialise=true;}
	
	inline int GetTubeId(){return TubeId;}
	inline TimeClass GetTime(){return Time;}
	inline double GetCharge(){return Charge;}
	
	inline void SetTubeId(int tubeid){TubeId=tubeid;}
	inline void SetTime(TimeClass tc){Time=tc;}
	inline void SetCharge(double chg){Charge=chg;}
	
	bool Print() {
		cout<<"TubeId : "<<TubeId<<endl;
		cout<<"Time : "; Time.Print();
		cout<<"Charge : "<<Charge<<endl;
		
		return true;
	}
	
	protected:
	int TubeId;
	TimeClass Time;
	double Charge;
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & TubeId;
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
