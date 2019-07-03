/* vim:set noexpandtab tabstop=4 wrap */
#include "Geometry.h"

Geometry::Geometry(double ver, Position tankc, double tankr, double tankhh, double pmtencr, double pmtenchh, double mrdw, double mrdh, double mrdd, double mrds, int ntankpmts, int nmrdpmts, int nvetopmts, int nlappds, geostatus statin, std::map<std::string,std::map<unsigned long,Detector> >dets){
	NextFreeChannelKey=0;
	NextFreeDetectorKey=0;
	Version=ver;
	Status=statin;
	tank_centre=tankc;
	tank_radius=tankr;
	tank_halfheight=tankhh;
	pmt_enclosed_radius=pmtencr;
	pmt_enclosed_halfheight=pmtenchh;
	mrd_width=mrdw;
	mrd_height=mrdh;
	mrd_depth=mrdd;
	mrd_start=mrds;
	numtankpmts=ntankpmts;
	nummrdpmts=nmrdpmts;
	numvetopmts=nvetopmts;
	numlappds=nlappds;
	RealDetectors=dets;
	serialise=true;
}

Detector*  Geometry::GetDetector(unsigned long DetectorKey){
	for(std::map<std::string,std::map<unsigned long,Detector>>::iterator it = RealDetectors.begin();
		it!=RealDetectors.end();
		++it){
		for (std::map<unsigned long,Detector>::iterator it2=it->second.begin();
														it2!=it->second.end();
														++it2){
			if(DetectorKey==it2->first){
				Detector* det= &it2->second;
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
	// loop over detector sets
	for(std::map<std::string,std::map<unsigned long,Detector>>::iterator it = RealDetectors.begin();
																		 it!=RealDetectors.end();
																		 ++it){
		// loop over detectors in a set
		for(std::map<unsigned long,Detector>::iterator it2=it->second.begin();
													   it2!=it->second.end();
													   ++it2){
			// loop over channels in a detector
			for(std::map<unsigned long,Channel>::iterator it3=it2->second.GetChannels()->begin();
														  it3!=it2->second.GetChannels()->end();
														  ++it3){
				if(ChannelMap.count(it3->first)!=0){
					cerr<<"ERROR: Geometry::InitChannelMap(): Detector "
						<<it2->first<<" channel "
						<<std::distance(it2->second.GetChannels()->begin(),it3)
						<<" has channel key "<<it3->first<<" which is not unique!"<<endl;
				} else {
					ChannelMap.emplace(it3->first,&(it2->second));
				}
			}
		}
	}
}

void Geometry::PrintChannels(){
	cout<<"scanning "<<RealDetectors.size()<<" detector sets"<<endl;
	// loop over detector sets
	for(std::map<std::string,std::map<unsigned long,Detector>>::iterator it = RealDetectors.begin();
																		 it!=RealDetectors.end();
																		 ++it){
		cout<<"set "<<std::distance(RealDetectors.begin(),it)
			<<" has "<<it->second.size()<<" RealDetectors"<<endl;
		// loop over detectors in this set
		for(std::map<unsigned long,Detector>::iterator it2=it->second.begin();
													   it2!=it->second.end();
													   ++it2){
			cout<<"Detector "<<std::distance(it->second.begin(),it2)
				<<" has detectorkey "<<it2->first<<" and "
				<<it2->second.GetChannels()->size()<<" channels"<<endl;
			cout<<"calling Detector::PrintChannels()"<<endl;
			it2->second.PrintChannels();
			cout<<"doing scan over retrieved channels"<<endl;
			// loop over channels in this detector
			for(std::map<unsigned long,Channel>::iterator it3=it2->second.GetChannels()->begin();
														  it3!=it2->second.GetChannels()->end();
														  ++it3){
					cout<<"next channel"<<endl;
					cout<<"Channel "<<std::distance(it2->second.GetChannels()->begin(),it3);
					cout<<" has channelkey "<<it3->first;
					cout<<" at "<<(&(it3->second))<<endl;
					cout<<" and Detector "<<(&(it2->second))<<endl;
			}
		}
	}
}

void Geometry::CartesianToPolar(Position posin, double& R, double& Phi, double& Theta, bool tankcentered){
	// Calculate angle from beam axis, measured clockwise while looking down
	// first shift to place relative to the tank origin if needed
	if(not tankcentered){ posin -= tank_centre; }
	// calculate the angle from the beam axis
	double thethetaval = atan(posin.X()/abs(posin.Z()));
	if(posin.Z()<0.){ (posin.X()<0.) ? thethetaval=(-M_PI+thethetaval) : thethetaval=(M_PI-thethetaval); }
	Phi = thethetaval;
	// calculate angle from the x-z plane
	Theta = atan(posin.Y() / sqrt(pow(posin.X(),2.)+pow(posin.Z(),2.)));
	// calculate the radial distance from the tank centre
	R = sqrt(pow(posin.X(),2.)+pow(posin.Z(),2.));
	return;
}
