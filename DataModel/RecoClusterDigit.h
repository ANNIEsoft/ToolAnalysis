#ifndef RECOCLUSTERDIGIT_H
#define RECOCLUSTERDIGIT_H

#include<SerialisableObject.h>
#include "RecoDigit.h"

#include <vector>

using namespace std;

class RecoClusterDigit : public SerialisableObject {

 public:
  RecoClusterDigit( RecoDigit* recoDigit );
  ~RecoClusterDigit();

  void AddClusterDigit(RecoClusterDigit* clusterDigit);
  
  int GetNClusterDigits(); 
  RecoClusterDigit* GetClusterDigit(int idigit); 
  std::vector<RecoClusterDigit*>* GetClusterDigitList();

  double GetX(){ return fRecoDigit->GetPosition().X(); }
  double GetY(){ return fRecoDigit->GetPosition().Y(); }
  double GetZ(){ return fRecoDigit->GetPosition().Z(); }
  double GetTime(){ return fRecoDigit->GetCalTime(); }
  int GetDigitType() {return fRecoDigit->GetDigitType();}

  void SetClustered( bool yesno = 1 ){ fIsClustered = yesno; }
  bool IsClustered(){ return fIsClustered; }

  bool IsAllClustered();

  RecoDigit* GetRecoDigit(){ return fRecoDigit; }
  
  bool Print() {
  	std::string name = "RecoClusterDigit::Print():";
  	std::cout<<name<<"    "<<"x, y, z, t = "<<GetX()<<", "<<GetY()<<", "<<GetZ()<<", "<<GetTime()<<std::endl;
		return true;
	}

 private:

  bool fIsClustered;
  bool fIsAllClustered;

  RecoDigit* fRecoDigit;

  std::vector<RecoClusterDigit*>* fClusterDigitList;

  protected:
  template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & fIsClustered;
			ar & fIsAllClustered;
			ar & fRecoDigit;
		}
	}
};

#endif
