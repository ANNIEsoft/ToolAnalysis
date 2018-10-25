#include "VertexGeometry.h"
#include "TRandom.h"

#include <vector>
#include <cmath>
#include <iostream>
#include <cassert>

static VertexGeometry* fgVertexGeometry = 0;

VertexGeometry* VertexGeometry::Instance()
{
  if( !fgVertexGeometry ){
    fgVertexGeometry = new VertexGeometry();
  }
  if( !fgVertexGeometry ){
    assert(fgVertexGeometry);
  }
  if( fgVertexGeometry ){
  }
  return fgVertexGeometry;
}

VertexGeometry::VertexGeometry()
{
  fNDigitsMax = 10000;
  fNDigits = 0;
  fNFilterDigits = 0;
  fThisDigit = 0;
  fLastEntry = 0; 
  fCounter = 0;
  
  fMeanQ = 0.0;
  fTotalQ = 0.0;
  fMeanFilteredQ = 0.0;
  fTotalFilteredQ = 0.0;
  fMinTime = 0.0;
  fMaxTime = 0.0;  
  fVtxX1 = 0.0;
  fVtxY1 = 0.0;
  fVtxZ1 = 0.0;
  fVtxTime1 = 0.0;
  fVtxX2 = 0.0;
  fVtxY2 = 0.0;
  fVtxZ2 = 0.0;
  fVtxTime2 = 0.0;
  fQmin = 0;
  fIsFiltered = new bool[fNDigitsMax];
  fDigitType = new int[fNDigitsMax];

  fDigitX = new double[fNDigitsMax];
  fDigitY = new double[fNDigitsMax];
  fDigitZ = new double[fNDigitsMax];
  fDigitT = new double[fNDigitsMax];
  fDigitQ = new double[fNDigitsMax];
  fDigitPE = new int[fNDigitsMax];

  fConeAngle = new double[fNDigitsMax];
  fZenith = new double[fNDigitsMax];
  fAzimuth = new double[fNDigitsMax];
  fSolidAngle = new double[fNDigitsMax];

  fDistPoint = new double[fNDigitsMax];
  fDistTrack = new double[fNDigitsMax];
  fDistPhoton = new double[fNDigitsMax];
  fDistScatter = new double[fNDigitsMax];

  fDeltaTime = new double[fNDigitsMax];
  fDeltaSigma = new double[fNDigitsMax];

  fDeltaAngle = new double[fNDigitsMax];
  fDeltaPoint = new double[fNDigitsMax];
  fDeltaTrack = new double[fNDigitsMax];
  fDeltaPhoton = new double[fNDigitsMax];
  fDeltaScatter = new double[fNDigitsMax];

  fPointPath = new double[fNDigitsMax];
  fExtendedPath = new double[fNDigitsMax];
  fPointResidual = new double[fNDigitsMax];
  fExtendedResidual = new double[fNDigitsMax];

  fDelta = new double[fNDigitsMax];
  
  for( int n=0; n<fNDigitsMax; n++ ){
    fIsFiltered[n] = 0.0;
    fDigitX[n] = 0.0;
    fDigitY[n] = 0.0;
    fDigitZ[n] = 0.0;
    fDigitT[n] = 0.0;
    fDigitQ[n] = 0.0;
    fDigitPE[n] = 0;
    fConeAngle[n] = 0.0;
    fZenith[n] = 0.0;
    fAzimuth[n] = 0.0;
    fSolidAngle[n] = 0.0;
    fDistPoint[n] = 0.0;
    fDistTrack[n] = 0.0;
    fDistPhoton[n] = 0.0; 
    fDistScatter[n] = 0.0; 
    fDeltaTime[n] = 0.0;
    fDeltaSigma[n] = 0.0;
    fDeltaAngle[n] = 0.0;    
    fDeltaPoint[n] = 0.0;
    fDeltaTrack[n] = 0.0;
    fDeltaPhoton[n] = 0.0;
    fDeltaScatter[n] = 0.0;
    fPointPath[n] = 0.0;
    fExtendedPath[n] = 0.0;
    fPointResidual[n] = 0.0;
    fExtendedResidual[n] = 0.0;
    fDelta[n] = 0.0;
  }
}

void VertexGeometry::Clear() {
  
  // clear vertices
  // ==============
  for(int i=0; i<vVertexList.size(); i++ ){
    delete (RecoVertex*)(vVertexList.at(i));
  }
  vVertexList.clear();

  // clear seed vertices
  // ===================
  vSeedVtxX.clear();
  vSeedVtxY.clear();
  vSeedVtxZ.clear();
  vSeedVtxTime.clear();
  vSeedDigitList.clear();
}

