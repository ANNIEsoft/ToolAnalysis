#include "Parameters.h"
//#include "ConfigReader.h"

#include <cmath>
#include <iostream>
#include <cassert>

static Parameters* fgParameters = 0;

Parameters* Parameters::Instance()
{
  if( !fgParameters ){
    fgParameters = new Parameters();
  }

  if( !fgParameters ){
    assert(fgParameters);
  }

  if( fgParameters ){

  }

  return fgParameters;
}
  
Parameters::Parameters()
{
  fUseSimpleTimeResolution = 0;
  fUseSimpleTimeSlew = 0;
  fUseSimpleRefractiveIndex = 0;
  fCherenkovAngle = 42.0;
  fRefractiveIndex = 1.33;
  fPMTTimeResolution = 1.0; // ns
  fPMTPositionResolution = 1e-3; //PMT position is already taken care by the input file. Set a negligible value here
  fLAPPDTimeResolution = 0.1; // ns
  fLAPPDPositionResolution = 0.1; //0.1 cm
  fSeedDigitType = RecoDigit::lappd_v0;
  fConfigurationType = "PMT";
}

Parameters::~Parameters()
{

}
  
int Parameters::SeedDigitType() {
  return Parameters::Instance()->GetSeedDigitType();
}

void Parameters::UseSimpleParameters()
{
  Parameters::UseSimpleTimeResolution();
  Parameters::UseSimpleTimeSlew();
  Parameters::UseSimpleRefractiveIndex();
}

void Parameters::UseSimpleTimeResolution()
{
  Parameters::Instance()->SetSimpleTimeResolution();
}

void Parameters::UseSimpleTimeSlew()
{
  Parameters::Instance()->SetSimpleTimeSlew();
}

void Parameters::UseSimpleRefractiveIndex()
{
  Parameters::Instance()->SetSimpleRefractiveIndex();
}

double Parameters::TimeResolution(double Q)
{
  if( Parameters::Instance()->SimpleTimeResolution() ){
    return Parameters::Instance()->GetSimpleTimeResolution(Q);
  }
  else {
    return Parameters::Instance()->GetTimeResolution(Q);
  }
}

double Parameters::TimeResolution(int DigitType, double Q)
{
  
  return Parameters::Instance()->GetTimeResolution(DigitType, Q);
}

double Parameters::TimeResolution(int DigitType)
{
  
  return Parameters::Instance()->GetTimeResolution(DigitType);
}

double Parameters::PositionResolution(int DigitType)
{
  return Parameters::Instance()->GetPositionResolution(DigitType);
}

double Parameters::TimeSlew(double Q)
{
  if( Parameters::Instance()->SimpleTimeSlew() ){
    return Parameters::Instance()->GetSimpleTimeSlew();
  }
  else{
    return Parameters::Instance()->GetTimeSlew(Q);
  }
}

double Parameters::RefractiveIndex(double L)
{
  if( Parameters::Instance()->SimpleRefractiveIndex() ){
    return Parameters::Instance()->GetSimpleRefractiveIndex();
  }
  else{
    return Parameters::Instance()->GetRefractiveIndex(L);
  }
}

void Parameters::PrintParameters()
{
  Parameters::Instance()->RunPrintParameters();
}

void Parameters::RunPrintParameters()
{
  std::cout << " *** Parameters::PrintParameters() *** " << std::endl;
  std::cout << "  CheronkovAngle = "<< fCherenkovAngle << std::endl;
  std::cout << "  RefractiveIndex = "<< fRefractiveIndex << std::endl;
  std::cout << "  PMTTimeResolution = "<< fPMTTimeResolution << " ns "<< std::endl;
  std::cout << "  LAPPDTimeResolution = "<< fLAPPDTimeResolution << " ns "<<std::endl;
  std::cout << "  LAPPDPositionResolution = "<< fLAPPDPositionResolution << " cm "<<std::endl;

  std::cout << "  ***Reco Parameters: " << std::endl
            << "  UseSimpleTimeResolution = " << fUseSimpleTimeResolution << std::endl
            << "  UseSimpleTimeSlew = " << fUseSimpleTimeSlew << std::endl
            << "  UseSimpleRefractiveIndex = " << fUseSimpleRefractiveIndex << std::endl
            << "  SeedDigitType = " << fSeedDigitType << std::endl
            << "  ConfigurationType = " << fConfigurationType << std::endl;

  return;
}

