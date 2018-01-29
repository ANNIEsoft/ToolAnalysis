/* vim:set noexpandtab tabstop=4 wrap */
#ifndef DIRECTIONCLASS_H
#define DIRECTIONCLASS_H

#include<SerialisableObject.h>
#include <math.h> // defines M_PI
#include <cmath>  // atan2

using namespace std;

class Direction : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
  Direction() : x(0), y(0), z(0), phi(0), theta(0){serialise=true;}
	Direction(double x, double y, double z){
	  serialise=true;
	  double mag=sqrt(pow(x,2.)+pow(y,2.)+pow(z,2.));
	  if(mag==1.){ x=x; y=y; z=z; }
	  else { x=x/mag; y=y/mag; z=z/mag; }
	  theta = (x == 0.0 && y == 0.0 && z == 0.0) ? 0.0 : atan2(sqrt(x*x + y*y),z);
	  phi = (x == 0.0 && y == 0.0) ? 0. : atan2(y, x);
	};
	
	Direction(double phiin, double thetain){
	  serialise=true;	
	  theta=thetain;
	  phi=phiin;
	  x=cos(theta)*cos(phi);
	  y=cos(theta)*sin(phi);
	  z=sin(theta);
	};
  
	inline double X(){return x;}
	inline double Y(){return y;}
	inline double Z(){return z;}
	inline double Phi(){return phi;}
	inline double PhiDeg(){return phi*(180./M_PI);}
	inline double Theta(){return theta;}
	inline double ThetaDeg(){return theta*(180./M_PI);}
	
	inline void SetX(double xx){x=xx;}
	inline void SetY(double yy){y=yy;}
	inline void SetZ(double zz){z=zz;}
	inline void SetPhi(double ph){phi=ph;}
	inline void SetPhiDeg(double phd){phi=phd*(M_PI/180.);}
	inline void SetTheta(double th){theta=th;}
	inline void SetThetaDeg(double thd){theta=thd*(M_PI/180.);}
	
	bool Print(){
		cout<<"x : "<<x<<" meters"<<endl;
		cout<<"y : "<<y<<" meters"<<endl;
		cout<<"z : "<<z<<" meters"<<endl;
		cout<<"theta : "<<theta<<" rads"<<endl;
		cout<<"phi : "<<phi<<" rads"<<endl;
		
		return true;
	}
	
	private:
	double x;      // meters
	double y;
	double z;
	double theta;  // relative to beam direction (z), rads
	double phi;    // relative to x axis, rads
	
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
