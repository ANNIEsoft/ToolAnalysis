#include "ANNIEGeometry.h"
#include "Parameters.h"

#include "TFile.h"
#include "TTree.h"
#include "TDirectory.h"
#include "TMath.h"
#include "TVector3.h"
#include "TMatrixD.h"
#include "TError.h"

#include <iostream>
#include <cmath>
#include <cassert>


static ANNIEGeometry* fgGeometryHandle = 0;

bool ANNIEGeometry::TouchGeometry()
{
  if( ANNIEGeometry::Instance()->GetNumPMTs()>0 ){
    return true;
  }
  else{
    std::cout << " ***  ANNIEGeometry::TouchData() *** " << std::endl;
    std::cout << "  <error> need to build geometry... " << std::endl
              << "    Call: ANNIEGeometry::BuildGeometry(WCSimRootGeom*) " << std::endl;
    return false;
  }
}

ANNIEGeometry* ANNIEGeometry::Instance()
{
  if( !fgGeometryHandle ){
    fgGeometryHandle = new ANNIEGeometry();
  }

  return fgGeometryHandle;
}


void ANNIEGeometry::WriteGeometry(const char*){

}

void ANNIEGeometry::PrintGeometry()
{
  std::cout << " *** ANNIEGeometry::PrintGeometry() *** " << std::endl;

  if( ANNIEGeometry::TouchGeometry() ){

    // geometry type
    if( fgGeometryHandle->GetGeoType()==ANNIEGeometry::kCylinder ){
      std::cout << "  Detector Geometry: CYLINDER " << std::endl
		<< "   radius[m]=" <<  1.0e-2*fgGeometryHandle->GetCylRadius() << std::endl
                << "   length[m]=" << 1.0e-2*fgGeometryHandle->GetCylLength() << std::endl
                << "   fiducial_radius[m]=" <<  1.0e-2*fgGeometryHandle->GetCylFiducialRadius() << std::endl
                << "   fiducial_length[m]=" << 1.0e-2*fgGeometryHandle->GetCylFiducialLength() << std::endl;
      std::cout << "   area[m^2]=" << 1.0e-4*fgGeometryHandle->GetArea() << std::endl
		<< "   volume[m^3]=" << 1.0e-6*fgGeometryHandle->GetVolume() << std::endl
                << "   fiducial_volume[m^3]=" << 1.0e-6*fgGeometryHandle->GetFiducialVolume() << std::endl;
    }

    if( fgGeometryHandle->GetGeoType()==ANNIEGeometry::kUnknown ){
      std::cout << "  Detector Geometry: UNKNOWN " << std::endl;
      std::cout << "   <warning> geometry unknown! " << std::endl;
    }

    // Detector PMTs
    std::cout << "  Detector PMTs: " << std::endl;
    std::cout << "   number=" << fgGeometryHandle->GetNumPMTs() << std::endl;
    std::cout << "   radius[cm]=" << fgGeometryHandle->GetPMTRadius() << std::endl;
    std::cout << "   separation[cm]=" << fgGeometryHandle->GetPMTSeparation() << std::endl;
    std::cout << "   coverage=" << fgGeometryHandle->GetPMTCoverage() << std::endl;

  }

  else {
    std::cout << " <warning> no geometry loaded " << std::endl;
  }

  return;
}


ANNIEGeometry::ANNIEGeometry() :
  fPMTs(0),
  fPmtX(0),
  fPmtY(0),
  fPmtZ(0),
  fPmtNormX(0),
  fPmtNormY(0),
  fPmtNormZ(0),
  fPmtRegion(0)
{
  this->SetGeometry();
}

ANNIEGeometry::~ANNIEGeometry()
{
  if( fPmtX )       delete [] fPmtX;
  if( fPmtY )       delete [] fPmtY;
  if( fPmtZ )       delete [] fPmtZ;
  if( fPmtNormX )   delete [] fPmtNormX;
  if( fPmtNormY )   delete [] fPmtNormY;
  if( fPmtNormZ )   delete [] fPmtNormZ;
  if( fPmtRegion )  delete [] fPmtRegion;
}

Int_t ANNIEGeometry::GetRegion( Int_t tube )
{
  if( tube>=0 && tube<fPMTs ){
    return fPmtRegion[tube];
  }

  return -1;
}

double ANNIEGeometry::GetX( Int_t tube )
{
  if( tube>=0 && tube<fPMTs ){
    return fPmtX[tube];
  }

  return -99999.9;
}

double ANNIEGeometry::GetY( Int_t tube )
{
  if( tube>=0 && tube<fPMTs ){
    return fPmtY[tube];
  }

  return -99999.9;
}

double ANNIEGeometry::GetZ( Int_t tube )
{
  if( tube>=0 && tube<fPMTs ){
    return fPmtZ[tube];
  }

  return -99999.9;
}

double ANNIEGeometry::GetNormX( Int_t tube )
{
  if( tube>=0 && tube<fPMTs ){
    return fPmtNormX[tube];
  }

  return -99999.9;
}

double ANNIEGeometry::GetNormY( Int_t tube )
{
  if( tube>=0 && tube<fPMTs ){
    return fPmtNormY[tube];
  }

  return -99999.9;
}

double ANNIEGeometry::GetNormZ( Int_t tube )
{
  if( tube>=0 && tube<fPMTs ){
    return fPmtNormZ[tube];
  }

  return -99999.9;
}

void ANNIEGeometry::Reset()
{
  if( ANNIEGeometry::Instance()->GetNumPMTs()>0 ){
    std::cout << " *** ANNIEGeometry::Reset() *** " << std::endl;
  }

  ANNIEGeometry::Instance()->SetGeometry();
}

void ANNIEGeometry::BuildGeometry(){

}

