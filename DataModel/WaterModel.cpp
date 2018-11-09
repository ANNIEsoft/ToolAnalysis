////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The class of the information related to the water, which includes the density of lmda, refractive index of lmda, group velocity 
// of lmda, absorption length of lmda, attenuation factor(only considering absorption) of lmda and distance and the initial 
// spectrum of cherenkov radiation and final wavelength or time spectrum of cherenkov rad with a single distance or multiple distance.
// \file WaterModel.cc
// \author Tian Xin, Email: txin@iastate.edu
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "Parameters.h"
#include "WaterModel.h"

#include "TMath.h"
#include "TH1.h"
#include "TGraph.h"
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cassert>

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

//=========================================================================================

static WaterModel* fgWaterModel = 0;

WaterModel* WaterModel::Instance()
{
  if( !fgWaterModel ){
    fgWaterModel = new WaterModel();
  }

  if( !fgWaterModel ){
    assert(fgWaterModel);
  }

  if( fgWaterModel ){

  }

  return fgWaterModel;
}

/*WaterModel* WaterModel::Instance(Int_t op1, Int_t op2, Int_t op3)
{
  if( !fgWaterModel ){
    fgWaterModel = new WaterModel(op1, op2, op3);
  }

  if( !fgWaterModel ){
    assert(fgWaterModel);
  }

  if( fgWaterModel ){

  }

  return fgWaterModel;
}*/
//====================================================
WaterModel::WaterModel() 
{
    fOP = 0; fopv=0; fopn = 0; fopa = 0;
    //Double_t Ephoton[30] = {1.56962, 1.61039, 1.65333, 1.69863, 1.74647,1.7971, 1.85074, 1.90769, 1.96825, 2.03278,2.10169, 2.17543, 2.25454, 2.33962, 2.43137, 2.53061, 2.63829, 2.75555, 2.88371, 3.02438,3.17948, 3.35134, 3.54285, 3.75757, 3.99999, 4.27585, 4.59258, 4.95999, 5.39129, 5.90475};
    Double_t lmdaarr[26] = {700, 680, 660, 640, 620, 600, 580, 560, 540, 520, 500, 480, 460, 440, 420, 400, 380, 360, 340, 320, 300, 280, 260, 240, 220, 200};
    Double_t indexarr[26]={1.33084, 1.33134, 1.33186, 1.33241, 1.33299, 1.3336, 1.33426, 1.33497, 1.33575, 1.33659, 1.33752, 1.33854, 1.33969, 1.34099, 1.34247, 1.34418, 1.34619, 1.34857, 1.35144, 1.35496, 1.35941, 1.36516, 1.37284, 1.38358, 1.3995, 1.42526};
    Double_t abslarr[26] = {159.557, 214.725, 243.231, 321.313, 362.554, 446.651, 1108.58, 1609.32, 2096.65, 2428.27, 4812.78, 7669.83, 12182.1, 19003.2, 29125.7, 42507.7, 56403.9, 65153.8, 64840.8, 57217.2, 46638.3, 36182.1, 27125.8, 19750.8, 13958.2, 9536.1};
    Double_t sctlarr[26] = {143198, 126102, 110566, 96492.5, 83790.6, 72371.2, 62147.8, 53034.2, 44951.4, 37820.9, 31566.5, 26116.6, 21400, 17349.8, 13902.2, 10995.7, 8571.31, 6573.62, 4949.88, 3650.61, 2629.36, 1843.01, 1251.84, 819.606, 513.862, 305.909};
    Double_t initarr[26] = {0.00058783, 0.000623303, 0.000661887, 0.000704106, 0.000750633, 0.000801703, 0.000858098, 0.000920523, 0.000990149, 0.00106792, 0.00115511, 0.00125334, 0.00136421, 0.00149061, 0.00163516, 0.00180112, 0.00199341, 0.00221762, 0.00248086, 0.00279134, 0.00316012, 0.00360209, 0.00412812, 0.00475436, 0.00543418, 0.00613601};
    Double_t veloarr[26] = {22.2376, 22.2297, 22.2153, 22.1984, 22.1845, 22.164, 22.1402, 22.1115, 22.083, 22.0511, 22.014, 21.9719, 21.918, 21.8599, 21.7913, 21.7058, 21.6051, 21.484, 21.3354, 21.1439, 20.8947, 20.5739, 20.1255, 19.509, 18.5164, 17.1344};
    Double_t qearr[26] = {0.0, 0.0, 0.0, 0.0061, 0.0178, 0.0323, 0.0499, 0.0727, 0.1102, 0.1641, 0.1971, 0.2268, 0.2655, 0.2915, 0.3168, 0.332, 0.3396, 0.3306, 0.2933, 0.2017, 0.0502, 0.0, 0.0, 0.0, 0.0, 0.0}; //high 10 inch PMT
    fIndexLg = new TGraph(26, lmdaarr, indexarr);
    fAbsLg = new TGraph(26, lmdaarr, abslarr);
    fSctLg = new TGraph(26, lmdaarr, sctlarr);
    fInitLg = new TGraph(26, lmdaarr, initarr);
    fVeloLg = new TGraph(26, lmdaarr, veloarr);
    fQEg = new TGraph(26, lmdaarr, qearr);
    
    std::cout << "WaterModel graphs made...... " << std::endl;   

}