double Parameters::SpeedOfLight()
{
  return 29.9792458;  // velocity of light [cm/ns]
}

double Parameters::Index0()
{
  return 1.333; //...chrom1.34
}

double Parameters::CherenkovAngle()
{
  //return (180.0/3.14)*(acos(30.0/(29.0*Index0())));
  //return 42.0;  // degrees
  return Parameters::Instance()->GetCherenkovAngle();
  //return 43.56128448; // degree,...chrom1.38 index of refraction
  //return 41.73181704; // degree,...chrom1.34 index of refraction
  //return 41.97005618; //...chrom1.345
}

double Parameters::ThetaC()
{
  //return (180.0/3.14)*(acos(30.0/(29.0*Index0())));
  return 42.0;  // degrees
  //return 43.56128448; // degree,...chrom1.38 index of refraction
  //return 41.73181704; // degree,...chrom1.34 index of refraction
  //return 41.97005618; //...chrom1.345
}

double Parameters::CosThetaC()
{
  //return (30.0/(29.0*Index0()));
  return 0.743144825477394244;  // return TMath::Cos(42.0*TMath::Pi()/180.0);
  //return 0.7246376812; // ...chrom1.38
  //return 0.7462686567; // ...chrom1.34
  //return 0.7434944238; //...chrom1.345
}
                                                                                                                                                                                                                                                         
double Parameters::TimeNoiseRate() {
	return 0.10;	
}

int Parameters::GetSeedDigitType() {
  return fSeedDigitType;
}

std::string Parameters::GetConfigurationType() {
  return fConfigurationType;
}

double Parameters::GetCherenkovAngle()
{
  return fCherenkovAngle;
}

double Parameters::GetPositionResolution(int detType) {
  if(detType == RecoDigit::PMT8inch) {
  	  return fPMTPositionResolution; //
  	}
    else if(detType == RecoDigit::lappd_v0) {
  	  return fLAPPDPositionResolution;
  	}
  	else {
  	  std::cout<<"Wrong detector type! Can't find position resolution!"<<std::endl;	
  	}	
}

double Parameters::GetTimeResolution(int detType) {
  	if(detType == RecoDigit::PMT8inch) { //PMT
  	  return fPMTTimeResolution;
  	}
    else if(detType == RecoDigit::lappd_v0) { //LAPPD
  	  return fLAPPDTimeResolution;
  	}
  	else {
  	  std::cout<<"Wrong detector type! Can't find time resolution!"<<std::endl;	
  	}
}

double Parameters::GetTimeResolution(int detType, double Q) {
  	double res = 0.;
  	if(detType == RecoDigit::PMT8inch) { //PMT
  	  double qpes = Q;
  	  if( qpes<0.5 ) qpes = 0.5;
  	  if( qpes>32.0 ) qpes = 32.0;
  	  res = 0.33 + sqrt(2.0/qpes);  
  	  if(res<0.58) res = 0.58;
  	  return res;
  	}
    else if(detType == RecoDigit::lappd_v0) { //LAPPD, extv1, extv2
      double timingResConstant=0.001;              
      double qpes = (Q> 0.5) ? Q : 0.5;
      // based on a roughly similar form that gives ~60ps for Q~1 and ~5ps for Q~30
      // XXX if changing this ensure it is mirrored above.
      res = 0.001 + 0.04*pow(qpes,-0.7);
      // saturates @ 0.065 with majority of entries.
      if (res < 0.005) res = 0.005;
      return res;
      //return 1.0;
  	}
//  	else if(detType == RecoDigit::lappd_v0) { //LAPPD, extv3
//      double timingResConstant=0.001;              // TTS: 50ps;
//      double qpes = (Q> 0.5) ? Q : 0.5;
//      // based on a roughly similar form that gives ~60ps for Q~1 and ~5ps for Q~30
//      // XXX if changing this ensure it is mirrored above.
//      res = 0.00 + 0.12*pow(qpes,-0.9);
//      // saturates @ 0.065 with majority of entries.
//      if (res < 0.005) res = 0.005;
//      return res;
//  	}
  	else {
  	  std::cout<<"Wrong detector type! Can't find time resolution!"<<std::endl;	
  	  return 0;
  	}
}