void ANNIEGeometry::SetGeometry(){
  fGeoType = ANNIEGeometry::kCylinder;
  fCylinder = 1;
  
  //WCSim
  fCylRadius = 152.0; //cm
  fCylLength = 396.0; //cm
  fCylFiducialRadius = 0.0;
  fCylFiducialLength = 0.0;
  fXoffset = 0.0;
  fYoffset = 0.0; 
  fZoffset = 0.0;

  fPMTs = 128;
  fLAPPDs = 5;

  fPmtX = new double[fPMTs];
  fPmtY = new double[fPMTs];
  fPmtZ = new double[fPMTs];
  fPmtNormX = new double[fPMTs];
  fPmtNormY = new double[fPMTs];
  fPmtNormZ = new double[fPMTs];
  fPmtRegion = new Int_t[fPMTs];

  for( Int_t n=0; n<fPMTs; n++ ){
    fPmtX[n] = -99999.9;
    fPmtY[n] = -99999.9;
    fPmtZ[n] = -99999.9;
    fPmtNormX[n] = -99999.9;
    fPmtNormY[n] = -99999.9;
    fPmtNormZ[n] = -99999.9;
    fPmtRegion[n] = -1;
  }

  // Calculate PMT Coverage
  // ======================

  // MW !!!! Nope
  fPMTRadius=200.;
  fPMTSurfaceArea = 0.0;
  fPMTCoverage = 0.0;
  fPMTSeparation = 0.0;

  if( fPMTs>0 && fDetectorArea>0.0 ){
    fPMTSurfaceArea = fPMTs*TMath::Pi()*fPMTRadius*fPMTRadius;
    fPMTCoverage = fPMTSurfaceArea/fDetectorArea;
    fPMTSeparation = sqrt(fDetectorArea/fPMTs);
  }
/*
  // Determine Detector Configuration
  // ================================
  std::cout << "   determing detector configuration " << std::endl;

  double kton = 0.5+1.0e-9*fDetectorFiducialVolume;
  Int_t kton_rounded = (Int_t)(10.0*(Int_t)(0.1*kton));
  double pmtdiam = 0.5+(2.0/2.54)*fPMTRadius;
  Int_t pmtdiam_rounded = (Int_t)(2.0*(Int_t)(0.5*pmtdiam));
  double coverage = 0.5 + 100.0*fPMTCoverage;
  Int_t coverage_rounded = (Int_t)(coverage);

  std::cout << "    DUSEL_" << kton_rounded << "kton";
  if( fgGeometryHandle->GetGeoType()==ANNIEGeometry::kMailBox ) std::cout << "Mailbox";
  std::cout << "_";
  std::cout << pmtdiam_rounded
            << "inch_" << coverage_rounded
            << "perCent " << std::endl;

  // Print Geometry
  // ==============
  PrintGeometry();*/

  return;
}

void ANNIEGeometry::WriteToFile(const char* filename)
{
  std::cout << " *** ANNIEGeometry::WriteGeometry() *** " << std::endl;

  if( fPMTs<=0 ){
    std::cout << "  warning: no geometry loaded " << std::endl;
    return;
  }

  std::cout << "  writing geometry to: "  << filename << std::endl;

  TDirectory* tmpd = 0;
  TFile* fFileGeometry = 0;
  TTree* fTreeGeometry = 0;

  if( !fFileGeometry ){
    tmpd = gDirectory;
    fFileGeometry = new TFile(filename,"recreate");
    fTreeGeometry = new TTree("ntuple","detector geometry");
    fTreeGeometry->Branch("tube",&fTube,"tube/I");
    fTreeGeometry->Branch("region",&fRegion,"region/I");
    fTreeGeometry->Branch("x",&fXpos,"x/D");
    fTreeGeometry->Branch("y",&fYpos,"y/D");
    fTreeGeometry->Branch("z",&fZpos,"z/D");
    fTreeGeometry->Branch("nx",&fXnorm,"nx/D");
    fTreeGeometry->Branch("ny",&fYnorm,"ny/D");
    fTreeGeometry->Branch("nz",&fZnorm,"nz/D");
    fTreeGeometry->SetAutoSave(100);
    gDirectory = tmpd;
  }

  for( Int_t n =0; n<fPMTs; n++ ){
    fTube = n;
    fRegion = fPmtRegion[n];
    fXpos  = fPmtX[n];
    fYpos  = fPmtY[n];
    fZpos  = fPmtZ[n];
    fXnorm = fPmtNormX[n];
    fYnorm = fPmtNormY[n];
    fZnorm = fPmtNormZ[n];

    if( fRegion>=0 ){
      tmpd = gDirectory;
      fFileGeometry->cd();
      fTreeGeometry->Fill();
      gDirectory = tmpd;
    }
  }

  tmpd = gDirectory;
  fFileGeometry->cd();
  fTreeGeometry->Write();
  fFileGeometry->Close();
  fFileGeometry = 0;
  fTreeGeometry = 0;
  gDirectory = tmpd;

  return;
}

bool ANNIEGeometry::InsideDetector(double x, double y, double z)
{
//  if( fGeoType==ANNIEGeometry::kCylinder ){
//    if( z>=-0.5*fCylLength && z<=+0.5*fCylLength
//     && x*x+y*y<=fCylRadius*fCylRadius ){
//      return 1;
//    }
//  }
  double xx = x - fXoffset;
  double yy = y - fYoffset;
  double zz = z - fZoffset;
  if(sqrt(x*x+z*z)<fCylRadius 
    	&& y>-0.5*fCylLength && y<0.5*fCylLength)
    	return 1;
  return 0;
}

bool ANNIEGeometry::InsideFiducialVolume(double x, double y, double z)
{
  if( fGeoType==ANNIEGeometry::kCylinder ){
    if( z>=-0.5*fCylFiducialLength && z<=+0.5*fCylFiducialLength
     && x*x+y*y<=fCylFiducialRadius*fCylFiducialRadius ){
      return 1;
    }
  } else std::cout<<"WTF ANNIE Should Be A Cylinder!!!!"<<std::endl;

  return 0;
}

bool ANNIEGeometry::InsideDetector(double vx, double vy, double vz, double ex, double ey, double ez)
{
  double dx = ex-vx;
  double dy = ey-vy;
  double dz = ez-vz;
  double ds = sqrt(dx*dx+dy*dy+dz*dz);

  double px = dx/ds;
  double py = dy/ds;
  double pz = dz/ds;

  if( this->ForwardProjectionToEdge(vx,vy,vz,px,py,pz) >= 0.0
   && this->BackwardProjectionToEdge(ex,ey,ez,px,py,pz) >= 0.0 ){
    return 1;
  }
  else{
    return 0;
  }
}

