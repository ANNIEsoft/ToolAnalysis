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
static FoMCalculator* fgFoMCalculator = 0;
static void vertex_time_lnl(int&, double*, double& f, double* par, int)
{  

  bool printDebugMessages = 0;
  
  double vtxTime = par[0]; // nanoseconds
  double fom = -9999.;
  fgMinuitOptimizer->time_fit_itr();  
  fgFoMCalculator->TimePropertiesLnL(vtxTime, fom);
  f = -fom; // note: need to maximize this fom
  if( printDebugMessages ){
    std::cout << "  [vertex_time_lnl] [" << fgMinuitOptimizer->time_fit_iterations() << "] vtime=" << vtxTime << " fom=" << fom << std::endl;
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

  double fom = -9999.;
  fgMinuitOptimizer->point_position_itr();
  fgFoMCalculator->PointPositionChi2(vtxX,vtxY,vtxZ,vtxTime,fom);


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

  double fom = -9999.;

  double coneAngle = fgMinuitOptimizer->fConeAngle; //Cherenkov cone angle

  fgMinuitOptimizer->point_direction_itr();
  fgFoMCalculator->PointDirectionChi2(vtxX,vtxY,vtxZ,
                                     dirX,dirY,dirZ,
                                     coneAngle, fom);
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
  double vtxTime  = par[5]; // ns

  double dirX = sin(dirTheta)*cos(dirPhi);
  double dirY = sin(dirTheta)*sin(dirPhi);
  double dirZ = cos(dirTheta);
  
  double fom = -9999.;
 
  double coneAngle = fgMinuitOptimizer->fConeAngle; //Cherenkov cone angle

  fgMinuitOptimizer->point_vertex_itr();
  fgFoMCalculator->PointVertexChi2(vtxX,vtxY,vtxZ,dirX,dirY,dirZ,coneAngle, vtxTime,fom);
  f = -fom; // note: need to maximize this fom

  if( printDebugMessages ){
    std::cout << " [point_vertex_chi2] [" << fgMinuitOptimizer->point_vertex_iterations() 
    	<< "] (x,y,z)=(" << vtxX << "," << vtxY << "," << vtxZ << ") (px,py,pz)=(" 
    	<< dirX << "," << dirY << "," << dirZ << ") vtime=" << vtxTime << " fom=" << fom << std::endl;
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
  double vtxTime  = par[5]; // ns

  double dirX = sin(dirTheta)*cos(dirPhi);
  double dirY = sin(dirTheta)*sin(dirPhi);
  double dirZ = cos(dirTheta);

  double coneAngle = fgMinuitOptimizer->fConeAngle; //Cherenkov cone angle

  double fom = -9999.;

  fgMinuitOptimizer->extended_vertex_itr();
  
  fgFoMCalculator->ExtendedVertexChi2(vtxX,vtxY,vtxZ,
                                     dirX,dirY,dirZ, 
                                     coneAngle, vtxTime,fom);

  f = -fom; // note: need to maximize this fom

  if( printDebugMessages ){
    std::cout << " [extended_vertex_chi2] [" << fgMinuitOptimizer->extended_vertex_iterations() 
    	<< "] (x,y,z)=(" << vtxX << "," << vtxY << "," << vtxZ << ") (px,py,pz)=(" << dirX << "," 
    	<< dirY << "," << dirZ << ") vtime=" << vtxTime << " fom=" << fom << std::endl;  
  }
  return;
}


//Constructor
MinuitOptimizer::MinuitOptimizer() {
  fgMinuitOptimizer = this;
  fgFoMCalculator = new FoMCalculator();
  fSeedVtx = 0;
  fFittedVtx = new RecoVertex();
  fVtxX = -9999.;
  fVtxY = -9999.;
  fVtxZ = -9999.;
  fVtxTime = -9999.;
  fDirX = -9999.;
  fDirY = -9999.;
  fDirZ = -9999.;
  fVtxFOM = -9999.;
  fConeAngle = Parameters::CherenkovAngle();
  fTimeFitItr = 0;
  fPointPosItr = 0;
  fPointDirItr = 0;
  fPointVtxItr = 0;
  fExtendedVtxItr = 0;
  fPass = 0;
  fItr = 0;
  fPrintLevel = -1;

  fFixTimeParam0 = 0.20;  // scattering parameter (not currently used)
  
  //KEPT FOR HISTORY; NOT IN USE (Cone parameters)
  //fConeFitItr = 0;
  //fCorrectedVtxItr = 0;
  //fFitConeParams = 1;     // do fit by default  
  //fConeParam0 = 0.;
  //fConeParam1 = 0.;
  //fConeParam2 = 0.;
  //fFixConeParam0 = 0.25;  // track length parameter
  //fFixConeParam1 = 0.50;  // track length parameter
  //fFixConeParam2 = 0.75;  // particle ID:  0.0[electron]->1.0[muon]

  //WCSIM 
  fXmin = -152.0;
  fXmax = 152.0;
  fYmin = -198.0;
  fYmax = 198.0;
  fZmin = -152.0;
  fZmax = 152.0;
  fTmin = -10.0;
  fTmax = 10.0;
  
  // default Mean time calculator type
  this->SetMeanTimeCalculatorType(0);
  
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
  
  fMinuitTimeFit = new TMinuit();
  fMinuitTimeFit->SetPrintLevel(-1);
  fMinuitTimeFit->SetMaxIterations(5000);   

  //fMinuitCorrectedVertex = new TMinuit();
  //fMinuitCorrectedVertex->SetPrintLevel(-1);
  //fMinuitCorrectedVertex->SetMaxIterations(5000);

  //fMinuitConeFit = new TMinuit();
  //fMinuitConeFit->SetPrintLevel(-1);
  //fMinuitConeFit->SetMaxIterations(5000); 
}

//Destructor
MinuitOptimizer::~MinuitOptimizer() {
	fSeedVtx = 0;
    delete fgFoMCalculator; fgFoMCalculator = 0;
	delete fMinuitTimeFit; fMinuitTimeFit = 0;
	delete fMinuitPointPosition; fMinuitPointPosition = 0;
	delete fMinuitPointDirection; fMinuitPointDirection = 0;
	delete fMinuitPointVertex; fMinuitPointVertex = 0;
	delete fMinuitExtendedVertex; fMinuitExtendedVertex = 0;
	delete fFittedVtx; fFittedVtx = 0;
	//delete fMinuitCorrectedVertex; fMinuitCorrectedVertex = 0;
	//delete fMinuitConeFit; fMinuitConeFit = 0;
}
	

void MinuitOptimizer::SetFitterTimeRange(double tmin, double tmax) {
  fTmin = tmin;
  fTmax = tmax;
}

void MinuitOptimizer::LoadVertexGeometry(VertexGeometry* vtxgeo) {
  fgFoMCalculator->fVtxGeo = vtxgeo;	
}

void MinuitOptimizer::SetNumberOfIterations(int iterations) {
  fMinuitPointPosition->SetMaxIterations(iterations);
  fMinuitPointDirection->SetMaxIterations(iterations);
  fMinuitPointVertex->SetMaxIterations(iterations);
  fMinuitExtendedVertex->SetMaxIterations(iterations); 
  fMinuitTimeFit->SetMaxIterations(iterations);   

  //fMinuitCorrectedVertex = new TMinuit();
  //fMinuitCorrectedVertex->SetPrintLevel(-1);
  //fMinuitCorrectedVertex->SetMaxIterations(5000);
}

void MinuitOptimizer::SetTimeFitWeight(double tweight) {
  fgFoMCalculator->SetTimeFitWeight(tweight);	
}

void MinuitOptimizer::SetConeFitWeight(double cweight) {
  fgFoMCalculator->SetConeFitWeight(cweight);	
}

void MinuitOptimizer::SetMeanTimeCalculatorType(int type) {
  fgFoMCalculator->SetMeanTimeCalculatorType(type);	
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



void MinuitOptimizer::FitPointTimeWithMinuit() {
  fgFoMCalculator->fVtxGeo->CalcPointResiduals(fVtxX, fVtxY, fVtxZ, 0.0, 0.0, 0.0, 0.0);

  // calculate mean and rms
  // ====================== 
  double meanvtxTime = 0.0;
  meanvtxTime = fgFoMCalculator->FindSimpleTimeProperties(fConeAngle);  //returns weighted average of the expected vertex time
  // reset counter
  // =============
  time_fit_reset_itr();
  
  // run Minuit
  // ==========  
  // one-parameter fit to time profile

  int err = 0;
  int flag = 0;

  double seedTime = meanvtxTime;
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
  
  // fitting done; calculate best-fit figure of merit
  // =========================
  double fom = -9999.;
  fgFoMCalculator->TimePropertiesLnL(fitTime, fom);
  
  fVtxTime = fitTime;
  fVtxFOM = fom;
  
  fFittedVtx->SetVertex(fVtxX, fVtxY, fVtxZ, fVtxTime);
  fFittedVtx->SetFOM(fVtxFOM, 1, 1);
  return;
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

  fVtxFOM = -9999.;
  
  fPass = 0;               // flag = 0: normal termination
  if( flag==0 ) fPass = 1; // anything else: abnormal termination 

  fItr = point_position_iterations();
  fgFoMCalculator->PointPositionChi2(fVtxX,fVtxY,fVtxZ,fVtxTime,fVtxFOM);
  
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
  
  fVtxFOM = -9999.;
  
  fPass = 0;               // flag = 0: normal termination
  if( flag==0 ) fPass = 1; // anything else: abnormal termination 
                           
  fItr = point_direction_iterations();
  
  // calculate vertex
  // ================
  fgFoMCalculator->PointDirectionChi2(fVtxX,fVtxY,fVtxZ,fDirX,fDirY,fDirZ,fConeAngle,fVtxFOM);

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
  double fitTime = 0.0;

  double fitTimeErr = 0.0;
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
  fMinuitExtendedVertex->mnparm(5,"vtxTime",seedTime,1.0,fTmin,fTmax,err); 

  flag = fMinuitPointVertex->Migrad();
  fMinuitPointVertex->GetParameter(0,fitXpos,fitXposErr);
  fMinuitPointVertex->GetParameter(1,fitYpos,fitYposErr);
  fMinuitPointVertex->GetParameter(2,fitZpos,fitZposErr);
  fMinuitPointVertex->GetParameter(3,fitTheta,fitThetaErr);
  fMinuitPointVertex->GetParameter(4,fitPhi,fitPhiErr);
  fMinuitPointVertex->GetParameter(5,fitTime,fitTimeErr);
  
  delete [] arglist;
  
  // sort results
  // ============
  fVtxX = fitXpos; 
  fVtxY = fitYpos;
  fVtxZ = fitZpos;
  fVtxTime = fitTime; //for test only

  fDirX = sin(fitTheta)*cos(fitPhi);
  fDirY = sin(fitTheta)*sin(fitPhi);
  fDirZ = cos(fitTheta);  

  fVtxFOM = -9999.0;
  
  fPass = 0;               // flag = 0: normal termination
  if( flag==0 ) fPass = 1; // anything else: abnormal termination 

  fItr = point_vertex_iterations();
  
  // fitting complete; calculate vertex FOM
  // ================
  fgFoMCalculator->PointVertexChi2(fVtxX,fVtxY,fVtxZ,fDirX,fDirY,fDirZ,fConeAngle, fVtxTime,fVtxFOM); 
  
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
  // seed vertex
  // ===========
  bool foundSeed = ( fSeedVtx->FoundVertex() 
                    && fSeedVtx->FoundDirection() );


  double seedTime = fSeedVtx->GetTime();
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
  // six-parameter fit to vertex position, time and direction

  int err = 0;
  int flag = 0;

  double fitXpos = 0.0;
  double fitYpos = 0.0;
  double fitZpos = 0.0;
  double fitTheta = 0.0;
  double fitPhi = 0.0;
  double fitTime = 0.0;
  
  double fitTimeErr = 0.0;  
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
  fMinuitExtendedVertex->mnparm(5,"vtxTime",seedTime,1.0,fTmin,fTmax,err); //....TX
  
  flag = fMinuitExtendedVertex->Migrad();
  
  fMinuitExtendedVertex->GetParameter(0,fitXpos,fitXposErr);
  fMinuitExtendedVertex->GetParameter(1,fitYpos,fitYposErr);
  fMinuitExtendedVertex->GetParameter(2,fitZpos,fitZposErr);
  fMinuitExtendedVertex->GetParameter(3,fitTheta,fitThetaErr);
  fMinuitExtendedVertex->GetParameter(4,fitPhi,fitPhiErr);
  fMinuitExtendedVertex->GetParameter(5,fitTime,fitTimeErr);

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
  fVtxTime = fitTime;
  fDirX = sin(fitTheta)*cos(fitPhi);
  fDirY = sin(fitTheta)*sin(fitPhi);
  fDirZ = cos(fitTheta);  
  
  fVtxFOM = -9999.0;
  
  fPass = 0;               // flag = 0: normal termination
  if( flag==0 ) fPass = 1; // anything else: abnormal termination 

  fItr = extended_vertex_iterations();
  
  // fit complete; calculate fit results
  // ================
  fgFoMCalculator->ExtendedVertexChi2(fVtxX,fVtxY,fVtxZ,
                           fDirX,fDirY,fDirZ, 
                           fConeAngle, fVtxTime,fVtxFOM);
                           
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

void MinuitOptimizer::FitExtendedVertexWithMinuit(TH1D pdf) {
    // seed vertex
    // ===========
    bool foundSeed = (fSeedVtx->FoundVertex()
        && fSeedVtx->FoundDirection());


    double seedTime = fSeedVtx->GetTime();
    double seedX = fSeedVtx->GetPosition().X();
    double seedY = fSeedVtx->GetPosition().Y();
    double seedZ = fSeedVtx->GetPosition().Z();

    double seedDirX = fSeedVtx->GetDirection().X();
    double seedDirY = fSeedVtx->GetDirection().Y();
    double seedDirZ = fSeedVtx->GetDirection().Z();

    double seedTheta = acos(seedDirZ);
    double seedPhi = 0.0;

    //modified by JW
    if (seedDirX > 0.0) {
        seedPhi = atan(seedDirY / seedDirX);
    }
    if (seedDirX < 0.0) {
        seedPhi = atan(seedDirY / seedDirX);
        if (seedDirY > 0.0) seedPhi += TMath::Pi();
        if (seedDirY <= 0.0) seedPhi -= TMath::Pi();
    }
    if (seedDirX == 0.0) {
        if (seedDirY > 0.0) seedPhi = 0.5 * TMath::Pi();
        else if (seedDirY < 0.0) seedPhi = -0.5 * TMath::Pi();
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
    if (foundSeed == 0) {
        if (fPrintLevel >= 0) {
            std::cout << "   <warning> extended vertex fit failed to find input vertex " << std::endl;
        }
        status |= RecoVertex::kFailExtendedVertex;
        fFittedVtx->SetStatus(status);
        return;
    }

    // run Minuit
    // ==========  
    // six-parameter fit to vertex position, time and direction

    int err = 0;
    int flag = 0;

    double fitXpos = 0.0;
    double fitYpos = 0.0;
    double fitZpos = 0.0;
    double fitTheta = 0.0;
    double fitPhi = 0.0;
    double fitTime = 0.0;

    double fitTimeErr = 0.0;
    double fitXposErr = 0.0;
    double fitYposErr = 0.0;
    double fitZposErr = 0.0;
    double fitThetaErr = 0.0;
    double fitPhiErr = 0.0;

    double* arglist = new double[10];
    arglist[0] = 2;  // 1: standard minimization
    // 2: try to improve minimum

// re-initialize everything...
    fMinuitExtendedVertex->mncler();
    fMinuitExtendedVertex->SetFCN(extended_vertex_chi2);
    fMinuitExtendedVertex->mnset();
    fMinuitExtendedVertex->mnexcm("SET STR", arglist, 1, err);
    fMinuitExtendedVertex->mnparm(0, "x", seedX, 1.0, fXmin, fXmax, err);
    fMinuitExtendedVertex->mnparm(1, "y", seedY, 1.0, fYmin, fYmax, err);
    fMinuitExtendedVertex->mnparm(2, "z", seedZ, 5.0, fZmin, fZmax, err);
    fMinuitExtendedVertex->mnparm(3, "theta", seedTheta, 0.125 * TMath::Pi(), -1.0 * TMath::Pi(), 2.0 * TMath::Pi(), err);
    fMinuitExtendedVertex->mnparm(4, "phi", seedPhi, 0.125 * TMath::Pi(), -2.0 * TMath::Pi(), 2.0 * TMath::Pi(), err);
    fMinuitExtendedVertex->mnparm(5, "vtxTime", seedTime, 1.0, fTmin, fTmax, err); //....TX

    flag = fMinuitExtendedVertex->Migrad();

    fMinuitExtendedVertex->GetParameter(0, fitXpos, fitXposErr);
    fMinuitExtendedVertex->GetParameter(1, fitYpos, fitYposErr);
    fMinuitExtendedVertex->GetParameter(2, fitZpos, fitZposErr);
    fMinuitExtendedVertex->GetParameter(3, fitTheta, fitThetaErr);
    fMinuitExtendedVertex->GetParameter(4, fitPhi, fitPhiErr);
    fMinuitExtendedVertex->GetParameter(5, fitTime, fitTimeErr);

    delete[] arglist;

    //correct angles, JW
    if (fitTheta < 0.0) fitTheta = -1.0 * fitTheta;
    if (fitTheta > TMath::Pi()) fitTheta = 2.0 * TMath::Pi() - fitTheta;
    if (fitPhi < -1.0 * TMath::Pi()) fitPhi += 2.0 * TMath::Pi();
    if (fitPhi > TMath::Pi()) fitPhi -= 2.0 * TMath::Pi();

    // sort results
    // ============
    fVtxX = fitXpos;
    fVtxY = fitYpos;
    fVtxZ = fitZpos;
    fVtxTime = fitTime;
    fDirX = sin(fitTheta) * cos(fitPhi);
    fDirY = sin(fitTheta) * sin(fitPhi);
    fDirZ = cos(fitTheta);

    fVtxFOM = -9999.0;

    fPass = 0;               // flag = 0: normal termination
    if (flag == 0) fPass = 1; // anything else: abnormal termination 

    fItr = extended_vertex_iterations();

    // fit complete; calculate fit results
    // ================
    fgFoMCalculator->ExtendedVertexChi2(fVtxX, fVtxY, fVtxZ,
        fDirX, fDirY, fDirZ,
        fConeAngle, fVtxTime, fVtxFOM, pdf);

    // set vertex and direction
    // ========================
    fFittedVtx->SetVertex(fVtxX, fVtxY, fVtxZ, fVtxTime);
    fFittedVtx->SetDirection(fDirX, fDirY, fDirZ);
    fFittedVtx->SetConeAngle(fConeAngle);
    fFittedVtx->SetFOM(fVtxFOM, fItr, fPass);

    // set status
    // ==========
    bool inside_det = ANNIEGeometry::Instance()->InsideDetector(fVtxX, fVtxY, fVtxZ);
    if (!fPass || !inside_det) status |= RecoVertex::kFailExtendedVertex;
    fFittedVtx->SetStatus(status);
    if (fPrintLevel >= 0) {
        if (!fPass) std::cout << "   <warning> extended vertex fit failed to converge! Error code: " << flag << std::endl;
    }
    // return vertex
    // =============  
    return;
}


//KEPT FOR HISTORY, BUT FITTER IS CURRENTLY NOT WORKING
//THESE SHOULD BE MOVED TO BEFORE THE CONSTRUCTOR
//static void corrected_vertex_chi2(int&, double*, double& f, double* par, int)
//{
//  bool printDebugMessages = 0;
//  
//  double dl = par[0];
////  double dirTheta = par[1]; // radians
////  double dirPhi   = par[2]; // radiansf
//  
////  double dirX = sin(dirTheta)*cos(dirPhi);
////  double dirY = sin(dirTheta)*sin(dirPhi);
////  double dirZ = cos(dirTheta);
//  
//  double dirX = fgMinuitOptimizer->fDirX;
//  double dirY = fgMinuitOptimizer->fDirY;
//  double dirZ = fgMinuitOptimizer->fDirZ;
//  
//  double vtxX = fgMinuitOptimizer->fVtxX + dl * dirX;
//  double vtxY = fgMinuitOptimizer->fVtxY + dl * dirY;;
//  double vtxZ = fgMinuitOptimizer->fVtxZ + dl * dirZ;
//
//  double vangle = 0.0;
//  double vtime  = 0.0;
//  double fom = 0.0;
//
//  if(TMath::Sqrt(vtxX*vtxX + vtxY*vtxY + vtxZ*vtxZ)>152 || vtxY>198 || vtxY<-198) {fom = 9999;}
//  fgMinuitOptimizer->corrected_vertex_itr();
//  
//  fgFoMCalculator->CorrectedVertexChi2(vtxX,vtxY,vtxZ,
//                                     dirX,dirY,dirZ,
//                                     vangle,vtime,fom);
//
//  f = -fom; // note: need to maximize this fom
//
//  if( printDebugMessages ){
//    std::cout << " [corrected_vertex_chi2] [" << fgMinuitOptimizer->extended_vertex_iterations() 
//    	<< "] (x,y,z)=(" << vtxX << "," << vtxY << "," << vtxZ << ") (px,py,pz)=(" << dirX << "," 
//    	<< dirY << "," << dirZ << ") vtime=" << vtime << " fom=" << fom << std::endl;  
//  }
//
//  return;
//}

//static void vertex_cone_lnl(int&, double*, double& f, double* par, int)
//{
//  bool printDebugMessages = 0;
//  
//  double vtxParam0 = fgMinuitOptimizer->fFixConeParam0;
//  double vtxParam1 = fgMinuitOptimizer->fFixConeParam1;
//  double vtxParam2 = fgMinuitOptimizer->fFixConeParam2;
//
//  if( fgMinuitOptimizer->fFitConeParams ){
//    vtxParam0 = par[0];
//    vtxParam1 = par[1];
//    vtxParam2 = par[2];
//  }
// 
//  double vangle = 0.0;
//  double fom = 0.0;
//
//  fgMinuitOptimizer->cone_fit_itr();
//  fgFoMCalculator->ConePropertiesLnL(vtxParam0,vtxParam1,vtxParam2,vangle,fom);
//
//  f = -fom; // note: need to maximize this fom
//
//  if( printDebugMessages ){
//    std::cout << "  [vertex_cone_lnl] [" << fgMinuitOptimizer->cone_fit_iterations() 
//    	<< "] vparam0=" << vtxParam0 << " vparam1=" << vtxParam1 << " vtxParam2=" << vtxParam2 
//    	<< " fom=" << fom << std::endl;
//  }
//
//  return;
//}
//KEPT FOR HISTORY, BUT NOT CURRENTLY WORKING
//void MinuitOptimizer::FitCorrectedVertexWithMinuit() {
//	// initialization
//  // ==============
//  double vtxAngle = Parameters::CherenkovAngle();
//  double vtxFOM = 0.0;
//  // seed vertex
//  // ===========
//  bool foundSeed = ( fSeedVtx->FoundVertex() 
//                    && fSeedVtx->FoundDirection() );
//
//  double seedX = fSeedVtx->GetPosition().X();
//  double seedY = fSeedVtx->GetPosition().Y();
//  double seedZ = fSeedVtx->GetPosition().Z();
//  double seedT = fSeedVtx->GetTime();
//
//  double seedDirX = fSeedVtx->GetDirection().X();
//  double seedDirY = fSeedVtx->GetDirection().Y();
//  double seedDirZ = fSeedVtx->GetDirection().Z();
//
//  double seedTheta = acos(seedDirZ);
//  double seedPhi = 0.0;
//
//  if( seedDirX!=0.0 ){
//    seedPhi = atan(seedDirY/seedDirX);
//  }
//  if( seedDirX<=0.0 ){
//    if( seedDirY>0.0 ) seedPhi += TMath::Pi();
//    if( seedDirY<0.0 ) seedPhi -= TMath::Pi();
//  }
//  // current status
//  // ==============
//  int status = fSeedVtx->GetStatus();
//  
//  // reset counter
//  // =============
//  corrected_vertex_reset_itr();
//  
//  // abort if necessary
//  // ==================
//  if( foundSeed==0 ){
//  	if( fPrintLevel >= 0) {
//      std::cout << "   <warning> corrected vertex fit failed to find input vertex " << std::endl;   
//    }
//    status |= RecoVertex::kFailCorrectedVertex;
//    fFittedVtx->SetStatus(status);
//    return;
//  }
//  
//  // run Minuit
//  // ==========  
//  // one-parameter fit to vertex parallel position
//
//  int err = 0;
//  int flag = 0;
//
//  double fitParallelcor = 0.0;
//  double fitTheta = 0.0;
//  double fitPhi = 0.0;
//
//  double fitParallelcorErr = 0.0;
//  double fitThetaErr = 0.0;
//  double fitPhiErr = 0.0;
//  
//  double* arglist = new double[10];
//  arglist[0]=2;  // 1: standard minimization
//                 // 2: try to improve minimum
//  
//   // re-initialize everything...
//  fMinuitCorrectedVertex->mncler();
//  fMinuitCorrectedVertex->SetFCN(corrected_vertex_chi2);
//  fMinuitCorrectedVertex->mnset();
//  fMinuitCorrectedVertex->mnexcm("SET STR",arglist,1,err);
//  fMinuitCorrectedVertex->mnparm(0,"dl",0.0, 5.0, -50.0, 50.0, err);
////  fMinuitCorrectedVertex->mnparm(1,"theta",seedTheta,0.125*TMath::Pi(),0.0,TMath::Pi(),err); //initial: 0.125*TMath::Pi(),0.0,TMath::Pi()
////  fMinuitCorrectedVertex->mnparm(2,"phi",seedPhi,0.25*TMath::Pi(),-1.0*TMath::Pi(),+3.0*TMath::Pi(),err); //initial: 0.25*TMath::Pi(),-1.0*TMath::Pi(),+3.0*TMath::Pi()
////  fMinuitCorrectedVertex->FixParameter(1);
////  fMinuitCorrectedVertex->FixParameter(2);
//  flag = fMinuitCorrectedVertex->Migrad();
//  
//  fMinuitCorrectedVertex->GetParameter(0,fitParallelcor,fitParallelcorErr);
//
//  delete [] arglist;
//  
//   // sort results
//  // ============
////  fDirX = sin(fitTheta)*cos(fitPhi);
////  fDirY = sin(fitTheta)*sin(fitPhi);
////  fDirZ = cos(fitTheta);
//  fVtxX += fitParallelcor * fDirX; 
//  fVtxY += fitParallelcor * fDirY;
//  fVtxZ += fitParallelcor * fDirZ;
//  
//  fVtxFOM = 0.0;
//  
//  fPass = 0;               // flag = 0: normal termination
//  if( flag==0 ) fPass = 1; // anything else: abnormal termination 
//
//  fItr = corrected_vertex_iterations();
//  	
//  // calculate vertex
//  // ================
//  fgFoMCalculator->CorrectedVertexChi2(fVtxX,fVtxY,fVtxZ,
//                           fDirX,fDirY,fDirZ, 
//                           fConeAngle,fVtxTime,fVtxFOM); //fit vertex time here
//                           
//  // set vertex and direction
//  // ========================
//  fFittedVtx->SetVertex(fVtxX,fVtxY,fVtxZ,fVtxTime);
//  fFittedVtx->SetDirection(fDirX,fDirY,fDirZ);
//  fFittedVtx->SetConeAngle(fConeAngle);
//  fFittedVtx->SetFOM(fVtxFOM,fItr,fPass);
//  // set status
//  // ==========
//  bool inside_det =  ANNIEGeometry::Instance()->InsideDetector(fVtxX,fVtxY,fVtxZ);
//  if( !fPass || ! inside_det) status |= RecoVertex::kFailExtendedVertex;
//  fFittedVtx->SetStatus(status);
//  if( fPrintLevel >= 0) {
//    if( !fPass ) std::cout << "   <warning> corrected vertex fit failed to converge! Error code: " << flag << std::endl;
//  }
//  // return vertex
//  // =============  
//  return;
//}

//void MinuitOptimizer::FitPointConePropertiesLnL(double& coneAngle, double& coneFOM)
//{  
//  coneAngle  = Parameters::CherenkovAngle(); //...chrom1.38, nominal cone angle
//  // calculate cone fom
//  // =========================
//  coneFOM = 0.0;
//  double ConeParam0 = this->fFixConeParam0;
//  double ConeParam1 = this->fFixConeParam1;
//  double ConeParam2 = this->fFixConeParam2;
//  fgFoMCalculator->ConePropertiesLnL(ConeParam0,ConeParam1,ConeParam2,coneAngle,coneFOM);  
//  return;
//}
//
//void MinuitOptimizer::FitExtendedConePropertiesLnL(double& coneAngle, double& coneFOM)
//{  
//  // return result from point fit
//  // ============================
//  if( this->fFitConeParams==0 ){
//    return this->FitPointConePropertiesLnL(coneAngle,coneFOM);
//  }
//
//  // reset counter
//  // =============
//  cone_fit_reset_itr();
//
//  // run Minuit
//  // ==========  
//  // one-parameter fit to angular distribution
//
//  int err = 0;
//  int flag = 0;
//
//  double seedParam0 = this->fFixConeParam0; //track length parameter
//  double seedParam1 = this->fFixConeParam1; //track length parameter
//  double seedParam2 = this->fFixConeParam2; //particle ID:  0.0[electron]->1.0[muon]
//
//  double fitParam0 = seedParam0;
//  double fitParam1 = seedParam1;
//  double fitParam2 = seedParam2;
//
//  double fitParam0Err = 0.0;
//  double fitParam1Err = 0.0; 
//  double fitParam2Err = 0.0;
//
//  double fitAngle  = Parameters::CherenkovAngle();
//  
//  double* arglist = new double[10];
//  arglist[0]=1;  // 1: standard minimization
//                 // 2: try to improve minimum
//
//  // re-initialize everything...
//  fMinuitConeFit->mncler();
//  fMinuitConeFit->SetFCN(vertex_cone_lnl);
//  fMinuitConeFit->mnexcm("SET STR",arglist,1,err);
//  fMinuitConeFit->mnparm(0,"vtxParam0",seedParam0,0.25,0.0,1.0,err);
//  fMinuitConeFit->mnparm(1,"vtxParam1",seedParam1,0.25,0.0,1.0,err);
//  fMinuitConeFit->mnparm(2,"vtxParam2",seedParam2,0.25,0.0,1.0,err);
//
//  flag = fMinuitConeFit->Migrad();
//  fMinuitConeFit->GetParameter(0,fitParam0,fitParam0Err);
//  fMinuitConeFit->GetParameter(1,fitParam1,fitParam1Err);
//  fMinuitConeFit->GetParameter(2,fitParam2,fitParam2Err);
//  
//  delete [] arglist;
//
//  // calculate figure of merit
//  // =========================
//  double fom = -9999.;
//  
//  this->ConePropertiesLnL(fitParam0,fitParam1,fitParam2,
//                          fitAngle,fom);  
//
//
//  // --- debug ---
//  fConeParam0 = fitParam0;
//  fConeParam1 = fitParam1;
//  fConeParam2 = fitParam2;
//
//  // return figure of merit
//  // ======================
//  coneAngle = fitAngle;
//  coneFOM = fom;
//
//  return;
//}


