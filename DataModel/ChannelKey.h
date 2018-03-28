/* vim:set noexpandtab tabstop=4 wrap */
#ifndef CHANNELKEY_H
#define CHANNELKEY_H

#include <iostream>
#include "SerialisableObject.h"

using namespace std;

enum class subdetector : uint8_t { TDC, ADC, LAPPD};

class ChannelKey : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
  ChannelKey() : SubDetectorType(subdetector::ADC), DetectorElementIndex(0){serialise=true;}
	//ChannelKey(uint32_t subdetin, uint32_t detelin) : SubDetectorType(subdetin), DetectorElementIndex(detelin){};
  ChannelKey(subdetector subdetin, uint32_t detelin) : SubDetectorType(subdetin), DetectorElementIndex(detelin){serialise=true;}
	
	//inline uint32_t GetSubDetectorType() {return SubDetectorType;}
	inline subdetector GetSubDetectorType() const{return SubDetectorType;}
	inline uint32_t GetDetectorElementIndex() const {return DetectorElementIndex;}
	//inline void SetSubDetectorType(uint32_t subdetin){SubDetectorType=subdetin;}
	inline void SetSubDetectorType(subdetector subdetin){SubDetectorType=subdetin;}
	inline void SetDetectorElementIndex(uint32_t detelin){DetectorElementIndex=detelin;}
	
	bool Print() {
		cout<<"SubDetectorType : "<<uint8_t(SubDetectorType)<<endl;
		cout<<"DetectorElementIndex : "<<DetectorElementIndex<<endl;
		
		return true;
	}
	
	bool operator <( const ChannelKey rhs) const{
		subdetector rhstype = rhs.GetSubDetectorType();
		uint32_t rhsindex = rhs.GetDetectorElementIndex();
		if(SubDetectorType == rhstype){
			return (DetectorElementIndex < rhsindex);
		} else {
			return (SubDetectorType < rhstype);
		}
	}
	
	private:
	//uint32_t SubDetectorType;
	subdetector SubDetectorType;
	uint32_t DetectorElementIndex;
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & SubDetectorType;
			ar & DetectorElementIndex;
		}
	}
};

#endif
