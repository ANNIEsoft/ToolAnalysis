/// This class stores the calibrated hits used for reconstruction
/// Jingbo Wang <jiowang@ucdavis.edu>
#ifndef RECODIGITCLASS_H
#define RECODIGITCLASS_H

#include<SerialisableObject.h>
#include "TimeClass.h"
#include "ANNIERecoObjectTable.h"

using namespace std;

class RecoDigit : public SerialisableObject{

	friend class boost::serialization::access;

	public:
	
	typedef enum EDigitType {
    PMT8inch  = 0, //0
    lappd_v0    = 1 //1
  } DigitType;
		
	RecoDigit() : fRegion(0), fPosition(0, 0, 0), fCalTime(0), fCalCharge(0), fIsFiltered(1), fDigitType(0) {serialise=true;}
	RecoDigit(int region, Position position, TimeClass calT, double calQ, int digitType) {
			serialise=true;
			fRegion = region;
			fCalTime = calT;
			fCalCharge = calQ;
			fIsFiltered = 1;
			fDigitType = digitType;
			ANNIERecoObjectTable::Instance()->NewDigit();
		}
	~RecoDigit() {ANNIERecoObjectTable::Instance()->DeleteDigit();}

	inline int                 GetRegion() const {return fRegion;}
	inline Position						 GetPosition() const {return fPosition;}
	inline TimeClass           GetCalTime() const {return fCalTime;}
	inline double              GetCalCharge() const {return fCalCharge;}
	inline bool                GetFilterStatus() const {return fIsFiltered;}
	inline int         				 GetDigitType() const {return fDigitType;}
		
	inline void                SetRegion(int reg) {fRegion = reg;}
	inline void                SetPosition(Position pos){fPosition = pos;}
	inline void                SetCalTime(TimeClass calt) {fCalTime = calt;}
	inline void                SetCalCharge(double calq) {fCalCharge = calq;}
	inline void                SetDigitType(int type) {fDigitType = type;}
	inline void                SetFilter(bool pass = 1){ fIsFiltered = pass;}
  inline void                ResetFilter() {SetFilter(0);}
  inline void                PassFilter()  {SetFilter(1);}

	bool Print() {
		cout<<"Region : "<<fRegion<<endl;
		cout<<"X Pos : "<<fPosition.X()<<endl;
		cout<<"Y Pos : "<<fPosition.Y()<<endl;
		cout<<"Z Pos : "<<fPosition.Z()<<endl;
		cout<<"Calibrated Time : "; fCalTime.Print();
		cout<<"Calibrated Charge : "<<fCalCharge<<endl;
		cout<<"Is filtered ? "<<fIsFiltered<<endl;
		cout<<"Digit Type : "<<fDigitType<<endl;
		return true;
	}

	protected:
	int fRegion;
	Position fPosition;
  TimeClass fCalTime;
  double fCalCharge; 
  bool fIsFiltered;
  int fDigitType;

	template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & fRegion;
			ar & fPosition;
			ar & fCalTime;
			ar & fCalCharge;
			ar & fIsFiltered;
			ar & fDigitType;
		}
	}
	
};

#endif
