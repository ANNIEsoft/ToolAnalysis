/* vim:set noexpandtab tabstop=4 wrap */
#include "Geometry.h"

Geometry::Geometry(double ver, Position tankc, double tankr, double tankhh, double mrdw, double mrdh, double mrdd, double mrds, int ntankpmts, int nmrdpmts, int nvetopmts, int nlappds, geostatus statin, std::vector<std::map<unsigned long,Detector>* >dets){
	NextFreeChannelKey=0;
	NextFreeDetectorKey=0;
	Version=ver;
	Status=statin;
	tank_centre=tankc;
	tank_radius=tankr;
	tank_halfheight=tankhh;
	mrd_width=mrdw;
	mrd_height=mrdh;
	mrd_depth=mrdd;
	mrd_start=mrds;
	numtankpmts=ntankpmts;
	nummrdpmts=nmrdpmts;
	numvetopmts=nvetopmts;
	numlappds=nlappds;
	Detectors=dets;
	serialise=true;
	RealDetectors.reserve(10);
}

Detector*  Geometry::GetDetector(unsigned long DetectorKey){
	for(int i=0 ; i<Detectors.size(); i++){
		for (std::map<unsigned long,Detector>::iterator it=Detectors.at(i)->begin();
														it!=Detectors.at(i)->end();
														++it){
			if(DetectorKey==it->first){
				Detector* det= &it->second;
				return det;
			}
		}
	}
	return 0;
}

Detector* Geometry::ChannelToDetector(unsigned long ChannelKey){
	if(ChannelMap.size()==0) InitChannelMap();
	if(ChannelMap.count(ChannelKey)) return ChannelMap.at(ChannelKey);
	return 0;
}

Channel* Geometry::GetChannel(unsigned long ChannelKey){
	if(ChannelMap.size()==0) InitChannelMap();
	if(ChannelMap.count(ChannelKey)) return &(ChannelMap.at(ChannelKey)->GetChannels()->at(ChannelKey));
	return 0;
}

void Geometry::InitChannelMap(){
	for(int i=0 ; i<Detectors.size(); i++){
		for(std::map<unsigned long,Detector>::iterator it=Detectors.at(i)->begin();
													   it!=Detectors.at(i)->end();
													   ++it){
			for(std::map<unsigned long,Channel>::iterator it2=it->second.GetChannels()->begin();
														  it2!=it->second.GetChannels()->end();
														  ++it2){
					if(ChannelMap.count(it2->first)!=0){
						cerr<<"ERROR: Geometry::InitChannelMap(): Detector "
							<<it->first<<" channel "
							<<std::distance(it->second.GetChannels()->begin(),it2)
							<<" has channel key "<<it2->first<<" which is not unique!"<<endl;
					} else {
						ChannelMap.emplace(it2->first,&(it->second));
					}
			}
		}
	}
}

void Geometry::PrintChannels(){
	cout<<"scanning "<<Detectors.size()<<" detector sets"<<endl;
	for(int i=0 ; i<Detectors.size(); i++){
		cout<<"set "<<i<<" has "<<Detectors.at(i)->size()<<" Detectors"<<endl;
		for(std::map<unsigned long,Detector>::iterator it=Detectors.at(i)->begin();
													   it!=Detectors.at(i)->end();
													   ++it){
			cout<<"Detector "<<std::distance(Detectors.at(i)->begin(),it)
			<<" has detectorkey "<<it->first<<" and "
				<<it->second.GetChannels()->size()<<" channels"<<endl;
			cout<<"calling Detector::PrintChannels()"<<endl;
			it->second.PrintChannels();
			cout<<"doing scan over retrieved channels"<<endl;
			for(std::map<unsigned long,Channel>::iterator it2=it->second.GetChannels()->begin();
														  it2!=it->second.GetChannels()->end();
														  ++it2){
					cout<<"next channel"<<endl;
					cout<<"Channel "<<std::distance(it->second.GetChannels()->begin(),it2);
					cout<<" has channelkey "<<it2->first;
					cout<<" at "<<(&(it2->second))<<endl;
					cout<<" and Detector "<<(&(it->second))<<endl;
			}
		}
	}
}
