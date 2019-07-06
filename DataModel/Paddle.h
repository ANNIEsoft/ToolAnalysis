/* vim:set noexpandtab tabstop=4 wrap */
#ifndef PADDLECLASS_H
#define PADDLECLASS_H

#include <iostream>

#include<SerialisableObject.h>
class Detector;

using namespace std;

class Paddle : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
	Paddle() : detectorkey(-1), x(-1), y(-1), z(-1), orientation(-1){serialise=true;};
	Paddle(unsigned long detkey, int xin, int yin, int zin, int orientationin, Position originin, std::pair<double,double> xextsin, std::pair<double,double> yextsin, std::pair<double,double> zextsin) :
	detectorkey(detkey), x(xin), y(yin), z(zin), orientation(orientationin), origin(originin), extents_x(xextsin), extents_y(yextsin), extents_z(zextsin){
		serialise=true;
		if((extents_x.first>extents_x.second) ||
		   (extents_y.first>extents_y.second) ||
		   (extents_z.first>extents_z.second) ){
			std::cerr<<"WARNING! Paddle extents should be passed {min,max}"<<std::endl;
		}
		
		// each MRD layer is split into two halves; top/bottom for vertical layers
		// and left/right for horizontal layers. 'half' will denote which half
		// 0=bottom/left, 1=top/right
		half = (orientation) ? (origin.Y()>0) : (origin.X()>0);
		
		// it's useful when coordinating with alternate view paddles to also know
		// which side of a half-layer this paddle is on (or if it straddles the origin)
		// this will be the paddle 'side'.
		std::pair<double,double> theextents = (orientation) ? extents_x : extents_y;
		if( (theextents.first>0) && (theextents.second>0)){
			side = 1;
		} else if( (theextents.first<0) && (theextents.second<0)){
			side = -1;
		} else {
			side = 0;
		}
		
	}
	
	unsigned long GetDetectorID(){ return detectorkey; }
	int GetPaddleX(){ return x; }
	int GetPaddleY(){ return y; }
	int GetPaddleZ(){ return z; }
	int GetLayer(){ return z; }
	Position GetOrigin(){ return origin; }
	int GetOrientation(){ return orientation; }
	int GetHalf(){ return half; }
	int GetSide(){ return side; }
	std::string GetOrientationString(){ return ((orientation) ? "Vertical" : "Horizontal"); }
	std::pair<double,double> GetExtentsX(){ return extents_x; }
	std::pair<double,double> GetExtentsY(){ return extents_y; }
	std::pair<double,double> GetExtentsZ(){ return extents_z; }
	double GetXmin(){ return extents_x.first; }
	double GetXmax(){ return extents_x.second; }
	double GetYmin(){ return extents_y.first; }
	double GetYmax(){ return extents_y.second; }
	double GetZmin(){ return extents_z.first; }
	double GetZmax(){ return extents_z.second; }
	double GetPaddleWidth(){
		if(orientation){
			return (extents_x.second-extents_x.first);
		} else {
			return (extents_y.second-extents_y.first);
		}
	}
	
	void SetDetectorID(unsigned long detkey){ detectorkey=detkey; }
	void SetX(int xin){ x=xin; }
	void SetY(int yin){ y=yin; }
	void SetZ(int zin){ z=zin; }
	void SetLayer(int layerin){ z = layerin; }
	void SetOrigin(Position posin){ origin=posin; }
	void SetOrigin(double x, double y, double z){ origin=Position(x,y,z); }
	void SetOrientation(int orientationin){ orientation=orientationin; }
	void SetSide(int sidein){ side=sidein; }
	void SetHalf(int halfin){ half=halfin; }
	bool SetExtentsX(std::pair<double,double> extsin){
		// for safety, check our extents have been ordered correctly
		if(extsin.first>extsin.second){
			std::cerr<<"WARNING! Paddle extents should be passed {min,max}"<<std::endl;
			// we could of course easily flip them, but it's probably safer to abort
			// as warnings are easy to miss and this may result in an unexpected geometry.
			return false;
		} else {
			extents_x = extsin;
		}
	}
	bool SetExtentsY(std::pair<double,double> extsin){
		if(extsin.first>extsin.second){
			std::cerr<<"WARNING! Paddle extents should be passed {min,max}"<<std::endl;
			return false;
		} else {
			extents_y = extsin;
		}
	}
	bool SetExtentsZ(std::pair<double,double> extsin){
		if(extsin.first>extsin.second){
			std::cerr<<"WARNING! Paddle extents should be passed {min,max}"<<std::endl;
			return false;
		} else {
			extents_z = extsin;
		}
	}
	bool SetExtentsX(double xmin, double xmax){
		if(xmin>xmax){
			std::cerr<<"WARNING! Paddle extents should be passed {min,max}"<<std::endl;
			return false;
		} else {
			extents_x=std::pair<double,double>{xmin,xmax};
		}
	};
	bool SetExtentsY(double ymin, double ymax){
		if(ymin>ymax){
			std::cerr<<"WARNING! Paddle extents should be passed {min,max}"<<std::endl;
			return false;
		} else {
			extents_y=std::pair<double,double>{ymin,ymax};
		}
	};
	bool SetExtentsZ(double zmin, double zmax){
		if(zmin>zmax){
			std::cerr<<"WARNING! Paddle extents should be passed {min,max}"<<std::endl;
			return false;
		} else {
			extents_z=std::pair<double,double>{zmin,zmax};
		}
	};
	
	bool Print(){
		std::cout<<"Paddle MRD_x"<<x<<"_y"<<y<<"_z"<<z
				 <<" is a "<<GetOrientationString()
				 <<" paddle in the ";
		if(orientation){
			if(half){ std::cout<<"top";   } else { std::cout<<"bottom"; }
			if(side){ std::cout<<"right"; } else { std::cout<<"left"; }
		} else {
			if(half){ std::cout<<"right"; } else { std::cout<<"left"; }
			if(side){ std::cout<<"top";   } else { std::cout<<"bottom"; }
		}
		std::cout<<" of layer "<<z;
		std::cout<<"; The paddle origin is ";
				 origin.Print(false);
		std::cout<<" and extents x={"<<extents_x.first<<" -> "<<extents_x.second<<" }"
				 <<", y={"<<extents_y.first<<" -> "<<extents_y.second<<" }"
				 <<", z={"<<extents_z.first<<" -> "<<extents_z.second<<" }";
		std::cout<<". It is associated with DetectorKey "<<detectorkey
				 <<std::endl;
		return true;
	}
	
	private:
	unsigned long detectorkey;
	int orientation;   // H is 0, V is 1
	int side; // -1, 0, 1. see constructor for definition.
	int half; // 0 or 1.
	// these are NUMBERS in the MRD numbering scheme, not positions (hence ints)
	int x;
	int y;
	int z;
	Position origin;
	std::pair<double,double> extents_x;
	std::pair<double,double> extents_y;
	std::pair<double,double> extents_z;
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & detectorkey;
			ar & orientation;
			ar & side;
			ar & half;
			ar & x;
			ar & y;
			ar & z;
			ar & origin;
			ar & extents_x;
			ar & extents_y;
			ar & extents_z;
		}
	}
	
};
#endif