double ANNIEGeometry::DistanceToEdge(double x, double y, double z)
{
  // Cylindrical Geometry
  // ====================
  if( fGeoType==ANNIEGeometry::kCylinder ){

    // inside detector (convention: +ve dr)
    if( this->InsideDetector(x,y,z) ){
      double dr = 0.0;
      if( fCylRadius>dr ) dr = fCylRadius;
      if( 0.5*fCylLength>dr ) dr = 0.5*fCylLength;
      if( -sqrt(x*x+z*z)+fCylRadius<dr ) dr = -sqrt(x*x+z*z)+fCylRadius;
      if( -y+0.5*fCylLength<dr ) dr = -y+0.5*fCylLength;
      if( y+0.5*fCylLength<dr ) dr = y+0.5*fCylLength;
      return dr;
    }

    // outside detector (convention: -ve dr)
    else{

      // side region
      if( y>=-0.5*fCylLength && y<=+0.5*fCylLength ){
        return -sqrt(x*x+z*z)+fCylRadius;
      }

      // below tank
      if( y<=-0.5*fCylLength
       && x*x+z*z<fCylRadius*fCylRadius ){
        return y+0.5*fCylLength;
      }
      // above tank
      if( y>=+0.5*fCylLength
       && x*x+z*z<fCylRadius*fCylRadius ){
        return -y+0.5*fCylLength;
      }

      // corner regions
      if( y>=+0.5*fCylLength
       && x*x+z*z>=fCylRadius ){
        double dr = sqrt(x*x+z*z)-fCylRadius;
        double dy = -y+0.5*fCylLength;
        return -sqrt(dr*dr+dy*dy);
      }
      if( y<=-0.5*fCylLength
       && x*x+z*z>=fCylRadius ){
        double dr = sqrt(x*x+z*z)-fCylRadius;
        double dy = y+0.5*fCylLength;
        return -sqrt(dr*dr+dy*dy);
      }
    }
  }

  return -99999.9;
}


double ANNIEGeometry::ForwardProjectionToEdge(double x, double y, double z, double px, double py, double pz)
{
  double xproj = 0.0;
  double yproj = 0.0;
  double zproj = 0.0;
  Int_t regionproj = 0;

  this->ProjectToNearEdge(x,y,z,
                          px,py,pz,
                          xproj,yproj,zproj,
                          regionproj);

  if( regionproj > ANNIEGeometry::kUnknown ){
    return sqrt( (xproj-x)*(xproj-x)
               + (yproj-y)*(yproj-y)
               + (zproj-z)*(zproj-z) );
  }

  this->ProjectToNearEdge(x,y,z,
                          -px,-py,-pz,
                          xproj,yproj,zproj,
                          regionproj);

  if( regionproj > ANNIEGeometry::kUnknown ){
    return -sqrt( (xproj-x)*(xproj-x)
                + (yproj-y)*(yproj-y)
                + (zproj-z)*(zproj-z) );
  }

  return -99999.9;
}

double ANNIEGeometry::BackwardProjectionToEdge(double x, double y, double z, double px, double py, double pz)
{
  double xproj = 0.0;
  double yproj = 0.0;
  double zproj = 0.0;
  Int_t regionproj = 0;

  this->ProjectToNearEdge(x,y,z,
                          -px,-py,-pz,
                          xproj,yproj,zproj,
                          regionproj);

  if( regionproj > ANNIEGeometry::kUnknown ){
    return sqrt( (xproj-x)*(xproj-x)
               + (yproj-y)*(yproj-y)
               + (zproj-z)*(zproj-z) );
  }

  this->ProjectToNearEdge(x,y,z,
                          px,py,pz,
                          xproj,yproj,zproj,
                          regionproj);

  if( regionproj > ANNIEGeometry::kUnknown ){
    return -sqrt( (xproj-x)*(xproj-x)
                + (yproj-y)*(yproj-y)
                + (zproj-z)*(zproj-z) );
  }

  return -99999.9;
}

void ANNIEGeometry::ProjectToNearEdge(double x0, double y0, double z0, double px, double py, double pz, double& xproj, double& yproj, double&zproj, Int_t& regionproj)
{
  this->ProjectToEdge(0,
                      x0,y0,z0,
                      px,py,pz,
                      xproj,yproj,zproj,
                      regionproj);

  //
  // this->ProjectToEdgeOld(0,
  //                        x0,y0,z0,
  //                        px,py,pz,
  //                        xproj,yproj,zproj,
  //                        regionproj);
  //

  return;
}

void ANNIEGeometry::ProjectToFarEdge(double x0, double y0, double z0, double px, double py, double pz, double& xproj, double& yproj, double&zproj, Int_t& regionproj)
{
  this->ProjectToEdge(1,
                      x0,y0,z0,
                      px,py,pz,
                      xproj,yproj,zproj,
                      regionproj);

  return;
}

