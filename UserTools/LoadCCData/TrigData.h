//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Sun Feb  5 17:42:14 2017 by ROOT version 5.34/34
// from TTree TrigData/TrigData
// found on file: /data/output/DataR607S0p0.root
//////////////////////////////////////////////////////////

#ifndef TrigData_h
#define TrigData_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.

// Fixed size dimensions of array or collections stored in the TTree if any.

class TrigData {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

   // Declaration of leaf types
   Int_t           FirmwareVersion;
   Int_t           SequenceID;
   Int_t           EventSize;
   Int_t           TriggerSize;
   Int_t           FIFOOverflow;
   Int_t           DriverOverfow;
   UShort_t        EventIDs[700];   //[EventSize]
   ULong64_t       EventTimes[700];   //[EventSize]
   UInt_t          TriggerMasks[25000];   //[TriggerSize]
   UInt_t          TriggerCounters[25000];   //[TriggerSize]

   // List of branches
   TBranch        *b_FirmwareVersion;   //!
   TBranch        *b_SequenceID;   //!
   TBranch        *b_EventSize;   //!
   TBranch        *b_TriggerSize;   //!
   TBranch        *b_FIFOOverflow;   //!
   TBranch        *b_DriverOverflow;   //!
   TBranch        *b_EventIDs;   //!
   TBranch        *b_EventTimes;   //!
   TBranch        *b_TriggerMasks;   //!
   TBranch        *b_TriggerCounters;   //!

   TrigData(TTree *tree=0);
   virtual ~TrigData();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);
};

#endif

#ifdef TrigData_cxx
TrigData::TrigData(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("/data/output/DataR607S0p0.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("/data/output/DataR607S0p0.root");
      }
      f->GetObject("TrigData",tree);

   }
   Init(tree);
}

TrigData::~TrigData()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t TrigData::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t TrigData::LoadTree(Long64_t entry)
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

void TrigData::Init(TTree *tree)
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

   fChain->SetBranchAddress("FirmwareVersion", &FirmwareVersion, &b_FirmwareVersion);
   fChain->SetBranchAddress("SequenceID", &SequenceID, &b_SequenceID);
   fChain->SetBranchAddress("EventSize", &EventSize, &b_EventSize);
   fChain->SetBranchAddress("TriggerSize", &TriggerSize, &b_TriggerSize);
   fChain->SetBranchAddress("FIFOOverflow", &FIFOOverflow, &b_FIFOOverflow);
   fChain->SetBranchAddress("DriverOverfow", &DriverOverfow, &b_DriverOverflow);
   fChain->SetBranchAddress("EventIDs", EventIDs, &b_EventIDs);
   fChain->SetBranchAddress("EventTimes", EventTimes, &b_EventTimes);
   fChain->SetBranchAddress("TriggerMasks", TriggerMasks, &b_TriggerMasks);
   fChain->SetBranchAddress("TriggerCounters", TriggerCounters, &b_TriggerCounters);
   Notify();
}

Bool_t TrigData::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return kTRUE;
}

void TrigData::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t TrigData::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
#endif // #ifdef TrigData_cxx
