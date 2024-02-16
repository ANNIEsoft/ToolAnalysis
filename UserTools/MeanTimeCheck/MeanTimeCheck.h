#ifndef MeanTimeCheck_H
#define MeanTimeCheck_H

#include <string>
#include <iostream>
#include "TROOT.h"
#include "TChain.h"
#include "TFile.h"

#include "Tool.h"
#include "TTree.h"
#include "TH2D.h"
#include "Parameters.h"
#include "VertexGeometry.h"
#include "Detector.h"


/**
 * \class MeanTimeCheck
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class MeanTimeCheck: public Tool {


 public:

  MeanTimeCheck(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.


 private:

double BasicAverage();
std::vector<RecoDigit>* DigitList = 0;
RecoVertex* TrueVtx = 0;
double GetPeakTime();
double GetWeightedAverage();
double GetWeightedPeak();
TFile* Output_tfile = nullptr;
TTree* fVertexGeometry = nullptr;
uint64_t MCEventNum;
uint16_t MCTriggerNum;
uint32_t EventNumber;
TH1D *delta;
TH1D *peakTime;
TH1D *basicAverage;
TH1D *weightedAverage;
TH1D *weightedPeak;
VertexGeometry* VtxGeo;

int verbosity = -1;
int ShowEvent = -1;
int v_error = 0;
int v_warning = 1;
int v_message = 2;
int v_debug = 3;
std::string logmessage;
int get_ok;

};


#endif