//coordinates adapted to ANNIE
void ANNIEGeometry::ProjectToEdge(bool useFarEdge, double x0, double y0, double z0, double px, double py, double pz, double& xproj, double& yproj, double&zproj, Int_t& regionproj)
{
  // default locations
  // =================
  xproj = -99999.9;
  yproj = -99999.9;
  zproj = -99999.9;
  regionproj = ANNIEGeometry::kUnknown;

  double xNear = -99999.9;
  double yNear = -99999.9;
  double zNear = -99999.9;
  Int_t regionNear = ANNIEGeometry::kUnknown;

  double xFar = -99999.9;
  double yFar = -99999.9;
  double zFar = -99999.9;
  Int_t regionFar = ANNIEGeometry::kUnknown;


  // CYLINDRICAL GEOMETRY
  // ====================
  if( fGeoType==ANNIEGeometry::kCylinder ){  

    double r = fCylRadius;
    double L = fCylLength;

    bool foundProjectionZX = 0;
    bool foundProjectionY = 0;

    double t1 = 0.0;
    double x1 = 0.0;
    double y1 = 0.0;
    double z1 = 0.0;
    Int_t region1 = -1;

    double t2 = 0.0;  
    double x2 = 0.0;
    double y2 = 0.0;
    double z2 = 0.0;
    Int_t region2 = -1;

    double rSq = r*r;
    double r0r0 = z0*z0 + x0*x0;
    double r0p = z0*pz + x0*px;
    double pSq = pz*pz+px*px;
    
    // calculate intersection in ZX
    if( pSq>0.0 ){
      if( r0p*r0p - pSq*(r0r0-rSq)>0.0 ){
        t1 = ( -r0p - sqrt(r0p*r0p-pSq*(r0r0-rSq)) ) / pSq;
	t2 = ( -r0p + sqrt(r0p*r0p-pSq*(r0r0-rSq)) ) / pSq;
        foundProjectionZX = 1;
      }
    }

    // propagation along y-axis
    else if( r0r0<=rSq ){

      if( py>0 ){
        t1 = -L/2.0 - y0;
        t2 = +L/2.0 - y0;
      }
      else{
        t1 = -L/2.0 + y0;
        t2 = +L/2.0 + y0;
      }
      foundProjectionZX = 1;
    }
    
    // found intersection in ZX
      if( foundProjectionZX ){

      y1 = y0 + t1*py;
      y2 = y0 + t2*py;

      if( ( y1>=-L/2.0 && y2<=+L/2.0 )
       || ( y2>=-L/2.0 && y1<=+L/2.0 ) ){
        foundProjectionY = 1;
      }
    }

    // found intersection in Y
    if( foundProjectionY ){

      // first intersection
      if( y1>-L/2.0 && y1<+L/2.0 ){
        region1 = ANNIEGeometry::kSide;
      }
      if( y1>=+L/2.0 ){
        region1 = ANNIEGeometry::kTop;
        if( y1>+L/2.0 ){
          y1 = +L/2.0; 
          t1 = (+L/2.0-y0)/py;
	}
      }
      if( y1<=-L/2.0 ){
        region1 = ANNIEGeometry::kBottom;
        if( y1<-L/2.0 ){
          y1 = -L/2.0; 
          t1 = (-L/2.0-y0)/py;
	}
      }

      z1 = z0 + t1*pz;
      x1 = x0 + t1*px;

      // second intersection
      if( y2>-L/2.0 && y2<+L/2.0 ){
        region2 = ANNIEGeometry::kSide;
      }
      if( y2>=+L/2.0 ){
        region2 = ANNIEGeometry::kTop;
        if( y2>+L/2.0 ){
          y2 = +L/2.0; 
          t2 = (+L/2.0-y0)/py;
	}
      }
      if( y2<=-L/2.0 ){
        region2 = ANNIEGeometry::kBottom;
        if( y2<-L/2.0 ){
          y2 = -L/2.0; 
          t2 = (-L/2.0-y0)/py;
	}
      }

      z2 = z0 + t2*pz;
      x2 = x0 + t2*px;

      // near/far projection
      if( t1>=0 ){
        xNear = x1;
        yNear = y1;
        zNear = z1;
        regionNear = region1;

        xFar = x2;
        yFar = y2;
        zFar = z2;
        regionFar = region2;
      }
      else if( t2>0 ){
        xNear = x2;
        yNear = y2;
        zNear = z2;
        regionNear = region2;

        xFar = x2;
        yFar = y2;
        zFar = z2;
        regionFar = region2;
      }
    }
 
  }
  // set projections
  // ===============
  if( useFarEdge ){
    xproj = xFar;
    yproj = yFar;
    zproj = zFar;
    regionproj = regionFar;
  }
  else{
    xproj = xNear;
    yproj = yNear;
    zproj = zNear;
    regionproj = regionNear;
  }
  return;
}

// Convert 3D coordinates to 2D conordinates (J.Wang), coordinates adapted to ANNIE
void ANNIEGeometry::XYZtoUV(Int_t region, double x, double y, double z, double& u, double& v)
{
  // default locations
  u = -99999.9;
  v = -99999.9;

  // CYLINDRICAL GEOMETRY
  // ====================
  if( fGeoType==ANNIEGeometry::kCylinder ){

    // PMTs on top face
    if( region==ANNIEGeometry::kTop ){
      u = x;
      v = +0.5*fCylLength+fCylRadius-z;
    }

    // PMTs on side
    if( region==ANNIEGeometry::kSide ){
      double theta = 0.0;
      if( z != 0.0 ) theta = TMath::ATan(x/z);
      if( z<=0 ){
        if( x>0.0 ) theta += TMath::Pi();
        if( x<0.0 ) theta -= TMath::Pi();
      }
      u = fCylRadius*theta;
      v = y;
    }

    // PMTs on bottom face
    if( region==ANNIEGeometry::kBottom ){
      u = x;
      v = -0.5*fCylLength-fCylRadius+z;
    }
  }
  return;
}

void ANNIEGeometry::FindCircle(double x0, double y0, double z0, 
	                             double x1, double y1, double z1, 
	                             double x2, double y2, double z2, 
	                             double& rx, double& ry, double& rz, 
	                             double& nx, double& ny, double& nz, 
	                             double& r)
{
  double centre[3] = {0.0,0.0,0.0};
  double normal[3] = {0.0,0.0,0.0};
  double radius = 0.0;

  double p01[3] = {x0-x1, y0-y1, z0-z1};
  double p12[3] = {x1-x2, y1-y2, z1-z2};
  double p20[3] = {x2-x0, y2-y0, z2-z0};

  double n[3] = {p01[1]*p12[2]-p01[2]*p12[1],
                   p01[2]*p12[0]-p01[0]*p12[2],
                   p01[0]*p12[1]-p01[1]*p12[0]};

  double D2 = n[0]*n[0]+n[1]*n[1]+n[2]*n[2];

  if( D2>0.0 ){
    double a0 = -(p12[0]*p12[0]+p12[1]*p12[1]+p12[2]*p12[2])*(p01[0]*p20[0]+p01[1]*p20[1]+p01[2]*p20[2])/(2.0*D2);
    double a1 = -(p20[0]*p20[0]+p20[1]*p20[1]+p20[2]*p20[2])*(p12[0]*p01[0]+p12[1]*p01[1]+p12[2]*p01[2])/(2.0*D2);
    double a2 = -(p01[0]*p01[0]+p01[1]*p01[1]+p01[2]*p01[2])*(p20[0]*p12[0]+p20[1]*p12[1]+p20[2]*p12[2])/(2.0*D2);
    double D = sqrt(D2);

    centre[0] = a0*x0 + a1*x1 + a2*x2;
    centre[1] = a0*y0 + a1*y1 + a2*y2;
    centre[2] = a0*z0 + a1*z1 + a2*z2;

    radius = sqrt( (p01[0]*p01[0]+p01[1]*p01[1]+p01[2]*p01[2])
                  *(p12[0]*p12[0]+p12[1]*p12[1]+p12[2]*p12[2])
                  *(p20[0]*p20[0]+p20[1]*p20[1]+p20[2]*p20[2]))/(2.0*D);

    if( n[0]*centre[0]
      + n[1]*centre[1]
      + n[2]*centre[2]>0.0 ){
      normal[0] = +n[0]/D;
      normal[1] = +n[1]/D;
      normal[2] = +n[2]/D;
    }
    else{
      normal[0] = -n[0]/D;
      normal[1] = -n[1]/D;
      normal[2] = -n[2]/D;
    }
  }

  rx = centre[0];
  ry = centre[1];
  rz = centre[2];

  nx = normal[0];
  ny = normal[1];
  nz = normal[2];

  r = radius;

  return;
}

