/*************************************************************************
    > File Name: MinuitOptimizer.hh
    > Author: Jingbo Wang
    > mail: jiowang@ucdavis.edu, wjingbo@anl.gov
    > Created Time: APR 12 15:30:13 2017
 ************************************************************************/

#ifndef MINUITOPTIMIZER_H
#define MINUITOPTIMIZER_H

#include "TObject.h"
#include "TMinuit.h"
#include "TFile.h"
#include "TH1.h"
#include "TH1D.h"
#include <sstream>

#include "RecoVertex.h"
#include "VertexGeometry.h"

#include <vector>
#include <iostream>
#include <iomanip>

class MinuitOptimizer {

public:
  double fVtxX;
  double fVtxY;
  double fVtxZ;
  double fVtxTime;
  double fDirX;
  double fDirY;
  double fDirZ;
  double fVtxFOM;
  double fConeAngle;
  double fConeFOM;
  double fBaseFOM;
  double fFoundVertex;
  
  int fPrintLevel;
  int fPass = 0;
  int fItr = 0;
  
  int fTimeFitItr;
  int fConeFitItr;
  int fPointPosItr;
  int fPointDirItr;
  int fPointVtxItr;
  int fExtendedVtxItr;
  int fCorrectedVtxItr;
  
  //ConeFit parameters
  double fSconeA;
  double fSconeB;
  double fSmu;
  double fSel;
  bool fIntegralsDone;
  
  //coneparameters
  double fConeParam0;
  double fConeParam1;
  double fConeParam2;
  
  //Fit range
  double fXmin, fXmax, fYmin, fYmax, fZmin, fZmax;
  double fTmin, fTmax;
  
  RecoVertex* fSeedVtx;
  RecoVertex* fFittedVtx;
  VertexGeometry* fVtxGeo;
  
  TMinuit* fMinuitPointPosition;
  TMinuit* fMinuitPointDirection;
  TMinuit* fMinuitPointVertex; 
  TMinuit* fMinuitExtendedVertex; 
  TMinuit* fMinuitCorrectedVertex;

  TMinuit* fMinuitTimeFit;
  TMinuit* fMinuitConeFit;
  
 	
 	MinuitOptimizer();
  ~MinuitOptimizer();
  void SetPrintLevel(int printlevel) {fPrintLevel = printlevel;}
  void SetMeanTimeCalculatorType(int type) {fMeanTimeCalculatorType = type;}
  void LoadVertexGeometry(VertexGeometry* vtxgeo);
  void LoadVertex(RecoVertex* vtx);
  void LoadVertex(double vtxX, double vtxY, double vtxZ, double vtxTime, double vtxDirX, double vtxDirY, double vtxDirZ);
  void FitPointTimePropertiesLnL(double& vtxTime, double& vtxFOM);
  void FitExtendedTimePropertiesLnL(double& vtxTime, double& vtxFOM);
  void FitConePropertiesFoM(double& coneAngle, double& coneFOM);
  void FitPointConePropertiesLnL(double& coneAngle, double& coneFOM);
  void FitExtendedConePropertiesLnL(double& coneAngle, double& coneFOM);
  void FitPointTimeWithMinuit();
  void FitPointPositionWithMinuit();
  void FitPointDirectionWithMinuit();
  void FitPointVertexWithMinuit();
  void FitExtendedVertexWithMinuit();
  void FitCorrectedVertexWithMinuit();
  
  double FindSimpleTimeProperties(VertexGeometry* vtxgeo);
  void TimePropertiesLnL(double vtxTime, double vtxParam, double& vtxFom);
  void ConePropertiesFoM(double& chi2);
  void ConePropertiesLnL(double coneParam0, double coneParam1, double coneParam2, double& coneAngle, double& coneFOM);
  void PointPositionChi2(double vtxX, double vtxY, double vtxZ, double& vtxTime, double& fom);
  void PointDirectionChi2(double vtxX, double vtxY, double vtxZ, double dirX, double dirY, double dirZ, double& vtxAngle, double& fom);
  void PointVertexChi2(double vtxX, double vtxY, double vtxZ,
	                                    double dirX, double dirY, double dirZ, 
	                                    double& vtxAngle, double& vtxTime, double& fom);
	void ExtendedVertexChi2(double vtxX, double vtxY, double vtxZ, 
	                                    double dirX, double dirY, double dirZ, 
	                                    double& vtxAngle, double& vtxTime, double& fom);
  void CorrectedVertexChi2(double vtxX, double vtxY, double vtxZ, 
	                                    double dirX, double dirY, double dirZ, 
	                                    double& vtxAngle, double& vtxTime, double& fom);
  
  double GetTime() {return fVtxTime;}
  double GetFOM() {return fVtxFOM;}
  RecoVertex* GetFittedVertex() {return fFittedVtx;}
  
  void time_fit_itr()        { fTimeFitItr++; }
  void cone_fit_itr()        { fConeFitItr++; }
  void point_position_itr()  { fPointPosItr++; }
  void point_direction_itr() { fPointDirItr++; }
  void point_vertex_itr()    { fPointVtxItr++; }
  void extended_vertex_itr() { fExtendedVtxItr++; }
  void corrected_vertex_itr() { fCorrectedVtxItr++; }

  void time_fit_reset_itr()        { fTimeFitItr = 0; }
  void cone_fit_reset_itr()        { fConeFitItr = 0; }
  void point_position_reset_itr()  { fPointPosItr = 0; }
  void point_direction_reset_itr() { fPointDirItr = 0; }
  void point_vertex_reset_itr()    { fPointVtxItr = 0; }
  void extended_vertex_reset_itr() { fExtendedVtxItr = 0; }
  void corrected_vertex_reset_itr() { fCorrectedVtxItr = 0; }

  int time_fit_iterations()        { return fTimeFitItr; }
  int cone_fit_iterations()        { return fConeFitItr; }
  int point_position_iterations()  { return fPointPosItr; }
  int point_direction_iterations() { return fPointDirItr; }
  int point_vertex_iterations()    { return fPointVtxItr; }
  int extended_vertex_iterations() { return fExtendedVtxItr; }
  int corrected_vertex_iterations() { return fCorrectedVtxItr; }
  
  
public: 
  //fitting parameters
  // --- fitting ---
  bool fFitTimeParams;
  double fFixTimeParam0;

  bool fFitConeParams;
  double fFixConeParam0;
  double fFixConeParam1;
  double fFixConeParam2;
  
  double fTimeFitWeight;
  double fConeFitWeight;
  
  int fMeanTimeCalculatorType;

};

#endif







