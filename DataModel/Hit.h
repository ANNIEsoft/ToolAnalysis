/* vim:set noexpandtab tabstop=4 wrap */
#ifndef HITCLASS_H
#define HITCLASS_H

#include<SerialisableObject.h>

using namespace std;

class Hit : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
	Hit() : TubeId(0), Time(0), Charge(0){serialise=true;}
	Hit(int thetubeid, double thetime, double thecharge) : TubeId(thetubeid), Time(thetime), Charge(thecharge){serialise=true;}
	virtual ~Hit(){};
	
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
	// XXX ~~~~~~ adding members? Don't forget to update the operators of derived classes! ~~~~~~ XXX
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

//  Derived classes

class MCHit : virtual public Hit {
	
	friend class boost::serialization::access;
	
	public:
	MCHit() : Hit(), Parents(std::vector<int>{}) {serialise=true;}
	MCHit(int tubeid, double thetime, double thecharge, std::vector<int> theparents) : Hit(tubeid, thetime, thecharge), Parents(theparents) {serialise=true;}
	virtual ~MCHit(){};
	
	const std::vector<int>* GetParents() const { return &Parents; }
	void SetParents(std::vector<int> parentsin){ Parents = parentsin; }
	
	// compiler warns about default implementation of move assignment operator for
	// classes inheriting from a virtual base class. We need to specify them manually.
	MCHit(const MCHit& rhs) : Hit(rhs){  // Copy Ctor
		Parents = rhs.Parents;
	};
	MCHit& operator=(const MCHit& rhs){  // Copy Assignment
		Hit::operator=(rhs);
		Parents = rhs.Parents;
		return *this;
	};
	MCHit(MCHit&& rhs) /*: Hit(rhs)*/ {      // Move Ctor
		// calling base Hit methods seems to invoke a copy construction to convert MCHit to Hit...
		TubeId  = std::move(rhs.TubeId);
		Time    = std::move(rhs.Time);
		Charge  = std::move(rhs.Charge);
		Parents = std::move(rhs.Parents);
		// no other initialization needed
	};
	MCHit& operator=(MCHit&& rhs){       // Move Assignment
		// no initial cleanup of 'this' needed
		//Hit::operator=(rhs);
		TubeId  = std::move(rhs.TubeId);
		Time    = std::move(rhs.Time);
		Charge  = std::move(rhs.Charge);
		Parents = std::move(rhs.Parents);
		return *this;
	};
	
	// see MCLAPPDHit for the usage of this method
	MCHit& PartialAssign(const MCHit& rhs){
		// Initialize only the non-inherited members of this class
		Parents = rhs.Parents;
		return *this;
	}
	
	// see MCLAPPDHit for the usage of this method
	MCHit& PartialAssign(MCHit&& rhs){
		// Initialize only the non-inherited members of this class
		Parents = std::move(rhs.Parents);
		return *this;
	}
	
	bool Print(){
		cout<<"TubeId : "<<TubeId<<endl;
		cout<<"Time : "<<Time<<endl;
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
	
	protected:
	// XXX ~~~~~~ adding members? Don't forget to update PartialAssign methods! ~~~~~~~ XXX
	std::vector<int> Parents;
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & TubeId;
			ar & Time;
			ar & Charge;
			// do not serialize parents; the indices by themselves are not meaningful
		}
	}
};

/*
class RecoHit : public Hit {
	public:
	RecoHit(double thetime, double thecharge) : Time(thetime), Charge(thecharge){};

	inline double GetCharge(){return Charge;}
	inline void SetCharge(double chg){Charge=chg;}

	protected:
	double Charge;
};
*/

#endif

//    person(const std::string& name, int age);        // Ctor
//    person(const person &) = default;                // Copy Ctor
//    person(person &&) noexcept = default;            // Move Ctor
//    person& operator=(const person &) = default;     // Copy Assignment
//    person& operator=(person &&) noexcept = default; // Move Assignment
//    ~person() noexcept = default;                    // Dtor
