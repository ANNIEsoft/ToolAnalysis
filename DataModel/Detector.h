/* vim:set noexpandtab tabstop=4 wrap */
#ifndef DETECTORCLASS_H
#define DETECTORCLASS_H

#include <SerialisableObject.h>
#include "Position.h"
#include "Direction.h"
#include "Channel.h"

using namespace std;

enum class detectorstatus : uint8_t { OFF, ON, UNSTABLE };


class Detector : public SerialisableObject{

	friend class boost::serialization::access;

	public:
	Detector() :DetectorElement(), DetectorPosition(), DetectorDirection(), DetectorID(0), DetectorType(""), Status(detectorstatus::OFF), AvgPulseRate(0.), Channels() {serialise=true;}
	  Detector(string DetEle,Position posin, Direction dirin, int detid, std::string detype, detectorstatus stat, double avgrate, map<unsigned long,Channel> channels) : DetectorElement(DetEle), DetectorPosition(posin), DetectorDirection(dirin), DetectorID(detid), DetectorType(detype), Status(stat), AvgPulseRate(avgrate), Channels(channels) {serialise=true;}
	string GetDetectorElement(){return DetectorElement;}
	Position GetDetectorPosition(){return DetectorPosition;}
	Direction GetDetectorDirection(){return DetectorDirection;}
	int GetDetectorID(){return DetectorID;}
	std::string GetDetectorType(){return DetectorType;}
	detectorstatus GetStatus(){return Status;}
	double GetAvgPulseRate(){return AvgPulseRate;}
	std::map<unsigned long,Channel> GetChannels() {return Channels;}

	void SetDetectorElement(string DetEleIn){DetectorElement=DetEleIn;}
	void SetDetectorPosition(Position DetectorPositionIn){DetectorPosition=DetectorPositionIn;}
	void SetDetectorDirection(Direction DetectorDirectionIn){DetectorDirection=DetectorDirectionIn;}
	void SetDetectorID(int DetectorIDIn){DetectorID=DetectorIDIn;}
	void SetDetectorType(std::string DetectorTypeIn){DetectorType=DetectorTypeIn;}
	void SetStatus(detectorstatus StatusIn){Status=StatusIn;}
	void SetAvgPulseRate(double AvgPulseRateIn){AvgPulseRate=AvgPulseRateIn;}
	bool Print() {
    cout<<"DetectorPosition : "; DetectorPosition.Print();
		cout<<"DetectorDirection : "; DetectorDirection.Print();
    cout<<"DetectorElement : "<<DetectorElement<<endl;
		cout<<"DetectorID : "<<DetectorID<<endl;
		cout<<"DetectorType : "<<DetectorType<<endl;
	  cout<<"Status : "; PrintStatus(Status);
		cout<<"AvgPulseRate : "<<AvgPulseRate<<endl;

		return true;
	}
  bool PrintStatus(detectorstatus status){
      switch(status){
        case (detectorstatus::OFF): cout<<"OFF"<<endl; break;
        case (detectorstatus::ON): cout<<"ON"<<endl; break;
        case (detectorstatus::UNSTABLE) : cout<<"UNSTABLE"<<endl; break;
      }
      return true;
  }
	private:
	void SetChannels(map<unsigned long,Channel> chans){Channels=chans;}
	std::string DetectorElement;   // "PMT", "MRD", "LAPPD"...
	Position DetectorPosition;     // meters
	Direction DetectorDirection;   //
	unsigned long DetectorID;                // ID within DetectorElement
	std::string DetectorType;      // e.g. "Hamamatsu R7081"
	detectorstatus Status;         // on, off, unstable....
	double AvgPulseRate;           //
	std::map<unsigned long,Channel> Channels;

	template<class Archive> void serialize(Archive & ar, const unsigned int version){

		if(serialise){
		ar & DetectorElement;
		ar & Channels;
		ar & DetectorPosition;
		ar & DetectorDirection;
		ar & DetectorID;
		ar & DetectorType;
		ar & Status;
		ar & AvgPulseRate;
		}
	}

};

#endif
