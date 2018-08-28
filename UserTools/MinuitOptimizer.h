/*************************************************************************
    > File Name: MinuitOptimizer.hh
    > Author: Jingbo Wang
    > mail: jiowang@ucdavis.edu, wjingbo@anl.gov
    > Created Time: APR 12 15:30:13 2017
 ************************************************************************/

#ifndef MINUITOPTIMIZER_HH
#define MINUITOPTIMIZER_HH

#include "TObject.h"
#include "TMinuit.h"
#include "TFile.h"
#include <sstream>

#include "RecoVertex.hh"
#include "VertexGeometry.hh"
#include "VertexFinder.hh"

#include <vector>
#include <iostream>
#include <iomanip>

class MinuitOptimizer : public TObject {

public:
  Double_t fVtxX;
  Double_t fVtxY;
  Double_t fVtxZ;
  Double_t fVtxTime;
  Double_t fDirX;
  Double_t fDirY;
  Double_t fDirZ;
  Double_t fVtxFOM;
  Double_t fConeAngle;
  Double_t fConeFOM;
  Double_t fBaseFOM;
  Double_t fFoundVertex;
  
  Int_t fPrintLevel;
  Int_t fPass = 0;
  Int_t fItr = 0;
  
  Int_t fTimeFitItr;
  Int_t fConeFitItr;
  Int_t fPointPosItr;
  Int_t fPointDirItr;
  Int_t fPointVtxItr;
  Int_t fExtendedVtxItr;
  Int_t fCorrectedVtxItr;
  
  //ConeFit parameters
  Double_t fSconeA;
  Double_t fSconeB;
  Double_t fSmu;
  Double_t fSel;
  Bool_t fIntegralsDone;
  
  //coneparameters
  Double_t fConeParam0;
  Double_t fConeParam1;
  Double_t fConeParam2;
  
  //Fit range
  Double_t fXmin, fXmax, fYmin, fYmax, fZmin, fZmax;
  Double_t fTmin, fTmax;
  
  RecoVertex* fSeedVtx;
  RecoVertex* fFittedVtx;
  VertexGeometryOpt* fVtxGeo;
  VertexFinderOpt* fVtxFinder;
  
  TMinuit* fMinuitPointPosition;
  TMinuit* fMinuitPointDirection;
  TMinuit* fMinuitPointVertex; 
  TMinuit* fMinuitExtendedVertex; 
  TMinuit* fMinuitCorrectedVertex;

  TMinuit* fMinuitTimeFit;
  TMinuit* fMinuitConeFit;
  
 	
 	MinuitOptimizer();
  ~MinuitOptimizer();
  void SetPrintLevel(Int_t printlevel) {fPrintLevel = printlevel;}
  void LoadVertexFinder(VertexFinderOpt* vtxfinder);
  void LoadVertexGeometry(VertexGeometryOpt* vtxgeo);
  void LoadVertex(RecoVertex* vtx);
  void LoadVertex(Double_t vtxX, Double_t vtxY, Double_t vtxZ, Double_t vtxTime, Double_t vtxDirX, Double_t vtxDirY, Double_t vtxDirZ);
  void FitPointTimePropertiesLnL(Double_t& vtxTime, Double_t& vtxFOM);
  void FitExtendedTimePropertiesLnL(Double_t& vtxTime, Double_t& vtxFOM);
  void FitConePropertiesFoM(Double_t& coneAngle, Double_t& coneFOM, std::string fitOption);
  void FitPointConePropertiesLnL(Double_t& coneAngle, Double_t& coneFOM);
  void FitExtendedConePropertiesLnL(Double_t& coneAngle, Double_t& coneFOM);
  void FitPointTimeWithMinuit();
  void FitPointPositionWithMinuit();
  void FitPointDirectionWithMinuit();
  void FitPointVertexWithMinuit();
  void FitExtendedVertexWithMinuit();
  void FitCorrectedVertexWithMinuit();
  
  Double_t FindSimpleTimeProperties(VertexGeometryOpt* vtxgeo);
  void TimePropertiesLnL(Double_t vtxTime, Double_t vtxParam, Double_t& vtxFom);
  void ConePropertiesFoM(Double_t& chi2, std::string fitOption);
  void ConePropertiesLnL(Double_t coneParam0, Double_t coneParam1, Double_t coneParam2, Double_t& coneAngle, Double_t& coneFOM);
  void PointPositionChi2(Double_t vtxX, Double_t vtxY, Double_t vtxZ, Double_t& vtxTime, Double_t& fom);
  void PointDirectionChi2(Double_t vtxX, Double_t vtxY, Double_t vtxZ, Double_t dirX, Double_t dirY, Double_t dirZ, Double_t& vtxAngle, Double_t& fom);
  void PointVertexChi2(Double_t vtxX, Double_t vtxY, Double_t vtxZ, 
	                                    Double_t dirX, Double_t dirY, Double_t dirZ, 
	                                    Double_t& vtxAngle, Double_t& vtxTime, Double_t& fom);
	void ExtendedVertexChi2(Double_t vtxX, Double_t vtxY, Double_t vtxZ, 
	                                    Double_t dirX, Double_t dirY, Double_t dirZ, 
	                                    Double_t& vtxAngle, Double_t& vtxTime, Double_t& fom);
  void CorrectedVertexChi2(Double_t vtxX, Double_t vtxY, Double_t vtxZ, 
	                                    Double_t dirX, Double_t dirY, Double_t dirZ, 
	                                    Double_t& vtxAngle, Double_t& vtxTime, Double_t& fom);
  
  Double_t GetTime() {return fVtxTime;}
  Double_t GetFOM() {return fVtxFOM;}
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

  Int_t time_fit_iterations()        { return fTimeFitItr; }
  Int_t cone_fit_iterations()        { return fConeFitItr; }
  Int_t point_position_iterations()  { return fPointPosItr; }
  Int_t point_direction_iterations() { return fPointDirItr; }
  Int_t point_vertex_iterations()    { return fPointVtxItr; }
  Int_t extended_vertex_iterations() { return fExtendedVtxItr; }
  Int_t corrected_vertex_iterations() { return fCorrectedVtxItr; }
  
  
private: 

 
  
  ClassDef(MinuitOptimizer,0)

};

#endif







