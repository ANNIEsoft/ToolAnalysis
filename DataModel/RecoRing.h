#ifndef RECORING_H
#define RECORING_H
#include<SerialisableObject.h>

class RecoRing : public SerialisableObject {

  friend class boost::serialization::access;
 	
  public:
  RecoRing( double vtxx, double vtxy, double vtxz,
		 double dirx, double diry, double dirz,
                 double angle, double height );
  ~RecoRing();

  double GetVtxX() { return fVtxX; }
  double GetVtxY() { return fVtxY; }
  double GetVtxZ() { return fVtxZ; }

  double GetDirX() { return fDirX; }
  double GetDirY() { return fDirY; }
  double GetDirZ() { return fDirZ; }

  double GetAngle() { return fAngle; }
  double GetHeight() { return fHeight; }

  private:

  double fVtxX;
  double fVtxY;
  double fVtxZ;

  double fDirX;
  double fDirY;
  double fDirZ;

  double fAngle;
  double fHeight;


};

#endif







