#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <string>
#include <vector>
#include "RecoDigit.h"

using namespace std;

class Parameters{

 public:
 	double fSpeedOfLight;
 	double fCherenkovAngle;
 	double fRefractiveIndex;
 	double fTimeNoiseRate;
 	double fPMTTimeResolution;
 	double fPMTPositionResolution;
 	double fLAPPDTimeResolution;
 	double fLAPPDPositionResolution;
 	int fSeedDigitType;
 	std::string fConfigurationType;
 	std::vector<int> fLappdId;
 	
  static Parameters* Instance();

  static void UseSimpleParameters();
  static void UseSimpleTimeResolution();
  static void UseSimpleTimeSlew();
  static void UseSimpleRefractiveIndex();

  static double SpeedOfLight();
  static double CherenkovAngle();
  static double Index0(); //...chrom1.34
  static double TimeResolution(double Q);
  static double TimeResolution(int DigitType);
  static double TimeResolution(int DigitType, double Q);
  static double PositionResolution(int DigitType);
  static double TimeSlew(double Q);
  static double RefractiveIndex(double r);

  static double ThetaC();     // Cherenkov Angle
  static double CosThetaC();  // Cosine of Cherenkov Angle
  static double TimeNoiseRate();
  static int SeedDigitType();
//  static void LoadConfigFile(string configfilename);

  static void PrintParameters();
  void RunPrintParameters();
//  void RunLoadConfigFile(string configfilename);

  void SetSimpleTimeResolution() { fUseSimpleTimeResolution = 1; }
  bool SimpleTimeResolution() { return fUseSimpleTimeResolution; }

  void SetSimpleTimeSlew() { fUseSimpleTimeSlew = 1; }
  bool SimpleTimeSlew() { return fUseSimpleTimeSlew; }

  void SetSimpleRefractiveIndex(){ fUseSimpleRefractiveIndex = 1; }
  bool SimpleRefractiveIndex() { return fUseSimpleRefractiveIndex; }

  double GetCherenkovAngle();
  double GetTimeResolution(double Q);
  
  double GetTimeResolution(int DigitType); 
  double GetTimeResolution(int DigitType, double Q); 
  double GetPositionResolution(int DigitType);
  
  double GetTimeSlew(double Q);
  double GetRefractiveIndex(double r);
  int GetSeedDigitType();
  std::string GetConfigurationType();
  
  double GetSimpleTimeResolution(double Q);
  double GetSimpleTimeSlew() { return 0.0;}
  double GetSimpleRefractiveIndex() { return 1.33;}
  
  
 private:
  Parameters();
  ~Parameters();

  bool fUseSimpleTimeResolution;
  bool fUseSimpleTimeSlew;
  bool fUseSimpleRefractiveIndex;

};

#endif







