#include "RecoRing.h"
#include "ANNIERecoObjectTable.h"

RecoRing::RecoRing(double vtxx, double vtxy, double vtxz, double dirx, double diry, double dirz, double angle, double height)
{
  fVtxX = vtxx;
  fVtxY = vtxy;
  fVtxZ = vtxz;
  fDirX = dirx;
  fDirY = diry;
  fDirZ = dirz;

  fAngle = angle;
  fHeight = height;

  ANNIERecoObjectTable::Instance()->NewRing();
}

RecoRing::~RecoRing()
{
  ANNIERecoObjectTable::Instance()->DeleteRing();
}

