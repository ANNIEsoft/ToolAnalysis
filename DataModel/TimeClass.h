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
  TimeClass(uint64_t timens) : unixns(timens), ps(0) {serialise=true;}
	
	inline uint64_t GetNs() const {return unixns;}
	inline uint32_t GetPsPart() const {return ps;}
	inline void SetNs(uint64_t tns){unixns=tns;}
	inline void SetPsPart(uint32_t tps){ps=tps;}
	
	bool Print() {
		cout<<"unixns : "<<unixns<<endl;
		cout<<"ps : "<<ps<<endl;
		
		return true;
	}
	
	private:
	uint64_t unixns;
	uint32_t ps;
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & unixns;
			ar & ps;
		}
	}
	
	void Clear(){
		unixns = 0;
		ps = 0;
	}
	
};

// Operator for printing the time since the Unix epoch (in ns) represented by
// a TimeClass object to a std::ostream
inline std::ostream& operator<<(std::ostream& out, const TimeClass& tc)
{
  out << tc.GetNs()<<"."<<tc.GetPsPart();
  return out;
}

#endif