/*WaterModel::WaterModel(Int_t op1) 
{
    fOP = op1;
}*/

WaterModel::~WaterModel()
{
    delete fIndexLg;
    delete fAbsLg;
    delete fSctLg;
    delete fInitLg;
    delete fVeloLg;
    delete fQEg;
    delete [] ffinltime;
}
//=====================================================
void WaterModel::Reset()
{
    ffinltime->TH1D::Reset();
    //finittime->TH1D::Reset();
    findex = 0.;
    fvelo = 0.;
    finitintensity = 0.;
    fabslength = 0.;
    fatten = 0.;
    ffinlintensity = 0.;
    fqe = 0.0;
    
    return;
}

//function to evaluate the TGhraphs for a wavelength
Double_t WaterModel::evalGraphs(Double_t flmda,char paramType) {
    
    Double_t evaluation = -999.0;
    
    if (paramType == '1') {
    	evaluation = fVeloLg->Eval(flmda,0,"S"); //velocity
    }
    if (paramType == '2') {
    	evaluation = fInitLg->Eval(flmda,0,"S"); //initial spectrum
    }
    if (paramType == '3') {
        evaluation = fAbsLg->Eval(flmda,0,"S"); //absolute length
    }
    if (paramType == '4') {
        evaluation = fSctLg->Eval(flmda,0,"S"); //
    }
    if (paramType == '5') {
        evaluation = fQEg->Eval(flmda,0,"S"); // QE
    }
    if (!(paramType == '1' || paramType == '2' || paramType == '3' || paramType == '4' || paramType == '5')) 
    	std::cout << "Error in evaluating the TGraph in WaterModel........." << std::endl;    
    
    return evaluation;
}

 
//===============================================================================================
// The function to calculate the index of refraction of water
//
// From: Schiebener, P., Straub, J., Levelt Sengers, J. M. H., and Gallagher, J. S.,
//  J. Phys. Chem. Ref. Data, 19, 677, 1990; 19, 1617, 1990.

Double_t WaterModel::N_Index(Double_t flmda)
{
  if( fopn == 0 ){
      findex = fIndexLg->Eval(flmda,0,"S");
    }
    
  if( fopn == 1 ){
    Temp=5.0; //in Celsius degree
    r5=999.95; r1=-3.983; r2=301.797; r3=522528.9; r4=69.349; 
    a0=0.2439; a1=9.5352*0.001; a2=3.6436*-0.001; a3=2.6567*0.0001; a4=1.5919*0.001; a5=2.4573*0.001; a6=0.8975; a7=1.6307*-0.01; lambdauv=0.2292; lambdair=5.4329; 
    rho = r5*(1-((Temp+r1)*(Temp+r1)*(Temp+r2))/(r3*(Temp+r4))); //water density From: http://iopscience.iop.org/0026-1394/38/4/3/pdf/0026-1394_38_4_3.pdf
    A0 = a0;
    A1 = (a1*rho*0.001);
    A2 = (a2*(273.15+Temp)/273.15);
    A3 = (a3*((273.15+Temp)/273.15)*((flmda*flmda)/(589.0*589.0)));
    A4 = (a4*(589.0*589.0)/(flmda*flmda));
    A5 = (a5/(((flmda*flmda)/(589.0*589.0))-(lambdauv*lambdauv)));
    A6 = (a6/(((flmda*flmda)/(589.0*589.0))-(lambdair*lambdair)));
    A7 = (a7*(A1/a1)*(A1/a1));
    A = A0+A1+A2+A3+A4+A5+A6+A7;
    findex = sqrt(((2.0*A*A1/a1)+1.0)/(1.0-(A*A1/a1)));
  }
  return findex;

}

