//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Sun Nov  6 06:04:43 2016 by ROOT version 5.34/04
// from TTree annie/annie
// found on file: r200s0p0.root
//////////////////////////////////////////////////////////

#ifndef PulseTree_h
#define PulseTree_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.
#include <TObject.h>

// Fixed size dimensions of array or collections stored in the TTree if any.
const Int_t kMaxfPmts = 64;

class PulseTree {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

   // Declaration of leaf types
 //Trigger         *Trig;
   UInt_t          fUniqueID;
   UInt_t          fBits;
   Int_t           NChannels;
   Int_t           BufferSize;
   Float_t         GlobalTime;
   Int_t           TrigNo;
   Int_t           NPmts;
   Int_t           fPmts_;
   UInt_t          fPmts_fUniqueID[kMaxfPmts];   //[fPmts_]
   UInt_t          fPmts_fBits[kMaxfPmts];   //[fPmts_]
   Int_t           fPmts_ID[kMaxfPmts];   //[fPmts_]
   Int_t           fPmts_x[kMaxfPmts];   //[fPmts_]
   Int_t           fPmts_y[kMaxfPmts];   //[fPmts_]
   Int_t           fPmts_z[kMaxfPmts];   //[fPmts_]
   Int_t           fPmts_AdcCard[kMaxfPmts];   //[fPmts_]
   Int_t           fPmts_AdcChannel[kMaxfPmts];   //[fPmts_]
   Int_t           fPmts_BufferSize[kMaxfPmts];   //[fPmts_]
   Int_t           fPmts_Npulsesgreater15[kMaxfPmts];   //[fPmts_]
   Float_t         fPmts_Pedestal[kMaxfPmts];   //[fPmts_]
   Float_t         fPmts_PedestalNoise[kMaxfPmts];   //[fPmts_]
   Float_t         fPmts_PulseTraceColl[kMaxfPmts][15][30];   //[fPmts_]
   Int_t           fPmts_Npulses[kMaxfPmts];   //[fPmts_]
   Float_t         fPmts_t[kMaxfPmts][15];   //[fPmts_]
   Float_t         fPmts_q[kMaxfPmts][15];   //[fPmts_]
   Float_t         fPmts_amp[kMaxfPmts][15];   //[fPmts_]
   Float_t         fPmts_fwhm[kMaxfPmts][15];   //[fPmts_]
   Float_t         fPmts_risetime[kMaxfPmts][15];   //[fPmts_]
   Float_t         fPmts_pulsestatus[kMaxfPmts][15];   //[fPmts_]
   UInt_t          fPmts_Rate[kMaxfPmts];   //[fPmts_]
   Int_t           fPmts_Pmtf[kMaxfPmts];   //[fPmts_]
   Int_t           fPmts_SignalType[kMaxfPmts];   //[fPmts_]
   Int_t           _nsampreduced;
   Int_t           hNPMTs_vs_time[40000];
   Float_t         hcharge_vs_time[40000];
   Int_t           SequenceID;
   Int_t           StartTimeSec[16];
   Int_t           StartTimeNSec[16];
   ULong64_t       StartCount[16];
   ULong64_t       TriggerCounts[16];
   ULong64_t       LastSync[16];
   Int_t           TriggerTimeSec[16];
   Int_t           TriggerTimeNSec[16];
   Int_t           RunNumber;
   Int_t           SubrunNumber;
   Int_t           FileNumber;
   Bool_t          debug;
   Int_t           fnPmt;
   Int_t           fnMrd;
   Int_t           fnRwm;

