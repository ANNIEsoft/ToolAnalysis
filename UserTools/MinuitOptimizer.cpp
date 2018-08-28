/*************************************************************************
    > File Name: MinuitOptimizer.cc
    > Author: Jingbo Wang
    > mail: jiowang@ucdavis.edu, wjingbo@anl.gov
    > Created Time: APR 12 15:36:00 2017
 ************************************************************************/

#include "MinuitOptimizer.hh"
#include "VertexGeometryOpt.hh"

#include "Interface.hh"
#include "ANNIEGeometry.hh"
#include "Parameters.hh"

#include "RecoDigit.hh"
#include "RecoEvent.hh"
#include "TrueEvent.hh"

#include "TStopwatch.h" // <----------
#include "TMath.h"

#include <cmath>
#include <iostream>
#include <iomanip>
#include <cassert>
using namespace std;

ClassImp(MinuitOptimizer)

static MinuitOptimizer* fgMinuitOptimizer = 0;
static void vertex_time_lnl(Int_t&, Double_t*, Double_t& f, Double_t* par, Int_t)
{  

  Bool_t printDebugMessages = 0;
  
  Double_t vtxTime = par[0]; // nanoseconds
//  Double_t vtxParam0 = fgMinuitOptimizer->fFixTimeParam0;
  Double_t vtxParam0 = fgMinuitOptimizer->fVtxFinder->fFixTimeParam0; //scatterring parameter
  if( fgMinuitOptimizer->fVtxFinder->fFitTimeParams ){
    vtxParam0 = par[1]; 
  }
  Double_t fom = 0.0;
  fgMinuitOptimizer->time_fit_itr();  
  fgMinuitOptimizer->TimePropertiesLnL(vtxTime,vtxParam0, fom);
  f = -fom; // note: need to maximize this fom
  if( printDebugMessages ){
    std::cout << "  [vertex_time_lnl] [" << fgMinuitOptimizer->time_fit_iterations() << "] vtime=" << vtxTime << " vparam=" << vtxParam0 << " fom=" << fom << std::endl;
  }
  return;
}