/*Double_t WaterModel::N_Index_d(Double_t dist)
{
    Double_t norm = 0.0;
    Double_t sumindex = 0.0;
    for(Double_t l=200.;l<700.;l+=10.){
      norm += FinlSpect(l, dist);
      sumindex += N_Index(l);}
    return (sumindex/norm);
}

Double_t WaterModel::N_Index(Double_t flmda, Double_t Temp)
{
    r5=999.95; r1=-3.983; r2=301.797; r3=522528.9; r4=69.349; 
    a0=0.2439; a1=9.5352*0.001; a2=3.6436*-0.001; a3=2.6567*0.0001; a4=1.5919*0.001; a5=2.4573*0.001; a6=0.8975; a7=1.6307*-0.01; lambdauv=0.2292; lambdair=5.4329; 
    rho = r5*(1-((Temp+r1)*(Temp+r1)*(Temp+r2))/(r3*(Temp+r4))); //water density From: http://iopscience.iop.org/0026-1394/38/4/3/pdf/0026-1394_38_4_3.pdf
    A0 = a0;
    A1 = (a1*rho*0.001);
    A2 = (a2*(273.15+Temp)/273.15);
    A3 = (a3*((273.15+Temp)/273.15)*((flmda*flmda)/(589.0*589.0)));
    A4 = (a4*(589.0*589.0)/(flmda*flmda));
    A5 = (a5/(((flmda*flmda)/(589.0*589.0))-(lambdauv*lambdauv)));
    A6 = (a6/(((flmda*flmda)/(589.0*589.0))-(lambdair*lambdair)));
    A7 = (a7*(A1/a1)*(A1/a1));
    A = A0+A1+A2+A3+A4+A5+A6+A7;
    findex = sqrt(((2.0*A*A1/a1)+1.0)/(1.0-(A*A1/a1)));
    return findex;
}*/
//==================================================================================================
// A function to calculate the group velocity of a photon in water with a given index of refraction. 
//Partial derivative of (c/(n*lmda)) with respect to (1/lmda)

Double_t WaterModel::Vg(Double_t flmda)
{   
    if( fopv==0 ) fvelo = fVeloLg->Eval(flmda,0,"S"); 
    if( fopv==1 ) { 
      C=Parameters::SpeedOfLight(); N=N_Index(flmda);
      dndl = ((N_Index(flmda + 0.001)) - (N_Index(flmda - 0.001)))/0.002;
      fvelo = C*((1.0/N) + ((flmda/(N*N))*dndl));// cm/ns*/
    }
    return fvelo;
}


Double_t WaterModel::TimeMu(Double_t mudist)
{
   Double_t tmu = (1195.82 - sqrt(4*mudist*mudist - 4800.0*mudist + 1430000))/60.0;
   return tmu;
}

Double_t WaterModel::ChereAngle(Double_t mudist)
{
   Double_t tmu = (1195.82 - sqrt(4*mudist*mudist - 4800.0*mudist + 1430000))/60.0;
   return tmu;
}
//====================================================================
// the function to calculate the Cherenkov Radiation Spectrum
//
// From: John David Jackson, Classical Electrodynamics, Third Edition

Double_t WaterModel::InitSpect(Double_t flmda)
{   
    if( fopn==0 ){ finitintensity = fInitLg->Eval(flmda,0,"S"); }
    if( fopn==1 ){
       finitintensity = ((((N_Index(flmda+0.001) - N_Index(flmda-0.001))*(flmda)/(0.002*N_Index(flmda)))+1.0)*(1.0-1.0/(N_Index(flmda)*N_Index(flmda))))/((flmda)*(flmda)*N_Index(flmda)*0.00112105);  // number of photons per nm
    }
    if(finitintensity>=0.0) return finitintensity;
    else return 0.0;
}

//=====================================================================
//calculating absorption length
//
//From Table 1, George M. Hale and Marvin R. Querry, Applied Optics, Wo. 12, Issue 3, pp. 555-563(1973)
void WaterModel::SetAbsTable(TGraph* abslg)
{ 
    fAbsLg = abslg;
}

