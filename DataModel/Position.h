/* vim:set noexpandtab tabstop=4 wrap */
#ifndef POSITIONCLASS_H
#define POSITIONCLASS_H

#include<SerialisableObject.h>
#include <iostream>
#include <TRandom3.h>

using namespace std;

class Position : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
  Position() : x(0), y(0), z(0){serialise=true;}
  Position(double xin, double yin, double zin) : x(xin), y(yin), z(zin){serialise=true;}
	
	inline double X() const {return x;}
	inline double Y() const {return y;}
	inline double Z() const {return z;}
	inline void SetX(double xx){x=xx;}
	inline void SetY(double yy){y=yy;}
	inline void SetZ(double zz){z=zz;}
	void UnitToCentimeter() {
	  x = x*100;
	  y = y*100;
	  z = z*100;	
	}
	void GaussianSmear(double sigmax, double sigmay, double sigmaz) {
		x = r.Gaus(x, sigmax); // cm
  	y = r.Gaus(y, sigmay);
  	z = r.Gaus(z, sigmaz);		
	}
	
	bool Print() {
		std::cout<<"("<<x<<", "<<y<<", "<<z<<")"<<std::endl;
		return true;
	}
	
	private:
	double x;
	double y;
	double z;
	TRandom r;
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & x;
			ar & y;
			ar & z;
		}
	}
};

#endif
