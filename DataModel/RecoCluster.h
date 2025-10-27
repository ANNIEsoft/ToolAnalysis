/// This class is used to store the reconstructed hit clusters
/// Jingbo Wang <jiowang@ucdavis.edu>
#ifndef RECOCLUSTER_H
#define RECOCLUSTER_H
#include "RecoDigit.h"
#include "Position.h"
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
  
  void SetClusterMode(int cmode);
  
  int GetClusterMode();
  
  std::vector<RecoDigit*> GetDigitList() {return this->fDigitList;}
  
  int GetNDigits();
  
  bool Print() {
		cout<<"Number of digits in this cluster : "<<GetNDigits()<<endl;
		cout<<"Clustering mode : "<<GetNDigits()<<endl;
		return true;
	}

  inline double GetTime() { return clusterTime; }
  inline double GetCharge() { return clusterCharge; }

  void CalcTime();
  void CalcCharge();

  inline double GetCB() { return clusterCB; }
  inline Position GetCV() {return ChargeVector; }

  void CalcCB();
  void CalcAS();
  void CalcCV();
  void CalcAMD();
  void CalcSA();
  void CalcCA();
  void CalcParameters();
  int calcBestParent();
  double GetAS(int mode);
  inline double GetASC(){return ASC;}
  inline double GetAMD(){return AMD;}
  inline Position GetSA(){return SpatialAverage;}
  inline double GetCA(){return ContainingAngle;}
  std::vector<RecoDigit*> convexHull();

  void SetParticle(int pID,int pPDG, double eff, double pur);
  inline int GetBestParent(){return bestParticleID;}
  inline void SetPDG(int pPDG) {bestParticlePDG=pPDG;}
  inline int GetPDG(){return bestParticlePDG;}
  inline double Efficiency(){return efficiency;}
  inline double Purity(){return purity;}

  private:
	  double clusterTime;
	  double clusterCharge;
	  double clusterCB;
	  double AS0, AS1, ASC;
	  double AMD;
	  Position ChargeVector;
	  Position SpatialAverage;
	  double ContainingAngle;
	  int bestParticleID;
	  int bestParticlePDG;
	  double efficiency;
	  double purity;

  
  int fClusterMode = -999;
  std::vector<RecoDigit*> fDigitList;
  std::vector<RecoDigit*> hullDigits;
  Position TwoDCenter;

  	
  protected:
  template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & fDigitList;
		}
	}

};

#endif







