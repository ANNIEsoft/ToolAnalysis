/* vim:set noexpandtab tabstop=4 wrap */
#ifndef PARTICLECLASS_H
#define PARTICLECLASS_H

#include <map>
#include <utility>
#include <string>
#include <iostream>

#include<SerialisableObject.h>
#include "Position.h"
#include "Direction.h"

using namespace std;
// world extent in WCSim is +-600cm in all directions!
enum class tracktype : uint8_t { STARTONLY, ENDONLY, CONTAINED, UNCONTAINED, UNDEFINED };

class Particle : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
	
	Particle() : ParticlePDG(0), startEnergy(0), stopEnergy(0), startVertex(),
	  stopVertex(), startTime(), stopTime(), startDirection(),
		     trackLength(0), StartStopType(tracktype::UNDEFINED) {serialise=true;}
	
	Particle(int pdg, double sttE, double stpE, Position sttpos, Position stppos, 
	  double sttt, double stpt, Direction startdir, double len, tracktype tracktypein) 
	: ParticlePDG(pdg), startEnergy(sttE), stopEnergy(stpE), startVertex(sttpos),
	  stopVertex(stppos), startTime(sttt), stopTime(stpt), startDirection(startdir),
	  trackLength(len) {
		serialise=true;
		if(tracktypein!=tracktype::UNDEFINED){
			StartStopType=tracktypein;
		} else {
			// calculate track containment type
//			if(startinbounds && stopinbounds) startstoptype = tracktype::CONTAINED;
//			else if( startinbounds ) startstoptype = tracktype::STARTONLY;
//			else if( stopinbounds  ) startstoptype = tracktype::ENDONLY;
//			else startstoptype = tracktype::UNCONTAINED;
		}
	}
	
	inline void SetPdgCode(int code){ParticlePDG=code;}
	inline void SetStartEnergy(double E){startEnergy=E;}
	inline void SetStopEnergy(double E){stopEnergy=E;}
	inline void SetStartVertex(Position pos){startVertex=pos;}
	inline void SetStopVertex(Position pos){stopVertex=pos;}
	inline void SetStartTime(double stime){startTime=stime;}
	inline void SetStopTime(double stime){stopTime=stime;}
	inline void SetstartDirection(Direction dir){startDirection=dir;}
	inline void SetTrackLength(double len){trackLength=len;}
	inline void SetTrackStartStopType(tracktype tracktypein){StartStopType=tracktypein;}
	
	inline int GetPdgCode(){return ParticlePDG;}
	inline double GetStartEnergy(){return startEnergy;}
	inline double GetStopEnergy(){return stopEnergy;}
	inline Position GetStartVertex(){return startVertex;}
	inline Position GetStopVertex(){return stopVertex;}
	inline double GetStartTime(){return startTime;}
	inline double GetStopTime(){return stopTime;}
	inline Direction GetStartDirection(){return startDirection;}
	inline double GetTrackLength(){return trackLength;}
	inline tracktype GetStartStopType(){return StartStopType;}
	
	virtual bool Print() {
		std::cout<<"ParticlePDG : "<<ParticlePDG<<std::endl;
		std::cout<<"Particle Name : "<<PdgToString(ParticlePDG)<<std::endl;
		std::cout<<"startEnergy : "<<startEnergy<<std::endl;
		std::cout<<"stopEnergy : "<<stopEnergy<<std::endl;
		std::cout<<"startVertex : "; startVertex.Print();
		std::cout<<"stopVertex : "; stopVertex.Print();
		std::cout<<"startTime : "<<startTime<<std::endl;
		std::cout<<"stopTime : "<<stopTime<<std::endl;
		std::cout<<"startDirection : "; startDirection.Print();
		std::cout<<"trackLength : "<<trackLength<<std::endl;
		std::cout<<"StartStopType : "; PrintStartStopType(StartStopType);
		
		return true;
	}
	
	void PrintStartStopType(tracktype typein){
		std::string printstring="unknown type " + to_string(uint8_t(typein));
		switch (typein){
			case tracktype::STARTONLY: printstring = "STARTONLY";
			case tracktype::ENDONLY: printstring = "ENDONLY";
			case tracktype::CONTAINED: printstring = "CONTAINED";
			case tracktype::UNCONTAINED: printstring = "UNCONTAINED";
			case tracktype::UNDEFINED: printstring = "UNDEFINED";
		}
		std::cout<<printstring<<std::endl;
	}
	
	std::string PdgToString (int pdgcode) const{
		if(pdgcodetoname.count(pdgcode)!=0) return pdgcodetoname.at(pdgcode);
		else return to_string(pdgcode);
	}
	
	protected:
	int ParticlePDG;
	tracktype StartStopType;         // does it start and/or stop in the detector / simulated volume?
	double startEnergy;              // GeV
	double stopEnergy;
	Position startVertex;            // meters
	Position stopVertex;
	double startTime;                // ns since Trigger time
	double stopTime;                 //
	Direction startDirection;        // for primary particle initial scattering dir is most important.
	double trackLength;              // meters
	
	// convenience member
	static const std::map<int,std::string> pdgcodetoname;
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & ParticlePDG;
			ar & startEnergy;
			ar & stopEnergy;
			ar & startVertex;
			ar & stopVertex;
			ar & startTime;
			ar & stopTime;
			ar & startDirection;
			ar & trackLength;
			ar & StartStopType;
		}
	}
	
};

