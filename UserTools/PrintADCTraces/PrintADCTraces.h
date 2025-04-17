#ifndef PrintADCTraces_H
#define PrintADCTraces_H

#include "Tool.h"
#include "TFile.h"
#include "TTree.h"
#include "TGraph.h"
#include "ADCPulse.h"
#include "Hit.h"

/**
 * \class PrintADCTraces
 *
 * \brief Tool to extract and store ADC traces in TGraphs
 *
 * $Author: S. Doran $
 * $Date: Apr 2025 $
 * Contact: doran@iastate.edu
 */
class PrintADCTraces: public Tool {


 public:

  PrintADCTraces(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
    
 private:

  TFile* fOutFile = nullptr;                   // output root file
  TTree* fTraceSummaryTree = nullptr;          // summary TTree in root file
  int fchan;                                   // branches in summary TTree
  float fhitT, fhitPE;

  int totalGraphs = 0;                         // keep track how many TGraphs were written
  std::map<unsigned long, int> graphsPerChannel;

  float fhitTmin = 0;                          // config parameters
  float fhitTmax = 0;
  float fhitPEmin = 0;
  float fhitPEmax = 0;
  int fMaxTraces = 0;
  int fMaxTracesPerChan = 0;
  std::string filename;

  bool gotTmin = false;
  bool gotTmax = false;
  bool gotPEmin = false;
  bool gotPEmax = false;

                                               // maps to Store objects
  std::map<unsigned long, std::vector<std::vector<ADCPulse>>> fRecoADCData;
  std::map<int, double> fChannelKeyToSPEMap;
  
  int verbosity;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;
  std::string logmessage;
};


#endif



