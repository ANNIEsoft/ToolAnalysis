#ifndef FindTrackLengthInWater_H
#define FindTrackLengthInWater_H

#include <string>
#include <iostream>
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
  int maxhits0=1100;
  bool first=1; bool deny_access=0;
  // counters to keep track of cut efficiencies
  int count1=0, count2=0, count3=0, count4=0;
  std::ofstream csvfile;
  Geometry* anniegeom=nullptr;
  double tank_radius;
  double tank_halfheight;
  int fDoTraining=0;
  
	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int verbosity=1;
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	int get_ok;
};


#endif
