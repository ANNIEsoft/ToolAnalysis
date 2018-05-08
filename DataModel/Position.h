/* vim:set noexpandtab tabstop=4 wrap */
#ifndef POSITIONCLASS_H
#define POSITIONCLASS_H

#include<SerialisableObject.h>

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
	
	bool Print() {
		cout<<"("<<x<<", "<<y<<", "<<z<<")"<<endl;
		return true;
	}
	
	private:
	double x;
	double y;
	double z;
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & x;
			ar & y;
			ar & z;
		}
	}
};

#endif
