/* vim:set noexpandtab tabstop=4 wrap */
#ifndef TIMECLASS_H
#define TIMECLASS_H

#include<SerialisableObject.h>

using namespace std;

class TimeClass : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
  TimeClass() : unixns(0) {serialise=true;}
  TimeClass(uint64_t timens) : unixns(timens) {serialise=true;}
	
	inline uint64_t GetNs(){return unixns;}
	inline void SetNs(uint64_t tns){unixns=tns;}
	
	bool Print(){
		cout<<"unixns : "<<unixns<<endl;
		
		return true;
	}
	
	private:
	uint64_t unixns;
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & unixns;
		}
	}
	
	void Clear(){
		unixns = 0;
	}
	
};

#endif
