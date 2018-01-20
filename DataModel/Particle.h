/* vim:set noexpandtab tabstop=4 wrap */
#ifndef PARTICLECLASS_H
#define PARTICLECLASS_H

#include<SerialisableObject.h>
#include "Position.h"
#include "Direction.h"
#include "TimeClass.h"
class Geometry;

using namespace std;
// world extent in WCSim is +-600cm in all directions!
enum class tracktype : uint8_t { STARTONLY, ENDONLY, CONTAINED, UNCONTAINED };

class ANNIEEvent;

class Particle : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
	
	Particle() : ParticlePDG(0), startEnergy(0), stopEnergy(0), startVertex(),
	  stopVertex(), startTime(), stopTime(), startDirection(),
	  trackLength(0), StartStopType(tracktype::UNCONTAINED) {};
	
	Particle(int pdg, double sttE, double stpE, Position sttpos, Position stppos, 
	  TimeClass sttt, TimeClass stpt, Direction startdir, double len, tracktype tracktypein) 
	: ParticlePDG(pdg), startEnergy(sttE), stopEnergy(stpE), startVertex(sttpos),
	  stopVertex(stppos), startTime(sttt), stopTime(stpt), startDirection(startdir),
	  trackLength(len), StartStopType(tracktypein) {};
	
	inline void SetPdgCode(int code){ParticlePDG=code;}
	inline void SetStartEnergy(double E){startEnergy=E;}
	inline void SetStopEnergy(double E){stopEnergy=E;}
	inline void SetStartVertex(Position pos){startVertex=pos;}
	inline void SetStopVertex(Position pos){stopVertex=pos;}
	inline void SetStartTime(TimeClass stime){startTime=stime;}
	inline void SetStopTime(TimeClass stime){stopTime=stime;}
	inline void SetstartDirection(Direction dir){startDirection=dir;}
	inline void SetTrackLength(double len){trackLength=len;}
	inline void SetTrackStartStopType(tracktype tracktypein){StartStopType=tracktypein;}
	
	inline int GetPdgCode(){return ParticlePDG;}
	inline double GetStartEnergy(){return startEnergy;}
	inline double GetStopEnergy(){return stopEnergy;}
	inline Position GetStartVertex(){return startVertex;}
	inline Position GetStopVertex(){return stopVertex;}
	inline TimeClass GetStartTime(){return startTime;}
	inline TimeClass GetStopTime(){return stopTime;}
	inline Direction GetstartDirection(){return startDirection;}
	inline double GetTrackLength(){return trackLength;}
	inline tracktype GetStartStopType(){return StartStopType;}
	
	virtual bool Print(){
		cout<<"ParticlePDG : "<<ParticlePDG<<endl;
		cout<<"startEnergy : "<<startEnergy<<endl;
		cout<<"stopEnergy : "<<stopEnergy<<endl;
		cout<<"startVertex : "; startVertex.Print();
		cout<<"stopVertex : "; stopVertex.Print();
		cout<<"startTime : "; startTime.Print();
		cout<<"stopTime : "; stopTime.Print();
		cout<<"startDirection : "; startDirection.Print();
		cout<<"trackLength : "<<trackLength<<endl;
		cout<<"StartStopType : "<<uint8_t(StartStopType)<<endl;
		
		return true;
	}
	
	protected:
	int ParticlePDG;
	tracktype StartStopType;         // does it start and/or stop in the detector / simulated volume?
	double startEnergy;              // GeV
	double stopEnergy;
	Position startVertex;            // meters
	Position stopVertex;
	TimeClass startTime;
	TimeClass stopTime;
	Direction startDirection;        // for primary particle initial scattering dir is most important.
	double trackLength;              // meters
	
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
	public:
	
	MCParticle() : Particle(0, 0., 0., Position(), Position(), TimeClass(), TimeClass(), Direction(), 0.,
	  tracktype::UNCONTAINED), ParticleID(0), ParentID(0) {};
	
	MCParticle(int partid, int parentid, int pdg, double sttE, double stpE, Position sttpos, Position stppos, 
	  TimeClass sttt, TimeClass stpt, Direction startdir, double len, tracktype tracktypein) 
	: Particle(pdg, sttE, stpE, sttpos, stppos, sttt, stpt, startdir, len, tracktypein), 
	  ParticleID(partid), ParentID(parentid){};
	
	inline int GetParticleID(){return ParticleID;}
	inline int GetParentID(){return ParentID;}
	inline void SetParticleID(int partidin){ParticleID=partidin;}
	inline void SetParentID(int parentidin){ParentID=parentidin;}
	
	bool Print(){
		cout<<"ParticleID : "<<ParticleID<<endl;
		cout<<"ParentID : "<<ParentID<<endl;
		cout<<"ParticlePDG : "<<ParticlePDG<<endl;
		cout<<"startEnergy : "<<startEnergy<<endl;
		cout<<"stopEnergy : "<<stopEnergy<<endl;
		cout<<"startVertex : "; startVertex.Print();
		cout<<"stopVertex : "; stopVertex.Print();
		cout<<"startTime : "; startTime.Print();
		cout<<"stopTime : "; stopTime.Print();
		cout<<"startDirection : "; startDirection.Print();
		cout<<"trackLength : "<<trackLength<<endl;
		cout<<"StartStopType : "<<uint8_t(StartStopType)<<endl;
		
		return true;
	}
	
	private:
	int ParticleID;
	int ParentID;
	
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
			ar & ParentID;
		}
	}
};

#endif