void ANNIEGeometry::FindCircleOld( double xp, double yp, double zp, 
	                                 double x0, double y0, double z0, 
	                                 double angle_degrees, double omega_degrees, 
	                                 double& rx, double& ry, double& rz, 
	                                 double& nx, double& ny, double& nz, 
	                                 double& r )
{
  // inputs
  // ======
  double phi = 0.0;
  double theta = 0.0;
  double angle = (TMath::Pi()/180.0)*angle_degrees;
  double omega = (TMath::Pi()/180.0)*omega_degrees;

  double x = xp;
  double y = yp;
  double z = zp;

  double xtemp = 0.0;
  double ytemp = 0.0;
  double ztemp = 0.0;

  double dx = 0.0;
  double dy = 0.0;
  double dz = 0.0;
  double ds = 0.0;

  double radius = 0.0;

  // subtract vertex
  // ===============
  x -= x0;
  y -= y0;
  z -= z0;

  // forward rotation (x,y,z)->(x,0,z)
  // =================================
  phi = 0.0;

  if( x!=0.0 ){
    phi = atan(y/x);
  }
  if( x<=0.0 ){
    if( y>0.0 ) phi += TMath::Pi();
    if( y<0.0 ) phi -= TMath::Pi();
  }

  xtemp = x;
  ytemp = y;
  ztemp = z;

  x = + xtemp*cos(phi) + ytemp*sin(phi);
  y = - xtemp*sin(phi) + ytemp*cos(phi);
  z = + ztemp;

  // forward rotation (x,0,z)->(0,0,z)
  // =================================
  theta = 0.0;

  if( z!=0.0 ){
    theta = atan(x/z);
  }
  if( z<=0.0 ){
    if( x>0.0 ) theta += TMath::Pi();
    if( x<0.0 ) theta -= TMath::Pi();
  }

  xtemp = x;
  ytemp = y;
  ztemp = z;

  x = + xtemp*cos(theta) - ztemp*sin(theta);
  y = + ytemp;
  z = + xtemp*sin(theta) + ztemp*cos(theta);

  // apply rotation: cone angle + azimuthal angle
  // ============================================
  xtemp = x;
  ytemp = y;
  ztemp = z;

  x = ztemp*sin(angle)*cos(omega);
  y = ztemp*sin(angle)*sin(omega);
  z = ztemp*cos(angle);

  radius = ztemp*sin(angle);

  // backward rotation (0,0,z)->(x,0,z)
  // ==================================
  xtemp = x;
  ytemp = y;
  ztemp = z;

  x = + xtemp*cos(-theta) - ztemp*sin(-theta);
  y = + ytemp;
  z = + xtemp*sin(-theta) + ztemp*cos(-theta);

  // backward rotation (x,0,z)->(x,y,z)
  // =================================
  xtemp = x;
  ytemp = y;
  ztemp = z;

  x = + xtemp*cos(-phi) + ytemp*sin(-phi);
  y = - xtemp*sin(-phi) + ytemp*cos(-phi);
  z = + ztemp;

  // add on vertex
  // =============
  x += x0;
  y += y0;
  z += z0;

  // return coordinates
  // ==================
  rx = x;
  ry = y;
  rz = z;

  // return normal
  // =============
  dx = rx - x0;
  dy = ry - y0;
  dz = rz - z0;
  ds = sqrt(dx*dx+dy*dy+dz*dz);

  nx = dx/ds;
  ny = dy/ds;
  nz = dz/ds;

  // return radius
  // =============
  r = radius;

  return;
}

void ANNIEGeometry::FindCircle( double xp, double yp, double zp, 
	                              double x0, double y0, double z0, 
	                              double angle_degrees, double omega_degrees, 
	                              double& rx, double& ry, double& rz, 
	                              double& nx, double& ny, double& nz, 
	                              double& r )
{
  // default
  // =======
  rx = xp;
  ry = yp;
  rz = zp;

  nx = 0.0;
  ny = 0.0;
  nz = 0.0;

  r = 0.0;

  // inputs
  // ======
  double angle = (TMath::Pi()/180.0)*angle_degrees; // radians
  double omega = (TMath::Pi()/180.0)*omega_degrees; // radians

  double dx = xp-x0;
  double dy = yp-y0;
  double dz = zp-z0;
  double ds = sqrt(dx*dx+dy*dy+dz*dz);

  double px = 0.0;
  double py = 0.0;
  double pz = 0.0;

  if( ds>0.0 ){
    px = dx/ds;
    py = dy/ds;
    pz = dz/ds;
  }
  else{
    return;
  }

  // do the rotations
  // ================
  TVector3 myVtx(x0,y0,z0);
  TVector3 myDir(px,py,pz);

  TVector3 initDir = myDir;
  TVector3 orthDir = myDir.Orthogonal();
  myDir.Rotate(angle,orthDir);
  myDir.Rotate(omega,initDir);

  // outputs
  // =======
  rx = x0 + ds*myDir.x();
  ry = y0 + ds*myDir.y();
  rz = z0 + ds*myDir.z();

  nx = myDir.x();
  ny = myDir.y();
  nz = myDir.z();

  r = ds*sin(angle);

  return;
}

