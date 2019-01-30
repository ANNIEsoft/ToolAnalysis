/* vim:set noexpandtab tabstop=4 wrap */
#ifndef HITCLASS_H
#define HITCLASS_H

#include<SerialisableObject.h>

using namespace std;

class Hit : public SerialisableObject{

	friend class boost::serialization::access;

	public:
  Hit() : TubeId(0), Time(0), Charge(0){serialise=true;}
  Hit(int tubeid, double thetime, double charge) : TubeId(tubeid), Time(thetime), Charge(charge){serialise=true;}

	inline int GetTubeId() const {return TubeId;}
	inline double GetTime() const {return Time;}
	inline double GetCharge() const {return Charge;}

	inline void SetTubeId(int tubeid){TubeId=tubeid;}
	inline void SetTime(double tc){Time=tc;}
	inline void SetCharge(double chg){Charge=chg;}

	bool Print() {
		cout<<"TubeId : "<<TubeId<<endl;
		cout<<"Time : "<<Time<<endl;
		cout<<"Charge : "<<Charge<<endl;
		return true;
	}

	protected:
	int TubeId;
	double Time;
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
	RecoHit(double thetime, double thecharge) : Time(thetime), Charge(thecharge){};

	inline double GetCharge(){return Charge;}
	inline void SetCharge(double chg){Charge=chg;}

	private:
	double Charge;
};
*/

#endif
