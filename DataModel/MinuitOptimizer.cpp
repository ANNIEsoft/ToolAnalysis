/*************************************************************************
    > File Name: MinuitOptimizer.cc
    > Author: Jingbo Wang
    > mail: jiowang@ucdavis.edu, wjingbo@anl.gov
    > Created Time: APR 12 15:36:00 2017
 ************************************************************************/

#include "MinuitOptimizer.h"
#include "ANNIEGeometry.h"
#include "Parameters.h"
#include "RecoDigit.h"
#include "TMath.h"

#include<algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <cassert>
using namespace std;

static MinuitOptimizer* fgMinuitOptimizer = 0;
static void vertex_time_lnl(int&, double*, double& f, double* par, int)
{  

  bool printDebugMessages = 0;
  
  double vtxTime = par[0]; // nanoseconds
//  double vtxParam0 = fgMinuitOptimizer->fFixTimeParam0;
  double vtxParam0 = fgMinuitOptimizer->fFixTimeParam0; //scatterring parameter
  if( fgMinuitOptimizer->fFitTimeParams ){
    vtxParam0 = par[1]; 
  }
  double fom = 0.0;
  fgMinuitOptimizer->time_fit_itr();  
  fgMinuitOptimizer->TimePropertiesLnL(vtxTime,vtxParam0, fom);
  f = -fom; // note: need to maximize this fom
  if( printDebugMessages ){
    std::cout << "  [vertex_time_lnl] [" << fgMinuitOptimizer->time_fit_iterations() << "] vtime=" << vtxTime << " vparam=" << vtxParam0 << " fom=" << fom << std::endl;
  }
  return;
}

static void point_position_chi2(int&, double*, double& f, double* par, int)
{
  bool printDebugMessages = 0;

  double vtxX = par[0]; // centimetres
  double vtxY = par[1]; // centimetres
  double vtxZ = par[2]; // centimetres
  double vtxTime = par[3]; //ns, added by JW
  double vtxParam0 = 0.2; //scatterring parameter

//  double vtime  = 0.0;
  double fom = 0.0;
  fgMinuitOptimizer->point_position_itr();
  fgMinuitOptimizer->PointPositionChi2(vtxX,vtxY,vtxZ, vtxTime,fom);
//  fgMinuitOptimizer->fVtxGeo->CalcPointResiduals(vtxX, vtxY, vtxZ, 0.0, 0.0, 0.0, 0.0); //load expected vertex time for each digit
//  fgMinuitOptimizer->TimePropertiesLnL(vtxTime,vtxParam0, fom);


  f = -fom; // note: need to maximize this fom

  if( printDebugMessages ){
    std::cout << " [point_position_chi2] [" << fgMinuitOptimizer->point_position_iterations() << "] (x,y,z)=(" << vtxX << "," << vtxY << "," << vtxZ << ") vtime=" << vtxTime << " fom=" << fom << std::endl;
  }

  return;
}

static void point_direction_chi2(int&, double*, double& f, double* par, int)
{
  bool printDebugMessages = 0;
  
  double vtxX = fgMinuitOptimizer->fVtxX;
  double vtxY = fgMinuitOptimizer->fVtxY;
  double vtxZ = fgMinuitOptimizer->fVtxZ;
  
  double dirX = 0.0;
  double dirY = 0.0;
  double dirZ = 0.0;

  double dirTheta = par[0]; // radians
  double dirPhi   = par[1]; // radians
  
  dirX = sin(dirTheta)*cos(dirPhi);
  dirY= sin(dirTheta)*sin(dirPhi);
  dirZ = cos(dirTheta);

  double vangle = 0.0;
  double fom = 0.0;

  fgMinuitOptimizer->point_direction_itr();
  fgMinuitOptimizer->PointDirectionChi2(vtxX,vtxY,vtxZ,
                                     dirX,dirY,dirZ,
                                     vangle,fom);
//  fgMinuitOptimizer->fVtxGeo->CalcPointResiduals(vtxX, vtxY, vtxZ, 0.0, 
//                                 dirX, dirY, dirZ); //load expected vertex time for each digit
//  //Cone fit parameters
//  double ConeParam0 = fgMinuitOptimizer->fFixConeParam0;
//  double ConeParam1 = fgMinuitOptimizer->fFixConeParam1;
//  double ConeParam2 = fgMinuitOptimizer->fFixConeParam2;
//  fgMinuitOptimizer->ConePropertiesLnL(ConeParam0, ConeParam1, ConeParam2, vangle, fom); 
//  
  f = -fom; // note: need to maximize this fom

  if( printDebugMessages ){
    std::cout << " [point_direction_chi2] [" << fgMinuitOptimizer->point_direction_iterations() 
    	<< "] (px,py,pz)=(" << fgMinuitOptimizer->fDirX << "," << fgMinuitOptimizer->fDirY << "," 
    	<< fgMinuitOptimizer->fDirZ << ") fom=" << fom << std::endl;
  }

  return; 
}

static void point_vertex_chi2(int&, double*, double& f, double* par, int)
{
  bool printDebugMessages = 0;
  
  double vtxX     = par[0]; // centimetres
  double vtxY     = par[1]; // centimetres
  double vtxZ     = par[2]; // centimetres  
  
  double dirTheta = par[3]; // radians
  double dirPhi   = par[4]; // radians

  double dirX = sin(dirTheta)*cos(dirPhi);
  double dirY = sin(dirTheta)*sin(dirPhi);
  double dirZ = cos(dirTheta);

  double vangle = 0.0;
  double vtime = 0.0;
  double fom = 0.0;
  
  fgMinuitOptimizer->point_vertex_itr();
  fgMinuitOptimizer->PointVertexChi2(vtxX,vtxY,vtxZ,dirX,dirY,dirZ,vangle,vtime,fom);
  f = -fom; // note: need to maximize this fom

  if( printDebugMessages ){
    std::cout << " [point_vertex_chi2] [" << fgMinuitOptimizer->point_vertex_iterations() 
    	<< "] (x,y,z)=(" << vtxX << "," << vtxY << "," << vtxZ << ") (px,py,pz)=(" 
    	<< dirX << "," << dirY << "," << dirZ << ") vtime=" << vtime << " fom=" << fom << std::endl;
  }
  return;
}

static void extended_vertex_chi2(int&, double*, double& f, double* par, int)
{
  bool printDebugMessages = 0;
  
  double vtxX     = par[0]; // centimetres
  double vtxY     = par[1]; // centimetres
  double vtxZ     = par[2]; // centimetres  

  double dirTheta = par[3]; // radians
  double dirPhi   = par[4]; // radians

  double dirX = sin(dirTheta)*cos(dirPhi);
  double dirY = sin(dirTheta)*sin(dirPhi);
  double dirZ = cos(dirTheta);

  double vangle = 0.0;
  double vtime  = 0.0;
  double fom = 0.0;

  fgMinuitOptimizer->extended_vertex_itr();
  
  fgMinuitOptimizer->ExtendedVertexChi2(vtxX,vtxY,vtxZ,
                                     dirX,dirY,dirZ,
                                     vangle,vtime,fom);

  f = -fom; // note: need to maximize this fom

  if( printDebugMessages ){
    std::cout << " [extended_vertex_chi2] [" << fgMinuitOptimizer->extended_vertex_iterations() 
    	<< "] (x,y,z)=(" << vtxX << "," << vtxY << "," << vtxZ << ") (px,py,pz)=(" << dirX << "," 
    	<< dirY << "," << dirZ << ") vtime=" << vtime << " fom=" << fom << std::endl;  
  }
  return;
}

static void corrected_vertex_chi2(int&, double*, double& f, double* par, int)
{
  bool printDebugMessages = 0;
  
  double dl = par[0];
//  double dirTheta = par[1]; // radians
//  double dirPhi   = par[2]; // radiansf
  
//  double dirX = sin(dirTheta)*cos(dirPhi);
//  double dirY = sin(dirTheta)*sin(dirPhi);
//  double dirZ = cos(dirTheta);
  
  double dirX = fgMinuitOptimizer->fDirX;
  double dirY = fgMinuitOptimizer->fDirY;
  double dirZ = fgMinuitOptimizer->fDirZ;
  
  double vtxX = fgMinuitOptimizer->fVtxX + dl * dirX;
  double vtxY = fgMinuitOptimizer->fVtxY + dl * dirY;;
  double vtxZ = fgMinuitOptimizer->fVtxZ + dl * dirZ;

  double vangle = 0.0;
  double vtime  = 0.0;
  double fom = 0.0;

  if(TMath::Sqrt(vtxX*vtxX + vtxY*vtxY + vtxZ*vtxZ)>152 || vtxY>198 || vtxY<-198) {fom = 9999;}
  fgMinuitOptimizer->corrected_vertex_itr();
  
  fgMinuitOptimizer->CorrectedVertexChi2(vtxX,vtxY,vtxZ,
                                     dirX,dirY,dirZ,
                                     vangle,vtime,fom);

  f = -fom; // note: need to maximize this fom

  if( printDebugMessages ){
    std::cout << " [corrected_vertex_chi2] [" << fgMinuitOptimizer->extended_vertex_iterations() 
    	<< "] (x,y,z)=(" << vtxX << "," << vtxY << "," << vtxZ << ") (px,py,pz)=(" << dirX << "," 
    	<< dirY << "," << dirZ << ") vtime=" << vtime << " fom=" << fom << std::endl;  
  }

  return;
}

