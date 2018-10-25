/* vim:set noexpandtab tabstop=4 wrap */
#ifndef LAPPDHITCLASS_H
#define LAPPDHITCLASS_H

#include<Hit.h>
#include<SerialisableObject.h>
#include "TimeClass.h"

using namespace std;

class LAPPDHit : public Hit{

	friend class boost::serialization::access;

	public:
	LAPPDHit() : Hit(), Position(0), LocalPosition(0) {serialise=true;}
	LAPPDHit(int tubeid, double thetime, double charge, std::vector<double> Position, std::vector<double> LocalPosition) : Hit(tubeid,thetime,charge), Position(Position), LocalPosition(LocalPosition) {serialise=true;}

	inline std::vector<double> GetPosition() const {return Position;}
	inline std::vector<double> GetLocalPosition() const {return LocalPosition;}
	inline void SetPosition(std::vector<double> pos){Position=pos;}
	inline void SetLocalPosition(std::vector<double> locpos){LocalPosition=locpos;}

	bool Print() {
		cout<<"TubeId : "<<TubeId<<endl;
		cout<<"Time : "<<Time<<endl;
		cout<<"X Pos : "<<Position.at(0)<<endl;
		cout<<"Y Pos : "<<Position.at(1)<<endl;
		cout<<"Z Pos : "<<Position.at(2)<<endl;
		cout<<"Parallel Pos : "<<LocalPosition.at(0)<<endl;
		cout<<"Transverse Pos : "<<LocalPosition.at(1)<<endl;
		cout<<"Charge : "<<Charge<<endl;
		return true;
	}

	protected:
	std::vector<double> Position;
	std::vector<double> LocalPosition;


	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & TubeId;
			ar & Time;
			ar & Position;
			ar & LocalPosition;
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
