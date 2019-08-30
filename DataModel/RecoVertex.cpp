#include "RecoVertex.h"

#include <iostream>
using namespace std;

RecoVertex::RecoVertex()
{
  this->Reset();

  ANNIERecoObjectTable::Instance()->NewVertex();
}

RecoVertex::RecoVertex( Position pos)
{
  this->Reset();
  this->SetVertex(pos);
  this->SetFOM(-9999.,1,1);

  ANNIERecoObjectTable::Instance()->NewVertex();
}
  
RecoVertex::RecoVertex( Position pos, Direction dir)
{
  this->Reset();

  this->SetVertex(pos);
  this->SetDirection(dir);
  this->SetFOM(-9999.,1,1);
  
  ANNIERecoObjectTable::Instance()->NewVertex();
}


RecoVertex::RecoVertex( Position pos, double t, Direction dir, double fom, int nsteps, bool pass, int status )
{
  this->Reset();
  this->SetVertex(pos);
  this->SetDirection(dir);
  this->SetFOM(fom,nsteps,pass);
  this->SetStatus(status);
  ANNIERecoObjectTable::Instance()->NewVertex();
}

RecoVertex::RecoVertex( Position pos, double t,Direction dir, double angle, double length,double fom, int nsteps, bool pass, int status )
{
  this->Reset();
  this->SetVertex(pos,t);
  this->SetDirection(dir);
  this->SetConeAngle(angle);
  this->SetTrackLength(length);
  this->SetFOM(fom,nsteps,pass);
  this->SetStatus(status);

  ANNIERecoObjectTable::Instance()->NewVertex();
}

RecoVertex::~RecoVertex()
{
  ANNIERecoObjectTable::Instance()->DeleteVertex();
}

void RecoVertex::SetVertex( Position pos )
{
  double t = 0.0;
  this->SetVertex(pos, t);
}

void RecoVertex::SetVertex( double vtxX, double vtxY, double vtxZ )
{
  double t = 0.0;
  Position pos(vtxX, vtxY, vtxZ);
  this->SetVertex(pos, t);
  
}

void RecoVertex::SetVertex( Position pos, double t )
{
  fPosition = pos;
  fTime  = t;
  fFoundVertex = 1;
}

void RecoVertex::SetVertex( double vtxX, double vtxY, double vtxZ, double vtxT )
{
  double t(vtxT);
  Position pos(vtxX, vtxY, vtxZ);
  this->SetVertex(pos, t);
}

void RecoVertex::SetDirection( Direction dir )
{
  fDirection = dir;
  fFoundDirection = 1;
}

void RecoVertex::SetDirection( double dirX, double dirY, double dirZ)
{
  fDirection.SetX(dirX);
  fDirection.SetY(dirY);
  fDirection.SetZ(dirZ);
  fFoundDirection = 1;
}

void RecoVertex::SetConeAngle( Double_t angle )
{
  fConeAngle = angle;
}

void RecoVertex::SetTrackLength( Double_t length )
{
  fTrackLength = length;
}

void RecoVertex::SetFOM( Double_t fom, Int_t nsteps, Bool_t pass )
{
  fFOM = fom;
  fIterations = nsteps;
  fPass = pass;
}

void RecoVertex::SetTime(double vtxtime) {
  fTime = vtxtime;	
}

void RecoVertex::SetStatus( Int_t status )
{
  fStatus = status;
}

RecoVertex* RecoVertex::CloneVertex(RecoVertex* b) { 
  fPosition = b->GetPosition();
  fTime = b->GetTime();
  fFoundVertex = b->FoundVertex();
  fDirection = b->GetDirection();
  fFoundDirection = b->FoundDirection();

  fConeAngle = b->GetConeAngle() ;
  fTrackLength = b->GetTrackLength();

  fFOM = b->GetFOM();
  fIterations = b->GetIterations();
  fPass = b->GetPass();

  fStatus = b->GetStatus();	
}

void RecoVertex::Reset()
{ 
  fPosition = Position(-9999.,-9999.,-9999.);
  fTime = double(-9999.);
  fFoundVertex = 0;
  fDirection = Direction(0.,0.,0.);
  fFoundDirection = 0;

  fConeAngle = 0.0;
  fTrackLength = 0.0;

  fFOM = -9999.;
  fIterations = 0;
  fPass = 0;
  
  fStatus = RecoVertex::kOK;
}

bool RecoVertex::Print() {
	std::cout<<"-------------------------------------------------------"<<std::endl;
  std::cout<<"     PrintVertex Vertex Information:"<<std::endl;
  std::cout<<"     (x, y, z, t) = ("<<fPosition.X()<<", "<<fPosition.Y()<<", "<<fPosition.Z()<<", "<<fTime<<")"<<std::endl;
  std::cout<<"     (DirX, DirY, DirZ) = ("<<fDirection.X()<<", "<<fDirection.Y()<<", "<<fDirection.Z()<<")"<<std::endl;
  std::cout<<"     Figure Of Merit = "<<fFOM<<std::endl;
  std::cout<<"-------------------------------------------------------"<<std::endl;
  return true;
}

