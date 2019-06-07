/* vim:set noexpandtab tabstop=4 wrap */
#ifndef DETECTORCLASS_H
#define DETECTORCLASS_H

#include <math.h> // defines M_PI
//constexpr double PI=4.*atan(1.); // can also do this

#include <SerialisableObject.h>
#include "Position.h"
#include "Direction.h"
#include "Channel.h"
class Geometry;

enum class detectorstatus : uint8_t { OFF, ON, UNSTABLE };

class Detector : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
	Detector() : DetectorID(0), DetectorElement(), TankLocation(), DetectorPosition(), DetectorDirection(), DetectorType(""),
		Status(detectorstatus::OFF), Channels() {serialise=true;}
	Detector(int detid, std::string DetEle, std::string CylLoc, Position posin, Direction dirin, std::string detype, detectorstatus stat, double avgrate, map<unsigned long,Channel> channelsin={}) : DetectorID(detid), DetectorElement(DetEle), TankLocation(CylLoc), DetectorPosition(posin), DetectorDirection(dirin), DetectorType(detype), Status(stat), Channels(channelsin) { serialise=true; }
	
	std::string GetDetectorElement(){return DetectorElement;}
	Position GetDetectorPosition(){return DetectorPosition;}
	Position GetPositionInTank();
	Direction GetDetectorDirection(){return DetectorDirection;}
	int GetDetectorID(){return static_cast<int>(DetectorID);}
	std::string GetDetectorType(){return DetectorType;}
	detectorstatus GetStatus(){return Status;}
	std::map<unsigned long,Channel>* GetChannels() {return &Channels;}
	void AddChannel(Channel chanin){ Channels.emplace(chanin.GetChannelID(),chanin); }
	std::string GetTankLocation(){ return TankLocation; }
	Geometry* GetGeometryPtr(){ return GeometryPtr; }
	
	void SetDetectorElement(std::string DetEleIn){DetectorElement=DetEleIn;}
	void SetDetectorPosition(Position DetectorPositionIn){DetectorPosition=DetectorPositionIn;}
	void SetDetectorDirection(Direction DetectorDirectionIn){DetectorDirection=DetectorDirectionIn;}
	void SetDetectorID(int DetectorIDIn){DetectorID=DetectorIDIn;}
	void SetDetectorType(std::string DetectorTypeIn){DetectorType=DetectorTypeIn;}
	void SetStatus(detectorstatus StatusIn){Status=StatusIn;}
	void SetTankLocation(std::string locin){TankLocation=locin;}
	void SetGeometryPtr(Geometry* geomin){ GeometryPtr=geomin; }
	bool Print(){
		std::cout<<"DetectorPosition  : "; DetectorPosition.Print();
		std::cout<<"DetectorDirection : "; DetectorDirection.Print();
		std::cout<<"Location          : "<<TankLocation<<std::endl;
		std::cout<<"DetectorElement   : "<<DetectorElement<<std::endl;
		std::cout<<"DetectorID        : "<<DetectorID<<std::endl;
		std::cout<<"DetectorType      : "<<DetectorType<<std::endl;
		std::cout<<"Status            : "; PrintStatus(Status);
		return true;
	}
	bool PrintStatus(detectorstatus status){
		switch(status){
			case (detectorstatus::OFF): std::cout<<"OFF"<<std::endl; break;
			case (detectorstatus::ON): std::cout<<"ON"<<std::endl; break;
			case (detectorstatus::UNSTABLE) : std::cout<<"UNSTABLE"<<std::endl; break;
		}
		return true;
	}
	void PrintChannels(){
		cout<<"Detector "<<DetectorID<<" has "<<Channels.size()<<" channels: {";
		for(auto&& achan : Channels){ cout<<"("<<achan.first<<":"<<(&achan.second)<<"), "; }
		cout<<endl;
	}
	
	private:
	void SetChannels(map<unsigned long,Channel> chans){Channels=chans;}
	std::string DetectorElement;   // "PMT", "MRD", "LAPPD"...
	Position DetectorPosition;     // meters
	Direction DetectorDirection;   //
	unsigned long DetectorID;      // unique DetectorKey
	std::string DetectorType;      // e.g. "Hamamatsu R7081"
	detectorstatus Status;         // on, off, unstable....
	std::map<unsigned long,Channel> Channels;
	std::string TankLocation;      // "Barrel", "TopCap", "BottomCap", "MRD"*, "FACC"*, or "NA". *may change
	Geometry* GeometryPtr=nullptr; // a pointer to the parent geometry to which this Detector belongs
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
		ar & DetectorElement;
		ar & Channels;
		ar & DetectorPosition;
		ar & DetectorDirection;
		ar & TankLocation;
		ar & DetectorID;
		ar & DetectorType;
		ar & Status;
		}
	}
	
};

#endif
