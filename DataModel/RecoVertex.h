/// This class stores the vertex object
/// Jingbo Wang <jiowang@ucdavis.edu>
#ifndef RECOVERTEXCLASS_H
#define RECOVERTEXCLASS_H

#include<SerialisableObject.h>
#include "Position.h"
#include "Direction.h"
#include "ANNIERecoObjectTable.h"
using namespace std;

class RecoVertex : public SerialisableObject {
	
  friend class boost::serialization::access;
  	
  public: 
  typedef enum EFitStatus {
   kOK  = 0x00, //0
   kFailSimpleVertex    = 0x01, //1
   kFailSimpleDirection = 0x02, //2
   kFailPointPosition   = 0x04, //4
   kFailPointDirection  = 0x08, //8
   kFailPointVertex     = 0x10, //16
   kFailExtendedVertex  = 0x20, //32
   kFailCorrectedVertex = 0x40, //64
  } FitStatus_t;

  RecoVertex();
  RecoVertex( Position pos );
  RecoVertex( Position pos, Direction dir );
  RecoVertex( Position pos, double t, Direction dir,
  								 double fom, int nsteps, bool pass, int status );
  RecoVertex( Position pos, double t,Direction dir, 
                   double angle, double length,
                   double fom, int nsteps, bool pass, int status );
  ~RecoVertex();

  void SetVertex( Position pos, double t);
  void SetVertex( Position pos);
  void SetVertex( double vtxX, double vtxY, double vtxZ);
  void SetVertex( double vtxX, double vtxY, double vtxZ, double vtxT);
  void SetDirection( Direction dir);
  void SetDirection(double dirX, double dirY, double dirZ);
  void SetConeAngle( double angle );
  void SetTrackLength( double length );
  void SetFOM(double fom, int nsteps, bool pass);
  void SetTime(double vtxtime);
  void SetStatus(int status);
  RecoVertex* CloneVertex(RecoVertex* b); 

  Position GetPosition() { return fPosition;}
  double GetTime() { return fTime; }
  bool FoundVertex() { return fFoundVertex; }

  Direction GetDirection() { return fDirection; }
  bool FoundDirection() { return fFoundDirection; }

  double GetConeAngle() { return fConeAngle; }
  double GetTrackLength() { return fTrackLength; }

  double GetFOM() { return fFOM; }
  int GetIterations(){ return fIterations; }
  bool GetPass() { return fPass; }
  int GetStatus(){ return fStatus; }

  void Reset();
  bool Print();

  private:
  	
  Position fPosition;
  double fTime;
  Direction fDirection; 
  bool fFoundVertex;
  bool fFoundDirection;
  double fConeAngle;
  double fTrackLength;
  double fFOM;
  int fIterations;
  bool fPass;
  int fStatus;
  
  template<class Archive> void serialize(Archive & ar, const unsigned int version){
		if(serialise){
			ar & fPosition;
			ar & fTime;
			ar & fDirection;
			ar & fFoundVertex;
			ar & fFoundDirection;
			ar & fConeAngle;
			ar & fTrackLength;
			ar & fFOM;
			ar & fIterations;
			ar & fPass;
			ar & fStatus;
		}
	}

};

#endif







