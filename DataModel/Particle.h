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
	  trackLength(len), StartStopType(tracktypein) {
		serialise=true;
//		if(tracktypein!=tracktype::UNDEFINED){
//			StartStopType=tracktypein;
//		} else {
//			// calculate track containment type
////			if(startinbounds && stopinbounds) startstoptype = tracktype::CONTAINED;
////			else if( startinbounds ) startstoptype = tracktype::STARTONLY;
////			else if( stopinbounds  ) startstoptype = tracktype::ENDONLY;
////			else startstoptype = tracktype::UNCONTAINED;
//		}
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
				tracktype::UNCONTAINED), ParticleID(0), ParentPdg(0), StartsInFiducialVolume(false), TrackAngleX(0), TrackAngleY(0), TrackAngleFromBeam(0), EntersTank(false), TankEntryPoint(Position()), ExitsTank(false), TankExitPoint(Position()), TrackLengthInTank(0), EntersMrd(false), MrdEntryPoint(Position()), ExitsMrd(false), MrdExitPoint(Position()), PenetratesMrd(false), TrackLengthInMrd(0), MrdPenetration(0), MrdLayersPenetrated(0), MrdEnergyLoss(0), Flag(0), MCTriggerNum(0) {serialise=true;}
	
	MCParticle(int pdg, double sttE, double stpE, Position sttpos, Position stppos, 
	  double sttt, double stpt, Direction startdir, double len, tracktype tracktypein,
	  int partid, int parentpdg, int flagid, int triggernum)
	: Particle(pdg, sttE, stpE, sttpos, stppos, sttt, stpt, startdir, len, tracktypein), 
	  ParticleID(partid), ParentPdg(parentpdg), StartsInFiducialVolume(false), TrackAngleX(0), TrackAngleY(0), TrackAngleFromBeam(0), EntersTank(false), TankEntryPoint(Position()), ExitsTank(false), TankExitPoint(Position()), TrackLengthInTank(0), EntersMrd(false), MrdEntryPoint(Position()), ExitsMrd(false), MrdExitPoint(Position()), PenetratesMrd(false), TrackLengthInMrd(0), MrdPenetration(0), MrdLayersPenetrated(0), MrdEnergyLoss(0), Flag(flagid), MCTriggerNum(triggernum)
	  {
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
	inline int GetFlag(){return Flag;}
	inline int GetMCTriggerNum(){return MCTriggerNum;}
	
	inline bool GetStartsInFiducialVolume(){return StartsInFiducialVolume;}
	
	inline double GetTrackAngleX(){return TrackAngleX;}
	inline double GetTrackAngleY(){return TrackAngleY;}
	inline double GetTrackAngleFromBeam(){return TrackAngleFromBeam;}
	
	inline bool GetEntersTank(){return EntersTank;}
	inline Position GetTankEntryPoint(){return TankEntryPoint;}
	inline bool GetExitsTank(){return ExitsTank;}
	inline Position GetTankExitPoint(){return TankExitPoint;}
	inline double GetTrackLengthInTank(){return TrackLengthInTank;}
	
	inline bool GetProjectedHitMrd(){return ProjectedHitMrd;}
	inline bool GetEntersMrd(){return EntersMrd;}
	inline Position GetMrdEntryPoint(){return MrdEntryPoint;}
	inline bool GetExitsMrd(){return ExitsMrd;}
	inline Position GetMrdExitPoint(){return MrdExitPoint;}
	inline bool GetPenetratesMrd(){return PenetratesMrd;} // full penetration: enters front face, exits back
	inline double GetTrackLengthInMrd(){return TrackLengthInMrd;}
	inline double GetMrdPenetration(){return MrdPenetration;} // [m], depth of reconstructed track in MRD
	inline int GetNumMrdLayersPenetrated(){return MrdLayersPenetrated;}
	inline double GetMrdEnergyLoss(){return MrdEnergyLoss;}
	
	inline void SetParticleID(int partidin){ParticleID=partidin;}
	inline void SetParentPdg(int parentpdgin){ParentPdg=parentpdgin;}
	inline void SetFlag(int flagidin){Flag=flagidin;}
	inline void SetMCTriggerNum(int triggernumin){MCTriggerNum=triggernumin;}
	
	inline void SetStartsInFiducialVolume(bool iStartsInFiducialVolume){StartsInFiducialVolume = iStartsInFiducialVolume;}
	
	inline void SetEntersTank(bool iEntersTank){EntersTank = iEntersTank;}
	inline void SetTankEntryPoint(Position iTankEntryPoint){TankEntryPoint = iTankEntryPoint;}
	inline void SetExitsTank(bool iExitsTank){ExitsTank = iExitsTank;}
	inline void SetTankExitPoint(Position iTankExitPoint){TankExitPoint = iTankExitPoint;}
	inline void SetTrackLengthInTank(double iTrackLengthInTank){TrackLengthInTank = iTrackLengthInTank;}
	
	inline void SetProjectedHitMrd(bool iProjectedHitMrd){ProjectedHitMrd = iProjectedHitMrd;}
	inline void SetEntersMrd(bool iEntersMrd){EntersMrd = iEntersMrd;}
	inline void SetMrdEntryPoint(Position iMrdEntryPoint){MrdEntryPoint = iMrdEntryPoint;}
	inline void SetExitsMrd(bool iExitsMrd){ExitsMrd = iExitsMrd;}
	inline void SetMrdExitPoint(Position iMrdExitPoint){MrdExitPoint = iMrdExitPoint ;}
	inline void SetPenetratesMrd(bool iPenetratesMrd){PenetratesMrd = iPenetratesMrd;}
	inline void SetTrackLengthInMrd(double iTrackLengthInMrd){TrackLengthInMrd = iTrackLengthInMrd;}
	inline void SetMrdPenetration(double iMrdPenetration){MrdPenetration = iMrdPenetration;}
	inline void SetNumMrdLayersPenetrated(int iMrdLayersPenetrated){MrdLayersPenetrated = iMrdLayersPenetrated;}
	inline void SetMrdEnergyLoss(double iMrdEnergyLoss){MrdEnergyLoss = iMrdEnergyLoss;}
	
	inline void SetTrackAngleX(double iTrackAngleX){TrackAngleX = iTrackAngleX;}
	inline void SetTrackAngleY(double iTrackAngleY){TrackAngleY = iTrackAngleY;}
	inline void SetTrackAngleFromBeam(double iTrackAngleFromBeam){TrackAngleFromBeam = iTrackAngleFromBeam;}
	
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
		std::cout<<"track angle from beam axis: "<<TrackAngleFromBeam<<std::endl;
		std::cout<<"track angle in horizontal paddles: "<<TrackAngleY<<std::endl;
		std::cout<<"track angle in vertical paddles: "<<TrackAngleX<<std::endl;
		std::cout<<"StartStopType : "; PrintStartStopType(StartStopType);
		std::cout<<"ParticleID : "<<ParticleID<<std::endl;
		std::cout<<"ParentPdg : "<<ParentPdg<<std::endl;
		std::cout<<"MCTriggerNum : "<<MCTriggerNum<<std::endl;
		std::cout<<"Parent Particle Name : "<<PdgToString(ParentPdg)<<std::endl;
		
		std::cout <<"StartsInFiducialVolume = "<<StartsInFiducialVolume <<std::endl;
		std::cout <<"TrackAngleX = "<<TrackAngleX <<std::endl;
		std::cout <<"TrackAngleY = "<<TrackAngleY <<std::endl;
		std::cout <<"TrackAngleFromBeam = "<<TrackAngleFromBeam <<std::endl;
		std::cout <<"EntersTank = "<<EntersTank <<std::endl;
		std::cout <<"TankEntryPoint = "; TankEntryPoint.Print();
		std::cout <<"ExitsTank = "<<ExitsTank <<std::endl;
		std::cout <<"TankExitPoint = "; TankExitPoint.Print();
		std::cout <<"TrackLengthInTank = "<<TrackLengthInTank <<std::endl;
		std::cout <<"ProjectedHitMrd = "<<ProjectedHitMrd << std::endl;
		std::cout <<"EntersMrd = "<<EntersMrd <<std::endl;
		std::cout <<"MrdEntryPoint = "; MrdEntryPoint.Print();
		std::cout <<"ExitsMrd = "<<ExitsMrd <<std::endl;
		std::cout <<"MrdExitPoint = "; MrdExitPoint.Print();
		std::cout <<"PenetratesMrd = "<<PenetratesMrd <<std::endl;
		std::cout <<"TrackLengthInMrd = "<<TrackLengthInMrd <<std::endl;
		std::cout <<"MrdPenetration = "<<MrdPenetration <<std::endl;       // [m]
		std::cout <<"MrdLayersPenetrated = "<<MrdLayersPenetrated <<std::endl;  // scint layers hit
		std::cout <<"MrdEnergyLoss = "<<MrdEnergyLoss <<std::endl;        // [MeV]
		
		return true;
	}
	
	protected:
	int ParticleID;
	int ParentPdg;
	int Flag;
	int MCTriggerNum; // trigger window in which the particle was created
	
	bool StartsInFiducialVolume;
	
	double TrackAngleX;
	double TrackAngleY;
	double TrackAngleFromBeam;  // [rads]
	
	bool EntersTank;
	Position TankEntryPoint;
	bool ExitsTank;
	Position TankExitPoint;
	double TrackLengthInTank;
	
	bool ProjectedHitMrd;
	double EntersMrd;
	Position MrdEntryPoint;
	bool ExitsMrd;
	Position MrdExitPoint;
	bool PenetratesMrd;          // full: enters at front, exits at back
	double TrackLengthInMrd;
	double MrdPenetration;       // [m]
	int MrdLayersPenetrated;     // scint layers hit
	double MrdEnergyLoss;        // [MeV]
	
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
			
			ar & StartsInFiducialVolume;
			
			ar & TrackAngleX;
			ar & TrackAngleY;
			ar & TrackAngleFromBeam;
			
			ar & EntersTank;
			ar & TankEntryPoint;
			ar & ExitsTank;
			ar & TankExitPoint;
			ar & TrackLengthInTank;
			
			ar & ProjectedHitMrd;
			ar & EntersMrd;
			ar & MrdEntryPoint;
			ar & ExitsMrd;
			ar & MrdExitPoint;
			ar & PenetratesMrd;
			ar & TrackLengthInMrd;
			ar & MrdPenetration;
			ar & MrdLayersPenetrated;
			ar & MrdEnergyLoss;
			ar & Flag;
		}
	}
};

#endif