VertexGeometry::~VertexGeometry()
{
  this->Clear(); 
  if( fIsFiltered ) delete [] fIsFiltered;
  if( fDigitType ) delete [] fDigitType;
  	
  if( fDigitX ) delete [] fDigitX;
  if( fDigitY ) delete [] fDigitY;
  if( fDigitZ ) delete [] fDigitZ;
  if( fDigitT ) delete [] fDigitT;
  if( fDigitQ ) delete [] fDigitQ; 
  if( fDigitPE ) delete [] fDigitPE;

  if( fConeAngle ) delete [] fConeAngle;
  if( fZenith ) delete [] fZenith;
  if( fAzimuth ) delete [] fAzimuth;
  if( fSolidAngle ) delete [] fSolidAngle;

  if( fDistPoint )  delete [] fDistPoint;
  if( fDistTrack )  delete [] fDistTrack;
  if( fDistPhoton ) delete [] fDistPhoton;   
  if( fDistScatter ) delete [] fDistScatter;

  if( fDeltaTime )   delete [] fDeltaTime;
  if( fDeltaSigma ) delete [] fDeltaSigma;

  if( fDeltaAngle )  delete [] fDeltaAngle;  
  if( fDeltaPoint )  delete [] fDeltaPoint;
  if( fDeltaTrack )  delete [] fDeltaTrack;
  if( fDeltaPhoton ) delete [] fDeltaPhoton;
  if( fDeltaScatter ) delete [] fDeltaScatter;

  if( fPointPath ) delete [] fPointPath;
  if( fExtendedPath ) delete [] fExtendedPath;
  if( fPointResidual )  delete [] fPointResidual;
  if( fExtendedResidual ) delete [] fExtendedResidual;

  if( fDelta ) delete [] fDelta; 
}

void VertexGeometry::LoadDigits(std::vector<RecoDigit>* vDigitList)
{

  fNDigits = 0;
  fNFilterDigits = 0;
  
  fThisDigit = 0;
  fLastEntry = 0;
  fCounter = 0;

  // clear vertices
  // ==============
  for(int i=0; i<vVertexList.size(); i++ ){
    delete (RecoVertex*)(vVertexList.at(i));
  }
  vVertexList.clear();

  // clear seed vertices
  // ===================
  vSeedVtxX.clear();
  vSeedVtxY.clear();
  vSeedVtxZ.clear();
  vSeedVtxTime.clear();
  vSeedDigitList.clear();

  // get lists of digits
  // ==================

  fDigitList = vDigitList;
  fNDigits = fDigitList->size();
  
  // sanity checks
  // =============
  if( fNDigits<=0 ){
    std::cout << " *** VertexGeometry::LoadDigits(...) *** " << std::endl;
    std::cout << "   <warning> event has no digits! " << std::endl;
    return;
  }

  if( fNDigits>fNDigitsMax ){
    std::cout << " *** VertexGeometry::LoadDigits(...) *** " << std::endl;
    std::cout << "   <warning> event will be truncated! " << std::endl;
    std::cout << fNDigitsMax << std::endl;
    fNDigits = fNDigitsMax;
  }

  // load digits
  // ===========
  fTotalQ = 0.0;
  fMeanQ = 0.0;

  fTotalFilteredQ = 0.0;
  fMeanFilteredQ = 0.0;

  fMinTime = -999999.9;
  fMaxTime = -999999.9;

  double Swx = 0.0;
  double Sw = 0.0;

  double SFwx = 0.0;
  double SFw = 0.0;
  
  for( int idigit=0; idigit<fNDigits; idigit++ ){

    RecoDigit recoDigit = fDigitList->at(idigit);
    fDigitType[idigit] = recoDigit.GetDigitType();
    fDigitX[idigit] = recoDigit.GetPosition().X();
    fDigitY[idigit] = recoDigit.GetPosition().Y();
    fDigitZ[idigit] = recoDigit.GetPosition().Z();
    fDigitT[idigit] = recoDigit.GetCalTime();
    fDigitQ[idigit] = recoDigit.GetCalCharge();
    //fDigitPE[idigit] = recoDigit.GetRawPEtube();
    fIsFiltered[idigit] = recoDigit.GetFilterStatus();

    fConeAngle[idigit] = 0.0;
    fZenith[idigit] = 0.0;
    fAzimuth[idigit] = 0.0;
    fSolidAngle[idigit] = 0.0;

    fDistPoint[idigit] = 0.0;
    fDistTrack[idigit] = 0.0;
    fDistPhoton[idigit] = 0.0;
    fDistScatter[idigit] = 0.0;

    fDeltaTime[idigit] = 0.0;
    fDeltaSigma[idigit] = 0.0;
    
    fDeltaAngle[idigit] = 0.0;
    fDeltaPoint[idigit] = 0.0;
    fDeltaTrack[idigit] = 0.0;
    fDeltaPhoton[idigit] = 0.0;
    fDeltaScatter[idigit] = 0.0;

    fPointPath[idigit] = 0.0;
    fExtendedPath[idigit] = 0.0;
    fPointResidual[idigit] = 0.0;
    fExtendedResidual[idigit] = 0.0;

    fDelta[idigit] = 0.0;

    Swx += recoDigit.GetCalCharge();
    Sw += 1.0;

    if( fMinTime<0 
     || recoDigit.GetCalTime()<fMinTime ){
      fMinTime = recoDigit.GetCalTime();
    }

    if( fMaxTime<0 
     || recoDigit.GetCalTime()>fMaxTime ){
      fMaxTime = recoDigit.GetCalTime();
    }

    if( recoDigit.GetFilterStatus() ){
      SFwx += recoDigit.GetCalCharge();
      SFw += 1.0;
    }
  }

  if( Sw>0.0 ){
    fTotalQ = Swx;
    fMeanQ = Swx/Sw;
  }

  if( SFw>0.0 ){
    fTotalFilteredQ = SFwx;
    fMeanFilteredQ = SFwx/SFw;
  }

  if( fMinTime<0 ) fMinTime = 0.0;
  if( fMaxTime<0 ) fMaxTime = 0.0;
  return;
}
  
