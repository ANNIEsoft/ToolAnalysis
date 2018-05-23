/* vim:set noexpandtab tabstop=4 wrap */
#ifndef TIMECLASS_H
#define TIMECLASS_H

#include <iostream>
#include<SerialisableObject.h>

using namespace std;

class TimeClass : public SerialisableObject{

	friend class boost::serialization::access;

	public:
  TimeClass() : unixns(0) , psec(0) {serialise=true;}
  TimeClass(uint64_t timens) : unixns(timens) , psec(0) {serialise=true;}
	TimeClass(uint64_t timens , double psc) : unixns(timens) , psec(psc) {serialise=true;}

	inline uint64_t GetNs() const {return unixns;}
	inline double GetPsec() const {return psec;}
	inline void SetNs(uint64_t tns){unixns=tns;}
	inline void SetPsec(double psc){psec=psc;}

	bool Print() {
		cout<<"unixns : "<<unixns<<endl;

		return true;
	}

	private:
	uint64_t unixns;
  double psec;
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & unixns;
		}
	}

	void Clear(){
		unixns = 0;
		psec = 0;
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