   // List of branches
   TBranch        *b_Trig_fUniqueID;   //!
   TBranch        *b_Trig_fBits;   //!
   TBranch        *b_Trig_NChannels;   //!
   TBranch        *b_Trig_BufferSize;   //!
   TBranch        *b_Trig_GlobalTime;   //!
   TBranch        *b_Trig_TrigNo;   //!
   TBranch        *b_Trig_NPmts;   //!
   TBranch        *b_Trig_fPmts_;   //!
   TBranch        *b_fPmts_fUniqueID;   //!
   TBranch        *b_fPmts_fBits;   //!
   TBranch        *b_fPmts_ID;   //!
   TBranch        *b_fPmts_x;   //!
   TBranch        *b_fPmts_y;   //!
   TBranch        *b_fPmts_z;   //!
   TBranch        *b_fPmts_AdcCard;   //!
   TBranch        *b_fPmts_AdcChannel;   //!
   TBranch        *b_fPmts_BufferSize;   //!
   TBranch        *b_fPmts_Npulsesgreater15;   //!
   TBranch        *b_fPmts_Pedestal;   //!
   TBranch        *b_fPmts_PedestalNoise;   //!
   TBranch        *b_fPmts_PulseTraceColl;   //!
   TBranch        *b_fPmts_Npulses;   //!
   TBranch        *b_fPmts_t;   //!
   TBranch        *b_fPmts_q;   //!
   TBranch        *b_fPmts_amp;   //!
   TBranch        *b_fPmts_fwhm;   //!
   TBranch        *b_fPmts_risetime;   //!
   TBranch        *b_fPmts_pulsestatus;   //!
   TBranch        *b_fPmts_Rate;   //!
   TBranch        *b_fPmts_Pmtf;   //!
   TBranch        *b_fPmts_SignalType;   //!
   TBranch        *b_Trig__nsampreduced;   //!
   TBranch        *b_Trig_hNPMTs_vs_time;   //!
   TBranch        *b_Trig_hcharge_vs_time;   //!
   TBranch        *b_Trig_SequenceID;   //!
   TBranch        *b_Trig_StartTimeSec;   //!
   TBranch        *b_Trig_StartTimeNSec;   //!
   TBranch        *b_Trig_StartCount;   //!
   TBranch        *b_Trig_TriggerCounts;   //!
   TBranch        *b_Trig_LastSync;   //!
   TBranch        *b_Trig_TriggerTimeSec;   //!
   TBranch        *b_Trig_TriggerTimeNSec;   //!
   TBranch        *b_Trig_RunNumber;   //!
   TBranch        *b_Trig_SubrunNumber;   //!
   TBranch        *b_Trig_FileNumber;   //!
   TBranch        *b_Trig_debug;   //!
   TBranch        *b_Trig_fnPmt;   //!
   TBranch        *b_Trig_fnMrd;   //!
   TBranch        *b_Trig_fnRwm;   //!

   PulseTree(TTree *tree=0);
   PulseTree(std::string name);
   virtual ~PulseTree();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);
};

#endif

#ifdef PulseTree_cxx
PulseTree::PulseTree(std::string name){

  fChain=new TTree(name.c_str(),name.c_str());
  
  b_Trig_TrigNo=fChain->Branch("TrigNo",&TrigNo,"TrigNo/I");
  b_Trig_TrigNo->SetCompressionLevel(4);

  //... etc fill out as you normally would

}
PulseTree::PulseTree(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("r200s0p0.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("r200s0p0.root");
      }
      f->GetObject("annie",tree);

   }
   Init(tree);
}

PulseTree::~PulseTree()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t PulseTree::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t PulseTree::LoadTree(Long64_t entry)
{
// Set the environment to read one entry
   if (!fChain) return -5;
   Long64_t centry = fChain->LoadTree(entry);
   if (centry < 0) return centry;
   if (fChain->GetTreeNumber() != fCurrent) {
      fCurrent = fChain->GetTreeNumber();
      Notify();
   }
   return centry;
}