static void vertex_cone_lnl(int&, double*, double& f, double* par, int)
{
  bool printDebugMessages = 0;
  
  double vtxParam0 = fgMinuitOptimizer->fFixConeParam0;
  double vtxParam1 = fgMinuitOptimizer->fFixConeParam1;
  double vtxParam2 = fgMinuitOptimizer->fFixConeParam2;

  if( fgMinuitOptimizer->fFitConeParams ){
    vtxParam0 = par[0];
    vtxParam1 = par[1];
    vtxParam2 = par[2];
  }
 
  double vangle = 0.0;
  double fom = 0.0;

  fgMinuitOptimizer->cone_fit_itr();
  fgMinuitOptimizer->ConePropertiesLnL(vtxParam0,vtxParam1,vtxParam2,vangle,fom);

  f = -fom; // note: need to maximize this fom

  if( printDebugMessages ){
    std::cout << "  [vertex_cone_lnl] [" << fgMinuitOptimizer->cone_fit_iterations() 
    	<< "] vparam0=" << vtxParam0 << " vparam1=" << vtxParam1 << " vtxParam2=" << vtxParam2 
    	<< " fom=" << fom << std::endl;
  }

  return;
}

//Constructor
MinuitOptimizer::MinuitOptimizer() {
	fgMinuitOptimizer = this;
	fSeedVtx = 0;
	fVtxGeo = 0;
	fFittedVtx = new RecoVertex();
	fVtxX = 0.;
  fVtxY = 0.;
  fVtxZ = 0.;
  fVtxTime = 0.;
  fDirX = 0.;
  fDirY = 0.;
  fDirZ = 0.;
  fVtxFOM = 0.;
  fConeAngle = 0.;
  fBaseFOM = 100.0;
  fTimeFitItr = 0;
  fConeFitItr = 0;
  fPointPosItr = 0;
  fPointDirItr = 0;
  fPointVtxItr = 0;
  fExtendedVtxItr = 0;
  fCorrectedVtxItr = 0;
  fPass = 0;
  fItr = 0;
  fPrintLevel = -1;
  
  // fitting parameters ported from vertexFinder (FIXME)
  fFitTimeParams = 0;     // don't fit by default
  fFixTimeParam0 = 0.20;  // scattering parameter
  fFitConeParams = 1;     // do fit by default  
  fFixConeParam0 = 0.25;  // track length parameter
  fFixConeParam1 = 0.50;  // track length parameter
  fFixConeParam2 = 0.75;  // particle ID:  0.0[electron]->1.0[muon]
  fTimeFitWeight = 0.50;  // nominal time weight
  fConeFitWeight = 0.50;  // nominal cone weight
  
  //Cone paramters
  fConeParam0 = 0.;
  fConeParam1 = 0.;
  fConeParam2 = 0.;
	
	//WCSIM 
	fXmin = -152.0;
	fXmax = 152.0;
	fYmin = -198.0;
	fYmax = 198.0;
	fZmin = -152.0;
	fZmax = 152.0;
	fTmin = -10.0;
	fTmax = 20.0;
	
	// default Mean time calculator type
	fMeanTimeCalculatorType = 0;
	
	fMinuitPointPosition = new TMinuit();
  fMinuitPointPosition->SetPrintLevel(-1);
  fMinuitPointPosition->SetMaxIterations(5000);

  fMinuitPointDirection = new TMinuit();
  fMinuitPointDirection->SetPrintLevel(-1);
  fMinuitPointDirection->SetMaxIterations(5000);

  fMinuitPointVertex = new TMinuit();
  fMinuitPointVertex->SetPrintLevel(-1);
  fMinuitPointVertex->SetMaxIterations(5000);

  fMinuitExtendedVertex = new TMinuit();
  fMinuitExtendedVertex->SetPrintLevel(-1);
  fMinuitExtendedVertex->SetMaxIterations(5000); 
  
  fMinuitCorrectedVertex = new TMinuit();
  fMinuitCorrectedVertex->SetPrintLevel(-1);
  fMinuitCorrectedVertex->SetMaxIterations(5000);

  fMinuitTimeFit = new TMinuit();
  fMinuitTimeFit->SetPrintLevel(-1);
  fMinuitTimeFit->SetMaxIterations(5000);   

  fMinuitConeFit = new TMinuit();
  fMinuitConeFit->SetPrintLevel(-1);
  fMinuitConeFit->SetMaxIterations(5000); 
}

//Destructor
MinuitOptimizer::~MinuitOptimizer() {
	fSeedVtx = 0;
	fVtxGeo = 0;
	delete fMinuitTimeFit; fMinuitTimeFit = 0;
	delete fMinuitConeFit; fMinuitConeFit = 0;
	delete fMinuitPointPosition; fMinuitPointPosition = 0;
	delete fMinuitPointDirection; fMinuitPointDirection = 0;
	delete fMinuitPointVertex; fMinuitPointVertex = 0;
	delete fMinuitExtendedVertex; fMinuitExtendedVertex = 0;
	delete fMinuitCorrectedVertex; fMinuitCorrectedVertex = 0;
	delete fFittedVtx; fFittedVtx = 0;
}
	
	
//100 digits
void MinuitOptimizer::LoadVertexGeometry(VertexGeometry* vtxgeo) {
  this->fVtxGeo = vtxgeo;	
}

//Load vertex
void MinuitOptimizer::LoadVertex(RecoVertex* vtx) {
  this->fSeedVtx = vtx;
  
  this->fVtxX = vtx->GetPosition().X();
  this->fVtxY = vtx->GetPosition().Y();
  this->fVtxZ = vtx->GetPosition().Z();
  this->fVtxTime = vtx->GetTime();
  this->fDirX = vtx->GetDirection().X();
  this->fDirY = vtx->GetDirection().Y();
  this->fDirZ = vtx->GetDirection().Z();
  this->fFoundVertex = vtx->FoundVertex();
}

//Load vertex
void MinuitOptimizer::LoadVertex(double vtxX, double vtxY, double vtxZ, double vtxTime, double vtxDirX, double vtxDirY, double vtxDirZ) {
  this->fVtxX = vtxX;
  this->fVtxY = vtxY;
  this->fVtxZ = vtxZ;
  this->fVtxTime = vtxTime;
  this->fDirX = vtxDirX;
  this->fDirY = vtxDirY;
  this->fDirZ = vtxDirZ;
}

void MinuitOptimizer::TimePropertiesLnL(double vtxTime, double vtxParam, double& vtxFOM)
{ 
  // nuisance parameters
  // ===================
  double scatter = vtxParam; 

  // internal variables
  // ==================
  double weight = 0.0;
  double delta = 0.0;       // time residual of each hit
  double sigma = 0.0;       // time resolution of each hit
  double A = 0.0;           // normalisation of first Gaussian

  double Preal = 0.0;       // probability of real hit
  double P = 0.0;           // probability of hit

  double chi2 = 0.0;        // log-likelihood: chi2 = -2.0*log(L)
  double ndof = 0.0;        // total number of hits
  double fom = 0.0;         // figure of merit
  double coneAngle = Parameters::CherenkovAngle();
  double myConeAngleSigma = 2.0;
  double deltaAngle = 0.0;

  // tuning parameters
  // =================
  double fTimeFitNoiseRate = 0.02;  // hits/ns [0.40 for electrons, 0.02 for muons]
  // need to simulate the rate of the cosmic background
  // add noise to model
  // ==================
  double nFilterDigits = this->fVtxGeo->GetNFilterDigits(); 
  double Pnoise = fTimeFitNoiseRate/nFilterDigits;//FIXME
  Pnoise = 1e-8;//FIXME
  
  //bool istruehits = (Interface::Instance())->IsTrueHits();
  bool istruehits = 0; // for test only. JW

  // loop over digits
  // ================
  
  for( int idigit=0; idigit<this->fVtxGeo->GetNDigits(); idigit++ ){    
    	int detType = this->fVtxGeo->GetDigitType(idigit); 
      delta = this->fVtxGeo->GetDelta(idigit) - vtxTime;
      sigma = this->fVtxGeo->GetDeltaSigma(idigit);
      sigma = 1.2*sigma; 
      A  = 1.0 / ( 2.0*sigma*sqrt(0.5*TMath::Pi()) ); //normalisation constant
      Preal = A*exp(-(delta*delta)/(2.0*sigma*sigma));
      P = (1.0-Pnoise)*Preal + Pnoise; 
      chi2 += -2.0*log(P);
      ndof += 1.0; 
//      cout<<"(tm, t0, dt, sigma) = "<<this->fVtxGeo->GetDelta(idigit)<<", "<<vtxTime<<", "<<delta<<", "<<sigma<<endl;
  }	
  
//  TString configType = Parameters::Instance()->GetConfigurationType();
//  	
//  if(configType == "LAPPD")	{
//    for( int idigit=0; idigit<this->fVtxGeo->GetNDigits(); idigit++ ){    
//    	TString detType = this->fVtxGeo->GetDigitType(idigit); 
//      if( detType == "lappd_v0" /*&& this->fVtxGeo->IsFiltered(idigit)*/){
//        delta = this->fVtxGeo->GetDelta(idigit) - vtxTime;
//        sigma = 1.2 * this->fVtxGeo->GetDeltaSigma(idigit); //chose factor 1.2 to optimize the fitter performance
//        A  = 1.0 / ( 2.0*sigma*sqrt(0.5*TMath::Pi()) ); //normalisation constant
//        Preal = A*exp(-(delta*delta)/(2.0*sigma*sigma));
//        P = (1.0-Pnoise)*Preal + Pnoise; 
//        chi2 += -2.0*log(P);
//        ndof += 1.0; 
//      }
//    }
//  }
//  
//  else if(configType == "PMT") {
//    for( int idigit=0; idigit<this->fVtxGeo->GetNDigits(); idigit++ ){    
//    	TString detType = this->fVtxGeo->GetDigitType(idigit); 
//      if( detType == "PMT8inch" && this->fVtxGeo->GetDigitQ(idigit)>5/*&& this->fVtxGeo->IsFiltered(idigit)*/){
//        delta = this->fVtxGeo->GetDelta(idigit) - vtxTime;
//        sigma = 1.5 * this->fVtxGeo->GetDeltaSigma(idigit); //chose factor 1.2 to optimize the fitter performance
//        A  = 1.0 / ( 2.0*sigma*sqrt(0.5*TMath::Pi()) ); //normalisation constant
//        Preal = A*exp(-(delta*delta)/(2.0*sigma*sigma));
//        P = (1.0-Pnoise)*Preal + Pnoise; 
//        chi2 += -2.0*log(P);
//        ndof += 1.0; 
//      }
//    }	
//  }
//  
//  else if(configType == "Combined") {
//    for( int idigit=0; idigit<this->fVtxGeo->GetNDigits(); idigit++ ){    
//    	TString detType = this->fVtxGeo->GetDigitType(idigit); 
//      delta = this->fVtxGeo->GetDelta(idigit) - vtxTime;
//      sigma = this->fVtxGeo->GetDeltaSigma(idigit);
//      if(detType == "lappd_v0") sigma = 1.2*sigma;
//      if(detType == "PMT8inch") sigma = 1.5*sigma;
//      A  = 1.0 / ( 2.0*sigma*sqrt(0.5*TMath::Pi()) ); //normalisation constant
//      Preal = A*exp(-(delta*delta)/(2.0*sigma*sigma));
//      P = (1.0-Pnoise)*Preal + Pnoise; 
//      chi2 += -2.0*log(P);
//      ndof += 1.0; 
//    }	
//  }
//  
//  else if (configType == "Two-stage") {
//    for( int idigit=0; idigit<this->fVtxGeo->GetNDigits(); idigit++ ){    
//    	TString detType = this->fVtxGeo->GetDigitType(idigit); 
//      if( detType == "lappd_v0" /*&& this->fVtxGeo->IsFiltered(idigit)*/){
//        delta = this->fVtxGeo->GetDelta(idigit) - vtxTime;
//        sigma = 1.2 * this->fVtxGeo->GetDeltaSigma(idigit); //chose factor 1.2 to optimize the fitter performance
//        A  = 1.0 / ( 2.0*sigma*sqrt(0.5*TMath::Pi()) ); //normalisation constant
//        Preal = A*exp(-(delta*delta)/(2.0*sigma*sigma));
//        P = (1.0-Pnoise)*Preal + Pnoise; 
//        chi2 += -2.0*log(P);
//        ndof += 1.0; 
//      }
//    }	
//  }


  // calculate figure of merit
  if( ndof>0.0 ){
    fom = fBaseFOM - 5.0*chi2/ndof;
  }  

  // return figure of merit
  // ======================
  vtxFOM = fom;
  

  return;
}

