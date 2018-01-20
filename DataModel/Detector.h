/* vim:set noexpandtab tabstop=4 wrap */
#ifndef DETECTORCLASS_H
#define DETECTORCLASS_H

#include<SerialisableObject.h>
#include "Position.h"
#include "Direction.h"

using namespace std;

enum class detectorstatus : uint8_t { OFF, ON, UNSTABLE };
bool PrintStatus(detectorstatus status){
	switch(status){
		case (detectorstatus::OFF): cout<<"OFF"<<endl; break;
		case (detectorstatus::ON): cout<<"ON"<<endl; break;
		case (detectorstatus::UNSTABLE): cout<<"UNSTABLE"<<endl; break;
	}
	return true;
}

class Detector : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
	Detector() : DetectorElement(""), DetectorPosition(), DetectorDirection(), DetectorId(0), DetectorType(""), Status(detectorstatus::OFF), AvgPulseRate(0.) {};
	
	Detector(std::string detel, Position posin, Direction dirin, int detid, std::string detype, detectorstatus stat, double avgrate) : DetectorElement(detel), DetectorPosition(posin), DetectorDirection(dirin), DetectorId(detid), DetectorType(detype), Status(stat), AvgPulseRate(avgrate){};
	
	std::string GetDetectorElement(){return DetectorElement;}
	Position GetDetectorPosition(){return DetectorPosition;}
	Direction GetDetectorDirection(){return DetectorDirection;}
	int GetDetectorId(){return DetectorId;}
	std::string GetDetectorType(){return DetectorType;}
	detectorstatus GetStatus(){return Status;}
	double GetAvgPulseRate(){return AvgPulseRate;}
	
	void SetDetectorElement(std::string DetectorElementIn){DetectorElement=DetectorElementIn;}
	void SetDetectorPosition(Position DetectorPositionIn){DetectorPosition=DetectorPositionIn;}
	void SetDetectorDirection(Direction DetectorDirectionIn){DetectorDirection=DetectorDirectionIn;}
	void SetDetectorId(int DetectorIdIn){DetectorId=DetectorIdIn;}
	void SetDetectorType(std::string DetectorTypeIn){DetectorType=DetectorTypeIn;}
	void SetStatus(detectorstatus StatusIn){Status=StatusIn;}
	void SetAvgPulseRate(double AvgPulseRateIn){AvgPulseRate=AvgPulseRateIn;}
	
	bool Print(){
		cout<<"DetectorElement : "<<DetectorElement<<endl;
		cout<<"DetectorPosition : "; DetectorPosition.Print();
		cout<<"DetectorDirection : "; DetectorDirection.Print();
		cout<<"DetectorId : "<<DetectorId<<endl;
		cout<<"DetectorType : "<<DetectorType<<endl;
		cout<<"Status : "; PrintStatus(Status);
		cout<<"AvgPulseRate : "<<AvgPulseRate<<endl;
		
		return true;
	}
	
	private:
	std::string DetectorElement;   // "Tank", "MRD"... 
	Position DetectorPosition;     // meters
	Direction DetectorDirection;   // 
	int DetectorId;                // ID within DetectorElement
	std::string DetectorType;      // e.g. "Hamamatsu R7081"
	detectorstatus Status;         // on, off, unstable....
	double AvgPulseRate;           // 
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & DetectorElement;
			ar & DetectorPosition;
			ar & DetectorDirection;
			ar & DetectorId;
			ar & DetectorType;
			ar & Status;
			ar & AvgPulseRate;
		}
	}
};

#endif
