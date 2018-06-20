/* vim:set noexpandtab tabstop=4 wrap */
#ifndef TRIGGERCLASS_H
#define TRIGGERCLASS_H

#include<SerialisableObject.h>
#include "TimeClass.h"

using namespace std;

class TriggerClass : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
  TriggerClass() : TriggerName(""), TriggerOccurred(false), TriggerTime(){serialise=true;}
  TriggerClass(std::string trigname, bool trigocc, TimeClass trigtime) : TriggerName(trigname), TriggerOccurred(trigocc), TriggerTime(trigtime){serialise=true;}
	
	inline std::string GetName(){return TriggerName;}
	inline TimeClass GetTime(){return TriggerTime;}
	inline bool GetOccurred(){return TriggerOccurred;}
	
	inline void SetName(std::string namein){TriggerName=namein;}
	inline void SetOccurred(bool trigoc){TriggerOccurred=trigoc;}
	inline void SetTime(TimeClass trigt){TriggerTime=trigt;}
	
	bool Print() {
		cout<<"TriggerName : "<<TriggerName<<endl;
		cout<<"TriggerTime : "; TriggerTime.Print();
		cout<<"TriggerOccurred : "<<TriggerOccurred<<endl;
		
		return true;
	}
	
	private:
	std::string TriggerName;
	bool TriggerOccurred;
	TimeClass TriggerTime;
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & TriggerName;
			ar & TriggerTime;
			ar & TriggerOccurred;
		}
	}
};

#endif
