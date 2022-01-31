//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Thu Jan 20 16:48:31 2022 by ROOT version 6.22/06
// from TTree ffmytree/ffmytree
// found on file: Analysis.root
//////////////////////////////////////////////////////////

#ifndef AnaTree_h
#define AnaTree_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.

class AnaTree {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

// Fixed size dimensions of array or collections stored in the TTree if any.

   // Declaration of leaf types
   Int_t           T0Bin;
   Int_t           WraparoundBin;
   Int_t           QualityVar;
   Double_t        TrigDeltaT;
   Double_t        PulseHeight;
   Int_t           NHits;
   Double_t        Q[1];   //[NHits]
   Double_t        xpar[1];   //[NHits]
   Double_t        xperp[1];   //[NHits]
   Double_t        time[1];   //[NHits]
   Double_t        deltime[1];   //[NHits]
   Double_t        Vpeak[1];   //[NHits]
   Int_t           NHits_simp;
   Double_t        Q_simp[1];   //[NHits_simp]
   Double_t        xpar_simp[1];   //[NHits_simp]
   Double_t        xperp_simp[1];   //[NHits_simp]
   Double_t        time_simp[1];   //[NHits_simp]
   Int_t           Npulses_simp;
   Double_t        pulsestart_simp[17];   //[Npulses_simp]
   Double_t        pulseend_simp[17];   //[Npulses_simp]
   Double_t        pulsepeakbin_simp[17];   //[Npulses_simp]
   Double_t        pulseamp_simp[17];   //[Npulses_simp]
   Double_t        pulseQ_simp[17];   //[Npulses_simp]
   Int_t           pulsestrip_simp[17];   //[Npulses_simp]
   Int_t           pulseside_simp[17];   //[Npulses_simp]
   Int_t           Npulses_cfd;
   Double_t        pulsestart_cfd[17];   //[Npulses_cfd]
   Int_t           pulsestrip_cfd[17];   //[Npulses_cfd]
   Int_t           pulseside_cfd[17];   //[Npulses_cfd]
   Double_t        pulseamp_cfd[17];   //[Npulses_simp]
   Double_t        pulseQ_cfd[17];   //[Npulses_simp]

   // List of branches
   TBranch        *b_T0Bin;   //!
   TBranch        *b_WraparoundBin;   //!
   TBranch        *b_QualityVar;   //!
   TBranch        *b_TrigDeltaT;   //!
   TBranch        *b_PulseHeight;   //!
   TBranch        *b_NHits;   //!
   TBranch        *b_Q;   //!
   TBranch        *b_xpar;   //!
   TBranch        *b_xperp;   //!
   TBranch        *b_time;   //!
   TBranch        *b_deltime;   //!
   TBranch        *b_Vpeak;   //!
   TBranch        *b_NHits_simp;   //!
   TBranch        *b_Q_simp;   //!
   TBranch        *b_xpar_simp;   //!
   TBranch        *b_xperp_simp;   //!
   TBranch        *b_time_simp;   //!
   TBranch        *b_Npulses_simp;   //!
   TBranch        *b_pulsestart_simp;   //!
   TBranch        *b_pulseend_simp;   //!
   TBranch        *b_pulsepeakbin_simp;   //!
   TBranch        *b_pulseamp_simp;   //!
   TBranch        *b_pulseQ_simp;   //!
   TBranch        *b_pulsestrip_simp;   //!
   TBranch        *b_pulseside_simp;   //!
   TBranch        *b_Npulses_cfd;   //!
   TBranch        *b_pulsestart_cfd;   //!
   TBranch        *b_pulsestrip_cfd;   //!
   TBranch        *b_pulseside_cfd;   //!
   TBranch        *b_pulseamp_cfd;   //!
   TBranch        *b_pulseQ_cfd;   //!

   AnaTree(TTree *tree=0);
   virtual ~AnaTree();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);
};

#endif

#ifdef AnaTree_cxx
AnaTree::AnaTree(TTree *tree) : fChain(0)
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("Analysis.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("Analysis.root");
      }
      f->GetObject("ffmytree",tree);

   }
   Init(tree);
}

AnaTree::~AnaTree()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t AnaTree::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t AnaTree::LoadTree(Long64_t entry)
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

void AnaTree::Init(TTree *tree)
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

   fChain->SetBranchAddress("T0Bin", &T0Bin, &b_T0Bin);
   fChain->SetBranchAddress("WraparoundBin", &WraparoundBin, &b_WraparoundBin);
   fChain->SetBranchAddress("QualityVar", &QualityVar, &b_QualityVar);
   fChain->SetBranchAddress("TrigDeltaT", &TrigDeltaT, &b_TrigDeltaT);
   fChain->SetBranchAddress("PulseHeight", &PulseHeight, &b_PulseHeight);
   fChain->SetBranchAddress("NHits", &NHits, &b_NHits);
   fChain->SetBranchAddress("Q", Q, &b_Q);
   fChain->SetBranchAddress("xpar", xpar, &b_xpar);
   fChain->SetBranchAddress("xperp", xperp, &b_xperp);
   fChain->SetBranchAddress("time", time, &b_time);
   fChain->SetBranchAddress("deltime", deltime, &b_deltime);
   fChain->SetBranchAddress("Vpeak", Vpeak, &b_Vpeak);
   fChain->SetBranchAddress("NHits_simp", &NHits_simp, &b_NHits_simp);
   fChain->SetBranchAddress("Q_simp", Q_simp, &b_Q_simp);
   fChain->SetBranchAddress("xpar_simp", xpar_simp, &b_xpar_simp);
   fChain->SetBranchAddress("xperp_simp", xperp_simp, &b_xperp_simp);
   fChain->SetBranchAddress("time_simp", time_simp, &b_time_simp);
   fChain->SetBranchAddress("Npulses_simp", &Npulses_simp, &b_Npulses_simp);
   fChain->SetBranchAddress("pulsestart_simp", pulsestart_simp, &b_pulsestart_simp);
   fChain->SetBranchAddress("pulseend_simp", pulseend_simp, &b_pulseend_simp);
   fChain->SetBranchAddress("pulsepeakbin_simp", pulsepeakbin_simp, &b_pulsepeakbin_simp);
   fChain->SetBranchAddress("pulseamp_simp", pulseamp_simp, &b_pulseamp_simp);
   fChain->SetBranchAddress("pulseQ_simp", pulseQ_simp, &b_pulseQ_simp);
   fChain->SetBranchAddress("pulsestrip_simp", pulsestrip_simp, &b_pulsestrip_simp);
   fChain->SetBranchAddress("pulseside_simp", pulseside_simp, &b_pulseside_simp);
   fChain->SetBranchAddress("Npulses_cfd", &Npulses_cfd, &b_Npulses_cfd);
   fChain->SetBranchAddress("pulsestart_cfd", pulsestart_cfd, &b_pulsestart_cfd);
   fChain->SetBranchAddress("pulsestrip_cfd", pulsestrip_cfd, &b_pulsestrip_cfd);
   fChain->SetBranchAddress("pulseside_cfd", pulseside_cfd, &b_pulseside_cfd);
   fChain->SetBranchAddress("pulseamp_cfd", pulseamp_cfd, &b_pulseamp_cfd);
   fChain->SetBranchAddress("pulseQ_cfd", pulseQ_cfd, &b_pulseQ_cfd);
   Notify();
}

Bool_t AnaTree::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return kTRUE;
}

void AnaTree::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t AnaTree::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
#endif // #ifdef AnaTree_cxx