RecoVertex* VertexGeometry::CalcSimpleVertex(std::vector<RecoDigit>* vDigitList)
{
  // load event
  // ==========
  this->LoadDigits(vDigitList);

  // calculate simple vertex
  // =======================

  double vtxX = 0.0;
  double vtxY = 0.0;
  double vtxZ = 0.0;
  double vtxTime = 0.0;

  int itr = 1;
  bool pass = 1; 
  double fom = 1.0;

  // create new vertex
  // =================
  RecoVertex* newVertex = new RecoVertex();
  vVertexList.push_back(newVertex);

  // calculate vertex
  // ================
  this->CalcSimpleVertex(vtxX,vtxY,vtxZ,vtxTime);

  // set vertex
  // ==========
  newVertex->SetVertex(vtxX,vtxY,vtxZ,vtxTime);
  newVertex->SetFOM(fom,itr,pass);  

  // set status
  // ==========
  newVertex->SetStatus(RecoVertex::kOK);
  
  // return vertex
  // =============
  return newVertex;
}

void VertexGeometry::CalcSimpleVertex(double& vtxX, double& vtxY, double& vtxZ, double& vtxTime)
{
  // simple vertex
  // =============

  // default vertex
  // ==============
  vtxX = 0.0;
  vtxY = 0.0;
  vtxZ = 0.0;
  vtxTime = 0.0;

  // loop over digits
  // ================
  double Swx = 0.0;
  double Swy = 0.0;
  double Swz = 0.0;
  double Swt = 0.0;
  double Sw = 0.0;

  for( int idigit=0; idigit<fNDigits; idigit++ ){
    if( fIsFiltered[idigit] ){
      Swx += fDigitQ[idigit]*fDigitX[idigit];
      Swy += fDigitQ[idigit]*fDigitY[idigit];
      Swz += fDigitQ[idigit]*fDigitZ[idigit];
      Swt += fDigitQ[idigit]*fDigitT[idigit];
      Sw  += fDigitQ[idigit];
    }
  }

  //std::cout << fDigitQ[10] << " " << fDigitX[10] << " " << fDigitY[10] << " " << fDigitZ[10] << std::endl;
  // average position
  // ================
  if( Sw>0.0 ){
    vtxX = Swx/Sw;
    vtxY = Swy/Sw;
    vtxZ = Swz/Sw;
    vtxTime = Swt/Sw;
//    std::cout << Swx << " " << Sw << std::endl;
  }   

  return;
}

void VertexGeometry::CalcResiduals(std::vector<RecoDigit>* vDigitList, RecoVertex* myVertex)
{
  // load event
  // ==========
  this->LoadDigits(vDigitList);

  // calculate residuals
  // ===================
  return this->CalcResiduals(myVertex);
}

