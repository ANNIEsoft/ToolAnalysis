/* vim:set noexpandtab tabstop=4 wrap */
#ifndef LAPPDHITCLASS_H
#define LAPPDHITCLASS_H

#include<Hit.h>
#include<SerialisableObject.h>

using std::cout;
using std::endl;

class LAPPDHit : virtual public Hit{
	
	friend class boost::serialization::access;
	
	public:
	LAPPDHit() : Hit(), Position(0), LocalPosition(0) {serialise=true;}
	LAPPDHit(int thetubeid, double thetime, double thecharge, std::vector<double> theposition, std::vector<double> thelocalposition) : Hit(thetubeid,thetime,thecharge), Position(theposition), LocalPosition(thelocalposition) {serialise=true;}
	
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

//  Derived classes, if there's a reason to have them

class MCLAPPDHit : public MCHit, public LAPPDHit{
	
	friend class boost::serialization::access;
	
	public:
	MCLAPPDHit() : LAPPDHit(), MCHit() {serialise=true;}
	MCLAPPDHit(int thetubeid, double thetime, double thecharge, std::vector<double> theposition, std::vector<double> thelocalposition, std::vector<int> theparents){
		TubeId = thetubeid;
		Time=thetime;
		Charge=thecharge;
		Position=theposition;
		LocalPosition=thelocalposition;
		Parents=theparents;
		serialise=true;
	}
	
	bool Print() {
		cout<<"TubeId : "<<TubeId<<endl;
		cout<<"Time : "<<Time<<endl;
		cout<<"X Pos : "<<Position.at(0)<<endl;
		cout<<"Y Pos : "<<Position.at(1)<<endl;
		cout<<"Z Pos : "<<Position.at(2)<<endl;
		cout<<"Parallel Pos : "<<LocalPosition.at(0)<<endl;
		cout<<"Transverse Pos : "<<LocalPosition.at(1)<<endl;
		cout<<"Charge : "<<Charge<<endl;
		if(Parents.size()){
			cout<<"Parent MCPartice indices: {";
			for(int parenti=0; parenti<Parents.size(); ++parenti){
				cout<<Parents.at(parenti);
				if((parenti+1)<Parents.size()) cout<<", ";
			}
			cout<<"}"<<endl;
		} else {
			cout<<"No recorded parents"<<endl;
		}
		return true;
	}
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & TubeId;
			ar & Time;
			ar & Position;
			ar & LocalPosition;
			ar & Charge;
			// n.b. at time of writing MCHit stores no additional persistent members
			// - it only adds parent MCParticle indices, and these aren't saved... 
		}
	}
};

/*
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
