/* vim:set noexpandtab tabstop=4 wrap */
	
	
	std::vector<Particle> MCParticles;
	std::vector<Particle> RecoParticles;
	std::map<ChannelKey,std::vector<Hit>> MCHits;  // FIXME ADC and LAPPD combined?
	std::map<ChannelKey,std::vector<Hit>> TDCData; // FIXME veto and MRD combined?
	std::map<ChannelKey,std::vector<Waveform<uint16_t>>> RawADCData;
	std::map<ChannelKey,std::vector<Waveform<uint16_t>>> RawLAPPDData;
	std::map<ChannelKey,std::vector<Waveform<double>>> CalibratedADCData;
	std::map<ChannelKey,std::vector<Waveform<double>>> CalibratedLAPPDData;
	std::vector<TriggerClass> TriggerData;


/* vim:set noexpandtab tabstop=4 wrap */
#ifndef ANNIEWRAPPERS_H
#define ANNIEWRAPPERS_H

#include<SerialisableObject.h>
#include "Particle.h"
#include "Hit.h"
#include "Waveform.h"
#include "TriggerClass.h"

template <typename T>
class annievector : public std::vector<T>, public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
	
	bool Print(){
		cout<<"{";
		for(int i=0; i<this.size(); i++){
			auto&& athing=this.at(i);
			cout<<"athing";
			if((i+1)<this.size()) cout<<", ";
		}
		cout<<"}"<<endl;
		
		return true;
	}
	
	private:
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & x;
			ar & y;
			ar & z;
		}
	}
};

#endif