void VertexGeometry::CalcResiduals(RecoVertex* myVertex)
{
  // sanity check
  // ============
  if( myVertex==0 ) return;

  // get vertex information
  // ======================
  double vtxX = myVertex->GetPosition().X();
  double vtxY = myVertex->GetPosition().Y();
  double vtxZ = myVertex->GetPosition().Z();
  double vtxTime = myVertex->GetTime();
  double dirX = myVertex->GetDirection().X();
  double dirY = myVertex->GetDirection().Y();
  double dirZ = myVertex->GetDirection().Z();

  // calculate residuals
  // ===================
  return this->CalcResiduals( vtxX, vtxY, vtxZ, vtxTime,
                              dirX, dirY, dirZ );
}

void VertexGeometry::CalcPointResiduals(double vtxX, double vtxY, double vtxZ, double vtxTime, double dirX, double dirY, double dirZ)
{
  this->CalcResiduals( vtxX, vtxY, vtxZ, vtxTime,
                       dirX, dirY, dirZ );

  for( int idigit=0; idigit<fNDigits; idigit++ ){
    fDelta[idigit] = fPointResidual[idigit];
  }

  return;
}

void VertexGeometry::CalcExtendedResiduals(double vtxX, double vtxY, double vtxZ, double vtxTime, double dirX, double dirY, double dirZ )
{
  this->CalcResiduals( vtxX, vtxY, vtxZ, vtxTime,
                       dirX, dirY, dirZ );

  for( int idigit=0; idigit<fNDigits; idigit++ ){
    fDelta[idigit] = fExtendedResidual[idigit];
  }

  //std::cout << fDelta[46] << std::endl;
  return;
}

