//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Thu Sep 15 10:08:30 2016 by ROOT version 5.34/34
// from TTree RunInformation/RunInformation
// found on file: DataR248S1p0.root
//////////////////////////////////////////////////////////

#ifndef RunInformation_h
#define RunInformation_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.
#include <string>
#include <iostream>
// Fixed size dimensions of array or collections stored in the TTree if any.

class RunInformation {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

   // Declaration of leaf types
   std::string          *InfoTitle;
   std::string          *InfoMessage;

   // List of branches
   TBranch        *b_InfoTitle;   //!
   TBranch        *b_InfoMessage;   //!

   RunInformation(TTree *tree=0);
   virtual ~RunInformation();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);
};

#endif

#ifdef RunInformation_cxx
RunInformation::RunInformation(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
  if (tree == 0) {
    fChain=new TTree("RunInformation","RunInformation");
    //    std::cout<<" createing strings"<<std::endl;
    InfoTitle=new std::string;
    InfoMessage=new std::string;
    // std::cout<<" assigning strings "<<InfoTitle<<std::endl;
    *InfoTitle="hello";
    *InfoMessage="hello";
    // std::cout<<" done strings"<<std::endl;

    fChain->Branch("InfoTitle", InfoTitle);
    fChain->Branch("InfoMessage", InfoMessage);
    //         TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("DataR248S1p0.root");
    // if (!f || !f->IsOpen()) {
    //     f = new TFile("DataR248S1p0.root");
    //  }
    //  f->GetObject("RunInformation",tree);

   }
  
  else   Init(tree);
}

RunInformation::~RunInformation()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t RunInformation::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t RunInformation::LoadTree(Long64_t entry)
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

void RunInformation::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   // Set object pointer
  InfoTitle = 0;
  InfoMessage = 0;
  // Set branch addresses and branch pointers
  if (!tree) return;
  fChain = tree;
  fCurrent = -1;
  fChain->SetMakeClass(1);
  
  fChain->SetBranchAddress("InfoTitle", &InfoTitle, &b_InfoTitle);
  fChain->SetBranchAddress("InfoMessage", &InfoMessage, &b_InfoMessage);
  Notify();
}

Bool_t RunInformation::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return kTRUE;
}

void RunInformation::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t RunInformation::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
#endif // #ifdef RunInformation_cxx
