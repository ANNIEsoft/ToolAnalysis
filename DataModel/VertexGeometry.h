
#ifndef VERTEXGEOMETRY_H
#define VERTEXGEOMETRY_H

#include "RecoVertex.h"
#include "RecoDigit.h"
#include "WaterModel.h"
#include "ANNIEGeometry.h"
#include "Parameters.h"
#include "TMath.h"

#include <vector>

class VertexGeometry {

 public:
 	
  static VertexGeometry* Instance();

  void LoadDigits(std::vector<RecoDigit>* vDigitList);

  void CalcResiduals(std::vector<RecoDigit>* vDigitList, RecoVertex* vtx);
  void CalcResiduals(RecoVertex* vtx);
  void CalcResiduals(double vx, double vy, double vz, double vtxTime,
		      double px, double py, double pz);

  void CalcPointResiduals(double vx, double vy, double vz, double vtxTime,
		           double px, double py, double pz);
  void CalcExtendedResiduals(double vx, double vy, double vz, double vtxTime,
		              double px, double py, double pz);

  void CalcVertexSeeds(int NSeeds = 1);

  RecoVertex* CalcSimpleVertex(std::vector<RecoDigit>* vDigitList);

  int GetNDigits() { return fNDigits; }
  int GetNFilterDigits() { return fNFilterDigits; }

