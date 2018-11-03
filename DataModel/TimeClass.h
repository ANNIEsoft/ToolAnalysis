/* vim:set noexpandtab tabstop=4 wrap */
#ifndef TIMECLASS_H
#define TIMECLASS_H

#include <iostream>
#include<SerialisableObject.h>

using namespace std;

class TimeClass : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
  TimeClass() : unixns(0) {serialise=true;}
  TimeClass(float timens) : unixns(timens) {serialise=true;}
	
	inline float GetNs() const {return unixns;}
	inline void SetNs(float tns){unixns=tns;}
	
	bool Print() {
		cout<<"unixns : "<<unixns<<endl;
		
		return true;
	}
	
	private:
	float unixns;
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & unixns;
		}
	}
	
	void Clear(){
		unixns = 0;
	}
	
};

// Operator for printing the time since the Unix epoch (in ns) represented by
// a TimeClass object to a std::ostream
inline std::ostream& operator<<(std::ostream& out, const TimeClass& tc)
{
  out << tc.GetNs();
  return out;
}

#endif