void ANNIEGeometry::FindVertex(double x0, double y0, double z0, double t0, double x1, double y1, double z1, double t1, double x2, double y2, double z2, double t2, double x3, double y3, double z3, double t3, double& vxm, double& vym, double& vzm, double& vtm, double& vxp, double& vyp, double& vzp, double& vtp)
{
  // switch off error messages
  // =========================
  // suppress messages like:
  //  Error in <TDecompLU::DecomposeLUCrout>: matrix is singular
  Int_t myIgnoreLevel = gErrorIgnoreLevel;
  gErrorIgnoreLevel = kFatal;

  // default vertex
  // ==============
  vxm = -99999.9;
  vym = -99999.9;
  vzm = -99999.9;
  vtm = -99999.9;

  vxp = -99999.9;
  vyp = -99999.9;
  vzp = -99999.9;
  vtp = -99999.9;

  // speed of light in water
  // =======================
  double c = 29.98/1.33; // cm/ns [water]

  // causality checks
  // ================
  bool causality = (x1-x0)*(x1-x0) + (y1-y0)*(y1-y0) + (z1-z0)*(z1-z0) >= c*c*(t1-t0)*(t1-t0)
   && (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) + (z2-z1)*(z2-z1) >= c*c*(t2-t1)*(t2-t1)
   && (x3-x2)*(x3-x2) + (y3-y2)*(y3-y2) + (z3-z2)*(z3-z2) >= c*c*(t3-t2)*(t3-t2)
   && (x2-x0)*(x2-x0) + (y2-y0)*(y2-y0) + (z2-z0)*(z2-z0) >= c*c*(t2-t0)*(t2-t0)
   && (x3-x1)*(x3-x1) + (y3-y1)*(y3-y1) + (z3-z1)*(z3-z1) >= c*c*(t3-t1)*(t3-t1)
   && (x3-x0)*(x3-x0) + (y3-y0)*(y3-y0) + (z3-z0)*(z3-z0) >= c*c*(t3-t0)*(t3-t0);
  //std::cout<<"Does the fout hits pass the causality check? "<<causality<<std::endl;
  if( causality ){

    // [Note: for causality, require that |x_{i}-x_{j}| >= c*|t_{i}-t_{j}|
    //        for each pair of points]

    double dx1 = x1-x0;  double dy1 = y1-y0;  double dz1 = z1-z0;  double dt1 = c*(t1-t0);
    double dx2 = x2-x0;  double dy2 = y2-y0;  double dz2 = z2-z0;  double dt2 = c*(t2-t0);
    double dx3 = x3-x0;  double dy3 = y3-y0;  double dz3 = z3-z0;  double dt3 = c*(t3-t0);

    double epsilon = 1.0e-7;
    
    // check that points don't all lie in a plane
    bool planecheck = !( fabs(dx1)<epsilon && fabs(dx2)<epsilon && fabs(dx3)<epsilon )
     && !( fabs(dy1)<epsilon && fabs(dy2)<epsilon && fabs(dy3)<epsilon )
     && !( fabs(dz1)<epsilon && fabs(dz2)<epsilon && fabs(dz3)<epsilon )
     && !( fabs(dx1)<epsilon && fabs(dy1)<epsilon && fabs(dz1)<epsilon )
     && !( fabs(dx2)<epsilon && fabs(dy2)<epsilon && fabs(dz2)<epsilon )
     && !( fabs(dx3)<epsilon && fabs(dy3)<epsilon && fabs(dz3)<epsilon ) ;
    //std::cout<<"Is the four hits in the same plane? "<<!notinplane<<std::endl;
    
    // check the distance between each pair of points, added by JWang
    double dx12 = x2-x1;  double dy12 = y2-y1;  double dz12 = z2-z1;  double dt12 = c*(t2-t1);
    double dx13 = x3-x1;  double dy13 = y3-y1;  double dz13 = z3-z1;  double dt13 = c*(t3-t1);
    double dx23 = x3-x2;  double dy23 = y3-y2;  double dz23 = z3-z2;  double dt23 = c*(t3-t2);   
    
    double dr01 = sqrt(dx1*dx1 + dy1*dy1 + dz1*dz1);
    double dr02 = sqrt(dx2*dx2 + dy2*dy2 + dz2*dz2);
    double dr03 = sqrt(dx3*dx3 + dy3*dy3 + dz3*dz3);  
    double dr12 = sqrt(dx12*dx12 + dy12*dy12 + dz12*dz12);
    double dr13 = sqrt(dx13*dx13 + dy13*dy13 + dz13*dz13);
    double dr23 = sqrt(dx23*dx23 + dy23*dy23 + dz23*dz23);
    
    double drmin = 50.0; //50cm
    bool distancecheck = dr01>drmin && dr02>drmin && dr03>drmin && dr12>drmin && dr13>drmin && dr23>drmin;
    ///////////////////////////////////////////////////
    if( planecheck && distancecheck){
//    if( planecheck){

      // [Note: this is a problem for detectors with flat faces!]

      double Mdata[9] = { dx1, dy1, dz1,
                            dx2, dy2, dz2,
                            dx3, dy3, dz3 };

      double Qdata[3] = { 0.5*( dx1*dx1 + dy1*dy1 + dz1*dz1 - dt1*dt1 ),
                            0.5*( dx2*dx2 + dy2*dy2 + dz2*dz2 - dt2*dt2 ),
                            0.5*( dx3*dx3 + dy3*dy3 + dz3*dz3 - dt3*dt3 ) };

      double Tdata[3] = { dt1,
                            dt2,
                            dt3 };

      TMatrixD M(3,3,Mdata);
      TMatrixD Q(3,1,Qdata);
      TMatrixD T(3,1,Tdata);

      if( M.Determinant() != 0.0 ){

        TMatrixD A(3,1);
        TMatrixD B(3,1);

        M.Invert();
        A.Mult(M,T);
        B.Mult(M,Q);

        double ax = A(0,0);
        double ay = A(1,0);
        double az = A(2,0);

        double bx = B(0,0);
        double by = B(1,0);
        double bz = B(2,0);

        double ab = ax*bx + ay*by + az*bz;
        double a2 = ax*ax + ay*ay + az*az;
        double b2 = bx*bx + by*by + bz*bz;

        double qa = a2-1.0;
        double qb = 2.0*ab;
        double qc = b2;

        // check for solutions
        if( qb*qb-4.0*qa*qc>0.0 ){

	  // The common vertex is given by a quadratic equation, which has two solutions.
          // Typically, one solution corresponds to photons travelling forwards in time,
          // and the other solution corresponds to photons travelling backwards in time.
          // However, sometimes there appear to be two valid solutions.

          double ctm = ( -qb - sqrt(qb*qb-4.0*qa*qc) ) / ( 2.0*qa );
          double ctp = ( -qb + sqrt(qb*qb-4.0*qa*qc) ) / ( 2.0*qa );

          double tm = t0 + ctm/c;
          double xm = x0 + ctm*ax + bx;
          double ym = y0 + ctm*ay + by;
          double zm = z0 + ctm*az + bz;
          bool foundVertexM = 0;

          if( tm<t0 && tm<t1
           && tm<t2 && tm<t3 ){
            vxm = xm;
            vym = ym;
            vzm = zm;
            vtm = tm;
            foundVertexM = 1;
          }

          double tp = t0 + ctp/c;
          double xp = x0 + ctp*ax + bx;
          double yp = y0 + ctp*ay + by;
          double zp = z0 + ctp*az + bz;
          bool foundVertexP = 0;

          if( tp<t0 && tp<t1
           && tp<t2 && tp<t3 ){
            vxp = xp;
            vyp = yp;
            vzp = zp;
            vtp = tp;
            foundVertexP = 1;
	  }

          // std::cout << "  Vertex: [a=" << qa << ",b=" << qb << ",c=" << qc << "] [ctm=" << ctm << ",ctp=" << ctp << "] " << std::endl
          //           << "   Low T:  (x,y,z,t)=(" << xm << "," << ym << "," << zm << "," << tm << ") [foundVtx=" << foundVertexM << "] " << std::endl
          //           << "   High T: (x,y,z,t)=(" << xp << "," << yp << "," << zp << "," << tp << ") [foundVtx=" << foundVertexP << "] " << std::endl;

	}
      }
    }
  }

  // switch on error messages
  // ========================
  gErrorIgnoreLevel = myIgnoreLevel;

  return;
}



