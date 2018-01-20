/* vim:set noexpandtab tabstop=4 wrap */
#ifndef WAVEFORMCLASS_H
#define WAVEFORMCLASS_H

#include<SerialisableObject.h>
#include "TimeClass.h"

using namespace std;

template <class T>
class Waveform : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
	Waveform() : StartTime(), Samples(std::vector<T>{}) {};
	Waveform(TimeClass tsin, std::vector<T> samplesin) : StartTime(tsin), Samples(samplesin){};
	
	inline TimeClass GetStartTime(){return StartTime;}
	inline std::vector<T>* GetSamples(){return &Samples;}
	inline T GetSample(int i){return Samples.at(i);}
	
	inline void SetStartTime(TimeClass ts){StartTime=ts;}
	inline void SetSamples(std::vector<T> sam){Samples=sam;}
	inline void PushSample(T asample){Samples.push_back(asample);}
	inline void ClearSamples(){Samples.clear();}
	
	bool Print(){
		int verbose=0;
		cout<<"StartTime : "; StartTime.Print();
		cout<<"NSamples : "<<Samples.size()<<endl;
		if(verbose){
			cout<<"Samples : {";
			for(int samplei=0; samplei<Samples.size(); samplei++){
				cout<<Samples.at(samplei);
				if((samplei+1)!=Samples.size()) cout<<", ";
			}
			cout<<"}"<<endl;
		}
		
		return true;
	}
	
	protected:
	TimeClass StartTime;
	std::vector<T> Samples;
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & StartTime;
			ar & Samples;
		}
	}
};

#endif
