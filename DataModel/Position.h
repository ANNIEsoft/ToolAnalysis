/* vim:set noexpandtab tabstop=4 wrap */
#ifndef POSITIONCLASS_H
#define POSITIONCLASS_H
// This class takes some implementation of some methods from the Physics Vector Package
// (based on TVector3) written by Pasha Murat and Peter Malzacher
// https://root.cern.ch

#include<SerialisableObject.h>
#include <iostream>
#include <sstream>
#include <cmath>

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
	
	inline double GetPhi(){
		// Angle from beam axis, measured clockwise while looking down [rads]
		Position thisunit = Unit();
		// if z=0, the vector is in the X-Y plane, and phi is 0
		double Phi = (abs(thisunit.Z())<0.0000001) ? 0. : atan(x/abs(z));
		// z<0 corresponds to the upstream wall, so add Pi as necessary
		if(z<0.){ (x<0.) ? Phi=(-M_PI-Phi) : Phi=(M_PI-Phi); }
		return Phi;
	}
	inline double GetTheta(){
		// Angle measured relative to the x-z plane [rads]
		return (abs(x)<0.0000001&&abs(z)<0.0000001) ? 0 : atan(y/sqrt(pow(x,2.)+pow(z,2.)));
	}
	inline double GetR(){ return sqrt(pow(x,2.)+pow(z,2.));}
	
	void UnitToCentimeter() { x = x*100.;
							  y = y*100.;
							  z = z*100.;
							}
	void UnitToMeter(){ x = x/100.;
					    y = y/100.;
					    z = z/100.;
					  }
	inline Position Unit(){
		double thismag = Mag();
		Position unitvec(x/thismag,y/thismag,z/thismag);
		return unitvec;
	}
	
	inline double M() const { return Mag(); }
	inline double M2() const { return Mag2(); }
	
	bool Print(bool withendline) const {
		std::cout<<"("<<x<<", "<<y<<", "<<z<<")";
		if(withendline) cout<<std::endl;
		return true;
	}
	
	bool Print(){
		return Print(true);
	}
	
	std::string AsString(){
		std::stringstream buf;
		buf <<"("<<x<<", "<<y<<", "<<z<<")";
		return buf.str();
	}
	
	bool operator==(const Position &a) const {
		return ((x==a.X()) && (y==a.Y()) && (z==a.Z()));
	}
	
	bool operator!=(const Position &a) const {
		return ((x!=a.X()) || (y!=a.Y()) || (z!=a.Z()));
	}
	
	
	template<typename T>
	Position operator *= (T a){
		x *= a;
		y *= a;
		z *= a;
		return *this;
	}
	
	inline Position operator - () const {
		return Position(-x, -y, -z);
	}
	
	inline Position& operator-= (const Position & b){
		//(*this) = (*this) - b;
		x -= b.x;
		y -= b.y;
		z -= b.z;
		return *this;
	}
	
	inline Position& operator+= (const Position & b){
		//(*this) = (*this) + b;
		x += b.x;
		y += b.y;
		z += b.z;
		return *this;
	}
	
	inline double Dot(const Position & p) const {
		return x*p.x + y*p.y + z*p.z;
	}
	
	inline Position Cross(const Position & p) const {
		return Position(y*p.z-p.y*z, z*p.x-p.z*x, x*p.y-p.x*y);
	}
	
	inline double Mag2() const { return x*x + y*y + z*z; }
	
	inline double Mag() const { return sqrt(Mag2()); }
	
	double Angle(const Position & q) const {
		double ptot2 = Mag2()*q.Mag2();
		if(ptot2 <= 0) {
			return 0.0;
		} else {
			double arg = Dot(q)/sqrt(ptot2);
			if(arg > 1.0) arg = 1.0;
			if(arg < -1.0) arg = -1.0;
			return cos(arg);
		}
	}
	
	inline Position Orthogonal() const {
		double xx = x < 0.0 ? -x : x;
		double yy = y < 0.0 ? -y : y;
		double zz = z < 0.0 ? -z : z;
		if (xx < yy) {
			return xx < zz ? Position(0,z,-y) : Position(y,-x,0);
		} else {
			return yy < zz ? Position(-z,0,x) : Position(y,-x,0);
		}
	}
	
	inline double Perp2() const { return x*x + y*y; }
	
	inline double Perp2(const Position & p)  const {
		double tot = p.Mag2();
		double ss  = Dot(p);
		double per = Mag2();
		if (tot > 0.0) per -= ss*ss/tot;
		if (per < 0) per = 0;
		return per;
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

Position operator + (const Position & a, const Position & b);
Position operator - (const Position & a, const Position & b);
Position operator * (const Position & p, double a);
Position operator * (double a, const Position & p);
double operator * (const Position & a, const Position & b);

class FourVector : public SerialisableObject{
	
	friend class boost::serialization::access;
	
	public:
	FourVector() : t(0), x(0), y(0), z(0){serialise=true;}
	FourVector(double tin, double xin, double yin, double zin) : t(tin), x(xin), y(yin), z(zin){serialise=true;}
	
	inline double T() const {return t;}
	inline double E() const {return t;}
	inline double X() const {return x;}
	inline double Y() const {return y;}
	inline double Z() const {return z;}
	inline void SetT(double tt){t=tt;}
	inline void SetE(double ee){t=ee;}
	inline void SetX(double xx){x=xx;}
	inline void SetY(double yy){y=yy;}
	inline void SetZ(double zz){z=zz;}
	void UnitToCentimeter(){
		x = x*100.;
		y = y*100.;
		z = z*100.;
	}
	
	Position Vect(){ return Position(x,y,z); }
        inline Position Unit(){
                double thismag = Mag(); 
                Position unitvec(x/thismag,y/thismag,z/thismag);
                return unitvec;
        }
        inline double Mag() const {
                return sqrt(Mag2());
        }
        inline double M() const { return Mag(); }
        inline double Mag2() const{
                return (pow(x,2.)+pow(y,2.)+pow(z,2.));
        }
        inline double M2() const { return Mag2(); }
	
        bool Print(bool withendline) {
                std::cout<<"("<<t<<", "<<x<<", "<<y<<", "<<z<<")";
                if(withendline) cout<<std::endl;
                return true;
        }
	bool Print(){
		return Print(true);
	}
	
	bool operator==(const FourVector &a) const {
		return ((t==a.T()) && (x==a.X()) && (y==a.Y()) && (z==a.Z()));
	}
	
	bool operator!=(const FourVector &a) const {
		return ((t!=a.T()) || (x!=a.X()) || (y!=a.Y()) || (z!=a.Z()));
	}
	
	private:
	double t;
	double x;
	double y;
	double z;
	
	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & t;
			ar & x;
			ar & y;
			ar & z;
		}
	}
};

#endif
