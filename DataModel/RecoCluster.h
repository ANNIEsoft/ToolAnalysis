/// This class is used to store the reconstructed hit clusters
/// Jingbo Wang <jiowang@ucdavis.edu>
#ifndef RECOCLUSTER_H
#define RECOCLUSTER_H
#include "RecoDigit.h"
#include "Position.h"
#include<SerialisableObject.h>
#include "TVector3.h"
#include "TMatrixDSym.h"
#include "TMatrixDSymEigen.h"
#include "TVectorD.h"
#include "TMath.h"
#include "TF1.h"
#include "Math/IFunction.h"

class RecoCluster : public SerialisableObject {

  friend class boost::serialization::access;
 	
  public:
  RecoCluster() ;
  ~RecoCluster();

  void Reset();
  void SortCluster();

  void AddDigit(RecoDigit digit);
  inline void SetDigits(vector<RecoDigit> indigits){fDigitList=indigits;}

  RecoDigit GetDigit(int n);
  
  void SetClusterMode(int cmode);
  
  int GetClusterMode();
  
  std::vector<RecoDigit> GetDigitList() {return fDigitList;}
  
  int GetNDigits();
  
  bool Print() {
		cout<<"Number of digits in this cluster : "<<GetNDigits()<<endl;
		cout<<"Clustering mode : "<<GetClusterMode()<<endl;
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
  void CalcAW();
  void CalcPlanaritySphericity();
  double CalcBeta(int order);
  void CalcTR();
  void CalcParameters();
  int calcBestParent();
  double GetAS(int mode);
  inline double GetASC(){return ASC;}
  inline double GetAMD(){return AMD;}
  inline Position GetSA(){return SpatialAverage;}
  inline double GetAW(){return AngularWidth;}
  inline double GetPlanarity(){return Planarity;}
  inline double GetSphericity(){return Sphericity;}
  double GetBeta(int order);
  inline double GetTimeRangeT() {return TRTotal;}
  inline double GetTimeRangeQ() {return TRQ;}
  inline double GetTimeRangeC() {return TRCluster;}
  inline double GetTRRTQ(){return TRRatioTQ;}
  inline double GetTRRTC() { return TRRatioTC; }
  inline double GetTRRQC() { return TRRatioQC; }

  void SetParticle(int pID,int pPDG, double eff, double pur);
  inline int GetBestParent(){return bestParticleID;}
  inline void SetPDG(int pPDG) {bestParticlePDG=pPDG;}
  inline int GetPDG(){return bestParticlePDG;}
  inline double Efficiency(){return efficiency;}
  inline double Purity(){return purity;}

  bool CheckFilter();
  inline bool GetFilterStatus(){return fIsFiltered;}
  //void CleanDigits();

  private:
	  double clusterTime;
	  double clusterCharge;
	  double clusterCB;
	  double AS0, AS1, ASC;
	  double AMD;
	  Position ChargeVector;
	  Position SpatialAverage;
	  double AngularWidth;
	  double Planarity=-999;
	  double Sphericity=-999;
	  std::map<int,double> BetaParameters;
	  double TRTotal, TRQ, TRCluster;
	  double TRRatioTQ, TRRatioTC,TRRatioQC;
	  int bestParticleID;
	  int bestParticlePDG;
	  double efficiency;
	  double purity;
	  bool fIsFiltered=0;

  
  int fClusterMode = -999;
  std::vector<RecoDigit> fDigitList;
  Position TwoDCenter;

  	
  protected:
  template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & fDigitList;
		}
	}

};

#endif