static void point_position_chi2(Int_t&, Double_t*, Double_t& f, Double_t* par, Int_t)
{
  Bool_t printDebugMessages = 0;

  Double_t vtxX = par[0]; // centimetres
  Double_t vtxY = par[1]; // centimetres
  Double_t vtxZ = par[2]; // centimetres
  Double_t vtxTime = par[3]; //ns, added by JW
  Double_t vtxParam0 = 0.2; //scatterring parameter

//  Double_t vtime  = 0.0;
  Double_t fom = 0.0;
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

static void point_direction_chi2(Int_t&, Double_t*, Double_t& f, Double_t* par, Int_t)
{
  Bool_t printDebugMessages = 0;
  
  Double_t vtxX = fgMinuitOptimizer->fVtxX;
  Double_t vtxY = fgMinuitOptimizer->fVtxY;
  Double_t vtxZ = fgMinuitOptimizer->fVtxZ;
  
  Double_t dirX = 0.0;
  Double_t dirY = 0.0;
  Double_t dirZ = 0.0;

  Double_t dirTheta = par[0]; // radians
  Double_t dirPhi   = par[1]; // radians
  
  dirX = sin(dirTheta)*cos(dirPhi);
  dirY= sin(dirTheta)*sin(dirPhi);
  dirZ = cos(dirTheta);

  Double_t vangle = 0.0;
  Double_t fom = 0.0;

  fgMinuitOptimizer->point_direction_itr();
  fgMinuitOptimizer->PointDirectionChi2(vtxX,vtxY,vtxZ,
                                     dirX,dirY,dirZ,
                                     vangle,fom);
//  fgMinuitOptimizer->fVtxGeo->CalcPointResiduals(vtxX, vtxY, vtxZ, 0.0, 
//                                 dirX, dirY, dirZ); //load expected vertex time for each digit
//  //Cone fit parameters
//  Double_t ConeParam0 = fgMinuitOptimizer->fVtxFinder->fFixConeParam0;
//  Double_t ConeParam1 = fgMinuitOptimizer->fVtxFinder->fFixConeParam1;
//  Double_t ConeParam2 = fgMinuitOptimizer->fVtxFinder->fFixConeParam2;
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

static void point_vertex_chi2(Int_t&, Double_t*, Double_t& f, Double_t* par, Int_t)
{
  Bool_t printDebugMessages = 0;
  
  Double_t vtxX     = par[0]; // centimetres
  Double_t vtxY     = par[1]; // centimetres
  Double_t vtxZ     = par[2]; // centimetres  
  
  Double_t dirTheta = par[3]; // radians
  Double_t dirPhi   = par[4]; // radians

  Double_t dirX = sin(dirTheta)*cos(dirPhi);
  Double_t dirY = sin(dirTheta)*sin(dirPhi);
  Double_t dirZ = cos(dirTheta);

  Double_t vangle = 0.0;
  Double_t vtime = 0.0;
  Double_t fom = 0.0;
  
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

static void extended_vertex_chi2(Int_t&, Double_t*, Double_t& f, Double_t* par, Int_t)
{
  Bool_t printDebugMessages = 0;
  
  Double_t vtxX     = par[0]; // centimetres
  Double_t vtxY     = par[1]; // centimetres
  Double_t vtxZ     = par[2]; // centimetres  

  Double_t dirTheta = par[3]; // radians
  Double_t dirPhi   = par[4]; // radians

  Double_t dirX = sin(dirTheta)*cos(dirPhi);
  Double_t dirY = sin(dirTheta)*sin(dirPhi);
  Double_t dirZ = cos(dirTheta);

  Double_t vangle = 0.0;
  Double_t vtime  = 0.0;
  Double_t fom = 0.0;

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

static void corrected_vertex_chi2(Int_t&, Double_t*, Double_t& f, Double_t* par, Int_t)
{
  Bool_t printDebugMessages = 0;
  
  Double_t dl = par[0];
//  Double_t dirTheta = par[1]; // radians
//  Double_t dirPhi   = par[2]; // radiansf
  
//  Double_t dirX = sin(dirTheta)*cos(dirPhi);
//  Double_t dirY = sin(dirTheta)*sin(dirPhi);
//  Double_t dirZ = cos(dirTheta);
  
  Double_t dirX = fgMinuitOptimizer->fDirX;
  Double_t dirY = fgMinuitOptimizer->fDirY;
  Double_t dirZ = fgMinuitOptimizer->fDirZ;
  
  Double_t vtxX = fgMinuitOptimizer->fVtxX + dl * dirX;
  Double_t vtxY = fgMinuitOptimizer->fVtxY + dl * dirY;;
  Double_t vtxZ = fgMinuitOptimizer->fVtxZ + dl * dirZ;

  Double_t vangle = 0.0;
  Double_t vtime  = 0.0;
  Double_t fom = 0.0;

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

static void vertex_cone_lnl(Int_t&, Double_t*, Double_t& f, Double_t* par, Int_t)
{
  Bool_t printDebugMessages = 0;
  
  Double_t vtxParam0 = fgMinuitOptimizer->fVtxFinder->fFixConeParam0;
  Double_t vtxParam1 = fgMinuitOptimizer->fVtxFinder->fFixConeParam1;
  Double_t vtxParam2 = fgMinuitOptimizer->fVtxFinder->fFixConeParam2;

  if( fgMinuitOptimizer->fVtxFinder->fFitConeParams ){
    vtxParam0 = par[0];
    vtxParam1 = par[1];
    vtxParam2 = par[2];
  }
 
  Double_t vangle = 0.0;
  Double_t fom = 0.0;

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
	fVtxFinder = 0;
	fSeedVtx = 0;
	fVtxGeo = 0;
	fFittedVtx = new RecoVertex();
	fVtxGeo = 0;
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
//  WCsim	
	fTmin = 6600.0;
	fTmax = 6800.0;
	
//	//Sandbox
//	fTmin = -10.0;
//	fTmax = 20.0;	
	
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
	fVtxFinder = 0;
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

//Load VertexFinder to get access to all the reconstruction parameters
void MinuitOptimizer::LoadVertexFinder(VertexFinderOpt* vtxfinder) {
	fVtxFinder = vtxfinder;
} 
	
	
//100 digits
void MinuitOptimizer::LoadVertexGeometry(VertexGeometryOpt* vtxgeo) {
  this->fVtxGeo = vtxgeo;	
}

//Load vertex
void MinuitOptimizer::LoadVertex(RecoVertex* vtx) {
  this->fSeedVtx = vtx;
  this->fVtxX = vtx->GetX();
  this->fVtxY = vtx->GetY();
  this->fVtxZ = vtx->GetZ();
  this->fVtxTime = vtx->GetTime();
  this->fDirX = vtx->GetDirX();
  this->fDirY = vtx->GetDirY();
  this->fDirZ = vtx->GetDirZ();
  this->fFoundVertex = vtx->FoundVertex();
}

//Load vertex
void MinuitOptimizer::LoadVertex(Double_t vtxX, Double_t vtxY, Double_t vtxZ, Double_t vtxTime, Double_t vtxDirX, Double_t vtxDirY, Double_t vtxDirZ) {
  this->fVtxX = vtxX;
  this->fVtxY = vtxY;
  this->fVtxZ = vtxZ;
  this->fVtxTime = vtxTime;
  this->fDirX = vtxDirX;
  this->fDirY = vtxDirY;
  this->fDirZ = vtxDirZ;
}

void MinuitOptimizer::TimePropertiesLnL(Double_t vtxTime, Double_t vtxParam, Double_t& vtxFOM)
{ 
  // nuisance parameters
  // ===================
  Double_t scatter = vtxParam; 

  // internal variables
  // ==================
  Double_t weight = 0.0;
  Double_t delta = 0.0;       // time residual of each hit
  Double_t sigma = 0.0;       // time resolution of each hit
  Double_t A = 0.0;           // normalisation of first Gaussian

  Double_t Preal = 0.0;       // probability of real hit
  Double_t P = 0.0;           // probability of hit

  Double_t chi2 = 0.0;        // log-likelihood: chi2 = -2.0*log(L)
  Double_t ndof = 0.0;        // total number of hits
  Double_t fom = 0.0;         // figure of merit
  Double_t coneAngle = Parameters::CherenkovAngle();
  Double_t myConeAngleSigma = 2.0;
  Double_t deltaAngle = 0.0;

  // tuning parameters
  // =================
  Double_t fTimeFitNoiseRate = 0.02;  // hits/ns [0.40 for electrons, 0.02 for muons]
  // add noise to model
  // ==================
  Double_t nFilterDigits = this->fVtxGeo->GetNFilterDigits(); //FIXME
  Double_t Pnoise = fTimeFitNoiseRate/nFilterDigits;//FIXME
  Pnoise = 1e-8;//FIXME
  
  //Bool_t istruehits = (Interface::Instance())->IsTrueHits();
  Bool_t istruehits = 0; // for test only. JW

  // loop over digits
  // ================
  
  TString configType = Parameters::Instance()->GetConfigurationType();
  	
  if(configType == "LAPPD")	{
    for( Int_t idigit=0; idigit<this->fVtxGeo->GetNDigits(); idigit++ ){    
    	TString detType = this->fVtxGeo->GetDigitType(idigit); 
      if( detType == "lappd_v0" /*&& this->fVtxGeo->IsFiltered(idigit)*/){
        delta = this->fVtxGeo->GetDelta(idigit) - vtxTime;
        sigma = 1.2 * this->fVtxGeo->GetDeltaSigma(idigit); //chose factor 1.2 to optimize the fitter performance
        A  = 1.0 / ( 2.0*sigma*sqrt(0.5*TMath::Pi()) ); //normalisation constant
        Preal = A*exp(-(delta*delta)/(2.0*sigma*sigma));
        P = (1.0-Pnoise)*Preal + Pnoise; 
        chi2 += -2.0*log(P);
        ndof += 1.0; 
      }
    }
  }
  
  else if(configType == "PMT") {
    for( Int_t idigit=0; idigit<this->fVtxGeo->GetNDigits(); idigit++ ){    
    	TString detType = this->fVtxGeo->GetDigitType(idigit); 
      if( detType == "PMT8inch" && this->fVtxGeo->GetDigitQ(idigit)>5/*&& this->fVtxGeo->IsFiltered(idigit)*/){
        delta = this->fVtxGeo->GetDelta(idigit) - vtxTime;
        sigma = 1.5 * this->fVtxGeo->GetDeltaSigma(idigit); //chose factor 1.2 to optimize the fitter performance
        A  = 1.0 / ( 2.0*sigma*sqrt(0.5*TMath::Pi()) ); //normalisation constant
        Preal = A*exp(-(delta*delta)/(2.0*sigma*sigma));
        P = (1.0-Pnoise)*Preal + Pnoise; 
        chi2 += -2.0*log(P);
        ndof += 1.0; 
      }
    }	
  }
  
  else if(configType == "Combined") {
    for( Int_t idigit=0; idigit<this->fVtxGeo->GetNDigits(); idigit++ ){    
    	TString detType = this->fVtxGeo->GetDigitType(idigit); 
      delta = this->fVtxGeo->GetDelta(idigit) - vtxTime;
      sigma = this->fVtxGeo->GetDeltaSigma(idigit);
      if(detType == "lappd_v0") sigma = 1.2*sigma;
      if(detType == "PMT8inch") sigma = 1.5*sigma;
      A  = 1.0 / ( 2.0*sigma*sqrt(0.5*TMath::Pi()) ); //normalisation constant
      Preal = A*exp(-(delta*delta)/(2.0*sigma*sigma));
      P = (1.0-Pnoise)*Preal + Pnoise; 
      chi2 += -2.0*log(P);
      ndof += 1.0; 
    }	
  }
  
  else if (configType == "Two-stage") {
    for( Int_t idigit=0; idigit<this->fVtxGeo->GetNDigits(); idigit++ ){    
    	TString detType = this->fVtxGeo->GetDigitType(idigit); 
      if( detType == "lappd_v0" /*&& this->fVtxGeo->IsFiltered(idigit)*/){
        delta = this->fVtxGeo->GetDelta(idigit) - vtxTime;
        sigma = 1.2 * this->fVtxGeo->GetDeltaSigma(idigit); //chose factor 1.2 to optimize the fitter performance
        A  = 1.0 / ( 2.0*sigma*sqrt(0.5*TMath::Pi()) ); //normalisation constant
        Preal = A*exp(-(delta*delta)/(2.0*sigma*sigma));
        P = (1.0-Pnoise)*Preal + Pnoise; 
        chi2 += -2.0*log(P);
        ndof += 1.0; 
      }
    }	
  }
  

  // calculate figure of merit
  if( ndof>0.0 ){
    fom = fBaseFOM - 5.0*chi2/ndof;
  }  

  // return figure of merit
  // ======================
  vtxFOM = fom;
  

  return;
}

void MinuitOptimizer::ConePropertiesFoM(Double_t& coneFOM, std::string fitOption)
{  
  // calculate figure of merit
  // =========================
  Double_t coneEdge = Parameters::CherenkovAngle();     // nominal cone angle
  Double_t coneEdgeLow = 21.0;  // cone edge (low side)      
  Double_t coneEdgeHigh = 3.0;  // cone edge (high side)   [muons: 3.0, electrons: 7.0]

  Double_t deltaAngle = 0.0;
  Double_t digitCharge = 0.0;
 
  Double_t coneCharge = 0.0;
  Double_t allCharge = 0.0;

  Double_t fom = 0.0;

  for( Int_t idigit=0; idigit<this->fVtxGeo->GetNDigits(); idigit++ ){
  	if(fitOption == "LAPPD" ) {
  	  if(this->fVtxGeo->GetDigitType(idigit) != "lappd_v0") continue;	
  	}
  	if(fitOption == "PMT") {
  	  if(this->fVtxGeo->GetDigitType(idigit) != "PMT8inch" || this->fVtxGeo->GetDigitQ(idigit)<5) continue;	
  	}
  	
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

void MinuitOptimizer::ConePropertiesLnL(Double_t coneParam0, Double_t coneParam1, Double_t coneParam2, Double_t& coneAngle, Double_t& coneFOM)
{  
  
  // nuisance parameters
  // ===================
  Double_t alpha  = coneParam0; // track length parameter = 0.25
  Double_t alpha0 = coneParam1; // track length parameter = 0.5
  Double_t beta   = coneParam2; // particle ID:  0.0[electron]->1.0[muon] = 0.75

  // internal variables
  // ==================
  Double_t deltaAngle = 0.0; //
  Double_t sigmaAngle = 7.0; //Cherenkov Angle resolution
  Double_t Angle0 = Parameters::CherenkovAngle(); //Cherenkov Angle: 42 degree
  Double_t deltaAngle0 = Angle0*alpha0; //?
  
  Double_t digitQ = 0.0;
  Double_t sigmaQmin = 1.0;
  Double_t sigmaQmax = 10.0;
  Double_t sigmaQ = 0.0;

  Double_t A = 0.0;
  
  Double_t PconeA = 0.0;
  Double_t PconeB = 0.0;
  Double_t Pmu = 0.0;
  Double_t Pel = 0.0;

  Double_t Pcharge = 0.0;
  Double_t Pangle = 0.0;
  Double_t P = 0.0;

  Double_t chi2 = 0.0;
  Double_t ndof = 0.0;

  Double_t angle = 46.0;
  Double_t fom = 0.0;

  // hard-coded parameters: 200 kton (100 kton)
  // ==========================================
  Double_t lambdaMuShort = 0.5; //  0.5;
  Double_t lambdaMuLong  = 5.0; // 15.0;
  Double_t alphaMu =       1.0; //  4.5;

  Double_t lambdaElShort = 1.0; //  2.5;
  Double_t lambdaElLong =  7.5; // 15.0;
  Double_t alphaEl =       6.0; //  3.5;

  // numerical integrals
  // ===================
  fSconeA = 21.9938;  
  fSconeB =  0.0000;
  
  // Number of P.E. angular distribution
  // inside cone
  Int_t nbinsInside = 420; //divide the angle range by 420 bins
  for( Int_t n=0; n<nbinsInside; n++ ){
    deltaAngle = -Angle0 + (n+0.5)*(Angle0/(double)nbinsInside); // angle axis
    fSconeB += 1.4944765*sin( (Angle0+deltaAngle)*(TMath::Pi()/180.0) )
                           *( 1.0/(1.0+(deltaAngle*deltaAngle)/(deltaAngle0*deltaAngle0)) )
                           *( Angle0/(double)nbinsInside );
  }

  // outside cone
  if( fIntegralsDone == 0 ){ // default 0
    fSmu = 0.0;
    fSel = 0.0;

    Int_t nbinsOutside = 1380;
    for( Int_t n=0; n<nbinsOutside; n++ ){
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
  for( Int_t idigit=0; idigit<this->fVtxGeo->GetNDigits(); idigit++ ){

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
void MinuitOptimizer::FitPointTimePropertiesLnL(Double_t& vtxTime, Double_t& vtxFOM)
{
  // calculate mean and rms
  // ====================== 
  Double_t meanvtxTime = 0.0;
  meanvtxTime = this->FindSimpleTimeProperties(this->fVtxGeo);  //returns weighted average of the expected vertex time
  // reset counter
  // =============
  time_fit_reset_itr();
  
  // run Minuit
  // ==========  
  // one-parameter fit to time profile

  Int_t err = 0;
  Int_t flag = 0;

  Double_t seedTime = meanvtxTime;
  Double_t fitParam = fVtxFinder->fFixTimeParam0;
  Double_t fitTime = 0.0;
  Double_t fitTimeErr = 0.0;  
  
  Double_t* arglist = new Double_t[10];
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
  Double_t fom = -999.999*fBaseFOM;
  this->TimePropertiesLnL(fitTime,fitParam, fom);
  
  vtxTime = fitTime;
  vtxFOM = fom;
  
  fFittedVtx->SetVertex(fVtxX, fVtxY, fVtxZ, vtxTime);
  fFittedVtx->SetFOM(vtxFOM, 1, 1);
  return;
}

void MinuitOptimizer::FitExtendedTimePropertiesLnL(Double_t& vtxTime, Double_t& vtxFOM)
{    
  
  // return result from point fit
  // ============================
  if( fVtxFinder->fFitTimeParams==0 ){
    return this->FitPointTimePropertiesLnL(vtxTime,vtxFOM);
  }

  // calculate mean and rms
  // ====================== 
  Double_t meanTime = this->FindSimpleTimeProperties(this->fVtxGeo); 

  // reset counter
  // =============
  time_fit_reset_itr();

  // run Minuit
  // ==========  
  // two-parameter fit to time profile

  Int_t err = 0;
  Int_t flag = 0;

  Double_t seedTime = meanTime;
  Double_t seedParam = fVtxFinder->fFixTimeParam0;

  Double_t fitTime = seedTime;
  Double_t fitTimeErr = 0.0;  

  Double_t fitParam = seedParam;
  Double_t fitParamErr = 0.0;
  
  Double_t* arglist = new Double_t[10];
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
  Double_t fom = -999.999*fBaseFOM;

  this->TimePropertiesLnL(fitTime,fitParam,fom); 

  // return figure of merit
  // ======================
  vtxTime = fitTime;
  vtxFOM = fom;

  return;
}

void MinuitOptimizer::FitConePropertiesFoM(Double_t& coneAngle, Double_t& coneFOM, std::string fitOption)
{
  coneAngle = Parameters::CherenkovAngle(); //...chrom1.38 // nominal cone angle

  this->ConePropertiesFoM(coneFOM, fitOption);

  return;
}

void MinuitOptimizer::FitPointConePropertiesLnL(Double_t& coneAngle, Double_t& coneFOM)
{  
  coneAngle  = Parameters::CherenkovAngle(); //...chrom1.38, nominal cone angle
  // calculate cone fom
  // =========================
  coneFOM = 0.0;
  Double_t ConeParam0 = this->fVtxFinder->fFixConeParam0;
  Double_t ConeParam1 = this->fVtxFinder->fFixConeParam1;
  Double_t ConeParam2 = this->fVtxFinder->fFixConeParam2;
  this->ConePropertiesLnL(ConeParam0,ConeParam1,ConeParam2,coneAngle,coneFOM);  
  return;
}

void MinuitOptimizer::FitExtendedConePropertiesLnL(Double_t& coneAngle, Double_t& coneFOM)
{  
  // return result from point fit
  // ============================
  if( fVtxFinder->fFitConeParams==0 ){
    return this->FitPointConePropertiesLnL(coneAngle,coneFOM);
  }

  // reset counter
  // =============
  cone_fit_reset_itr();

  // run Minuit
  // ==========  
  // one-parameter fit to angular distribution

  Int_t err = 0;
  Int_t flag = 0;

  Double_t seedParam0 = fVtxFinder->fFixConeParam0;
  Double_t seedParam1 = fVtxFinder->fFixConeParam1;
  Double_t seedParam2 = fVtxFinder->fFixConeParam2;

  Double_t fitParam0 = seedParam0;
  Double_t fitParam1 = seedParam1;
  Double_t fitParam2 = seedParam2;

  Double_t fitParam0Err = 0.0;
  Double_t fitParam1Err = 0.0; 
  Double_t fitParam2Err = 0.0;

  Double_t fitAngle  = Parameters::CherenkovAngle();
  
  Double_t* arglist = new Double_t[10];
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
  Double_t fom = -999.999*fBaseFOM;
  
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
  Bool_t foundSeed = fSeedVtx->FoundVertex();
  Double_t seedX = fSeedVtx->GetX();
  Double_t seedY = fSeedVtx->GetY();
  Double_t seedZ = fSeedVtx->GetZ();
  Double_t seedTime = fSeedVtx->GetTime();
  
  // current status
  // ==============
  Int_t status = fSeedVtx->GetStatus();

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
  Int_t err = 0;
  Int_t flag = 0;

  Double_t fitXpos = 0.0;
  Double_t fitYpos = 0.0;
  Double_t fitZpos = 0.0;
  Double_t fitTimepos = 0.0; //JW

  Double_t fitXposErr = 0.0;
  Double_t fitYposErr = 0.0;
  Double_t fitZposErr = 0.0;
  Double_t fitTimeposErr = 0.0; //JW
  
  Double_t fitParam = 0.2; //scattering parameter

  Double_t* arglist = new Double_t[10];
  arglist[0]=1;  // 1: standard minimization
                 // 2: try to improve minimum
  
  // re-initialize everything...
  fMinuitPointPosition->mncler();
  fMinuitPointPosition->SetFCN(point_position_chi2);
  fMinuitPointPosition->mnexcm("SET STR",arglist,1,err);
//  fMinuitPointPosition->mnparm(0,"x",seedX,1.0,-152,152,err);
//  fMinuitPointPosition->mnparm(1,"y",seedY,1.0,-212.46,183.54,err);
//  fMinuitPointPosition->mnparm(2,"z",seedZ,1.0,15,319,err);
//  fMinuitPointPosition->mnparm(3,"Time",seedTime,10.0,6500.0,6700.0,err);
  fMinuitPointPosition->mnparm(0,"x",seedX,1.0,fXmin,fXmax,err);
  fMinuitPointPosition->mnparm(1,"y",seedY,1.0,fYmin,fYmax,err);
  fMinuitPointPosition->mnparm(2,"z",seedZ,5.0,fZmin,fZmax,err);
  fMinuitPointPosition->mnparm(3,"Time",seedTime,10.0,fTmin,fTmax,err);

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

  Double_t vtxFOM = 0.0;
  
  fPass = 0;               // flag = 0: normal termination
  if( flag==0 ) fPass = 1; // anything else: abnormal termination 

  fItr = point_position_iterations();
  // calculate vertex
  // ================
//  this->fVtxGeo->CalcPointResiduals(fVtxX, fVtxY, fVtxZ, 0.0, 0.0, 0.0, 0.0);
//  this->TimePropertiesLnL(fitTimepos,fitParam, vtxFOM); 
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
              << "     (vx,vy,vz)=(" << fFittedVtx->GetX() << "," << fFittedVtx->GetY() << "," << fFittedVtx->GetZ() << ") " << std::endl
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
  Bool_t foundSeed = ( fSeedVtx->FoundVertex() && fSeedVtx->FoundDirection() );
 
  Double_t vtxAngle = Parameters::CherenkovAngle();

  Double_t vtxFOM = 0.0;
  
  // seed direction
  // ==============
  Double_t seedDirX = fSeedVtx->GetDirX();
  Double_t seedDirY = fSeedVtx->GetDirY();
  Double_t seedDirZ = fSeedVtx->GetDirZ();
  
  Double_t seedTheta = acos(seedDirZ); //
  Double_t seedPhi = 0.0;

  if( seedDirX!=0.0 ){
    seedPhi = atan(seedDirY/seedDirX);
  }
  if( seedDirX<=0.0 ){
    if( seedDirY>0.0 ) seedPhi += TMath::Pi();
    if( seedDirY<0.0 ) seedPhi -= TMath::Pi();
  }  
  
  // current status
  // ==============
  Int_t status = fSeedVtx->GetStatus();

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
  
  Int_t err = 0;
  Int_t flag = 0;

  Double_t dirTheta;
  Double_t dirPhi;

  Double_t dirThetaErr;
  Double_t dirPhiErr;

  Double_t* arglist = new Double_t[10];
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
              << "    (vx,vy,vz)=(" << fFittedVtx->GetX() << "," << fFittedVtx->GetY() << "," << fFittedVtx->GetZ() << ") " << std::endl
              << "    (px,py,pz)=(" << fFittedVtx->GetDirX() << "," << fFittedVtx->GetDirY() << "," << fFittedVtx->GetDirZ() << ") " << std::endl
              << "      angle=" << fFittedVtx->GetConeAngle() << " vtime=" << fFittedVtx->GetTime() << " itr=" << fFittedVtx->GetIterations() << " fom=" << fFittedVtx->GetFOM() << std::endl;
  }
  // print fit info
  if(fPrintLevel >= 0) {
    if( !fPass ) std::cout << "   <warning> point direction fit failed to converge! Error code: " << flag << std::endl;
  }
  return;
}

void MinuitOptimizer::FitPointVertexWithMinuit() {
  
  Double_t vtxAngle = Parameters::CherenkovAngle();
  Double_t vtxFOM = 0.0;
  
  // seed vertex
  // ===========  
  Bool_t foundSeed = ( fSeedVtx->FoundVertex() 
                   && fSeedVtx->FoundDirection() );
                   
  Double_t seedX = fSeedVtx->GetX();
  Double_t seedY = fSeedVtx->GetY();
  Double_t seedZ = fSeedVtx->GetZ();
  Double_t seedTime = fSeedVtx->GetTime();
  
  seedTime = 0.0; // just for test

  Double_t seedDirX = fSeedVtx->GetDirX();
  Double_t seedDirY = fSeedVtx->GetDirY();
  Double_t seedDirZ = fSeedVtx->GetDirZ();

  Double_t seedTheta = acos(seedDirZ);
  Double_t seedPhi = 0.0;
  
  
  if( seedDirX!=0.0 ){
    seedPhi = atan(seedDirY/seedDirX);
  }
  if( seedDirX<=0.0 ){
    if( seedDirY>0.0 ) seedPhi += TMath::Pi();
    if( seedDirY<0.0 ) seedPhi -= TMath::Pi();
  }
  
  // current status
  // ==============
  Int_t status = fSeedVtx->GetStatus();

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

  Int_t err = 0;
  Int_t flag = 0;

  Double_t fitXpos = 0.0;
  Double_t fitYpos = 0.0;
  Double_t fitZpos = 0.0;
  Double_t fitTheta = 0.0;
  Double_t fitPhi = 0.0;

  Double_t fitXposErr = 0.0;
  Double_t fitYposErr = 0.0;
  Double_t fitZposErr = 0.0;
  Double_t fitThetaErr = 0.0;
  Double_t fitPhiErr = 0.0;
  
  Double_t* arglist = new Double_t[10];
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
              << "    (vx,vy,vz)=(" << fFittedVtx->GetX() << "," << fFittedVtx->GetY() << "," << fFittedVtx->GetZ() << ") " << std::endl
              << "    (px,py,pz)=(" << fFittedVtx->GetDirX() << "," << fFittedVtx->GetDirY() << "," << fFittedVtx->GetDirZ() << ") " << std::endl
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
  Double_t vtxAngle = Parameters::CherenkovAngle();
  Double_t vtxFOM = 0.0;
  // seed vertex
  // ===========
  Bool_t foundSeed = ( fSeedVtx->FoundVertex() 
                    && fSeedVtx->FoundDirection() );

  Double_t seedX = fSeedVtx->GetX();
  Double_t seedY = fSeedVtx->GetY();
  Double_t seedZ = fSeedVtx->GetZ();

  Double_t seedDirX = fSeedVtx->GetDirX();
  Double_t seedDirY = fSeedVtx->GetDirY();
  Double_t seedDirZ = fSeedVtx->GetDirZ();

  Double_t seedTheta = acos(seedDirZ);
  Double_t seedPhi = 0.0;
  
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
  Int_t status = fSeedVtx->GetStatus();

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

  Int_t err = 0;
  Int_t flag = 0;

  Double_t fitXpos = 0.0;
  Double_t fitYpos = 0.0;
  Double_t fitZpos = 0.0;
  Double_t fitTheta = 0.0;
  Double_t fitPhi = 0.0;

  Double_t fitXposErr = 0.0;
  Double_t fitYposErr = 0.0;
  Double_t fitZposErr = 0.0;
  Double_t fitThetaErr = 0.0;
  Double_t fitPhiErr = 0.0;
  
  Double_t* arglist = new Double_t[10];
  arglist[0]=2;  // 1: standard minimization
                 // 2: try to improve minimum
  
  // re-initialize everything...
  fMinuitExtendedVertex->mncler();
  fMinuitExtendedVertex->SetFCN(extended_vertex_chi2);
  fMinuitExtendedVertex->mnset();
  fMinuitExtendedVertex->mnexcm("SET STR",arglist,1,err);
//  fMinuitExtendedVertex->mnexcm("SET STR",arglist,1,err);
  fMinuitExtendedVertex->mnparm(0,"x",seedX,1.0,fXmin,fXmax,err);
  fMinuitExtendedVertex->mnparm(1,"y",seedY,1.0,fYmin,fYmax,err);
  fMinuitExtendedVertex->mnparm(2,"z",seedZ,5.0,fZmin,fZmax,err);
//  fMinuitExtendedVertex->mnparm(0,"x",seedX,5.0,seedX-50,seedX+50,err);
//  fMinuitExtendedVertex->mnparm(1,"y",seedY,5.0,seedY-50,seedY+50,err);
//  fMinuitExtendedVertex->mnparm(2,"z",seedZ,5.0,seedZ-50,seedZ+50,err);
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
  Double_t vtxAngle = Parameters::CherenkovAngle();
  Double_t vtxFOM = 0.0;
  // seed vertex
  // ===========
  Bool_t foundSeed = ( fSeedVtx->FoundVertex() 
                    && fSeedVtx->FoundDirection() );

  Double_t seedX = fSeedVtx->GetX();
  Double_t seedY = fSeedVtx->GetY();
  Double_t seedZ = fSeedVtx->GetZ();
  Double_t seedT = fSeedVtx->GetTime();

  Double_t seedDirX = fSeedVtx->GetDirX();
  Double_t seedDirY = fSeedVtx->GetDirY();
  Double_t seedDirZ = fSeedVtx->GetDirZ();

  Double_t seedTheta = acos(seedDirZ);
  Double_t seedPhi = 0.0;

  if( seedDirX!=0.0 ){
    seedPhi = atan(seedDirY/seedDirX);
  }
  if( seedDirX<=0.0 ){
    if( seedDirY>0.0 ) seedPhi += TMath::Pi();
    if( seedDirY<0.0 ) seedPhi -= TMath::Pi();
  }
  // current status
  // ==============
  Int_t status = fSeedVtx->GetStatus();
  
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

  Int_t err = 0;
  Int_t flag = 0;

  Double_t fitParallelcor = 0.0;
  Double_t fitTheta = 0.0;
  Double_t fitPhi = 0.0;

  Double_t fitParallelcorErr = 0.0;
  Double_t fitThetaErr = 0.0;
  Double_t fitPhiErr = 0.0;
  
  Double_t* arglist = new Double_t[10];
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
Double_t MinuitOptimizer::FindSimpleTimeProperties(VertexGeometryOpt* vtxgeo) {
  Double_t meanTime = 0.0;
  // calculate mean and rms of hits inside cone
  // ==========================================
  Double_t Swx = 0.0;
  Double_t Sw = 0.0;

  Double_t delta = 0.0;
  Double_t sigma = 0.0;
  Double_t weight = 0.0;
  Double_t deweight = 0.0;
  Double_t deltaAngle = 0.0;

  Double_t myConeEdge = Parameters::CherenkovAngle();      // [degrees]
  Double_t myConeEdgeSigma = 7.0;  // [degrees]
  TString configType = Parameters::Instance()->GetConfigurationType();
  
  if(configType == "LAPPD")	{
    for( Int_t idigit=0; idigit<vtxgeo->GetNDigits(); idigit++ ){
      TString detType = this->fVtxGeo->GetDigitType(idigit); 
      if( detType == "lappd_v0" && vtxgeo->IsFiltered(idigit) ){
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
  }
  
  if(configType == "PMT")	{
    for( Int_t idigit=0; idigit<vtxgeo->GetNDigits(); idigit++ ){
      TString detType = this->fVtxGeo->GetDigitType(idigit); 
      if( detType == "PMT8inch" && vtxgeo->IsFiltered(idigit) ){
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
  }
  
  if(configType == "Combined" || configType == "Two-stage")	{
    for( Int_t idigit=0; idigit<vtxgeo->GetNDigits(); idigit++ ){
      TString detType = this->fVtxGeo->GetDigitType(idigit); 
      if( (detType == "lappd_v0" || detType == "PMT8inch") && vtxgeo->IsFiltered(idigit) ){
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
  }
  
  
  if( Sw>0.0 ){
    meanTime = Swx*1.0/Sw;
  }
  return meanTime; //return expected vertex time
}


/*Double_t MinuitOptimizer::FindSimpleTimeProperties(VertexGeometryOpt* vtxgeo) {
  Double_t meanTime = 0.0;
  // calculate mean and rms of hits inside cone
  // ==========================================
  Double_t Swx = 0.0;
  Double_t Sw = 0.0;
  Double_t delta = 0.0;
  Double_t sigma = 0.0;
  Double_t weight = 0.0;

  TH1D *h1 = new TH1D("h1","h1", 2000, 6600, 6800);
  for( Int_t idigit=0; idigit<vtxgeo->GetNDigits(); idigit++ ){
  	if(this->fVtxGeo->GetDigitType(idigit) != "lappd_v0") continue; //use only the LAPPD timing
    Double_t delta = vtxgeo->GetDelta(idigit);
    Double_t charge = vtxgeo->GetDigitQ(idigit);
    h1->Fill(delta, charge);
  }
  for(int i=0;i<2000;i++) {
    weight = pow(h1->GetBinContent(i), 3);
    Swx += weight * h1->GetBinCenter(i);
    Sw += weight;
  }
  meanTime = Swx*1.0/Sw;
  delete h1; h1 = 0;
  return meanTime;
}*/


void MinuitOptimizer::PointPositionChi2(Double_t vtxX, Double_t vtxY, Double_t vtxZ, Double_t& vtxTime, Double_t& fom)
{  
  // figure of merit
  // ===============
  Double_t vtxFOM = 0.0;
  Double_t penaltyFOM = 0.0;
  Double_t fixPositionFOM = 0.0;
  
  // calculate residuals
  // ===================
  this->fVtxGeo->CalcPointResiduals(vtxX, vtxY, vtxZ, 0.0, 0.0, 0.0, 0.0); //calculate expected point vertex time for each digit

  // calculate figure of merit
  // =========================
  Double_t vtxParam0 = this->fVtxFinder->fFixTimeParam0;
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

void MinuitOptimizer::PointDirectionChi2(Double_t vtxX, Double_t vtxY, 
	                                       Double_t vtxZ, Double_t dirX, Double_t dirY, Double_t dirZ, 
	                                       Double_t& vtxAngle, Double_t& fom)
{  
  // figure of merit
  // ===============
  Double_t coneFOM = 0.0;
  Double_t fixPositionFOM = 0.0;
  
  // calculate residuals
  // ===================
  this->fVtxGeo->CalcPointResiduals(vtxX, vtxY, vtxZ, 0.0, 
                                 dirX, dirY, dirZ); //load expected vertex time for each digit

  // calculate figure of merit
  // =========================
  //this->FitPointConePropertiesLnL(vtxAngle,coneFOM);
  this->FitConePropertiesFoM(vtxAngle,coneFOM, Parameters::Instance()->GetConfigurationType());

  // calculate overall figure of merit
  // =================================
  fom = coneFOM;

  // truncate
  if( fom<-999.999*fBaseFOM ) fom = -999.999*fBaseFOM;

  return;
}

void MinuitOptimizer::PointVertexChi2(Double_t vtxX, Double_t vtxY, Double_t vtxZ, 
	                                    Double_t dirX, Double_t dirY, Double_t dirZ, 
	                                    Double_t& vtxAngle, Double_t& vtxTime, Double_t& fom)
{  
  // figure of merit
  // ===============
  Double_t vtxFOM = 0.0;
  Double_t penaltyFOM = 0.0;
  Double_t fixPositionFOM = 0.0;
  Double_t fixDirectionFOM = 0.0;

  // calculate residuals
  // ===================
  this->fVtxGeo->CalcPointResiduals(vtxX, vtxY, vtxZ, 0.0, 
                                 dirX, dirY, dirZ); //calculate expected vertex time for each digit
  // calculate figure of merit
  // =========================
  Double_t timeFOM = 0.0;
  Double_t coneFOM = 0.0;
  
  //this->FitPointConePropertiesLnL(vtxAngle, coneFOM);
  this->FitConePropertiesFoM(vtxAngle,coneFOM, Parameters::Instance()->GetConfigurationType());
  this->FitPointTimePropertiesLnL(vtxTime, timeFOM);
  
  Double_t fTimeFitWeight = fVtxFinder->fTimeFitWeight;
  Double_t fConeFitWeight = fVtxFinder->fConeFitWeight;
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

void MinuitOptimizer::ExtendedVertexChi2(Double_t vtxX, Double_t vtxY, Double_t vtxZ, Double_t dirX, Double_t dirY, Double_t dirZ, Double_t& vtxAngle, Double_t& vtxTime, Double_t& fom)
{  
  // figure of merit
  // ===============
  Double_t vtxFOM = 0.0;
  Double_t timeFOM = 0.0;
  Double_t coneFOM = 0.0;
  Double_t penaltyFOM = 0.0;
  Double_t fixPositionFOM = 0.0;
  Double_t fixDirectionFOM = 0.0;

  // calculate residuals
  // ===================
  this->fVtxGeo->CalcExtendedResiduals(vtxX,vtxY,vtxZ,0.0,dirX,dirY,dirZ);
  
  // calculate figure of merit
  // =========================

  //this->FitExtendedConePropertiesLnL(vtxAngle,coneFOM);
  this->FitConePropertiesFoM(vtxAngle,coneFOM, Parameters::Instance()->GetConfigurationType());
  this->FitExtendedTimePropertiesLnL(vtxTime,timeFOM);
  
  Double_t fTimeFitWeight = fVtxFinder->fTimeFitWeight;
  Double_t fConeFitWeight = fVtxFinder->fConeFitWeight;
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

void MinuitOptimizer::CorrectedVertexChi2(Double_t vtxX, Double_t vtxY, Double_t vtxZ, Double_t dirX, Double_t dirY, Double_t dirZ, Double_t& vtxAngle, Double_t& vtxTime, Double_t& fom)
{  
  // figure of merit
  // ===============
  Double_t vtxFOM = 0.0;
  Double_t timeFOM = 0.0;
  Double_t coneFOM = 0.0;
  Double_t penaltyFOM = 0.0;
  Double_t fixPositionFOM = 0.0;
  Double_t fixDirectionFOM = 0.0;

  // calculate residuals
  // ===================
  this->fVtxGeo->CalcExtendedResiduals(vtxX,vtxY,vtxZ,0.0,dirX,dirY,dirZ);
  
  // calculate figure of merit
  // =========================

  this->FitConePropertiesFoM(vtxAngle,coneFOM, "PMT8inch");
  fom = coneFOM;

  // truncate
  if( fom<-999.999*fBaseFOM ) fom = -999.999*fBaseFOM;

  return;
}




