#ifndef FindTrackLengthInWater_H
#define FindTrackLengthInWater_H

#include <string>
#include <iostream>
#include <sstream>
#include "ANNIEalgorithms.h"

#include "Tool.h"
#include "TFile.h"
#include "TTree.h"
#include "TMath.h"
#include "ExampleRoot.h"

class FindTrackLengthInWater: public Tool {


 public:

  FindTrackLengthInWater();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  double find_lambda(double xmu_rec,double ymu_rec,double zmu_rec,double xrecDir,double yrecDir,double zrecDir,double x_pmtpos,double y_pmtpos,double z_pmtpos,double theta_cher);
  bool Finalise();


 private:
  std::string infile;
  TFile* file;
  TTree* regTree;
  TTree * nu_eneNEW;
  ExampleRoot* Data;

  int maxhits0=1100;
  long currententry;
  long NumEvents;
  bool first=1; bool deny_access=0;
  double diffDirAbs2=0; double diffDirAbs=0;
  double recoDWallR2=0; double recoDWallZ2=0;
  int count1=0;
  
  std::ofstream csvfile;
  std::string myfile;
  std::string outputdir="";
  bool writefile=false;
  TFile* outputFile;
};


#endif
