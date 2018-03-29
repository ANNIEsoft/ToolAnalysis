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
  Waveform() : fStartTime(), fSamples(std::vector<T>{}) {serialise=true;}
  Waveform(TimeClass tsin, std::vector<T> samplesin) : fStartTime(tsin), fSamples(samplesin){serialise=true;}
	
	inline TimeClass GetStartTime() const {return fStartTime;}
	inline std::vector<T>* GetSamples() {return &fSamples;}
	inline const std::vector<T>& Samples() const { return fSamples; }
	inline T GetSample(int i) const {return fSamples.at(i);}
	
	inline void SetStartTime(TimeClass ts) {fStartTime=ts;}
	inline void SetSamples(std::vector<T> sam) {fSamples=sam;}
	inline void PushSample(T asample) {fSamples.push_back(asample);}
	inline void ClearSamples() {fSamples.clear();}
	
	bool Print() {
		int verbose=0;
		cout<<"StartTime : "; fStartTime.Print();
		cout<<"NSamples : "<<fSamples.size()<<endl;
		if(verbose){
			cout<<"Samples : {";
			for(int samplei=0; samplei<fSamples.size(); samplei++){
				cout<<fSamples.at(samplei);
				if((samplei+1)!=fSamples.size()) cout<<", ";
			}
			cout<<"}"<<endl;
		}
		
		return true;
	}
	
	protected:
	TimeClass fStartTime;
	std::vector<T> fSamples;
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & fStartTime;
			ar & fSamples;
		}
	}
};

#endif