void MinuitOptimizer::ConePropertiesFoM(double& coneFOM)
{  
  // calculate figure of merit
  // =========================
  double coneEdge = Parameters::CherenkovAngle();     // nominal cone angle
  double coneEdgeLow = 21.0;  // cone edge (low side)      
  double coneEdgeHigh = 3.0;  // cone edge (high side)   [muons: 3.0, electrons: 7.0]

  double deltaAngle = 0.0;
  double digitCharge = 0.0;
 
  double coneCharge = 0.0;
  double allCharge = 0.0;

  double fom = 0.0;

  for( int idigit=0; idigit<this->fVtxGeo->GetNDigits(); idigit++ ){ 	
    if( this->fVtxGeo->IsFiltered(idigit) ){
      deltaAngle = this->fVtxGeo->GetAngle(idigit) - coneEdge;
      digitCharge = this->fVtxGeo->GetDigitQ(idigit);

      if( deltaAngle<=0.0 ){
        coneCharge += digitCharge*( 0.75 + 0.25/( 1.0 + (deltaAngle*deltaAngle)/(coneEdgeLow*coneEdgeLow) ) );
      }
      else{
        coneCharge += digitCharge*( 0.00 + 1.00/( 1.0 + (deltaAngle*deltaAngle)/(coneEdgeHigh*coneEdgeHigh) ) );
      }

      allCharge += digitCharge;
    }
  }

  if( allCharge>0.0 ){
    fom = fBaseFOM*coneCharge/allCharge;
  }

  // return figure of merit
  // ======================
  coneFOM = fom;

  return;
}

void MinuitOptimizer::ConePropertiesLnL(double coneParam0, double coneParam1, double coneParam2, double& coneAngle, double& coneFOM)
{  
  
  // nuisance parameters
  // ===================
  double alpha  = coneParam0; // track length parameter = 0.25
  double alpha0 = coneParam1; // track length parameter = 0.5
  double beta   = coneParam2; // particle ID:  0.0[electron]->1.0[muon] = 0.75

  // internal variables
  // ==================
  double deltaAngle = 0.0; //
  double sigmaAngle = 7.0; //Cherenkov Angle resolution
  double Angle0 = Parameters::CherenkovAngle(); //Cherenkov Angle: 42 degree
  double deltaAngle0 = Angle0*alpha0; //?
  
  double digitQ = 0.0;
  double sigmaQmin = 1.0;
  double sigmaQmax = 10.0;
  double sigmaQ = 0.0;

  double A = 0.0;
  
  double PconeA = 0.0;
  double PconeB = 0.0;
  double Pmu = 0.0;
  double Pel = 0.0;

  double Pcharge = 0.0;
  double Pangle = 0.0;
  double P = 0.0;

  double chi2 = 0.0;
  double ndof = 0.0;

  double angle = 46.0;
  double fom = 0.0;

  // hard-coded parameters: 200 kton (100 kton)
  // ==========================================
  double lambdaMuShort = 0.5; //  0.5;
  double lambdaMuLong  = 5.0; // 15.0;
  double alphaMu =       1.0; //  4.5;

  double lambdaElShort = 1.0; //  2.5;
  double lambdaElLong =  7.5; // 15.0;
  double alphaEl =       6.0; //  3.5;

  // numerical integrals
  // ===================
  fSconeA = 21.9938;  
  fSconeB =  0.0000;
  
  // Number of P.E. angular distribution
  // inside cone
  int nbinsInside = 420; //divide the angle range by 420 bins
  for( int n=0; n<nbinsInside; n++ ){
    deltaAngle = -Angle0 + (n+0.5)*(Angle0/(double)nbinsInside); // angle axis
    fSconeB += 1.4944765*sin( (Angle0+deltaAngle)*(TMath::Pi()/180.0) )
                           *( 1.0/(1.0+(deltaAngle*deltaAngle)/(deltaAngle0*deltaAngle0)) )
                           *( Angle0/(double)nbinsInside );
  }

  // outside cone
  if( fIntegralsDone == 0 ){ // default 0
    fSmu = 0.0;
    fSel = 0.0;

    int nbinsOutside = 1380;
    for( int n=0; n<nbinsOutside; n++ ){
      deltaAngle = 0.0 + (n+0.5)*(138.0/(double)nbinsOutside);

      fSmu += 1.4944765*sin( (Angle0+deltaAngle)*(TMath::Pi()/180.0) )
                          *( 1.0/(1.0+alphaMu*(lambdaMuShort/lambdaMuLong)) )*( 1.0/(1.0+(deltaAngle*deltaAngle)/(lambdaMuShort*lambdaMuShort)) 
	  			            + alphaMu*(lambdaMuShort/lambdaMuLong)/(1.0+(deltaAngle*deltaAngle)/(lambdaMuLong*lambdaMuLong)) )
                          *( 138.0/(double)nbinsOutside );

      fSel += 1.4944765*sin( (Angle0+deltaAngle)*(TMath::Pi()/180.0) )
                          *( 1.0/(1.0+alphaEl*(lambdaElShort/lambdaElLong)) )*( 1.0/(1.0+(deltaAngle*deltaAngle)/(lambdaElShort*lambdaElShort)) 
				          + alphaEl*(lambdaElShort/lambdaElLong)/(1.0+(deltaAngle*deltaAngle)/(lambdaElLong*lambdaElLong)) )
                          *( 138.0/(double)nbinsOutside );
    }

    fIntegralsDone = 1;
  }

  // loop over digits
  // ================
  for( int idigit=0; idigit<this->fVtxGeo->GetNDigits(); idigit++ ){

    if( this->fVtxGeo->IsFiltered(idigit) ){
      digitQ = this->fVtxGeo->GetDigitQ(idigit);
      deltaAngle = this->fVtxGeo->GetAngle(idigit) - Angle0;

      // pulse height distribution
      // =========================
      if( deltaAngle<=0 ){ //inside Cone
	      sigmaQ = sigmaQmax;
      }
      else{ //outside Cone
        sigmaQ = sigmaQmin + (sigmaQmax-sigmaQmin)/(1.0+(deltaAngle*deltaAngle)/(sigmaAngle*sigmaAngle));
      }

      A = 1.0/(log(2.0)+0.5*TMath::Pi()*sigmaQ);

      if( digitQ<1.0 ){
        Pcharge = 2.0*A*digitQ/(1.0+digitQ*digitQ);
      }
      else{
        Pcharge = A/(1.0+(digitQ-1.0)*(digitQ-1.0)/(sigmaQ*sigmaQ));
      }

      // angular distribution
      // ====================
      A = 1.0/( alpha*fSconeA + (1.0-alpha)*fSconeB
               + beta*fSmu + (1.0-beta)*fSel ); // numerical integrals

      if( deltaAngle<=0 ){

        // pdfs inside cone:
        PconeA = 1.4944765*sin( (Angle0+deltaAngle)*(TMath::Pi()/180.0) );
        PconeB = 1.4944765*sin( (Angle0+deltaAngle)*(TMath::Pi()/180.0) )
                          *( 1.0/(1.0+(deltaAngle*deltaAngle)/(deltaAngle0*deltaAngle0)) );

        Pangle = A*( alpha*PconeA+(1.0-alpha)*PconeB );
      }		
      else{

        // pdfs outside cone
        Pmu = 1.4944765*sin( (Angle0+deltaAngle)*(TMath::Pi()/180.0) )
                       *( 1.0/(1.0+alphaMu*(lambdaMuShort/lambdaMuLong)) )*( 1.0/(1.0+(deltaAngle*deltaAngle)/(lambdaMuShort*lambdaMuShort)) 
                                          + alphaMu*(lambdaMuShort/lambdaMuLong)/(1.0+(deltaAngle*deltaAngle)/(lambdaMuLong*lambdaMuLong)) );

        Pel = 1.4944765*sin( (Angle0+deltaAngle)*(TMath::Pi()/180.0) )
                       *( 1.0/(1.0+alphaEl*(lambdaElShort/lambdaElLong)) )*( 1.0/(1.0+(deltaAngle*deltaAngle)/(lambdaElShort*lambdaElShort)) 
                                          + alphaEl*(lambdaElShort/lambdaElLong)/(1.0+(deltaAngle*deltaAngle)/(lambdaElLong*lambdaElLong)) );

        Pangle = A*( beta*Pmu+(1.0-beta)*Pel );
      }

      // overall probability
      // ===================
      P = Pcharge*Pangle;
      
      chi2 += -2.0*log(P);
      ndof += 1.0;
    }
  }

  // calculate figure of merit
  // =========================   
  if( ndof>0.0 ){
    fom = fBaseFOM - 5.0*chi2/ndof;
    angle = beta*43.0 + (1.0-beta)*49.0;
  }

  // return figure of merit
  // ======================
  coneAngle = angle;
  coneFOM = fom;

  return;
}