double Parameters::GetTimeResolution(double Q)
{  
  
   // Old Parameterisation (lifted from )
   // ========================================
   double qpes = Q;
   if( qpes<0.5 ) qpes = 0.5;
   if( qpes>32.0 ) qpes = 32.0;
   double res = 0.33 + sqrt(2.0/qpes);  
   if(res<0.58) res = 0.58;
  
  
  return res;
}

double Parameters::GetTimeSlew(double Q)
{   
  /*
   // Sep'2010: parameterisation, including scattered light:
   // ======================================================
   double c0 = +3.406, c1 = -2.423, c2 = +0.335;

   // Aug'2011: re-parameterisation, excluding scattered light:
   // =========================================================
   double c0 = +2.234, c1 = -1.362, c2 = +0.125;  

   // Nov'2011: re-parameterisation, for 200 kton geometry:
   // =====================================================
   double c0 = +2.436, c1 = -1.291, c2 = +0.089;  
  */

  double qpes = Q;
  if( qpes<0.25 ) qpes = 0.25;
  if( qpes>40.0 ) qpes = 40.0;

  double c0 = +2.436;
  double c1 = -1.291;
  double c2 = +0.089;

  double dt = c0 
              + c1*log(qpes) 
              + c2*log(qpes)*log(qpes);

  return dt;
}

double Parameters::GetRefractiveIndex(double r)
{
  double c = 29.98;       
  double n0 = Index0();        // Old Attempt:
  //double n0 = 1.38;        // Old Attempt:...index1.38
  double L0 = 0.0;         // 40.0     
  double dndx = 0.000123;  // 0.00015  

  double L = r/c;

  double n = n0*(1.0+dndx*(L-L0));
  
  return n;
}

double Parameters::GetSimpleTimeResolution(double Q)
{  
  double qpes = Q;
  if( qpes<0.25 ) qpes = 0.25;
  if( qpes>64.0 ) qpes = 64.0;

  double res = 2.0/sqrt(qpes);

  return res;
}

/*void Parameters::LoadConfigFile(string configfilename)
{
  Parameters::Instance()->RunLoadConfigFile(configfilename);
}


void Parameters::RunLoadConfigFile(string configfilename) {
  // Use Chad Jarvis's config class to obtain relevant parameters of the fit
  // Set global variables
  ConfigReader runconfig(configfilename);
  fCherenkovAngle = runconfig.getDouble("CherenkovAngle");
  fRefractiveIndex = runconfig.getDouble("RefractiveIndex");
  fPMTTimeResolution = runconfig.getDouble("PMTTimeResolution"); 
  fLAPPDTimeResolution = runconfig.getDouble("LAPPDTimeResolution"); 
  fLAPPDPositionResolution = runconfig.getDouble("LAPPDPositionResolution");
  fSeedDigitType = runconfig.getString("SeedDigitType");
  fConfigurationType = runconfig.getString("ConfigurationType");
  fLappdId = runconfig.getIntArray("LappdId");
  for(int i=0;i<fLappdId.size();i++) {
  	std::cout<<"NLappd = "<<fLappdId.size()<<std::endl;
  	std::cout<<"LappdId = "<<fLappdId.at(i)<<std::endl;
  }
}*/