void ANNIEGeometry::DistanceToIntersectLine(double x0, double y0, double z0, double vx, double vy, double vz, double ex, double ey, double ez, double& x, double& y, double& z, double& L)
{
  double dx = 0.0;
  double dy = 0.0;
  double dz = 0.0;
  double ds = 0.0;

  double px = 0.0;
  double py = 0.0;
  double pz = 0.0;
  double dsTrack = 0.0;

  double qx = 0.0;
  double qy = 0.0;
  double qz = 0.0;
  double dsPmt = 0.0;

  // vector from vertex to end
  dx = ex - vx;
  dy = ey - vy;
  dz = ez - vz;
  ds = sqrt( dx*dx+dy*dy+dz*dz );

  if( ds>0.0 ){
    px = dx/ds;
    py = dy/ds;
    pz = dz/ds;
    dsTrack = ds;
  }

  // vector from vertex to pmt
  dx = x0 - vx;
  dy = y0 - vy;
  dz = z0 - vz;
  ds = sqrt( dx*dx+dy*dy+dz*dz );

  if( ds>0.0 ){
    qx = dx/ds;
    qy = dy/ds;
    qz = dz/ds;
    dsPmt = ds;
  }

  // Cherenkov geometry
  // ==================
  double cosphi = px*qx+py*qy+pz*qz;
  double phi = acos(cosphi);
  double theta = Parameters::CherenkovAngle()*TMath::Pi()/180.0;

  double Ltrack = 0.0;
  double Lphoton = 0.0;

  if( phi<theta ){
    Ltrack  = +dsPmt*sin(theta-phi)/sin(theta);
    Lphoton = +dsPmt*sin(phi)/sin(theta);
  }
  else{
    Ltrack  = -dsPmt*sin(phi-theta)/sin(phi);
    Lphoton = -dsPmt*sin(phi)/sin(theta);
  }

  // return intersection
  // ===================
  x = vx + px*Ltrack;
  y = vy + py*Ltrack;
  z = vz + pz*Ltrack;
  L = Lphoton;

  return;
}


double ANNIEGeometry::DistanceToIntersectLine(double x0, double y0, double z0, double sx, double sy, double sz, double ex, double ey, double ez, double& x, double& y, double& z)
{
  double L = -1.0;

  ANNIEGeometry::DistanceToIntersectLine(x0,y0,z0,
                                         sx,sy,sz,
                                         ex,ey,ez,
                                         x,y,z,L);
  return L;
}

double ANNIEGeometry::DistanceToIntersectLine(double* pos, double* start, double* end, double* intersection)
{
  double x = -999.9;
  double y = -999.9;
  double z = -999.9;
  double L = -1.0;

  ANNIEGeometry::DistanceToIntersectLine(pos[0],   pos[1],   pos[2],
                                         start[0], start[1], start[2],
                                         end[0],   end[1],   end[2],
                                         x,y,z,L);

  intersection[0]=x; intersection[1]=y; intersection[2]=z;

  return L;
}