//Find the vertex time with the best FOM. Update the vertex with the new time
void MinuitOptimizer::FitPointTimePropertiesLnL(double& vtxTime, double& vtxFOM)
{
  // calculate mean and rms
  // ====================== 
  double meanvtxTime = 0.0;
  meanvtxTime = this->FindSimpleTimeProperties(this->fVtxGeo);  //returns weighted average of the expected vertex time
  // reset counter
  // =============
  time_fit_reset_itr();
  
  // run Minuit
  // ==========  
  // one-parameter fit to time profile

  int err = 0;
  int flag = 0;

  double seedTime = meanvtxTime;
  double fitParam = fFixTimeParam0;
  double fitTime = 0.0;
  double fitTimeErr = 0.0;  
  
  double* arglist = new double[10];
  arglist[0]=1;  // 1: standard minimization
                 // 2: try to improve minimum
  
  // re-initialize everything...
  fMinuitTimeFit->mncler();
  fMinuitTimeFit->SetFCN(vertex_time_lnl);
  //end
  fMinuitTimeFit->mnexcm("SET STR",arglist,1,err);
  fMinuitTimeFit->mnparm(0,"vtxTime",seedTime,1.0,fTmin,fTmax,err);
  
  flag = fMinuitTimeFit->Migrad();
  fMinuitTimeFit->GetParameter(0,fitTime,fitTimeErr); //get the best time
  delete [] arglist;
  
  // calculate figure of merit
  // =========================
  double fom = -999.999*fBaseFOM;
  this->TimePropertiesLnL(fitTime,fitParam, fom);
  
  vtxTime = fitTime;
  vtxFOM = fom;
  
  fFittedVtx->SetVertex(fVtxX, fVtxY, fVtxZ, vtxTime);
  fFittedVtx->SetFOM(vtxFOM, 1, 1);
  return;
}

void MinuitOptimizer::FitExtendedTimePropertiesLnL(double& vtxTime, double& vtxFOM)
{    
  
  // return result from point fit
  // ============================
  if( fFitTimeParams==0 ){
    return this->FitPointTimePropertiesLnL(vtxTime,vtxFOM);
  }

  // calculate mean and rms
  // ====================== 
  double meanTime = this->FindSimpleTimeProperties(this->fVtxGeo); 

  // reset counter
  // =============
  time_fit_reset_itr();

  // run Minuit
  // ==========  
  // two-parameter fit to time profile

  int err = 0;
  int flag = 0;

  double seedTime = meanTime;
  double seedParam = this->fFixTimeParam0;

  double fitTime = seedTime;
  double fitTimeErr = 0.0;  

  double fitParam = seedParam;
  double fitParamErr = 0.0;
  
  double* arglist = new double[10];
  arglist[0]=1;  // 1: standard minimization
                 // 2: try to improve minimum
		 

  // re-initialize everything...
  fMinuitTimeFit->mncler();
  fMinuitTimeFit->SetFCN(vertex_time_lnl);
  fMinuitTimeFit->mnexcm("SET STR",arglist,1,err);
  fMinuitTimeFit->mnparm(0,"vtxTime",seedTime,1.0,fTmin,fTmax,err); //....TX
  fMinuitTimeFit->mnparm(1,"vtxParam",seedParam,0.05,0.0,1.0,err);
  
  flag = fMinuitTimeFit->Migrad();
  fMinuitTimeFit->GetParameter(0,fitTime,fitTimeErr);
  fMinuitTimeFit->GetParameter(1,fitParam,fitParamErr);
  
  delete [] arglist;


  // calculate figure of merit
  // =========================
  double fom = -999.999*fBaseFOM;

  this->TimePropertiesLnL(fitTime,fitParam,fom); 

  // return figure of merit
  // ======================
  vtxTime = fitTime;
  vtxFOM = fom;

  return;
}

void MinuitOptimizer::FitConePropertiesFoM(double& coneAngle, double& coneFOM)
{
  coneAngle = Parameters::CherenkovAngle(); //...chrom1.38 // nominal cone angle

  this->ConePropertiesFoM(coneFOM);

  return;
}

void MinuitOptimizer::FitPointConePropertiesLnL(double& coneAngle, double& coneFOM)
{  
  coneAngle  = Parameters::CherenkovAngle(); //...chrom1.38, nominal cone angle
  // calculate cone fom
  // =========================
  coneFOM = 0.0;
  double ConeParam0 = this->fFixConeParam0;
  double ConeParam1 = this->fFixConeParam1;
  double ConeParam2 = this->fFixConeParam2;
  this->ConePropertiesLnL(ConeParam0,ConeParam1,ConeParam2,coneAngle,coneFOM);  
  return;
}

void MinuitOptimizer::FitExtendedConePropertiesLnL(double& coneAngle, double& coneFOM)
{  
  // return result from point fit
  // ============================
  if( this->fFitConeParams==0 ){
    return this->FitPointConePropertiesLnL(coneAngle,coneFOM);
  }

  // reset counter
  // =============
  cone_fit_reset_itr();

  // run Minuit
  // ==========  
  // one-parameter fit to angular distribution

  int err = 0;
  int flag = 0;

  double seedParam0 = this->fFixConeParam0; //track length parameter
  double seedParam1 = this->fFixConeParam1; //track length parameter
  double seedParam2 = this->fFixConeParam2; //particle ID:  0.0[electron]->1.0[muon]

  double fitParam0 = seedParam0;
  double fitParam1 = seedParam1;
  double fitParam2 = seedParam2;

  double fitParam0Err = 0.0;
  double fitParam1Err = 0.0; 
  double fitParam2Err = 0.0;

  double fitAngle  = Parameters::CherenkovAngle();
  
  double* arglist = new double[10];
  arglist[0]=1;  // 1: standard minimization
                 // 2: try to improve minimum

  // re-initialize everything...
  fMinuitConeFit->mncler();
  fMinuitConeFit->SetFCN(vertex_cone_lnl);
  fMinuitConeFit->mnexcm("SET STR",arglist,1,err);
  fMinuitConeFit->mnparm(0,"vtxParam0",seedParam0,0.25,0.0,1.0,err);
  fMinuitConeFit->mnparm(1,"vtxParam1",seedParam1,0.25,0.0,1.0,err);
  fMinuitConeFit->mnparm(2,"vtxParam2",seedParam2,0.25,0.0,1.0,err);

  flag = fMinuitConeFit->Migrad();
  fMinuitConeFit->GetParameter(0,fitParam0,fitParam0Err);
  fMinuitConeFit->GetParameter(1,fitParam1,fitParam1Err);
  fMinuitConeFit->GetParameter(2,fitParam2,fitParam2Err);
  
  delete [] arglist;

  // calculate figure of merit
  // =========================
  double fom = -999.999*fBaseFOM;
  
  this->ConePropertiesLnL(fitParam0,fitParam1,fitParam2,
                          fitAngle,fom);  


  // --- debug ---
  fConeParam0 = fitParam0;
  fConeParam1 = fitParam1;
  fConeParam2 = fitParam2;

  // return figure of merit
  // ======================
  coneAngle = fitAngle;
  coneFOM = fom;

  return;
}