class MCParticle : public Particle {
	
	friend class boost::serialization::access;
	
	public:
	
	MCParticle() : Particle(0, 0., 0., Position(), Position(), 0., 0., Direction(), 0.,
				tracktype::UNCONTAINED), ParticleID(0), ParentPdg(0) {serialise=true;}
	
	MCParticle(int pdg, double sttE, double stpE, Position sttpos, Position stppos, 
	  double sttt, double stpt, Direction startdir, double len, tracktype tracktypein,
	  int partid, int parentpdg) 
	: Particle(pdg, sttE, stpE, sttpos, stppos, sttt, stpt, startdir, len, tracktypein), 
	  ParticleID(partid), ParentPdg(parentpdg){
		serialise=true;
		// override Hit tracktype
		if(tracktypein!=tracktype::UNDEFINED){
			StartStopType=tracktypein;
		} else {
			// calculate track containment type
			if(GetWorldContained(0) && GetWorldContained(1)) StartStopType = tracktype::CONTAINED;
			else if(GetWorldContained(0)) StartStopType = tracktype::STARTONLY;
			else if(GetWorldContained(1)) StartStopType = tracktype::ENDONLY;
			else StartStopType = tracktype::UNCONTAINED;
		}
	}
	
	inline int GetParticleID(){return ParticleID;}
	inline int GetParentPdg(){return ParentPdg;}
	inline void SetParticleID(int partidin){ParticleID=partidin;}
	inline void SetParentPdg(int parentpdgin){ParentPdg=parentpdgin;}
	
	bool GetWorldContained(int startstop, Position aVertex=Position(0,0,0)){
		if(startstop==0) aVertex=startVertex;
		else if(startstop==1) aVertex=stopVertex;
		// else check the vertex passed in
		if( (abs(aVertex.X())<550) && (abs(aVertex.Y())<550) && (abs(aVertex.Z())<550) ){
			return true;
		} else {
			return false;
		}
	}
	
	bool Print() {
		std::cout<<"ParticlePDG : "<<ParticlePDG<<std::endl;
		std::cout<<"Particle Name : "<<PdgToString(ParticlePDG)<<std::endl;
		std::cout<<"startEnergy : "<<startEnergy<<std::endl;
		std::cout<<"stopEnergy : "<<stopEnergy<<std::endl;
		std::cout<<"startVertex : "; startVertex.Print();
		std::cout<<"stopVertex : "; stopVertex.Print();
		std::cout<<"startTime : "<<startTime<<std::endl;;
		std::cout<<"stopTime : "<<stopTime<<std::endl;
		std::cout<<"startDirection : "; startDirection.Print();
		std::cout<<"trackLength : "<<trackLength<<std::endl;
		std::cout<<"StartStopType : "; PrintStartStopType(StartStopType);
		std::cout<<"ParticleID : "<<ParticleID<<std::endl;
		std::cout<<"ParentPdg : "<<ParentPdg<<std::endl;
		std::cout<<"Parent Particle Name : "<<PdgToString(ParentPdg)<<std::endl;
		
		return true;
	}
	
	protected:
	int ParticleID;
	int ParentPdg;
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & ParticlePDG;
			ar & startEnergy;
			ar & stopEnergy;
			ar & startVertex;
			ar & stopVertex;
			ar & startTime;
			ar & stopTime;
			ar & startDirection;
			ar & trackLength;
			ar & StartStopType;
			ar & ParticleID;
			ar & ParentPdg;
		}
	}
};

#endif