void PulseTree::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("fUniqueID", &fUniqueID, &b_Trig_fUniqueID);
   fChain->SetBranchAddress("fBits", &fBits, &b_Trig_fBits);
   fChain->SetBranchAddress("NChannels", &NChannels, &b_Trig_NChannels);
   fChain->SetBranchAddress("BufferSize", &BufferSize, &b_Trig_BufferSize);
   fChain->SetBranchAddress("GlobalTime", &GlobalTime, &b_Trig_GlobalTime);
   fChain->SetBranchAddress("TrigNo", &TrigNo, &b_Trig_TrigNo);
   fChain->SetBranchAddress("NPmts", &NPmts, &b_Trig_NPmts);
   fChain->SetBranchAddress("fPmts", &fPmts_, &b_Trig_fPmts_);
   fChain->SetBranchAddress("fPmts.fUniqueID", fPmts_fUniqueID, &b_fPmts_fUniqueID);
   fChain->SetBranchAddress("fPmts.fBits", fPmts_fBits, &b_fPmts_fBits);
   fChain->SetBranchAddress("fPmts.ID", fPmts_ID, &b_fPmts_ID);
   fChain->SetBranchAddress("fPmts.x", fPmts_x, &b_fPmts_x);
   fChain->SetBranchAddress("fPmts.y", fPmts_y, &b_fPmts_y);
   fChain->SetBranchAddress("fPmts.z", fPmts_z, &b_fPmts_z);
   fChain->SetBranchAddress("fPmts.AdcCard", fPmts_AdcCard, &b_fPmts_AdcCard);
   fChain->SetBranchAddress("fPmts.AdcChannel", fPmts_AdcChannel, &b_fPmts_AdcChannel);
   fChain->SetBranchAddress("fPmts.BufferSize", fPmts_BufferSize, &b_fPmts_BufferSize);
   fChain->SetBranchAddress("fPmts.Npulsesgreater15", fPmts_Npulsesgreater15, &b_fPmts_Npulsesgreater15);
   fChain->SetBranchAddress("fPmts.Pedestal", fPmts_Pedestal, &b_fPmts_Pedestal);
   fChain->SetBranchAddress("fPmts.PedestalNoise", fPmts_PedestalNoise, &b_fPmts_PedestalNoise);
   fChain->SetBranchAddress("fPmts.PulseTraceColl[15][30]", fPmts_PulseTraceColl, &b_fPmts_PulseTraceColl);
   fChain->SetBranchAddress("fPmts.Npulses", fPmts_Npulses, &b_fPmts_Npulses);
   fChain->SetBranchAddress("fPmts.t[15]", fPmts_t, &b_fPmts_t);
   fChain->SetBranchAddress("fPmts.q[15]", fPmts_q, &b_fPmts_q);
   fChain->SetBranchAddress("fPmts.amp[15]", fPmts_amp, &b_fPmts_amp);
   fChain->SetBranchAddress("fPmts.fwhm[15]", fPmts_fwhm, &b_fPmts_fwhm);
   fChain->SetBranchAddress("fPmts.risetime[15]", fPmts_risetime, &b_fPmts_risetime);
   fChain->SetBranchAddress("fPmts.pulsestatus[15]", fPmts_pulsestatus, &b_fPmts_pulsestatus);
   fChain->SetBranchAddress("fPmts.Rate", fPmts_Rate, &b_fPmts_Rate);
   fChain->SetBranchAddress("fPmts.Pmtf", fPmts_Pmtf, &b_fPmts_Pmtf);
   fChain->SetBranchAddress("fPmts.SignalType", fPmts_SignalType, &b_fPmts_SignalType);
   fChain->SetBranchAddress("_nsampreduced", &_nsampreduced, &b_Trig__nsampreduced);
   fChain->SetBranchAddress("hNPMTs_vs_time[40000]", hNPMTs_vs_time, &b_Trig_hNPMTs_vs_time);
   fChain->SetBranchAddress("hcharge_vs_time[40000]", hcharge_vs_time, &b_Trig_hcharge_vs_time);
   fChain->SetBranchAddress("SequenceID", &SequenceID, &b_Trig_SequenceID);
   fChain->SetBranchAddress("StartTimeSec[16]", StartTimeSec, &b_Trig_StartTimeSec);
   fChain->SetBranchAddress("StartTimeNSec[16]", StartTimeNSec, &b_Trig_StartTimeNSec);
   fChain->SetBranchAddress("StartCount[16]", StartCount, &b_Trig_StartCount);
   fChain->SetBranchAddress("TriggerCounts[16]", TriggerCounts, &b_Trig_TriggerCounts);
   fChain->SetBranchAddress("LastSync[16]", LastSync, &b_Trig_LastSync);
   fChain->SetBranchAddress("TriggerTimeSec[16]", TriggerTimeSec, &b_Trig_TriggerTimeSec);
   fChain->SetBranchAddress("TriggerTimeNSec[16]", TriggerTimeNSec, &b_Trig_TriggerTimeNSec);
   fChain->SetBranchAddress("RunNumber", &RunNumber, &b_Trig_RunNumber);
   fChain->SetBranchAddress("SubrunNumber", &SubrunNumber, &b_Trig_SubrunNumber);
   fChain->SetBranchAddress("FileNumber", &FileNumber, &b_Trig_FileNumber);
   fChain->SetBranchAddress("debug", &debug, &b_Trig_debug);
   fChain->SetBranchAddress("fnPmt", &fnPmt, &b_Trig_fnPmt);
   fChain->SetBranchAddress("fnMrd", &fnMrd, &b_Trig_fnMrd);
   fChain->SetBranchAddress("fnRwm", &fnRwm, &b_Trig_fnRwm);
   Notify();
}

Bool_t PulseTree::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return kTRUE;
}

void PulseTree::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t PulseTree::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
#endif // #ifdef PulseTree_cxx