void VertexGeometry::CalcResiduals(double vtxX, double vtxY, double vtxZ, double vtxTime, double dirX, double dirY, double dirZ )
{
  // reset arrays
  // ============
  for( int idigit=0; idigit<fNDigits; idigit++ ){
    fConeAngle[idigit] = 0.0;
    fZenith[idigit] = 0.0;
    fAzimuth[idigit] = 0.0;
    fSolidAngle[idigit] = 0.0;
    fDistPoint[idigit] = 0.0;
    fDistTrack[idigit] = 0.0;
    fDistPhoton[idigit] = 0.0;  
    fDistScatter[idigit] = 0.0; 
    fDeltaTime[idigit] = 0.0; 
    fDeltaSigma[idigit] = 0.0;
    fDeltaAngle[idigit] = 0.0;
    fDeltaPoint[idigit] = 0.0;
    fDeltaTrack[idigit] = 0.0;
    fDeltaPhoton[idigit] = 0.0;
    fDeltaScatter[idigit] = 0.0;
    fPointPath[idigit] = 0.0;
    fExtendedPath[idigit] = 0.0;
    fPointResidual[idigit] = 0.0;
    fExtendedResidual[idigit] = 0.0;
    fDelta[idigit] = 0.0;
    fPointResidualMean = 0.0;
    fExtendedResidualMean = 0.0;
  }
  // cone angle
  // ==========
  double thetadeg = Parameters::CherenkovAngle(); // degrees
  double theta = thetadeg*(TMath::Pi()/180.0); // degrees->radians
  //theta = acos(30.0/(29.0*1.38));
  //bool truehits = (Interface::Instance())->IsTrueHits(); 

  // loop over digits
  // ================
  for( int idigit=0; idigit<fNDigits; idigit++ ){
    double dx = fDigitX[idigit]-vtxX;
    double dy = fDigitY[idigit]-vtxY;
    double dz = fDigitZ[idigit]-vtxZ;
    double ds = sqrt(dx*dx+dy*dy+dz*dz);

    //  std::cout<< "tttt "<< dx << " " << dy <<" " << dz << " " << ds << std::endl;

    double px = dx/ds;
    double py = dy/ds;
    double pz = dz/ds;

    double cosphi = 1.0;
    double sinphi = 1.0;
    double phi = 0.0; 
    double phideg = 0.0;

    double ax = 0.0;
    double ay = 0.0;
    double az = 0.0;
    double azideg = 0.0;
    

    // calculate angles if direction is known
    if( dirX*dirX + dirY*dirY + dirZ*dirZ>0.0 ){

      // zenith angle
      cosphi = px*dirX+py*dirY+pz*dirZ;
      phi = acos(cosphi); // radians
      phideg = phi/(TMath::Pi()/180.0); // radians->degrees
      sinphi = sqrt(1.0-cosphi*cosphi);
      sinphi += 0.24*exp(-sinphi/0.24);
      sinphi /= 0.684;  // sin(phideg)/sin(thetadeg)

      // azimuthal angle
      if( dirX*dirX+dirY*dirY>0.0 ){
        ax = (px*dirZ-pz*dirX) - (py*dirX-px*dirY)*(1.0-dirZ)*dirY/sqrt(dirX*dirX+dirY*dirY);
        ay = (py*dirZ-pz*dirY) - (px*dirY-py*dirX)*(1.0-dirZ)*dirX/sqrt(dirX*dirX+dirY*dirY);
        az = pz*dirZ + py*dirY + px*dirX;
      }
      else{
        ax = px;
        ay = py;
        az = pz;
      }

      azideg = atan2(ay,ax)/(TMath::Pi()/180.0); // radians->degrees
    }

    double Lpoint = ds;
    double Ltrack = 0.0;
    double Lphoton = 0.0;
    double Lscatter = 0.0;
 
    if( phi<theta ){
      Ltrack = Lpoint*sin(theta-phi)/sin(theta);
      Lphoton = Lpoint*sin(phi)/sin(theta);
      Lscatter = 0.0;
    }
    
    else{
      Ltrack = 0.0;
      Lphoton = Lpoint;
      Lscatter = Lpoint*(phi-theta);
    }

    double fC = Parameters::SpeedOfLight();
    double fVmu = fC;
    //double fN = Parameters::RefractiveIndex(Lphoton);
    //chrom.....
    double fN = Parameters::Index0(); //...chrom1.34, 1.333;	

    double dt = fDigitT[idigit] - vtxTime; 
    double qpes = fDigitQ[idigit];
//    double tres = Parameters::TimeResolution(qpes);
    double tres = Parameters::TimeResolution(fDigitType[idigit]); //add by JW
//    double tres = Parameters::TimeResolution(fDigitType[idigit], qpes); //add by JW
    fConeAngle[idigit] = thetadeg; // degrees
    fZenith[idigit] = phideg;      // degrees
    fAzimuth[idigit] = azideg;     // degrees
    fSolidAngle[idigit] = sinphi;

    fDistPoint[idigit] = Lpoint;       
    fDistTrack[idigit] = Ltrack;
    fDistPhoton[idigit] = Lphoton;
    fDistScatter[idigit] = Lscatter;

    fDeltaTime[idigit] = dt;
    fDeltaSigma[idigit] = tres;
    
    //    std::cout<<"REWRWEREWR "<<Lpoint<<" "<<fC<< " "<<fN<<" "<<(fC/fN)<<std::endl;



    fDeltaAngle[idigit] = phideg-thetadeg; // degrees
    fDeltaPoint[idigit] = Lpoint/(fC/fN);
    fDeltaTrack[idigit] = Ltrack/fVmu;
    fDeltaPhoton[idigit] = Lphoton/(fC/fN);
    fDeltaScatter[idigit] = Lscatter/(fC/fN);
 
    fPointPath[idigit] = fN*Lpoint;
    fExtendedPath[idigit] = Ltrack + fN*Lphoton;

    fPointResidual[idigit] = dt - Lpoint/(fC/fN);
    fExtendedResidual[idigit] = dt - Ltrack/fVmu - Lphoton/(fC/fN);

    fDelta[idigit] = fExtendedResidual[idigit]; // default.
  }
  
  for( int idigit=0; idigit<fNDigits; idigit++ ){
    fPointResidualMean += fPointResidual[idigit];
    fExtendedResidualMean += fExtendedResidual[idigit];
  }
  
  fPointResidualMean = fPointResidualMean/fNDigits;
  fExtendedResidualMean = fExtendedResidualMean/fNDigits;
 //std::cout << "[CalcResiduals] (vtxX,vtxY,vtxZ,vtxTime) = (" << vtxX <<","<<vtxY<<","<<vtxZ<<","<<vtxTime<<"), (dirX,dirY,dirZ) = (" << dirX <<","<<dirY<<","<<dirZ << ")" << std::endl;

  return;
}   

double VertexGeometry::GetDeltaCorrection(int idigit, double Length)
{
  if( Length<=0.0
   || GetDistTrack(idigit)<Length ){
    return 0.0;
  }

  else{
    double Lpoint = GetDistPoint(idigit);
    double Ltrack = GetDistTrack(idigit);
    double Lphoton = GetDistPhoton(idigit);
    double AngleRad = (TMath::Pi()/180.0)*GetAngle(idigit);    
    double ConeAngleRad = (TMath::Pi()/180.0)*GetConeAngle(idigit);  

    double LtrackNew = Length;
    double LphotonNew = sqrt( Lpoint*Lpoint + Length*Length
                                -2.0*Lpoint*Length*cos(AngleRad) );

    double theta = ConeAngleRad;
    double sinphi = (Lpoint/LphotonNew)*sin(AngleRad);
    double phi = asin(sinphi);
    double alpha = theta-phi;
    double LphotonNewCorrected = LphotonNew*alpha/sin(alpha);

    double fC = Parameters::SpeedOfLight();
    double fN = Parameters::RefractiveIndex(Lpoint);

    return ( Ltrack/fC + Lphoton/(fC/fN) )
         - ( LtrackNew/fC + LphotonNewCorrected/(fC/fN) );
  }
}

