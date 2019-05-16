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
	virtual ~LAPPDHit(){};
	
	inline std::vector<double> GetPosition() const {return Position;}
	inline std::vector<double> GetLocalPosition() const {return LocalPosition;}
	inline void SetPosition(std::vector<double> pos){Position=pos;}
	inline void SetLocalPosition(std::vector<double> locpos){LocalPosition=locpos;}
	
	// compiler warns about default implementation of move assignment operator for
	// classes inheriting from a virtual base class. We need to specify them manually.
	LAPPDHit(const LAPPDHit& rhs) : Hit(rhs){  // Copy Ctor
		Position = rhs.Position;
		LocalPosition = rhs.LocalPosition;
	};
	LAPPDHit& operator=(const LAPPDHit& rhs){  // Copy Assignment
		Hit::operator=(rhs);
		Position = rhs.Position;
		LocalPosition = rhs.LocalPosition;
		return *this;
	};
	LAPPDHit(LAPPDHit&& rhs) /*: Hit(rhs)*/ {      // Move Ctor
		// calling base Hit methods seems to invoke a copy construction to convert MCHit to Hit...
		TubeId  = std::move(rhs.TubeId);
		Time    = std::move(rhs.Time);
		Charge  = std::move(rhs.Charge);
		Position = std::move(rhs.Position);
		LocalPosition = std::move(rhs.LocalPosition);
		// no other initialization needed
	};
	LAPPDHit& operator=(LAPPDHit&& rhs){       // Move Assignment
		// no initial cleanup of 'this' needed
		//Hit::operator=(rhs);
		TubeId  = std::move(rhs.TubeId);
		Time    = std::move(rhs.Time);
		Charge  = std::move(rhs.Charge);
		Position = std::move(rhs.Position);
		LocalPosition = std::move(rhs.LocalPosition);
		return *this;
	};
	
	// see MCLAPPDHit for the usage of this method
	LAPPDHit& PartialAssign(const LAPPDHit& rhs){
		// Initialize only the non-inherited members of this class
		Position = rhs.Position;
		LocalPosition = rhs.LocalPosition;
		return *this;
	}
	
	// see MCLAPPDHit for the usage of this method
	LAPPDHit& PartialAssign(LAPPDHit&& rhs){
		// Initialize only the non-inherited members of this class
		Position = std::move(rhs.Position);
		LocalPosition = std::move(rhs.LocalPosition);
		return *this;
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
		return true;
	}
	
	protected:
	// XXX ~~~~~~ adding members? Don't forget to update PartialAssign methods! ~~~~~~~ XXX
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
	MCLAPPDHit() : Hit(), LAPPDHit(), MCHit() {serialise=true;}
	MCLAPPDHit(int thetubeid, double thetime, double thecharge, std::vector<double> theposition, std::vector<double> thelocalposition, std::vector<int> theparents) : Hit(thetubeid, thetime, thecharge), LAPPDHit(0,0,0,theposition,thelocalposition), MCHit(0,0,0,theparents){
//		TubeId = thetubeid;
//		Time=thetime;
//		Charge=thecharge;
//		Position=theposition;
//		LocalPosition=thelocalposition;
//		Parents=theparents;
//		serialise=true;
	}
	
	// because this class inherits the base Hit class through two paths, we need to use virtual inheritance
	// so that there is only one version of the base Hit class members. That's why MCHit and LAPPDHit inherit
	// from Hit as 'virtual public Hit'.
	// When performing construction and assignment of MCLAPPDHit, which inherited class
	// (MCHit or LAPPDHit) should handle initialization/copying/moving of the Hit members?
	// Niether - it must be handled by the most derived class (MCLAPPDHit).
	// We then operate on the base Hit directly ourselves, and use the PartialAssign members of
	// MCHit and MCLAPPDHit to handle the additional members they provide.
	
	// move assignment operator
	// we could probably use the default here too, as no memory management occurs... but let's be "safe".
	MCLAPPDHit& operator=(MCLAPPDHit&& rhs){
		// safety check for 'x = x;'
		if (this==&rhs) return *this;
		
//		// use LAPPDHit::operator= to move over the base Hit and LAPPDHit parameters
//		//static_cast<LAPPDHit&>(*this) = static_cast<LAPPDHit&&>(rhs);
//		LAPPDHit::operator=(std::move(rhs));  // or this way
//		
//		// then we need to handle the MCHit members manually. :(
//		std::swap(Parents, rhs.Parents);  // no membery cleanup of 'this' needed first
		
		Hit::operator=(std::move(rhs));
		MCHit::PartialAssign(std::move(rhs));
		LAPPDHit::PartialAssign(std::move(rhs));
		
		return *this;
	}
	
	// copy assignment operator
	// we could use the default in our case, since we have no memory management issues
	//MCLAPPDHit& operator=(const MCLAPPDHit &) = default;
	// but it would be inefficient as it would copy the Hit members twice... probably.
	MCLAPPDHit& operator=(MCLAPPDHit rhs){
		// safety check for 'x = x;'
		if (this==&rhs) return *this;
		
//		// use LAPPDHit::operator= to copy over the base Hit and LAPPDHit parameters
//		static_cast<LAPPDHit&>(*this) = static_cast<LAPPDHit&>(rhs);
//		// then copy over MCHit members manually
//		Parents = rhs.Parents;
		
		Hit::operator=(rhs);
		MCHit::PartialAssign(rhs);
		LAPPDHit::PartialAssign(rhs);
		
		return *this;
	}
	
	// copy constructor
	//MCLAPPDHit(const MCLAPPDHit&) = default;
	MCLAPPDHit(const MCLAPPDHit& rhs) : Hit(rhs), MCHit(rhs), LAPPDHit(rhs){};
	
	// move constructor
	//MCLAPPDHit(MCLAPPDHit&&) noexcept = default;
//	MCLAPPDHit(MCLAPPDHit&& rhs) : MCLAPPDHit(){
//		*this = rhs;  // is this sufficient: intialize and assign... 
//	}
	MCLAPPDHit(MCLAPPDHit&& rhs) : Hit(std::move(rhs)){
		MCHit::PartialAssign(std::move(rhs));
		LAPPDHit::PartialAssign(std::move(rhs));
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