void MinuitOptimizer::FitPointTimeWithMinuit() {
	this->fVtxGeo->CalcPointResiduals(fVtxX, fVtxY, fVtxZ, 0.0, 0.0, 0.0, 0.0);
  this->FitPointTimePropertiesLnL(fVtxTime, fVtxFOM);
}

//Fit point position in 3D
void MinuitOptimizer::FitPointPositionWithMinuit() {
	// seed vertex
  // ===========
  bool foundSeed = fSeedVtx->FoundVertex();
  double seedX = fSeedVtx->GetPosition().X();
  double seedY = fSeedVtx->GetPosition().Y();
  double seedZ = fSeedVtx->GetPosition().Z();
  double seedTime = fSeedVtx->GetTime();
  
  // current status
  // ==============
  int status = fSeedVtx->GetStatus();

  // reset counter
  // =============
  point_position_reset_itr();
  
  // create new vertex
  // =================
  //RecoVertex* newVertex = new RecoVertex();
  //vVertexList.push_back(newVertex);
  
  // abort if necessary
  // ==================
  if( foundSeed==0 ){
  	if(fPrintLevel >=0) {
      std::cout << "   <warning> point position fit failed to find input vertex " << std::endl;   
    } 
    status |= RecoVertex::kFailPointPosition;
    fFittedVtx->SetStatus(status);
    return;
  }
  
  // run Minuit
  // ==========  
  // three-parameter fit to vertex coordinates
  int err = 0;
  int flag = 0;

  double fitXpos = 0.0;
  double fitYpos = 0.0;
  double fitZpos = 0.0;
  double fitTimepos = 0.0; //JW

  double fitXposErr = 0.0;
  double fitYposErr = 0.0;
  double fitZposErr = 0.0;
  double fitTimeposErr = 0.0; //JW
  
  double fitParam = 0.2; //scattering parameter

  double* arglist = new double[10];
  arglist[0]=1;  // 1: standard minimization
                 // 2: try to improve minimum
  
  // re-initialize everything...
  fMinuitPointPosition->mncler();
  fMinuitPointPosition->SetFCN(point_position_chi2);
  fMinuitPointPosition->mnexcm("SET STR",arglist,1,err);
  fMinuitPointPosition->mnparm(0,"x",seedX,1.0,fXmin,fXmax,err);
  fMinuitPointPosition->mnparm(1,"y",seedY,1.0,fYmin,fYmax,err);
  fMinuitPointPosition->mnparm(2,"z",seedZ,5.0,fZmin,fZmax,err);
  fMinuitPointPosition->mnparm(3,"Time",seedTime,1.0,fTmin,fTmax,err);

  flag = fMinuitPointPosition->Migrad();
  fMinuitPointPosition->GetParameter(0,fitXpos,fitXposErr);
  fMinuitPointPosition->GetParameter(1,fitYpos,fitYposErr);
  fMinuitPointPosition->GetParameter(2,fitZpos,fitZposErr);
  fMinuitPointPosition->GetParameter(3,fitTimepos,fitTimeposErr);

  delete [] arglist;
  
  // sort results
  // ============
  fVtxX = fitXpos; 
  fVtxY = fitYpos;
  fVtxZ = fitZpos;
  fVtxTime = fitTimepos;

  double vtxFOM = 0.0;
  
  fPass = 0;               // flag = 0: normal termination
  if( flag==0 ) fPass = 1; // anything else: abnormal termination 

  fItr = point_position_iterations();
  this->PointPositionChi2(fVtxX,fVtxY,fVtxZ,fVtxTime,fVtxFOM);
  
  // set vertex and direction
  // ========================
  fFittedVtx->SetVertex(fVtxX, fVtxY, fVtxZ, fVtxTime);
  fFittedVtx->SetDirection(0.0, 0.0, 0.0);
  fFittedVtx->SetFOM(fVtxFOM, fItr,fPass);  
  
  // set status
  // ==========
  bool inside_det =  ANNIEGeometry::Instance()->InsideDetector(fVtxX,fVtxY,fVtxZ);
  if( !fPass || !inside_det) status |= RecoVertex::kFailPointPosition;
  fFittedVtx->SetStatus(status);
  if(fPrintLevel >0) cout<<"Fit Status = "<<status<<endl;
  
  // print vertex
  // ============
  if(fPrintLevel >0)  {
    std::cout << "  fitted point position: " << std::endl
              << "     (vx,vy,vz)=(" << fFittedVtx->GetPosition().X() << "," << fFittedVtx->GetPosition().Y() << "," << fFittedVtx->GetPosition().Z() << ") " << std::endl
              << "      vtime=" << fFittedVtx->GetTime() << " itr=" << fFittedVtx->GetIterations() << " fom=" << fFittedVtx->GetFOM() << std::endl;
  }
  if(fPrintLevel >=0) {
    if( !fPass ) std::cout << "   <warning> point position fit failed to converge! Error code: " << flag << std::endl;
  }
  return;
}

void MinuitOptimizer::FitPointDirectionWithMinuit() {
  // initialization
  // ==============
  bool foundSeed = ( fSeedVtx->FoundVertex() && fSeedVtx->FoundDirection() );
 
  double vtxAngle = Parameters::CherenkovAngle();

  double vtxFOM = 0.0;
  
  // seed direction
  // ==============
  double seedDirX = fSeedVtx->GetDirection().X();
  double seedDirY = fSeedVtx->GetDirection().Y();
  double seedDirZ = fSeedVtx->GetDirection().Z();
  
  double seedTheta = acos(seedDirZ); //
  double seedPhi = 0.0;

  if( seedDirX!=0.0 ){
    seedPhi = atan(seedDirY/seedDirX);
  }
  if( seedDirX<=0.0 ){
    if( seedDirY>0.0 ) seedPhi += TMath::Pi();
    if( seedDirY<0.0 ) seedPhi -= TMath::Pi();
  }  
  
  // current status
  // ==============
  int status = fSeedVtx->GetStatus();

  // reset counter
  // =============
  point_direction_reset_itr();
  
  // create new vertex
  // =================
  //RecoVertex* newVertex = new RecoVertex();
  //vVertexList.push_back(newVertex);
  //fPointDirection = newVertex;
  
  // abort if necessary
  // ==================
  if( foundSeed==0 ){
  	if(fPrintLevel >=0) {
      std::cout << "   <warning> point position fit failed to find input vertex " << std::endl;   
    } 
    status |= RecoVertex::kFailPointPosition;
    fFittedVtx->SetStatus(status);
    return;
  }
  
  // run Minuit
  // ==========  
  // two-parameter fit to direction coordinates
  
  int err = 0;
  int flag = 0;

  double dirTheta;
  double dirPhi;

  double dirThetaErr;
  double dirPhiErr;

  double* arglist = new double[10];
  arglist[0]=1;  // 1: standard minimization
                 // 2: try to improve minimum
  // re-initialize everything...
  fMinuitPointDirection->mncler();
  fMinuitPointDirection->SetFCN(point_direction_chi2);
  fMinuitPointDirection->mnexcm("SET STR",arglist,1,err);
  fMinuitPointDirection->mnparm(0,"theta",seedTheta,0.125*TMath::Pi(),0.0,TMath::Pi(),err);
  fMinuitPointDirection->mnparm(1,"phi",seedPhi,0.25*TMath::Pi(),-1.0*TMath::Pi(),+3.0*TMath::Pi(),err);

  flag = fMinuitPointDirection->Migrad();
  fMinuitPointDirection->GetParameter(0,dirTheta,dirThetaErr);
  fMinuitPointDirection->GetParameter(1,dirPhi,dirPhiErr);

  delete [] arglist;

  // sort results
  // ============
  this->fDirX = sin(dirTheta)*cos(dirPhi);
  this->fDirY = sin(dirTheta)*sin(dirPhi);
  this->fDirZ = cos(dirTheta);
  
  fVtxFOM = 0.0;
  
  fPass = 0;               // flag = 0: normal termination
  if( flag==0 ) fPass = 1; // anything else: abnormal termination 
                           
  fItr = point_direction_iterations();
  
  // calculate vertex
  // ================
  this->PointDirectionChi2(fVtxX,fVtxY,fVtxZ,fDirX,fDirY,fDirZ,fConeAngle,fVtxFOM);
  //Fix position, scan and update time 
  //this->FitPointTimePropertiesLnL();

  // set vertex and direction
  // ========================
  fFittedVtx->SetVertex(fVtxX,fVtxY,fVtxZ,fVtxTime);
  fFittedVtx->SetDirection(fDirX,fDirY,fDirZ); //fitted direction
  fFittedVtx->SetConeAngle(fConeAngle);
  fFittedVtx->SetFOM(fVtxFOM,fItr,fPass);

  // set status
  // ==========
  if( !fPass ) status |= RecoVertex::kFailPointPosition;
  this->fFittedVtx->SetStatus(status);
  
  // print vertex
  // ============
  if(fPrintLevel > 0) {
    std::cout << "  fitted point direction: " << std::endl
              << "    (vx,vy,vz)=(" << fFittedVtx->GetPosition().X() << "," << fFittedVtx->GetPosition().Y() << "," << fFittedVtx->GetPosition().Z() << ") " << std::endl
              << "    (px,py,pz)=(" << fFittedVtx->GetDirection().X() << "," << fFittedVtx->GetDirection().Y() << "," << fFittedVtx->GetDirection().Z() << ") " << std::endl
              << "      angle=" << fFittedVtx->GetConeAngle() << " vtime=" << fFittedVtx->GetTime() << " itr=" << fFittedVtx->GetIterations() << " fom=" << fFittedVtx->GetFOM() << std::endl;
  }
  // print fit info
  if(fPrintLevel >= 0) {
    if( !fPass ) std::cout << "   <warning> point direction fit failed to converge! Error code: " << flag << std::endl;
  }
  return;
}

