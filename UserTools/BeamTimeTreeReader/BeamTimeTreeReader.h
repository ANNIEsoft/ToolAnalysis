#ifndef BeamTimeTreeReader_H
#define BeamTimeTreeReader_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TMath.h"

class BeamTimeTreeReader: public Tool {


 public:

  BeamTimeTreeReader();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

   TFile* tf;

   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

   // Declaration of leaf types
   Float_t         beamwgt;
   Int_t           ntp;
   Int_t           npart;
   Int_t           id[7];   //[npart]
   Float_t         ini_pos[7][3];   //[npart]
   Float_t         ini_mom[7][3];   //[npart]
   Float_t         ini_eng[7];   //[npart]
   Float_t         ini_t[7];   //[npart]
   Float_t         fin_mom[7][3];   //[npart]
   Float_t         fin_pol[7][3];   //[npart]
   Float_t         mul_weight[7][1000];

   Int_t           ientry;

   // List of branches
   TBranch        *b_beamwgt;   //!
   TBranch        *b_ntp;   //!
   TBranch        *b_npart;   //!
   TBranch        *b_id;   //!
   TBranch        *b_ini_pos;   //!
   TBranch        *b_ini_mom;   //!
   TBranch        *b_ini_eng;   //!
   TBranch        *b_ini_t;   //!
   TBranch        *b_fin_mom;   //!
   TBranch        *b_fin_pol;   //!
   TBranch        *b_mul_weight;   //!

   TString InFile;
   TString OutFile;
   double targetR;
   double baseline;
   double tc1;
   double tc2;
   double tc3;

};


#endif
