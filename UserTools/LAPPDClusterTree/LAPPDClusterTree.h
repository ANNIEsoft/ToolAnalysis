#ifndef LAPPDClusterTree_H
#define LAPPDClusterTree_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TTree.h"
#include "TFile.h"
#include "TString.h"
/**
 * \class LAPPDClusterTree
 *
 * This is a blank template for a Tool used by the script to generate a new custom tool. Please fill out the description and author information.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class LAPPDClusterTree: public Tool {


 public:

  LAPPDClusterTree(); ///< Simple constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Initialise Function for setting up Tool resources. @param configfile The path and name of the dynamic configuration file to read in. @param data A reference to the transient data class used to pass information between Tools.
  bool Execute(); ///< Execute function used to perform Tool purpose.
  bool Finalise(); ///< Finalise function used to clean up resources.
  TFile* nottf;
  TTree* fMyTree;   //!pointer to the Tree

  bool simpleClusters;
  string CFDClusterLabel;
  string SimpleClusterLabel;
  Geometry* _geom;


  // declare the tree variables

  Int_t         NHits;

  // NEW!!!
  Int_t         WraparoundBin;
  Int_t         T0Bin;
  Int_t         QualityVar;
  Double_t      TrigDeltaT;
  Double_t      PulseHeight;

  Double_t      BeamTime;
  Double_t      EventTime;
  Double_t      TotalCharge;

  // END NEW!!!

  Double_t      hQ[60];
  Double_t      hT[60];
  double_t      hxpar[60];
  double_t      hxperp[60];
  double_t      htime[60];
  double_t      hdeltime[60];
  double_t      hvpeak[60];

  //NEW!!!
  Int_t         NHits_simp;
  Double_t      hQ_simp[60];
  Double_t      hT_simp[60];
  Double_t      htime_simp[60];
  double_t      hxpar_simp[60];
  double_t      hxperp_simp[60];

  Int_t         Npulses_cfd;
  Double_t      pulsestart_cfd[60];
  Int_t         pulsestrip_cfd[60];
  Int_t         pulseside_cfd[60];
  Double_t      pulseamp_cfd[60];
  Double_t      pulseQ_cfd[60];

  Int_t         Npulses_simp;
  Double_t      pulsestart_simp[60];
  Double_t      pulseend_simp[60];
  Double_t      pulseamp_simp[60];
  Double_t      pulseQ_simp[60];
  Double_t      pulsepeakbin_simp[60];
  Int_t         pulsestrip_simp[60];
  Int_t         pulseside_simp[60];


 private:

    int LAPPDClusterTreeVerbosity;



};


#endif