//
//
// // Note: have replaced the below code with an implementation
// //       using the sine rule, which I think is quicker
//
// double ANNIEGeometry::DistanceToIntersectLine(double x0, double y0, double z0,
//                                    double sx, double sy, double sz,
//                                    double ex, double ey, double ez,
//                                    double x, double y, double z)
// {
//   double vstart[3], dist[3], pos[3], start[3], end[3], intersection[3]; /// dist = vector(end - start), vstart = vector(pos - start)
//   pos[0] = x0; pos[1] = y0; pos[2] = z0;
//   start[0] = sx; start[1] = sy; start[2] = sz;
//   end[0] = ex; end[1] = ey; end[2] = ez;
//   intersection[0] = x; intersection[1] = y; intersection[2] = z;
//   double ctc    = ANNIEParameterseters::CosThetaC;
//   double length = WCSimMathUtil::Magnitude(start,end);
//   double arg_a  = pow(length,4)*(1-WCSimMathUtil::sqr(ctc));
//
//
//   WCSimMathUtil::SubtractVec(pos,start,vstart);
//   WCSimMathUtil::SubtractVec(end,start,dist);
//
//   double arg_b  = -2*WCSimMathUtil::sqr(length)*WCSimMathUtil::DotProduct(vstart,dist)*(1-WCSimMathUtil::sqr(ctc));
//   double arg_c = WCSimMathUtil::sqr(WCSimMathUtil::DotProduct(vstart,dist))-WCSimMathUtil::sqr(WCSimMathUtil::Magnitude(pos,start))*WCSimMathUtil::sqr(length)*WCSimMathUtil::sqr(ctc);
//
//   double firstSol;
//   double secondSol;
//   double det = WCSimMathUtil::Determinant(arg_a, arg_b, arg_c);
//
//   if (det >= 0) {
//     WCSimMathUtil::RealRoots(arg_a, arg_b, sqrt(det), firstSol, secondSol);  // roots are real
//     double newPos[3], newvstart[3], newdist[3];
//     double theta1, theta2;
//     double lim = 0.001;
//     if((firstSol  >= 0.0) && (firstSol  <= 1.0)) {
//       for(Int_t i=0;i<3;i++) newPos[i] = start[i] + firstSol*(end[i]-start[i]);
//       WCSimMathUtil::SubtractVec(pos,newPos,newvstart);
//       WCSimMathUtil::SubtractVec(end,newPos,newdist);
//       theta1 = WCSimMathUtil::DotProduct(newvstart,newdist)/(WCSimMathUtil::Magnitude(pos,newPos)*WCSimMathUtil::Magnitude(end,newPos));
//
//     }
//     if((secondSol >= 0.0) && (secondSol <= 1.0)) {
//       for(Int_t i=0;i<3;i++) newPos[i] = start[i] + secondSol*(end[i]-start[i]);
//       WCSimMathUtil::SubtractVec(pos,newPos,newvstart);
//       WCSimMathUtil::SubtractVec(end,newPos,newdist);
//       theta2 = WCSimMathUtil::DotProduct(newvstart,newdist)/(WCSimMathUtil::Magnitude(pos,newPos)*WCSimMathUtil::Magnitude(end,newPos));
//     }
//     if((fabs(theta1-ctc) < lim) && (fabs(theta2-ctc) < lim)) return -1.0; // two good solutions
//     if((fabs(theta1+ctc) < lim) && (fabs(theta2+ctc) < lim)) return -1.0; // two bad solutions
//     if((fabs(theta1-ctc) < lim) && (theta2 <= 0)) {
//       for(Int_t i=0;i<3;i++) intersection[i] = start[i] + firstSol*(end[i]-start[i]);
//       return WCSimMathUtil::Magnitude(pos,intersection);
//     }
//     if(fabs(theta2-ctc) < lim && theta1 <= 0){
//       for(Int_t i=0;i<3;i++) intersection[i] = start[i] + secondSol*(end[i]-start[i]);
//       return WCSimMathUtil::Magnitude(pos,intersection);
//     }
//     return -1.0; // no classification
//   } // end if
//   else {
//     return -1.0;}
//   return -1.0;
// }
//
// double ANNIEGeometry::DistanceToIntersectLine(double *pos, double *start, double *end, double *intersection)
// {
//   double ctc    = ANNIEParameterseters::CosThetaC;
//   double length = WCSimMathUtil::Magnitude(start,end);
//   double arg_a  = pow(length,4)*(1-WCSimMathUtil::sqr(ctc));
//
//   double vstart[3], dist[3];    /// dist = vector(end - start), vstart = vector(pos - start)
//   WCSimMathUtil::SubtractVec(pos,start,vstart);
//   WCSimMathUtil::SubtractVec(end,start,dist);
//
//   double arg_b  = -2*WCSimMathUtil::sqr(length)*WCSimMathUtil::DotProduct(vstart,dist)*(1-WCSimMathUtil::sqr(ctc));
//
//   double arg_c = WCSimMathUtil::sqr(WCSimMathUtil::DotProduct(vstart,dist))-WCSimMathUtil::sqr(WCSimMathUtil::Magnitude(pos,start))*WCSimMathUtil::sqr(length)*WCSimMathUtil::sqr(ctc);
//
//   double firstSol;
//   double secondSol;
//   double det = WCSimMathUtil::Determinant(arg_a, arg_b, arg_c);
//
//   if (det >= 0) {
//     WCSimMathUtil::RealRoots(arg_a, arg_b, sqrt(det), firstSol, secondSol);  // roots are real
//
//     double newPos[3], newvstart[3], newdist[3];
//     double costheta1 = 0.0;
//     double costheta2 = 0.0;
//     double lim = 0.001;
//
//     if((firstSol  >= 0.0) && (firstSol  <= 1.0)) {
//       for(Int_t i=0;i<3;i++) newPos[i] = start[i] + firstSol*(end[i]-start[i]);
//       WCSimMathUtil::SubtractVec(pos,newPos,newvstart);
//       WCSimMathUtil::SubtractVec(end,newPos,newdist);
//       costheta1 = WCSimMathUtil::DotProduct(newvstart,newdist)/(WCSimMathUtil::Magnitude(pos,newPos)*WCSimMathUtil::Magnitude(end,newPos));
//     }
//     if((secondSol >= 0.0) && (secondSol <= 1.0)) {
//       for(Int_t i=0;i<3;i++) newPos[i] = start[i] + secondSol*(end[i]-start[i]);
//       WCSimMathUtil::SubtractVec(pos,newPos,newvstart);
//       WCSimMathUtil::SubtractVec(end,newPos,newdist);
//       costheta2 = WCSimMathUtil::DotProduct(newvstart,newdist)/(WCSimMathUtil::Magnitude(pos,newPos)*WCSimMathUtil::Magnitude(end,newPos));
//     }
//
//     if((fabs(costheta1-ctc) < lim) && (fabs(costheta2-ctc) < lim)) { return -1.0; }// two good solutions
//     if((fabs(costheta1+ctc) < lim) && (fabs(costheta2+ctc) < lim)) { return -1.0; }// two bad solutions
//     if((fabs(costheta1-ctc) < lim) && (costheta2 == 0)) {
//       for(Int_t i=0;i<3;i++) intersection[i] = start[i] + firstSol*(end[i]-start[i]);
//       return WCSimMathUtil::Magnitude(pos,intersection);
//     }
//     if((fabs(costheta2-ctc) < lim) && (costheta1 == 0)){
//       for(Int_t i=0;i<3;i++) intersection[i] = start[i] + secondSol*(end[i]-start[i]);
//       return WCSimMathUtil::Magnitude(pos,intersection);
//     }
//     return -1.0; // no classification
//   } // end if
//   else {
//     return -1.0;
//   }
//   return -1.0;
// }
//
//


