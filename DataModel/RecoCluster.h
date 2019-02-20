/// This class is used to store the reconstructed hit clusters
/// Jingbo Wang <jiowang@ucdavis.edu>
#ifndef RECOCLUSTER_H
#define RECOCLUSTER_H
#include "RecoDigit.h"
#include<SerialisableObject.h>

class RecoCluster : public SerialisableObject {

  friend class boost::serialization::access;
 	
  public:
  RecoCluster() ;
  ~RecoCluster();

  void Reset();
  void SortCluster();

  void AddDigit(RecoDigit* digit);

  RecoDigit* GetDigit(int n);
  int GetNDigits();
  
  bool Print() {
		cout<<"Number of digits in this cluster : "<<GetNDigits()<<endl;
		return true;
	}

  private:

  std::vector<RecoDigit*> fDigitList;
  	
  protected:
  template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & fDigitList;
		}
	}

};

#endif







