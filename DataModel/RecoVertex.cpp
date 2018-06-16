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
  this->SetFOM(0.0,1,1);

  ANNIERecoObjectTable::Instance()->NewVertex();
}
  
RecoVertex::RecoVertex( Position pos, Direction dir)
{
  this->Reset();

  this->SetVertex(pos);
  this->SetDirection(dir);
  this->SetFOM(0.0,1,1);
  
  ANNIERecoObjectTable::Instance()->NewVertex();
}


RecoVertex::RecoVertex( Position pos, TimeClass t, Direction dir, double fom, int nsteps, bool pass, int status )
{
  this->Reset();
  this->SetVertex(pos);
  this->SetDirection(dir);
  this->SetFOM(fom,nsteps,pass);
  this->SetStatus(status);
  ANNIERecoObjectTable::Instance()->NewVertex();
}

RecoVertex::RecoVertex( Position pos, TimeClass t,Direction dir, double angle, double length,double fom, int nsteps, bool pass, int status )
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
  TimeClass t(950.0);
  this->SetVertex(pos, t);
}

void RecoVertex::SetVertex( Position pos, TimeClass t )
{
  fPosition = pos;
  fTime  = t;
  fFoundVertex = 1;
}

void RecoVertex::SetDirection( Direction dir )
{
  fDirection = dir;
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

void RecoVertex::SetTime(TimeClass vtxtime) {
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
  fPosition = Position(0,0,0);
  fTime = TimeClass(0.0);
  fFoundVertex = 0;

  fDirection = Direction(0,0,0);
  fFoundDirection = 0;

  fConeAngle = 0.0;
  fConeAngle = 42.0;
  fTrackLength = 0.0;

  fFOM = 0.0;
  fIterations = 0;
  fPass = 0;
  
  fStatus = RecoVertex::kOK;
}

void RecoVertex::PrintVertex() {
	std::cout<<"-------------------------------------------------------"<<std::endl;
  std::cout<<"     PrintVertex Vertex Information:"<<std::endl;
  std::cout<<"     (x, y, z, t) = ("<<fPosition.X()<<", "<<fPosition.Y()<<", "<<fPosition.Z()<<", "<<fTime.GetNs()<<")"<<std::endl;
  std::cout<<"     (DirX, DirY, DirZ) = ("<<fDirection.X()<<", "<<fDirection.Y()<<", "<<fDirection.Z()<<")"<<std::endl;
  std::cout<<"     Figure Of Merit = "<<fFOM<<std::endl;
  std::cout<<"-------------------------------------------------------"<<std::endl;
}