Double_t WaterModel::AbsLength(Double_t flmda)
{
    if( fopa==0 ){
      fabslength = fAbsLg->Eval(flmda,0,"S");
    }
    if( fopa==1 ){
      if (flmda<200.0) {fabslength = 9.09;}
      if (flmda>=200 && flmda<350) {fabslength = -1060.317+12.816*flmda-0.0516*flmda*flmda+0.0000712*flmda*flmda*flmda;}
      if (flmda>=350 && flmda<425) {fabslength = 89543.154-695.489*flmda + 1.7825*flmda*flmda - 0.0015*flmda*flmda*flmda;}
      if (flmda>=425 && flmda<525) {fabslength = 122295.605 - 1083.224*flmda + 3.486*flmda*flmda - 0.00479995*flmda*flmda*flmda + 0.0000023808*flmda*flmda*flmda*flmda;}
      if (flmda>=525 && flmda<600) {fabslength = 5438.412 - 8.923*flmda;}
      if (flmda>=600 && flmda<700) {fabslength = -276.278+ 1.564*flmda- 0.00161*flmda*flmda; }
      if (flmda>=700) { fabslength = 29.851; }
      fabslength = 10*fabslength;
    }
    return fabslength;
}

//=====================================================================
//
//calculating the attenuating factor, the output spectrum when the input spectrum is uniform.

  Double_t WaterModel::Atten(Double_t flmda, Double_t fdist)
  {
    Double_t absl = fAbsLg->Eval(flmda,0,"S");
    Double_t sctl = fSctLg->Eval(flmda,0,"S");
    //Double_t absl = AbsLength(flmda);
    Double_t absP = exp(fdist/absl);
    Double_t sctP = exp(fdist/sctl);
    fatten = 1/(absP + sctP - 1.0);
    return fatten;
  }

//=======================================================================
//
//Calculate the QE of a particular wavelength, the min wavelength here is 280!!

void WaterModel::SetQETable(TGraph* mytg)
{
   fQEg = mytg;
}

Double_t WaterModel::QE(Double_t flmda)
{
    fqe = fQEg->Eval(flmda,0,"S");
    if( fqe>=0.0 ) return fqe;
    else return 0.0;
}

//======================================================================
//
//Calculating the intensity of output spectrum, when the input spectrum is a standard Cherenkov spectrum with a single d0.
Double_t WaterModel::FinlSpect(Double_t flmda, Double_t fdist)
{
    //Double_t norm2 = 0.0;
    //for( Double_t l=200.;l<700.;l=l+10.) norm2 = norm2 + 10*InitSpect(l+5.0)*Atten(l+5.0,fdist); 
    //ffinlintensity = InitSpect(flmda)*Atten(flmda,fdist)/norm2;
    //for( Double_t l=200.;l<700.;l=l+10.) norm2 = norm2 + 10*InitSpect(l+5.0)*Atten(l+5.0,fdist)*QE(flmda); 
    ffinlintensity = InitSpect(flmda)*Atten(flmda,fdist);
    return ffinlintensity;
}

//=======================================================================
//
//return several parameters at once 
WaterModel::waterM WaterModel::getParamsWM(Double_t v, Double_t initi, Double_t absl, Double_t sctl, Double_t qe, Double_t fdist)
{
WaterModel::waterM waterParams;

Double_t absP = exp(fdist/absl);
Double_t sctP = exp(fdist/sctl);
Double_t a = 1/(absP + sctP - 1.0); //attenuation

waterParams.velocity = v;
waterParams.finalSpectrum = initi*a*qe;

return waterParams;

}

//=======================================================================
//output spectrum when the input has multiple distance di.

TH1D *WaterModel::FinlTimeSpect(TH1D *dhist)
{ 
    TH1D *ffinltime = new TH1D("final time spectrum","final time spectrum",1000,0,500);
    numD = dhist->TH1D::GetNbinsX();
    minDist = dhist->TH1D::GetBinLowEdge(0);
    width = dhist->TH1D::GetBinWidth(0);
    for (Int_t i=0; i<numD; i++) {
       fdist = minDist + (i + 0.5)*width;
       numPh = dhist->TH1D::GetBinContent(i);
       for (Double_t flmda = 200.; flmda<700.; flmda = flmda + 5.0) {
           TimePh = fdist/Vg(flmda + 2.5);
           Double_t dTimeBin = TimePh/0.5;
           TimeBin = (Int_t) dTimeBin;
           Intensity = numPh*InitSpect(flmda)*Atten(flmda,fdist);
           Double_t tempcontent = ffinltime->TH1D::GetBinContent(TimeBin);
           Intensity += tempcontent;
           ffinltime->TH1D::SetBinContent(TimeBin,Intensity);
           }
       }
   return ffinltime;
}