void VertexGeometry::CalcVertexSeeds(int NSeeds)
{
  // reset list of seeds
  // ===================
  vSeedVtxX.clear();
  vSeedVtxY.clear();
  vSeedVtxZ.clear();
  vSeedVtxTime.clear();

  // always calculate the simple vertex
  // ==================================
  this->CalcSimpleVertex(fVtxX1,fVtxY1,fVtxZ1,fVtxTime1);

  // add this vertex
  vSeedVtxX.push_back(fVtxX1); 
  vSeedVtxY.push_back(fVtxY1);
  vSeedVtxZ.push_back(fVtxZ1);
  vSeedVtxTime.push_back(fVtxTime1);

  // check limit
  if( NSeeds<=1 ) return;
  
  // form list of golden digits.
  // ==========================
  vSeedDigitList.clear();  
  double tempQ = 0;
  for( fThisDigit=0; fThisDigit<fNDigits; fThisDigit++ ){
    if( fIsFiltered[fThisDigit] ){
//    	int istruehits = Interface::Instance()->IsTrueHits();
    	int istruehits = 0; //for test only. JW
      if (istruehits) tempQ = 1.0*fDigitPE[fThisDigit];
      else tempQ =  fDigitQ[fThisDigit]; //IsTrueHits = 0
      if( fDigitT[fThisDigit] - fMinTime>=0
       && fDigitT[fThisDigit] - fMinTime<300
       && tempQ >= fQmin 
       && fDigitType[fThisDigit] == Parameters::SeedDigitType()){ 
        vSeedDigitList.push_back(fThisDigit);
      }
    }
  }

//  std::cout <<"fMinTime = " <<fMinTime << " fDigitQ[10] = " << fDigitQ[10] << " NSeeds = " << vSeedDigitList.size() << std::endl;
  // check for enough digits
  if( vSeedDigitList.size()<=4 ) return;

  // generate new list of seeds
  // ==========================
  double x0 = 0.0;
  double y0 = 0.0;
  double z0 = 0.0;
  double t0 = 0.0;

  double x1 = 0.0;
  double y1 = 0.0;
  double z1 = 0.0;
  double t1 = 0.0; 

  double x2 = 0.0;
  double y2 = 0.0;
  double z2 = 0.0;
  double t2 = 0.0;

  double x3 = 0.0;
  double y3 = 0.0;
  double z3 = 0.0;
  double t3 = 0.0;

  int counter = 0;
  int NSeedsTarget = NSeeds;
//  if(GetNSeeds()<NSeedsTarget && counter<100*NSeedsTarget)
//    std::cout << "DEBUG [VertexGeometry::CalcVertexSeeds]: true" << std::endl;
//  else
//    std::cout << "DEBUG [VertexGeometry::CalcVertexSeeds]: false" << std::endl;

  while( GetNSeeds()<NSeedsTarget && counter<100*NSeedsTarget ){
    counter++;

    // choose next four digits
    this->ChooseNextQuadruple(x0,y0,z0,t0,
                              x1,y1,z1,t1,
                              x2,y2,z2,t2,
                              x3,y3,z3,t3);
       
    //
    //std::cout << "   digit0: (x,y,z,t)=(" << x0 << "," << y0 << "," << z0 << "," << t0 << ") " << std::endl;
    //std::cout << "   digit1: (x,y,z,t)=(" << x1 << "," << y1 << "," << z1 << "," << t1 << ") " << std::endl;
    //std::cout << "   digit2: (x,y,z,t)=(" << x2 << "," << y2 << "," << z2 << "," << t2 << ") " << std::endl;
    //std::cout << "   digit3: (x,y,z,t)=(" << x3 << "," << y3 << "," << z3 << "," << t3 << ") " << std::endl;
    //std::cout << std::endl;
    //

    // find common vertex
    ANNIEGeometry::FindVertex(x0,y0,z0,t0,
                              x1,y1,z1,t1,
                              x2,y2,z2,t2,
                              x3,y3,z3,t3,
                              fVtxX1,fVtxY1,fVtxZ1,fVtxTime1,
                              fVtxX2,fVtxY2,fVtxZ2,fVtxTime2);

    //
    //std::cout << "   result: (x,y,z,t)=(" << fVtxX1 << "," << fVtxY1 << "," << fVtxZ1 << "," << fVtxTime1 << ") " << std::endl
    //          << "   result: (x,y,z,t)=(" << fVtxX2 << "," << fVtxY2 << "," << fVtxZ2 << "," << fVtxTime2 << ") " << std::endl;
    //std::cout << std::endl;
    //

    //AEAEAEAEAEAE -HARD CODED SPHERICAL GEOMETRY/////////////// 

    if(fVtxX1==-99999.9 && fVtxX2==-99999.9) continue;
/*    bool inside_det;
    if(sqrt(fVtxX1*fVtxX1+fVtxY1*fVtxY1+fVtxZ1*fVtxZ1)<650) inside_det=true; else inside_det=false;
    std::cout<<"Solution1: inside_det = "<<inside_det<<"  X = "<<fVtxX1<<"  Y = "<<fVtxY1<<"  Z = "<<fVtxZ1<<"  T = "<<fVtxTime1<<std::endl;
    std::cout<<"LBNE_DET = "<<ANNIEGeometry::Instance()->InsideDetector(fVtxX1,fVtxY1,fVtxZ1)<<std::endl;
    if(!inside_det && ANNIEGeometry::Instance()->InsideDetector(fVtxX1,fVtxY1,fVtxZ1)) 
    {
      std::cout<<"ANNIEGeometry Mismatch"<<std::endl;
      continue;
    }
    // end of AEAEAEAEAE ///////////////////////////////////////
*/
    bool inside_det;
//    //======================================================================================================
//    //=================== decide if the vertex is inside the WCSIM annie tank, added by Jingbo ===================
//    double annie_tank_radius = 152.; //cm
//    double annie_tank_height = 396.; //cm
//    if(sqrt(fVtxX1*fVtxX1+(fVtxZ1-167.7)*(fVtxZ1-167.7))<annie_tank_radius &&
//    	 fVtxY1>-0.5*annie_tank_height-14.46 && fVtxY1<0.5*annie_tank_height-14.46)
//    	inside_det = true;
//    else inside_det = false;
//    //=======================================================================================================
    
//    //======================================================================================================
//    //=================== decide if the vertex is inside the SandBox annie tank, added by Jingbo ===================
//    double annie_tank_radius = 152.; //cm
//    double annie_tank_height = 396.; //cm
//    if(sqrt(fVtxX1*fVtxX1+(fVtxZ1)*(fVtxZ1))<annie_tank_radius &&
//    	 fVtxY1>-0.5*annie_tank_height && fVtxY1<0.5*annie_tank_height)
//    	inside_det = true;
//    else inside_det = false;
//    //=======================================================================================================

    // add first digit
    if( ANNIEGeometry::Instance()->InsideDetector(fVtxX1,fVtxY1,fVtxZ1) ){
      vSeedVtxX.push_back(fVtxX1); 
      vSeedVtxY.push_back(fVtxY1);
      vSeedVtxZ.push_back(fVtxZ1);
      vSeedVtxTime.push_back(fVtxTime1);
    }

   //AEAEAEAEAEAE -HARD CODED SPHERICAL GEOMETRY///////////////
    /*
    if(sqrt(fVtxX2*fVtxX2+fVtxY2*fVtxY2+fVtxZ2*fVtxZ2)<650) inside_det=true; else inside_det=false;
    std::cout<<"Solution2: inside_det = "<<inside_det<<"  X = "<<fVtxX2<<"  Y = "<<fVtxY2<<"  Z = "<<fVtxZ2<<"  T = "<<fVtxTime2<<std::endl;
    std::cout<<"LBNE_DET = "<<ANNIEGeometry::Instance()->InsideDetector(fVtxX2,fVtxY2,fVtxZ2)<<std::endl;
    if(!inside_det && ANNIEGeometry::Instance()->InsideDetector(fVtxX2,fVtxY2,fVtxZ2))
      {
      std::cout<<"ANNIEGeometry Mismatch"<<std::endl;
      continue;
    }
    // end of AEAEAEAEAE ///////////////////////////////////////
    */
//    //======================================================================================================
//    //=================== decide if the vertex is inside the WCSIM annie tank, added by Jingbo ===================
//    if(sqrt(fVtxX2*fVtxX2+(fVtxZ2-167.7)*(fVtxZ2-167.7))<annie_tank_radius && 
//    	fVtxY2>-0.5*annie_tank_height-14.46 && fVtxY2<0.5*annie_tank_height+14.46)
//    	inside_det = true;
//    else inside_det = false;
//    //=======================================================================================================
    
//    //======================================================================================================
//    //=================== decide if the vertex is inside the SandBox annie tank, added by Jingbo ===================
//    if(sqrt(fVtxX2*fVtxX2+(fVtxZ2)*(fVtxZ2))<annie_tank_radius &&
//    	 fVtxY2>-0.5*annie_tank_height && fVtxY2<0.5*annie_tank_height)
//    	inside_det = true;
//    else inside_det = false;
//    //=======================================================================================================

    // add second digit
    if( ANNIEGeometry::Instance()->InsideDetector(fVtxX2,fVtxY2,fVtxZ2) ){
      vSeedVtxX.push_back(fVtxX2);
      vSeedVtxY.push_back(fVtxY2);
      vSeedVtxZ.push_back(fVtxZ2);
      vSeedVtxTime.push_back(fVtxTime2);
    }
  }

  return;
}