  bool IsFiltered(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fIsFiltered[idigit];
    }
    else return 0;
  }
  
  int GetDigitType(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fDigitType[idigit];
    }
    else return -999;	
  }

  double GetDigitX(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fDigitX[idigit];
    }
    else return 0.0;
  }

  double GetDigitY(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fDigitY[idigit];
    }
    else return 0.0;
  }

  double GetDigitZ(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fDigitZ[idigit];
    }
    else return 0.0;
  }

  double GetDigitT(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fDigitT[idigit];
    }
    else return 0.0;
  }

  double GetDigitQ(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fDigitQ[idigit];
    }
    else return 0.0;
  }

  double GetConeAngle(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fConeAngle[idigit];
    }
    else return 0.0;
  }

  double GetAngle(int idigit) {
    return GetZenith(idigit);
  }

  double GetZenith(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fZenith[idigit];
    }
    else return 0.0;
  }

  double GetAzimuth(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fAzimuth[idigit];
    }
    else return 0.0;
  }

  double GetSolidAngle(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fSolidAngle[idigit];
    }
    else return 0.0;
  }

  double GetDistPoint(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fDistPoint[idigit];
    }
    else return 0.0;
  }

  double GetDistTrack(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fDistTrack[idigit];
    }
    else return 0.0;
  }

  double GetDistPhoton(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fDistPhoton[idigit];
    }
    else return 0.0;
  }

  double GetDistScatter(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fDistScatter[idigit];
    }
    else return 0.0;
  }

  double GetDeltaTime(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fDeltaTime[idigit];
    }
    else return 0.0;
  }

  double GetDeltaSigma(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fDeltaSigma[idigit];
    }
    else return 0.0;
  }

  double GetDeltaAngle(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fDeltaAngle[idigit];
    }
    else return 0.0;
  }

  double GetDeltaPoint(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fDeltaPoint[idigit];
    }
    else return 0.0;
  }

  double GetDeltaTrack(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fDeltaTrack[idigit];
    }
    else return 0.0;
  }

  double GetDeltaPhoton(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fDeltaPhoton[idigit];
    }
    else return 0.0;
  }

  double GetDeltaScatter(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fDeltaScatter[idigit];
    }
    else return 0.0;
  }

  double GetPointPath(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fPointPath[idigit];
    }
    else return 0.0;
  }

  double GetExtendedPath(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fExtendedPath[idigit];
    }
    else return 0.0;
  }

  double GetPointResidual(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fPointResidual[idigit];
    }
    else return 0.0;
  }

  double GetExtendedResidual(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fExtendedResidual[idigit];
    }
    else return 0.0;
  }

  double GetDelta(int idigit) {
    if( idigit>=0 && idigit<fNDigits ) {
      return fDelta[idigit];
    }
    else return 0.0;
  }

  double GetDeltaCorrection(int idigit, double length);

  int GetNSeeds() {
    return vSeedVtxX.size();
  }

  double GetSeedVtxX(int ivtx) {
    if (/*(ivtx >= 0) &&*/ (ivtx < GetNSeeds())) {
      return vSeedVtxX.at(ivtx);
    }
    else return 0.0;
  }

  double GetSeedVtxY(int ivtx) {
    if( /*(ivtx>=0) &&*/ (ivtx<GetNSeeds()) ) {
      return vSeedVtxY.at(ivtx);
    }
    else return 0.0;
  }

  double GetSeedVtxZ(int ivtx) {
    if( /*(ivtx>=0) &&*/ (ivtx<GetNSeeds()) ) {
      return vSeedVtxZ.at(ivtx);
    }
    else return 0.0;
  }

  double GetSeedVtxTime(int ivtx) {
    if( /*(ivtx>=0) &&*/ (ivtx<GetNSeeds()) ) {
      return vSeedVtxTime.at(ivtx);
    }
    else return 0.0;
  }
  
  bool Print() {return true;};

  private:
 	void Clear();
  VertexGeometry();
  ~VertexGeometry();

  void CalcSimpleVertex(double& vtxX, double& vtxY, double& vtxZ, double& vtxTime);

  void ChooseNextQuadruple(double& x0, double& y0, double& z0, double& t0,
                           double& x1, double& y1, double& z1, double& t1,
                           double& x2, double& y2, double& z2, double& t2,
                           double& x3, double& y3, double& z3, double& t3);

  void ChooseNextDigit(double& x, double& y, double& z, double& t);

  int fNDigitsMax;
  int fNDigits;
  int fNFilterDigits;

  int fThisDigit;
  int fLastEntry;
  int fCounter;

  double fVtxX1;
  double fVtxY1;
  double fVtxZ1;
  double fVtxTime1;

  double fVtxX2;
  double fVtxY2;
  double fVtxZ2;
  double fVtxTime2;

  double fMeanQ;
  double fTotalQ;

  double fMeanFilteredQ;
  double fTotalFilteredQ;

  double fMinTime;
  double fMaxTime;

  double fQmin;

  bool* fIsFiltered;
  
  int *fDigitType;   // Detector type (PMT or LAPPD)

  double* fDigitX;           // Digit X (cm)
  double* fDigitY;           // Digit Y (cm)
  double* fDigitZ;           // Digit Z (cm)
  double* fDigitT;           // Digit T (ns)
  double* fDigitQ;           // Digit Q (PEs)
  int* fDigitPE;

  double* fZenith;           // Zenith (degrees)
  double* fAzimuth;          // Azimuth (degrees)
  double* fSolidAngle;       // SolidAngle = sin(angle)/sin(42)
  double* fConeAngle;        // Cone Angle (degrees)

  double* fDistPoint;        // Distance from Vertex (S)
  double* fDistTrack;        // Distance along Track (S1)
  double* fDistPhoton;       // Distance along Photon (S2)
  double* fDistScatter;      // Distance of Scattering

  double* fDeltaTime;        // DeltaT = T - VtxT
  double* fDeltaSigma;       // SigmaT(Q)

  double* fDeltaAngle;       // DeltaAngle  = ConeAngle - Angle
  double* fDeltaPoint;       // DeltaPoint  = DistPoint/(fC/fN)
  double* fDeltaTrack;       // DeltaTrack  = DistTrack/(fC)
  double* fDeltaPhoton;      // DeltaPhoton = DistPhoton/(fC/fN)
  double* fDeltaScatter;     // DeltaScatter = DeltaScatter/(fC/fN)

  double* fPointPath;        // PointPath = fN*DistPoint
  double* fExtendedPath;     // ExtendedPath = DistTrack + fN*DistPhoton

  double* fPointResidual;    // PointResidual = DeltaTime - PointPath
  double* fExtendedResidual; // ExtendedResidual = DeltaTime - ExtendedPath
  double* fDelta;            // Chosen Residual [Point/Extended/Null]
  double fPointResidualMean;
  double fExtendedResidualMean;


  std::vector<double> vSeedVtxX;
  std::vector<double> vSeedVtxY;
  std::vector<double> vSeedVtxZ;
  std::vector<double> vSeedVtxTime;
  std::vector<int> vSeedDigitList;

  std::vector<RecoVertex*> vVertexList;
  std::vector<RecoDigit>* fDigitList;                      // reco digit list

};

#endif
