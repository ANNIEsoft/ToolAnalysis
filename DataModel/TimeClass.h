/* vim:set noexpandtab tabstop=4 wrap */
#ifndef TIMECLASS_H
#define TIMECLASS_H

#include <iostream>
#include <time.h>
#include <cmath>
#include<SerialisableObject.h>

using namespace std;
namespace {
	constexpr uint64_t NS_TO_SECONDS = 1000000000;
}

class TimeClass : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
  TimeClass() : unixns(0) {serialise=true;}
  TimeClass(uint64_t timens) : unixns(timens) {serialise=true;}
	
	inline uint64_t GetNs() const {return unixns;}
	inline void SetNs(uint64_t tns){unixns=tns;}
	
	bool Print() {
		cout<< AsString() << endl;
		return true;
	}
	
	std::string AsString(){
		// convert to human readable format
		uint64_t rounded_seconds = static_cast<uint64_t>(floor(unixns/NS_TO_SECONDS));
		time_t rawtime = rounded_seconds;
		uint64_t remaining_ns = unixns - rounded_seconds*NS_TO_SECONDS;
		struct tm * ptm;               // time structure from which components can be retrieved
		ptm = gmtime(&rawtime);        // fill timestructure with UTC time
		struct tm tm_mb = tm(*ptm);    // the struct filled by gmtime is internal, so gets overridden
							           // by subsequent calls. We need to make a copy.
		
		int bufsize=100;
		char logbuffer[bufsize];
		int discardedcharcount = snprintf(logbuffer, bufsize,
			"%d/%d/%d %02d:%02d:%02d.%09lu UTC (%lu NS since Unix Epoch)",
			tm_mb.tm_year+1900,tm_mb.tm_mon+1,tm_mb.tm_mday,tm_mb.tm_hour,tm_mb.tm_min,
			tm_mb.tm_sec,remaining_ns,unixns); // month+1 because it's "months since january"
		
		std::string timestamp_as_string(logbuffer);
		return timestamp_as_string;
	}
	
	void AsString(std::string& stringin){
		stringin=AsString();
		return;
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

// Operator for printing the time since the Unix epoch (in ns) represented by
// a TimeClass object to a std::ostream
inline std::ostream& operator<<(std::ostream& out, const TimeClass& tc)
{
  out << tc.GetNs();
  return out;
}

#endif
