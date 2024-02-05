/*************************************************************************
    > File Name: FoMCalculator.hh
    > Author: Jingbo Wang, Teal Pershing 
    > mail: jiowang@ucdavis.edu, tjpershing@ucdavis.edu 
    > Created Time: MAY 07, 2019
 ************************************************************************/

#ifndef FOMCALCULATOR_H
#define FOMCALCULATOR_H

#include "TObject.h"
#include "TMinuit.h"
#include "TFile.h"
#include "TH1.h"
#include "TH1D.h"
#include <sstream>

#include "VertexGeometry.h"
#include "Parameters.h"
#include <vector>
#include <iostream>
#include <iomanip>

class FoMCalculator {

public:
  //fitting parameters used in ExtendedVertexChi2
  double fBaseFOM;
  double fTimeFitWeight;
  double fConeFitWeight;

  int fMeanTimeCalculatorType;

  //ConeFit parameters
  //double fSconeA;
  //double fSconeB;
  //double fSmu;
  //double fSel;
  //bool fIntegralsDone;
  
  VertexGeometry* fVtxGeo;
  
 	
  FoMCalculator();
  ~FoMCalculator();
  void SetTimeFitWeight(double tweight){ fTimeFitWeight=tweight;}
  void SetConeFitWeight(double cweight){ fConeFitWeight=cweight;}
  void SetMeanTimeCalculatorType(int type) {fMeanTimeCalculatorType = type;}
  void LoadVertexGeometry(VertexGeometry* vtxgeo);
  double FindSimpleTimeProperties(double myConeEdge);
  void TimePropertiesLnL(double vtxTime, double& vtxFom);
  void ConePropertiesFoM(double coneEdge, double& chi2);
  void ConePropertiesLnL(double vtxX, double vtxY, double VtxZ, double dirX, double dirY, double dirZ, double coneEdge, double& chi2, TH1D angularDist, double& phimax, double& phimin);
  void PointPositionChi2(double vtxX, double vtxY, double vtxZ, double vtxTime, double& fom);
  void PointDirectionChi2(double vtxX, double vtxY, double vtxZ, double dirX, double dirY, double dirZ, double coneAngle, double& fom);
  void PointVertexChi2(double vtxX, double vtxY, double vtxZ,
	                                    double dirX, double dirY, double dirZ, 
	                                    double coneAngle, double vtxTime, double& fom);
  void ExtendedVertexChi2(double vtxX, double vtxY, double vtxZ, 
	                                    double dirX, double dirY, double dirZ, 
	                                    double coneAngle, double vtxTime, double& fom);
  void ExtendedVertexChi2(double vtxX, double vtxY, double vtxZ,
	  double dirX, double dirY, double dirZ,
	  double coneAngle, double vtxTime, double& fom, TH1D pdf);
//  void ConePropertiesLnL(double coneParam0, double coneParam1, double coneParam2, double& coneAngle, double& coneFOM);
//  void CorrectedVertexChi2(double vtxX, double vtxY, double vtxZ, 
//	                                    double dirX, double dirY, double dirZ, 
//                                      double& vtxAngle, double& vtxTime, double& fom);
  
  
};

#endif







