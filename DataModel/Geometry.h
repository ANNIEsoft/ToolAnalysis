/* vim:set noexpandtab tabstop=4 wrap */
#ifndef GEOMETRYCLASS_H
#define GEOMETRYCLASS_H

#include<SerialisableObject.h>
#include "ChannelKey.h"
#include "Detector.h"
#include "Paddle.h"
#include "Particle.h"
#include "Channel.h"
using namespace std;

enum class geostatus : uint8_t { FULLY_OPERATIONAL, TANK_ONLY, MRD_ONLY, };

class Geometry : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
	// Do we care to have the overloaded empty constructor?
	Geometry() : NextFreeChannelKey(0), NextFreeDetectorKey(0), Version(0.), tank_centre(Position(0,0,0)), tank_radius(0.), tank_halfheight(0.), pmt_enclosed_radius(0.), pmt_enclosed_halfheight(0.), mrd_width(0.), mrd_height(0.), mrd_depth(0.), mrd_start(0.), numtankpmts(0), nummrdpmts(0), numvetopmts(0), numlappds(0), Status(geostatus::FULLY_OPERATIONAL) {
		serialise=true;
	}
	
	Geometry(double ver, Position tankc, double tankr, double tankhh, double pmtencr, double pmtenchh, double mrdw, double mrdh, double mrdd, double mrds, int ntankpmts, int nmrdpmts, int nvetopmts, int nlappds, geostatus statin, std::map<std::string,std::map<unsigned long,Detector> >dets=std::map<std::string,std::map<unsigned long,Detector> >{});
	
	inline std::map<std::string, std::map<unsigned long,Detector*> >* GetDetectors(){return &Detectors;}
	inline double GetVersion(){return Version;}
	inline geostatus GetStatus(){return Status;}
	inline Position GetTankCentre(){return tank_centre;}
	inline double GetTankRadius(){return tank_radius;}
	inline double GetTankHalfheight(){return tank_halfheight;}
	inline double GetPMTEnclosedRadius(){return pmt_enclosed_radius;}
	inline double GetPMTEnclosedHalfheight(){return pmt_enclosed_halfheight;}
	inline double GetFiducialCutRadius(){return fiducialradius;}
	inline double GetFiducialCutY(){return fiducialcuty;}
	inline double GetFiducialCutZ(){return fiducialcutz;}
	inline double GetMrdWidth(){return mrd_width;}
	inline double GetMrdHeight(){return mrd_height;}
	inline double GetMrdDepth(){return mrd_depth;}
	inline double GetMrdStart(){return mrd_start;}
	inline double GetMrdEnd(){return mrd_start+mrd_depth;}
	
	inline void SetVersion(double VersionIn){Version = VersionIn;}
	inline void SetStatus(geostatus StatusIn){Status=StatusIn;}
	inline void SetTankCentre(Position tank_centrein){tank_centre = tank_centrein;}
	inline void SetTankRadius(double tank_radiusIn){tank_radius = tank_radiusIn;}
	inline void SetTankHalfheight(double tank_halfheightIn){tank_halfheight = tank_halfheightIn;}
	inline void SetPMTEnclosedRadius(double pmt_enclosed_radiusIn){pmt_enclosed_radius = pmt_enclosed_radiusIn;}
	inline void SetPMTEnclosedHalfheight(double pmt_enclosed_halfheightIn){pmt_enclosed_halfheight = pmt_enclosed_halfheightIn;}
	inline void SetFiducialCutRadius(double fidcutradiusin){fiducialradius = fidcutradiusin;}
	inline void SetFiducialCutZ(double fidcutzin){fiducialcutz = fidcutzin;}
	inline void SetFiducialCutY(double fidcutyin){fiducialcuty = fidcutyin;}
	inline void SetMrdWidth(double mrd_widthIn){mrd_width = mrd_widthIn;}
	inline void SetMrdHeight(double mrd_heightIn){mrd_height = mrd_heightIn;}
	inline void SetMrdDepth(double mrd_depthIn){mrd_depth = mrd_depthIn;}
	inline void SetMrdStart(double mrd_startIn){mrd_start = mrd_startIn;}
	void SetDetectors(std::map<std::string,std::map<unsigned long,Detector> >DetectorsIn){
		RealDetectors = DetectorsIn;  // copy them in; we want to own our detectors
		// although if we're going to use this, we may wish to provide a method for passing in
		// detectors on the heap and taking ownership of them to avoid the copy.. TODO
		// build the map of pointers
		Detectors.clear(); // just in case.
		for(auto&& aset : RealDetectors){
			std::map<unsigned long,Detector*> tempset;
			for(auto&& adetector : aset.second){
				tempset.emplace(adetector.first,&adetector.second);
			}
			Detectors.emplace(aset.first,tempset);
		}
	}
	void SetPaddles(std::map<unsigned long,Paddle> PaddlesIn){
		Paddles = PaddlesIn;
	}
	
	unsigned long ConsumeNextFreeChannelKey(){
		unsigned long thefreechannelkey = NextFreeChannelKey;
		NextFreeChannelKey++;
		return thefreechannelkey;
	}
	unsigned long ConsumeNextFreeDetectorKey(){
		unsigned long thefreedetectorkey = NextFreeDetectorKey;
		NextFreeDetectorKey++;
		return thefreedetectorkey;
	}
	
	bool SetDetectorPaddle(unsigned long detectorkey, Paddle paddlein){
		// quickly check we have a Detector associated to this DetectorKey
		if(DetectorKeys.count(detectorkey)==0){
			std::cerr<<"Geometry error! AddPaddle called with unknown DetectorKey "
					 <<detectorkey<<std::endl;
			return false;
		} else {
			// check if we've already allocated a Paddle to this DetectorKey
			if(Paddles.count(detectorkey)){
				std::cerr<<"Geometry error! SetDetectorPaddle called for detector "<<detectorkey
						 <<" but this Detector already has a Paddle allocated!"<<std::endl;
				return false;
			} else {
				// Add the paddle
				Paddles.emplace(detectorkey,paddlein);
				return true;
			}
		}
	}
	
	bool AddDetector(Detector detin){
		// quickly check if we've allocated this DetectorKey
		if(DetectorKeys.count(detin.GetDetectorID())){
			std::cerr<<"Geometry error! AddDetector called with non-unique DetectorKey "
					 <<detin.GetDetectorID()<<std::endl;
			return false;
		} else {
			DetectorKeys.emplace(detin.GetDetectorID(),1);
		}
		
		// Pass a pointer to it's owning geometry to this Detector
		// we need to do this before calling `emplace` as that must do a copy-construction
		detin.SetGeometryPtr(this);
		
		std::string thedetel = detin.GetDetectorElement();
		if(Detectors.count(thedetel)==0){
			// this is a new detector element - create a new entry in the Detectors map
			RealDetectors.emplace(thedetel,std::map<unsigned long,Detector>{});
			Detectors.emplace(thedetel,std::map<unsigned long,Detector*>{});
		}
		RealDetectors.at(thedetel).emplace(detin.GetDetectorID(), detin);
		Detectors.at(thedetel).emplace(detin.GetDetectorID(),
										&RealDetectors.at(thedetel).at(detin.GetDetectorID()));
		
		return true;
	}
	
	inline int GetNumDetectors(){return Detectors.size();}  // FIXME this is the num detector SETS
	Detector* GetDetector(unsigned long DetectorKey);
	Detector* ChannelToDetector(unsigned long ChannelKey);
	Channel* GetChannel(unsigned long ChannelKey);
	Paddle* GetDetectorPaddle(unsigned long DetectorKey){
		if(Paddles.count(DetectorKey)){
			return (&(Paddles.at(DetectorKey)));
		} else {
			std::cerr<<"Geometry Error: Attempt to retreive Paddle for detector "
					 <<DetectorKey<<" which has no paddle"<<std::endl;
			return 0;
		}
	}
	void InitChannelMap();
	
	int GetNumDetectorsInSet(std::string SetName){
		if(Detectors.count(SetName)==0){
			return 0;
		}
		return Detectors.at(SetName).size();
	}
	
	int GetNumTankPMTs(){  // n.b. this doesn't include OD PMTs
		if(numtankpmts==0){  // double check, in case this variable isn't set
			numtankpmts = GetNumDetectorsInSet("Tank");
		}
		return numtankpmts;
	}
	
	int GetNumMrdPMTs(){
		if(nummrdpmts==0){  // double check, in case this variable isn't set
			nummrdpmts = GetNumDetectorsInSet("MRD");
		}
		return nummrdpmts;
	}
	
	int GetNumVetoPMTs(){
		if(numvetopmts==0){  // double check, in case this variable isn't set
			numvetopmts = GetNumDetectorsInSet("Veto");
		}
		return numvetopmts;
	}
	
	int GetNumLAPPDs(){
		if(numlappds==0){  // double check, in case this variable isn't set
			numlappds = GetNumDetectorsInSet("LAPPD");
		}
		return numlappds;
	}
	
	int GetNumODPMTs(){
		if(numodpmts==0){  // double check, in case this variable isn't set
			numodpmts = GetNumDetectorsInSet("OD");
		}
		return numodpmts;
	}
	
	bool GetTankContained(Particle part, int startstop=0){
		Position aVertex = (startstop==0) ? part.GetStopVertex() : part.GetStartVertex();
		bool tankcontained = (sqrt(pow(aVertex.X(),2.)+pow(aVertex.Z()-tank_centre.Z(),2.)) < tank_radius) &&
							 (abs(aVertex.Y()-tank_centre.Y())<tank_halfheight);
		return tankcontained;
	}
	bool GetTankContained(Position aVertex){
		bool tankcontained = (sqrt(pow(aVertex.X(),2.)+pow(aVertex.Z()-tank_centre.Z(),2.)) < tank_radius) &&
							 (abs(aVertex.Y()-tank_centre.Y())<tank_halfheight);
		return tankcontained;
	}
	
	bool GetMrdContained(Particle part, int startstop=0){
		Position aVertex = (startstop==0) ? part.GetStopVertex() : part.GetStartVertex();
		bool MrdContained = (abs(aVertex.X())<mrd_width/2.) &&
							(aVertex.Z()>mrd_start) && (aVertex.Z()<(mrd_start+mrd_depth)) &&
							(abs(aVertex.Y())<mrd_height/2.);
		return MrdContained;
	}
	bool GetMrdContained(Position aVertex){
		bool MrdContained = (abs(aVertex.X())<mrd_width/2.) &&
							(aVertex.Z()>mrd_start) && (aVertex.Z()<(mrd_start+mrd_depth)) &&
							(abs(aVertex.Y())<mrd_height/2.);
		return MrdContained;
	}
	
	bool Print(){
		int verbose=0;
		cout<<"Num Detectors : "<<Detectors.size()<<endl;
//		if(verbose){    // FIXME
//			cout<<"Detectors : {"<<endl;
//			for(auto&& adet : Detectors){
//				unsigned long tmp = adet->first;
//				cout<<"ChannelKey : "<<tmp.Print();
//				cout<<"Detector : "<<adet->second.Print();
//			}
//			cout<<"}"<<endl;
//		}
		cout<<"Version : "<<Version<<endl;
		cout<<"Status : "; PrintStatus(Status);
		cout<<"tank_centre : "; tank_centre.Print();
		cout<<"tank_radius : "<<tank_radius<<endl;
		cout<<"tank_halfheight : "<<tank_halfheight<<endl;
		cout<<"pmt_enclosed_radius : "<<pmt_enclosed_radius<<endl;
		cout<<"pmt_enclosed_halfheight : "<<pmt_enclosed_halfheight<<endl;
		cout<<"tank fiducial radius: "<<fiducialradius<<endl;
		cout<<"tank fiducial z cut: "<<fiducialcutz<<endl;
		cout<<"tank fiducial y cut: "<<fiducialcuty<<endl;
		cout<<"mrd_width : "<<mrd_width<<endl;
		cout<<"mrd_height : "<<mrd_height<<endl;
		cout<<"mrd_depth : "<<mrd_depth<<endl;
		cout<<"mrd_start : "<<mrd_start<<endl;
		cout<<"Number of tank PMTs : " << numtankpmts << endl;
		cout<<"Number of MRD PMTs : " << nummrdpmts << endl;
		cout<<"Number of veto PMTs : " << numvetopmts << endl;
		cout<<"Number of OD PMTs : "<< numodpmts << endl;
		cout<<"Number of LAPPDs : "<< numlappds << endl;
		
		return true;
	}
	bool PrintStatus(geostatus status){
		switch(status){
			case (geostatus::FULLY_OPERATIONAL): cout<<"FULLY OPERATIONAL"<<endl; break;
			case (geostatus::TANK_ONLY): cout<<"TANK ONLY"<<endl; break;
			case (geostatus::MRD_ONLY) : cout<<"MRD ONLY"<<endl; break;
		}
		return true;
	}
	
	void PrintChannels();
	
	// helper functions
	Position GlobalToTankCentered(Position posin){ return (posin - tank_centre); }
	void CartesianToPolar(Position posin, double& R, double& Phi, double& Theta, bool tankcentered=false);
	
	private:
	unsigned long NextFreeDetectorKey;
	unsigned long NextFreeChannelKey;
	std::map<int,int> DetectorKeys;
	std::map<unsigned long,Detector*> ChannelMap;
	std::map<std::string,std::map<unsigned long,Detector>> RealDetectors;
	std::map<std::string,std::map<unsigned long,Detector*>> Detectors;
	std::map<unsigned long, Paddle> Paddles;
	double Version;
	geostatus Status;
	Position tank_centre;
	double tank_radius;
	double tank_halfheight;
	double pmt_enclosed_radius;
	double pmt_enclosed_halfheight;
	double mrd_width;
	double mrd_height;
	double mrd_depth;
	double mrd_start;
	int numtankpmts;
	int nummrdpmts;
	int numvetopmts;
	int numlappds;
	int numodpmts;
	double fiducialradius;
	double fiducialcutz;
	double fiducialcuty;
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & RealDetectors;
			ar & Paddles;
			ar & Version;
			ar & Status;
			ar & tank_centre;
			ar & tank_radius;
			ar & tank_halfheight;
			ar & pmt_enclosed_radius;
			ar & pmt_enclosed_halfheight;
			ar & mrd_width;
			ar & mrd_height;
			ar & mrd_depth;
			ar & mrd_start;
			ar & fiducialradius;
			ar & fiducialcutz;
			ar & fiducialcuty;
		}
	}
};

#endif
