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
//  Double_t      TrigDeltaT;
  Double_t      TrigDeltaT1;
  Double_t      TrigDeltaT2;
  Double_t      PulseHeight;

  Double_t      BeamTime;
  Double_t      EventTime;
  Double_t      TotalCharge;
  Double_t      MaxAmp0;
  Double_t      MaxAmp1;



  // END NEW!!!

  Double_t      hQ[60];
  Double_t      hT[60];
  Double_t      hxpar[60];
  Double_t      hxperp[60];
  Double_t      htime[60];
  Double_t      hdeltime[60];
  Double_t      hvpeak[60];

  // Strip-level variables

  Int_t         Nchannels;
  Double_t      StripPeak[60];
  Double_t      StripPeak_Sm[60];
  Double_t      StripPeakT[60];
  Double_t      StripPeakT_Sm[60];
  Double_t      StripQ[60];
  Double_t      StripQ_Sm[60];

  //NEW!!!
  Int_t         NHits_simp;
  Double_t      hQ_simp[60];
  Double_t      hT_simp[60];
  Double_t      htime_simp[60];
  Double_t      hxpar_simp[60];
  Double_t      hxperp_simp[60];

  Double_t      nnlsParallelP[60];
  Double_t      nnlsTransverseP[60];
  Double_t      nnlsArrivalTime[60];
  Double_t      nnlsAmplitude[60];


  Double_t      SelectedAmp0[60];
  Double_t      SelectedAmp1[60];
  Double_t      SelectedTime0[60];
  Double_t      SelectedTime1[60];
  Double_t      GMaxOn0[60];
  Double_t      GMaxOn1[60];

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
