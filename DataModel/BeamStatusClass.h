/* vim:set noexpandtab tabstop=4 wrap */
#ifndef BEAMSTATUSCLASS_H
#define BEAMSTATUSCLASS_H

#include<SerialisableObject.h>
#include "TimeClass.h"

using namespace std;

class BeamStatusClass : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
  BeamStatusClass() : Timestamp(), Intensity(0), Power(0), Stability(""){ serialise=true;}
	BeamStatusClass(TimeClass ts, double beaminten, double beampow, std::string beamstab) : Timestamp(ts), Intensity(beaminten), Power(beampow), Stability(beamstab){};
	
	inline TimeClass GetTimestamp(){return Timestamp;}
	inline double GetIntensity(){return Intensity;}
	inline double GetPower(){return Power;}
	inline std::string GetStability(){return Stability;}
	
	inline void SetTimestamp(TimeClass TimestampIn){Timestamp = TimestampIn;}
	inline void SetIntensity(double IntensityIn){Intensity = IntensityIn;}
	inline void SetPower(double PowerIn){Power = PowerIn;}
	inline void SetStability(std::string StabilityIn){Stability = StabilityIn;}
	
	bool Print(){
		cout<<"Beam Status Timestamp : "; Timestamp.Print();
		cout<<"Beam Intensity [ETOR875] : "<<Intensity<<endl;
		cout<<"Beam Power [protons/hr] : "<<Power<<endl;
		cout<<"Beam Stability : "<<Stability<<endl;
		
		return true;
	}
	
	void Clear(){
		Timestamp = TimeClass();
		Intensity=0;
		Power=0;
		Stability="";
	}
	
	private:
	TimeClass Timestamp;          // time these beam conditions were present
	double Intensity;             // ETOR875
	double Power;                 // protons/hr
	std::string Stability;        // is beam stable?  FIXME restrict to a set of known values
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & Timestamp;
			ar & Intensity;
			ar & Power;
			ar & Stability;
		}
	}
};

#endif