void MinuitOptimizer::FitPointVertexWithMinuit() {
  
  double vtxAngle = Parameters::CherenkovAngle();
  double vtxFOM = 0.0;
  
  // seed vertex
  // ===========  
  bool foundSeed = ( fSeedVtx->FoundVertex() 
                   && fSeedVtx->FoundDirection() );
                   
  double seedX = fSeedVtx->GetPosition().X();
  double seedY = fSeedVtx->GetPosition().Y();
  double seedZ = fSeedVtx->GetPosition().Z();
  double seedTime = fSeedVtx->GetTime();
  
  seedTime = 0.0; 

  double seedDirX = fSeedVtx->GetDirection().X();
  double seedDirY = fSeedVtx->GetDirection().Y();
  double seedDirZ = fSeedVtx->GetDirection().Z();

  double seedTheta = acos(seedDirZ);
  double seedPhi = 0.0;
  
  
  if( seedDirX!=0.0 ){
    seedPhi = atan(seedDirY/seedDirX);
  }
  if( seedDirX<=0.0 ){
    if( seedDirY>0.0 ) seedPhi += TMath::Pi();
    if( seedDirY<0.0 ) seedPhi -= TMath::Pi();
  }
  
  // current status
  // ==============
  int status = fSeedVtx->GetStatus();

  // reset counter
  // =============
  point_vertex_reset_itr();
  
//  // create new vertex
//  // =================
//  RecoVertex* newVertex = new RecoVertex();
//  vVertexList.push_back(newVertex);
//  fPointVertex = newVertex;
  
  // abort if necessary
  // ==================
  if( foundSeed==0 ){
    if(fPrintLevel >= 0) {
    	std::cout << "   <warning> point vertex fit failed to find input vertex " << std::endl;
    }   
    status |= RecoVertex::kFailPointVertex;
    this->fFittedVtx->SetStatus(status);
    return;
  }
  
  // run Minuit
  // ==========  
  // five-parameter fit to vertex and direction

  int err = 0;
  int flag = 0;

  double fitXpos = 0.0;
  double fitYpos = 0.0;
  double fitZpos = 0.0;
  double fitTheta = 0.0;
  double fitPhi = 0.0;

  double fitXposErr = 0.0;
  double fitYposErr = 0.0;
  double fitZposErr = 0.0;
  double fitThetaErr = 0.0;
  double fitPhiErr = 0.0;
  
  double* arglist = new double[10];
  arglist[0]=2;  // 1: standard minimization
                 // 2: try to improve minimum

  // re-initialize everything...
  fMinuitPointVertex->mncler();
  fMinuitPointVertex->SetFCN(point_vertex_chi2);
  fMinuitPointVertex->mnexcm("SET STR",arglist,1,err);
//  fMinuitPointVertex->mnparm(0,"x",seedX,1.0,-152,152,err); 
//  fMinuitPointVertex->mnparm(1,"y",seedY,1.0,-212.46,183.54,err);
//  fMinuitPointVertex->mnparm(2,"z",seedZ,1.0,15,319,err);
  fMinuitPointVertex->mnparm(0,"x",seedX,1.0,fXmin,fXmax,err); 
  fMinuitPointVertex->mnparm(1,"y",seedY,1.0,fYmin,fYmax,err); 
  fMinuitPointVertex->mnparm(2,"z",seedZ,5.0,fZmin,fZmax,err); 
  fMinuitPointVertex->mnparm(3,"theta",seedTheta,0.125*TMath::Pi(),0.0,TMath::Pi(),err);
  fMinuitPointVertex->mnparm(4,"phi",seedPhi,0.25*TMath::Pi(),-1.0*TMath::Pi(),+3.0*TMath::Pi(),err);
  
  flag = fMinuitPointVertex->Migrad();
  fMinuitPointVertex->GetParameter(0,fitXpos,fitXposErr);
  fMinuitPointVertex->GetParameter(1,fitYpos,fitYposErr);
  fMinuitPointVertex->GetParameter(2,fitZpos,fitZposErr);
  fMinuitPointVertex->GetParameter(3,fitTheta,fitThetaErr);
  fMinuitPointVertex->GetParameter(4,fitPhi,fitPhiErr);
  
  delete [] arglist;
  
  // sort results
  // ============
  fVtxX = fitXpos; 
  fVtxY = fitYpos;
  fVtxZ = fitZpos;
  //fVtxTime = 6600.; //for test only

  fDirX = sin(fitTheta)*cos(fitPhi);
  fDirY = sin(fitTheta)*sin(fitPhi);
  fDirZ = cos(fitTheta);  

  vtxFOM = 0.0;
  
  fPass = 0;               // flag = 0: normal termination
  if( flag==0 ) fPass = 1; // anything else: abnormal termination 

  fItr = point_vertex_iterations();
  
  // calculate vertex
  // ================
  this->PointVertexChi2(fVtxX,fVtxY,fVtxZ,fDirX,fDirY,fDirZ,fConeAngle,fVtxTime,fVtxFOM); 
  
  // set vertex and direction
  // ========================
  fFittedVtx->SetVertex(fVtxX,fVtxY,fVtxZ,fVtxTime);
  fFittedVtx->SetDirection(fDirX,fDirY,fDirZ);
  fFittedVtx->SetConeAngle(fConeAngle);
  fFittedVtx->SetFOM(fVtxFOM,fItr,fPass);
  
  // set status
  // ==========
  bool inside_det =  ANNIEGeometry::Instance()->InsideDetector(fVtxX,fVtxY,fVtxZ);
  if( !fPass ||!inside_det ) status |= RecoVertex::kFailPointVertex;
  this->fFittedVtx->SetStatus(status);
  
  // print vertex
  // ============
  if(fPrintLevel > 0) {
    std::cout << "  fitted point vertex: " << std::endl
              << "    (vx,vy,vz)=(" << fFittedVtx->GetPosition().X() << "," << fFittedVtx->GetPosition().Y() << "," << fFittedVtx->GetPosition().Z() << ") " << std::endl
              << "    (px,py,pz)=(" << fFittedVtx->GetDirection().X() << "," << fFittedVtx->GetDirection().Y() << "," << fFittedVtx->GetDirection().Z() << ") " << std::endl
              << "      angle=" << fFittedVtx->GetConeAngle() << " vtime=" << fFittedVtx->GetTime() << " itr=" << fFittedVtx->GetIterations() << " fom=" << fFittedVtx->GetFOM() << std::endl;
  }
  if(fPrintLevel >= 0) {
    if( !fPass ) std::cout << "   <warning> point vertex fit failed to converge! Error code: " << flag << std::endl;
  }
  return;
}