void VertexGeometry::ChooseNextQuadruple(double& x0, double& y0, double& z0, double& t0, double& x1, double& y1, double& z1, double& t1, double& x2, double& y2, double& z2, double& t2, double& x3, double& y3, double& z3, double& t3)
{
  this->ChooseNextDigit(x0,y0,z0,t0);
  this->ChooseNextDigit(x1,y1,z1,t1);
  this->ChooseNextDigit(x2,y2,z2,t2);
  this->ChooseNextDigit(x3,y3,z3,t3);

  return;
}

//void VertexGeometry::ChooseNextDigit(double& xpos, double& ypos, double& zpos, double& time)
//{
//  // default
//  xpos=0; ypos=0; zpos=0; time=0;
//
//  // ROOT random number generator
//  // double r = gRandom->Rndm();
//
//  // pseudo-random number generator
//  int numEntries = vSeedDigitList.size();
//
//  fCounter++;
//  if( fCounter>=fNDigits ) fCounter = 0;
//  fThisDigit = vSeedDigitList.at(fLastEntry);
//
//  double t0 = 0.5 + fDigitT[fCounter] - fMinTime;
//  double q0 = 0.5 + fDigitQ[fCounter];
//
//  double t1 = 0.5 + fDigitT[fThisDigit] - fMinTime;
//  double q1 = 0.5 + fDigitQ[fThisDigit];
//
//  double tq = 100.0*(t0*q0+t1*q1);
//  double r = tq - TMath::Floor(tq);
//  int counter = 0;
//  int ngoldendigit = vSeedDigitList.size();
//
//  while(counter<ngoldendigit*100) {
//    counter++;
//    r = gRandom->Uniform(); // Christoph Aberle, August 14: use of a proper RN generator since I saw that quadruplets were duplicated with the pseudo-random number generator used in the lines above
//    fLastEntry = (int)(r*numEntries);
//    if(fDigitType[vSeedDigitList.at(fLastEntry)] != Parameters::SeedDigitType()) continue;
//    // return the new digit
//    fThisDigit = vSeedDigitList.at(fLastEntry);
//    xpos = fDigitX[fThisDigit];
//    ypos = fDigitY[fThisDigit];
//    zpos = fDigitZ[fThisDigit];
//    time = fDigitT[fThisDigit];
//    break;
//  }
//
//  return;
//}

