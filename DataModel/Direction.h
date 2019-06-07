/* vim:set noexpandtab tabstop=4 wrap */
#ifndef DIRECTIONCLASS_H
#define DIRECTIONCLASS_H

#include<SerialisableObject.h>
#include <math.h> // defines M_PI
#include <cmath>  // atan2
#include <iostream>

using namespace std;

class Direction : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
	Direction() : x(0), y(0), z(0), phi(0), theta(0){serialise=true;}
	Direction(double xin, double yin, double zin){
		serialise=true;
		double mag=sqrt(pow(xin,2.)+pow(yin,2.)+pow(zin,2.));
		if(mag==1.){ x=xin; y=yin; z=zin; }
		else { x=xin/mag; y=yin/mag; z=zin/mag; }
		phi = (x == 0.0 && y == 0.0 && z == 0.0) ? 0.0 : atan2(sqrt(x*x + y*y),z);
		theta = (x == 0.0 && y == 0.0) ? 0. : atan2(y, x);
	};
	
	Direction(double phiin, double thetain){
		serialise=true;
		theta=thetain;
		phi=phiin;
		x=cos(phi)*cos(theta);
		y=cos(phi)*sin(theta);
		z=sin(phi);
	};
	  
	inline double X() const {return x;}
	inline double Y() const {return y;}
	inline double Z() const {return z;}
	inline double GetPhi() const {return phi;}
	inline double GetPhiDeg() const {return phi*(180./M_PI);}
	inline double GetTheta() const {return theta;}
	inline double GetThetaDeg() const {return theta*(180./M_PI);}
	
	inline void SetX(double xx){x=xx;}
	inline void SetY(double yy){y=yy;}
	inline void SetZ(double zz){z=zz;}
	inline void SetPhi(double ph){phi=ph;}
	inline void SetPhiDeg(double phd){phi=phd*(M_PI/180.);}
	inline void SetTheta(double th){theta=th;}
	inline void SetThetaDeg(double thd){theta=thd*(M_PI/180.);}
	
	bool Print() {
		std::cout<<"Unit Vector : ("<<x<<", "<<y<<", "<<z<<")"<<std::endl;
		std::cout<<"theta : "<<theta<<" rads"<<std::endl;
		std::cout<<"phi : "<<phi<<" rads"<<std::endl;
		
		return true;
	}
	
	private:
	double x;        // meters
	double y;
	double z;
	double phi;      // clockwise looking down, 0 pointing downstream along beam, rads
	double theta;    // pitch angle, relative to the x-z (beamline) plane, rads
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & x;
			ar & y;
			ar & z;
			ar & theta;
			ar & phi;
		}
	}
};

#endif