void MinuitOptimizer::FitExtendedVertexWithMinuit() {
  // initialization
  // ==============
  double vtxAngle = Parameters::CherenkovAngle();
  double vtxFOM = 0.0;
  // seed vertex
  // ===========
  bool foundSeed = ( fSeedVtx->FoundVertex() 
                    && fSeedVtx->FoundDirection() );

  double seedX = fSeedVtx->GetPosition().X();
  double seedY = fSeedVtx->GetPosition().Y();
  double seedZ = fSeedVtx->GetPosition().Z();

  double seedDirX = fSeedVtx->GetDirection().X();
  double seedDirY = fSeedVtx->GetDirection().Y();
  double seedDirZ = fSeedVtx->GetDirection().Z();

  double seedTheta = acos(seedDirZ);
  double seedPhi = 0.0;
  
  //modified by JW
  if( seedDirX>0.0 ){
    seedPhi = atan(seedDirY/seedDirX);
  } 
  if( seedDirX<0.0 ){
  	seedPhi = atan(seedDirY/seedDirX);
    if( seedDirY>0.0 ) seedPhi += TMath::Pi();
    if( seedDirY<=0.0 ) seedPhi -= TMath::Pi();
  }
  if( seedDirX==0.0 ){
    if( seedDirY>0.0 ) seedPhi = 0.5*TMath::Pi();
    else if( seedDirY<0.0 ) seedPhi = -0.5*TMath::Pi();
    else seedPhi = 0.0;
  }
  // current status
  // ==============
  int status = fSeedVtx->GetStatus();

  // reset counter
  // =============
  extended_vertex_reset_itr();
  
  // abort if necessary
  // ==================
  if( foundSeed==0 ){
  	if(fPrintLevel >= 0) {
      std::cout << "   <warning> extended vertex fit failed to find input vertex " << std::endl;   
    }
    status |= RecoVertex::kFailExtendedVertex;
    fFittedVtx->SetStatus(status);
    return;
  }
  
  // run Minuit
  // ==========  
  // five-parameter fit to vertex and direction

  int err = 0;
  int flag = 0;

  double fitXpos = 0.0;
  double fitYpos = 0.0;
  double fitZpos = 0.0;
  double fitTheta = 0.0;
  double fitPhi = 0.0;

  double fitXposErr = 0.0;
  double fitYposErr = 0.0;
  double fitZposErr = 0.0;
  double fitThetaErr = 0.0;
  double fitPhiErr = 0.0;
  
  double* arglist = new double[10];
  arglist[0]=2;  // 1: standard minimization
                 // 2: try to improve minimum
  
  // re-initialize everything...
  fMinuitExtendedVertex->mncler();
  fMinuitExtendedVertex->SetFCN(extended_vertex_chi2);
  fMinuitExtendedVertex->mnset();
  fMinuitExtendedVertex->mnexcm("SET STR",arglist,1,err);
  fMinuitExtendedVertex->mnparm(0,"x",seedX,1.0,fXmin,fXmax,err);
  fMinuitExtendedVertex->mnparm(1,"y",seedY,1.0,fYmin,fYmax,err);
  fMinuitExtendedVertex->mnparm(2,"z",seedZ,5.0,fZmin,fZmax,err);
  fMinuitExtendedVertex->mnparm(3,"theta",seedTheta,0.125*TMath::Pi(),-1.0*TMath::Pi(),2.0*TMath::Pi(),err); 
  fMinuitExtendedVertex->mnparm(4,"phi",seedPhi,0.125*TMath::Pi(),-2.0*TMath::Pi(), 2.0*TMath::Pi(),err);
  
  flag = fMinuitExtendedVertex->Migrad();
  
  fMinuitExtendedVertex->GetParameter(0,fitXpos,fitXposErr);
  fMinuitExtendedVertex->GetParameter(1,fitYpos,fitYposErr);
  fMinuitExtendedVertex->GetParameter(2,fitZpos,fitZposErr);
  fMinuitExtendedVertex->GetParameter(3,fitTheta,fitThetaErr);
  fMinuitExtendedVertex->GetParameter(4,fitPhi,fitPhiErr);

  delete [] arglist;
  
  //correct angles, JW
  if(fitTheta < 0.0) fitTheta = -1.0 * fitTheta;
  if(fitTheta > TMath::Pi()) fitTheta = 2.0*TMath::Pi() - fitTheta;
  if(fitPhi < -1.0*TMath::Pi()) fitPhi += 2.0*TMath::Pi();
  if(fitPhi > TMath::Pi()) fitPhi -= 2.0*TMath::Pi();
  
  // sort results
  // ============
  fVtxX = fitXpos; 
  fVtxY = fitYpos;
  fVtxZ = fitZpos;
  fDirX = sin(fitTheta)*cos(fitPhi);
  fDirY = sin(fitTheta)*sin(fitPhi);
  fDirZ = cos(fitTheta);  
  
  fVtxFOM = 0.0;
  
  fPass = 0;               // flag = 0: normal termination
  if( flag==0 ) fPass = 1; // anything else: abnormal termination 

  fItr = extended_vertex_iterations();
  
  // calculate vertex
  // ================
  this->ExtendedVertexChi2(fVtxX,fVtxY,fVtxZ,
                           fDirX,fDirY,fDirZ, 
                           fConeAngle,fVtxTime,fVtxFOM);
                           
  // set vertex and direction
  // ========================
  fFittedVtx->SetVertex(fVtxX,fVtxY,fVtxZ,fVtxTime);
  fFittedVtx->SetDirection(fDirX,fDirY,fDirZ);
  fFittedVtx->SetConeAngle(fConeAngle);
  fFittedVtx->SetFOM(fVtxFOM,fItr,fPass);
  
  // set status
  // ==========
  bool inside_det =  ANNIEGeometry::Instance()->InsideDetector(fVtxX,fVtxY,fVtxZ);
  if( !fPass || !inside_det) status |= RecoVertex::kFailExtendedVertex;
  fFittedVtx->SetStatus(status);
  if( fPrintLevel >= 0) {
    if( !fPass ) std::cout << "   <warning> extended vertex fit failed to converge! Error code: " << flag << std::endl;
  }
  // return vertex
  // =============  
  return;
}

void MinuitOptimizer::FitCorrectedVertexWithMinuit() {
	// initialization
  // ==============
  double vtxAngle = Parameters::CherenkovAngle();
  double vtxFOM = 0.0;
  // seed vertex
  // ===========
  bool foundSeed = ( fSeedVtx->FoundVertex() 
                    && fSeedVtx->FoundDirection() );

  double seedX = fSeedVtx->GetPosition().X();
  double seedY = fSeedVtx->GetPosition().Y();
  double seedZ = fSeedVtx->GetPosition().Z();
  double seedT = fSeedVtx->GetTime();

  double seedDirX = fSeedVtx->GetDirection().X();
  double seedDirY = fSeedVtx->GetDirection().Y();
  double seedDirZ = fSeedVtx->GetDirection().Z();

  double seedTheta = acos(seedDirZ);
  double seedPhi = 0.0;

  if( seedDirX!=0.0 ){
    seedPhi = atan(seedDirY/seedDirX);
  }
  if( seedDirX<=0.0 ){
    if( seedDirY>0.0 ) seedPhi += TMath::Pi();
    if( seedDirY<0.0 ) seedPhi -= TMath::Pi();
  }
  // current status
  // ==============
  int status = fSeedVtx->GetStatus();
  
  // reset counter
  // =============
  corrected_vertex_reset_itr();
  
  // abort if necessary
  // ==================
  if( foundSeed==0 ){
  	if( fPrintLevel >= 0) {
      std::cout << "   <warning> corrected vertex fit failed to find input vertex " << std::endl;   
    }
    status |= RecoVertex::kFailCorrectedVertex;
    fFittedVtx->SetStatus(status);
    return;
  }
  
  // run Minuit
  // ==========  
  // one-parameter fit to vertex parallel position

  int err = 0;
  int flag = 0;

  double fitParallelcor = 0.0;
  double fitTheta = 0.0;
  double fitPhi = 0.0;

  double fitParallelcorErr = 0.0;
  double fitThetaErr = 0.0;
  double fitPhiErr = 0.0;
  
  double* arglist = new double[10];
  arglist[0]=2;  // 1: standard minimization
                 // 2: try to improve minimum
  
   // re-initialize everything...
  fMinuitCorrectedVertex->mncler();
  fMinuitCorrectedVertex->SetFCN(corrected_vertex_chi2);
  fMinuitCorrectedVertex->mnset();
  fMinuitCorrectedVertex->mnexcm("SET STR",arglist,1,err);
  fMinuitCorrectedVertex->mnparm(0,"dl",0.0, 5.0, -50.0, 50.0, err);
//  fMinuitCorrectedVertex->mnparm(1,"theta",seedTheta,0.125*TMath::Pi(),0.0,TMath::Pi(),err); //initial: 0.125*TMath::Pi(),0.0,TMath::Pi()
//  fMinuitCorrectedVertex->mnparm(2,"phi",seedPhi,0.25*TMath::Pi(),-1.0*TMath::Pi(),+3.0*TMath::Pi(),err); //initial: 0.25*TMath::Pi(),-1.0*TMath::Pi(),+3.0*TMath::Pi()
//  fMinuitCorrectedVertex->FixParameter(1);
//  fMinuitCorrectedVertex->FixParameter(2);
  flag = fMinuitCorrectedVertex->Migrad();
  
  fMinuitCorrectedVertex->GetParameter(0,fitParallelcor,fitParallelcorErr);

  delete [] arglist;
  
   // sort results
  // ============
//  fDirX = sin(fitTheta)*cos(fitPhi);
//  fDirY = sin(fitTheta)*sin(fitPhi);
//  fDirZ = cos(fitTheta);
  fVtxX += fitParallelcor * fDirX; 
  fVtxY += fitParallelcor * fDirY;
  fVtxZ += fitParallelcor * fDirZ;
  
  fVtxFOM = 0.0;
  
  fPass = 0;               // flag = 0: normal termination
  if( flag==0 ) fPass = 1; // anything else: abnormal termination 

  fItr = corrected_vertex_iterations();
  	
  // calculate vertex
  // ================
  this->CorrectedVertexChi2(fVtxX,fVtxY,fVtxZ,
                           fDirX,fDirY,fDirZ, 
                           fConeAngle,fVtxTime,fVtxFOM); //fit vertex time here
                           
  // set vertex and direction
  // ========================
  fFittedVtx->SetVertex(fVtxX,fVtxY,fVtxZ,fVtxTime);
  fFittedVtx->SetDirection(fDirX,fDirY,fDirZ);
  fFittedVtx->SetConeAngle(fConeAngle);
  fFittedVtx->SetFOM(fVtxFOM,fItr,fPass);
  // set status
  // ==========
  bool inside_det =  ANNIEGeometry::Instance()->InsideDetector(fVtxX,fVtxY,fVtxZ);
  if( !fPass || ! inside_det) status |= RecoVertex::kFailExtendedVertex;
  fFittedVtx->SetStatus(status);
  if( fPrintLevel >= 0) {
    if( !fPass ) std::cout << "   <warning> corrected vertex fit failed to converge! Error code: " << flag << std::endl;
  }
  // return vertex
  // =============  
  return;
}