void VertexGeometry::ChooseNextDigit(double& xpos, double& ypos, double& zpos, double& time)
{
  // default
  xpos=0; ypos=0; zpos=0; time=0;

  // ROOT random number generator
  // double r = gRandom->Rndm();

  // pseudo-random number generator
  int numEntries = vSeedDigitList.size();

  fCounter++;
  if( fCounter>=fNDigits ) fCounter = 0;
  fThisDigit = vSeedDigitList.at(fLastEntry);

  double t0 = 0.5 + fDigitT[fCounter] - fMinTime;
  double q0 = 0.5 + fDigitQ[fCounter];

  double t1 = 0.5 + fDigitT[fThisDigit] - fMinTime;
  double q1 = 0.5 + fDigitQ[fThisDigit];

  double tq = 100.0*(t0*q0+t1*q1);
  double r = tq - TMath::Floor(tq);

  r = gRandom->Uniform(); // Christoph Aberle, August 14: use of a proper RN generator since I saw that quadruplets were duplicated with the pseudo-random number generator used in the lines above

  fLastEntry = (int)(r*numEntries);

  // return the new digit
  fThisDigit = vSeedDigitList.at(fLastEntry);
  xpos = fDigitX[fThisDigit];
  ypos = fDigitY[fThisDigit];
  zpos = fDigitZ[fThisDigit];
  time = fDigitT[fThisDigit];

  return;
}