//Given the position of the point vertex (x, y, z) and n digits, calculate the mean expected vertex time
double MinuitOptimizer::FindSimpleTimeProperties(VertexGeometry* vtxgeo) {
	double meanTime = 0.0;
	// weighted average
	if(fMeanTimeCalculatorType == 0) {
    // calculate mean and rms of hits inside cone
    // ==========================================
    double Swx = 0.0;
    double Sw = 0.0;
    
    double delta = 0.0;
    double sigma = 0.0;
    double weight = 0.0;
    double deweight = 0.0;
    double deltaAngle = 0.0;
    
    double myConeEdge = Parameters::CherenkovAngle();      // [degrees]
    double myConeEdgeSigma = 7.0;  // [degrees]
    
    for( int idigit=0; idigit<vtxgeo->GetNDigits(); idigit++ ){
      int detType = this->fVtxGeo->GetDigitType(idigit); 
      if( vtxgeo->IsFiltered(idigit) ){
        delta = vtxgeo->GetDelta(idigit);    
        sigma = vtxgeo->GetDeltaSigma(idigit);
        weight = 1.0/(sigma*sigma); 
        // profile in angle
        deltaAngle = vtxgeo->GetAngle(idigit) - myConeEdge;      
        // deweight hits outside cone
        if( deltaAngle<=0.0 ){
          deweight = 1.0;
        }
        else{
          deweight = 1.0/(1.0+(deltaAngle*deltaAngle)/(myConeEdgeSigma*myConeEdgeSigma));
        }
        Swx += deweight*weight*delta; //delta is expected vertex time 
        Sw  += deweight*weight;
      }
    }
    if( Sw>0.0 ){
      meanTime = Swx*1.0/Sw;
    }
	}
	
	// most probable time
	if(fMeanTimeCalculatorType == 1) {
		double sigma = 0.0;
		double deltaAngle = 0.0;
		double weight = 0.0;
		double deweight = 0.0;
		double myConeEdge = Parameters::CherenkovAngle();      // [degrees]
    double myConeEdgeSigma = 7.0;  // [degrees]
		vector<double> deltaTime1;
		vector<double> deltaTime2;
		vector<double> TimeWeight;
		
		for( int idigit=0; idigit<vtxgeo->GetNDigits(); idigit++ ){
      if(vtxgeo->IsFiltered(idigit)){
        deltaTime1.push_back(vtxgeo->GetDelta(idigit));  
        deltaTime2.push_back(vtxgeo->GetDelta(idigit));   
        sigma = vtxgeo->GetDeltaSigma(idigit);
        weight = 1.0/(sigma*sigma); 
        deltaAngle = vtxgeo->GetAngle(idigit) - myConeEdge;
        if( deltaAngle<=0.0 ){
          deweight = 1.0;
        }
        else{
          deweight = 1.0/(1.0+(deltaAngle*deltaAngle)/(myConeEdgeSigma*myConeEdgeSigma));
        }
        TimeWeight.push_back(deweight*weight);
      }
    }
    int n = deltaTime1.size();
    std::sort(deltaTime1.begin(),deltaTime1.end());
    double timeMin = deltaTime1.at(int((n-1)*0.05)); // 5% of the total entries
    double timeMax = deltaTime1.at(int((n-1)*0.90)); // 90% of the total entries
    int nbins = int(n/5);
    TH1D *hDeltaTime = new TH1D("hDeltaTime", "hDeltaTime", nbins, timeMin, timeMax);
    for(int i=0; i<n; i++) {
      //hDeltaTime->Fill(deltaTime2.at(i), TimeWeight.at(i));	
      hDeltaTime->Fill(deltaTime2.at(i));	
    }
    meanTime = hDeltaTime->GetBinCenter(hDeltaTime->GetMaximumBin());
    delete hDeltaTime; hDeltaTime = 0;
	}
	
	else std::cout<<"MinuitOptimizer Error: Wrong type of Mean time calculator! "<<std::endl;
  
  return meanTime; //return expected vertex time
}

void MinuitOptimizer::PointPositionChi2(double vtxX, double vtxY, double vtxZ, double& vtxTime, double& fom)
{  
  // figure of merit
  // ===============
  double vtxFOM = 0.0;
  double penaltyFOM = 0.0;
  double fixPositionFOM = 0.0;
  
  // calculate residuals
  // ===================
  this->fVtxGeo->CalcPointResiduals(vtxX, vtxY, vtxZ, 0.0, 0.0, 0.0, 0.0); //calculate expected point vertex time for each digit

  // calculate figure of merit
  // =========================
  double vtxParam0 = this->fFixTimeParam0;
  this->TimePropertiesLnL(vtxTime,vtxParam0, vtxFOM);

  // calculate penalty
  // =================
  //this->PenaltyChi2(vtxX,vtxY,vtxZ,penaltyFOM);

  // calculate overall figure of merit
  // =================================
  fom = vtxFOM + penaltyFOM + fixPositionFOM;
  // truncate
  if( fom<-999.999*fBaseFOM ) fom = -999.999*fBaseFOM;

  return;
}

void MinuitOptimizer::PointDirectionChi2(double vtxX, double vtxY, 
	                                       double vtxZ, double dirX, double dirY, double dirZ, 
	                                       double& vtxAngle, double& fom)
{  
  // figure of merit
  // ===============
  double coneFOM = 0.0;
  double fixPositionFOM = 0.0;
  
  // calculate residuals
  // ===================
  this->fVtxGeo->CalcPointResiduals(vtxX, vtxY, vtxZ, 0.0, 
                                 dirX, dirY, dirZ); //load expected vertex time for each digit

  // calculate figure of merit
  // =========================
  //this->FitPointConePropertiesLnL(vtxAngle,coneFOM);
  this->FitConePropertiesFoM(vtxAngle,coneFOM);

  // calculate overall figure of merit
  // =================================
  fom = coneFOM;

  // truncate
  if( fom<-999.999*fBaseFOM ) fom = -999.999*fBaseFOM;

  return;
}

void MinuitOptimizer::PointVertexChi2(double vtxX, double vtxY, double vtxZ, 
	                                    double dirX, double dirY, double dirZ, 
	                                    double& vtxAngle, double& vtxTime, double& fom)
{  
  // figure of merit
  // ===============
  double vtxFOM = 0.0;
  double penaltyFOM = 0.0;
  double fixPositionFOM = 0.0;
  double fixDirectionFOM = 0.0;

  // calculate residuals
  // ===================
  this->fVtxGeo->CalcPointResiduals(vtxX, vtxY, vtxZ, 0.0, 
                                 dirX, dirY, dirZ); //calculate expected vertex time for each digit
  // calculate figure of merit
  // =========================
  double timeFOM = 0.0;
  double coneFOM = 0.0;
  
  //this->FitPointConePropertiesLnL(vtxAngle, coneFOM);
  this->FitConePropertiesFoM(vtxAngle,coneFOM);
  this->FitPointTimePropertiesLnL(vtxTime, timeFOM);
  
  double fTimeFitWeight = this->fTimeFitWeight;
  double fConeFitWeight = this->fConeFitWeight;
  vtxFOM = (fTimeFitWeight*timeFOM+fConeFitWeight*coneFOM)/(fTimeFitWeight+fConeFitWeight);

  // calculate penalty
  // =================
  //this->PenaltyChi2(vtxX,vtxY,vtxZ,penaltyFOM);

  // calculate overall figure of merit
  // =================================
  
  fom = vtxFOM + penaltyFOM + fixPositionFOM + fixDirectionFOM;
  // truncate
  if( fom<-999.999*fBaseFOM ) fom = -999.999*fBaseFOM;

  return;
}

void MinuitOptimizer::ExtendedVertexChi2(double vtxX, double vtxY, double vtxZ, double dirX, double dirY, double dirZ, double& vtxAngle, double& vtxTime, double& fom)
{  
  // figure of merit
  // ===============
  double vtxFOM = 0.0;
  double timeFOM = 0.0;
  double coneFOM = 0.0;
  double penaltyFOM = 0.0;
  double fixPositionFOM = 0.0;
  double fixDirectionFOM = 0.0;

  // calculate residuals
  // ===================
  this->fVtxGeo->CalcExtendedResiduals(vtxX,vtxY,vtxZ,0.0,dirX,dirY,dirZ);
  
  // calculate figure of merit
  // =========================

  this->FitConePropertiesFoM(vtxAngle,coneFOM);
  this->FitExtendedTimePropertiesLnL(vtxTime,timeFOM);
  
  double fTimeFitWeight = this->fTimeFitWeight;
  double fConeFitWeight = this->fConeFitWeight;
  vtxFOM = (fTimeFitWeight*timeFOM+fConeFitWeight*coneFOM)/(fTimeFitWeight+fConeFitWeight);

  // calculate penalty
  // =================
  //this->PenaltyChi2(vtxX,vtxY,vtxZ,penaltyFOM);

  // calculate overall figure of merit
  // =================================
  fom = vtxFOM + penaltyFOM + fixPositionFOM + fixDirectionFOM;

  // truncate
  if( fom<-999.999*fBaseFOM ) fom = -999.999*fBaseFOM;

  return;
}

void MinuitOptimizer::CorrectedVertexChi2(double vtxX, double vtxY, double vtxZ, double dirX, double dirY, double dirZ, double& vtxAngle, double& vtxTime, double& fom)
{  
  // figure of merit
  // ===============
  double vtxFOM = 0.0;
  double timeFOM = 0.0;
  double coneFOM = 0.0;
  double penaltyFOM = 0.0;
  double fixPositionFOM = 0.0;
  double fixDirectionFOM = 0.0;

  // calculate residuals
  // ===================
  this->fVtxGeo->CalcExtendedResiduals(vtxX,vtxY,vtxZ,0.0,dirX,dirY,dirZ);
  
  // calculate figure of merit
  // =========================

  this->FitConePropertiesFoM(vtxAngle,coneFOM);
  fom = coneFOM;

  // truncate
  if( fom<-999.999*fBaseFOM ) fom = -999.999*fBaseFOM;

  return;
}




