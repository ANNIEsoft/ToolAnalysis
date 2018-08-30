/* vim:set noexpandtab tabstop=4 wrap */

//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Tue Jan  2 12:36:35 2018 by ROOT version 6.06/04
// from TTree wcsimT/WCSim Tree
// found on file: /pnfs/annie/persistent/users/moflaher/wcsim_lappd_24-09-17_BNB_Water_10k_22-05-17/wcsim_0.0.0.root
//////////////////////////////////////////////////////////

#ifndef wcsimT_h
#define wcsimT_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.
#include "TObject.h"
#include "WCSimRootEvent.hh"
#include "WCSimRootGeom.hh"
#include "WCSimRootOptions.hh"

using namespace std;

class wcsimT {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

// Fixed size dimensions of array or collections stored in the TTree if any.

   // Declaration of leaf types
   WCSimRootEvent     *wcsimrootevent=nullptr;
   UInt_t             fUniqueID;
   UInt_t             fBits;
   WCSimRootEvent     *wcsimrootevent_mrd=nullptr;
   UInt_t             mrd_fUniqueID;
   UInt_t             mrd_fBits;
   WCSimRootEvent     *wcsimrootevent_facc=nullptr;
   UInt_t             facc_fUniqueID;
   UInt_t             facc_fBits;
   WCSimRootGeom      *wcsimrootgeom=nullptr;
   WCSimRootOptions   *wcsimrootopts=nullptr;
   
   // TODO implement a verbosity arg
   int             verbose=1;

   // List of branches
   TBranch        *b_wcsimrootevent;                  //!
   TBranch        *b_wcsimrootevent_mrd;              //!
   TBranch        *b_wcsimrootevent_facc;             //!
   TBranch        *b_wcsimrootevent_fUniqueID;        //!
   TBranch        *b_wcsimrootevent_fBits;            //!
   TBranch        *b_wcsimrootevent_mrd_fUniqueID;    //!
   TBranch        *b_wcsimrootevent_mrd_fBits;        //!
   TBranch        *b_wcsimrootevent_facc_fUniqueID;   //!
   TBranch        *b_wcsimrootevent_facc_fBits;       //!
   TBranch        *b_wcsimrootgeom;                   //!
   TBranch        *b_wcsimrootopts;                   //!

   wcsimT(TTree *tree=0);
   virtual ~wcsimT();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree, TTree* geotree, TTree* optstree);
   virtual void     Loop();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);
};

#endif

#ifdef wcsimT_cxx
wcsimT::wcsimT(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   TFile* f=nullptr; 
   TTree* geotree=nullptr;
   TTree* optstree=nullptr;
   if(tree == 0) {
/*
      TChain * chain = new TChain("wcsimT","");
      std::string pattern="/pnfs/annie/persistent/users/moflaher/wcsim_lappd_24-09-17_BNB_Water_10k_22-05-17/wcsim_0.*.root";  // TODO TODO TODO make this an argument
      if(verbose) cout<<"creating chain from files "<<pattern<<endl;
      chain->Add(pattern.c_str());
      tree = chain;
*/
      cerr<<"wcsimT constructed in Chain mode with null TChain!"<<endl;
      return;
   } else {
     LoadTree(0); // need to load the tree from first file
     f = tree->GetCurrentFile();
   }
   f->GetObject("wcsimGeoT",geotree);
   f->GetObject("wcsimRootOptionsT",optstree);
   if(verbose) cout<<"constructed wcsimT class from TChain within files "<<f->GetName()<<endl;
   
   Init(tree, geotree, optstree);
}

wcsimT::~wcsimT()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t wcsimT::GetEntry(Long64_t entry)
{
   if(verbose>2) cout<<"getting wcsimT entry "<<entry<<endl;
   Long64_t ientry = LoadTree(entry);
   if(verbose>3) cout<<"corresponding localentry is "<<ientry;
   if (ientry < 0) return -4;
// Read contents of entry.
   if(verbose>3) cout<<" from fChain "<<fChain;
   if (!fChain) return 0;
   if(verbose>3) cout<<" (which isn't 0)"<<endl;
   return fChain->GetEntry(entry);    // FIXME ientry or entry???
}

Long64_t wcsimT::LoadTree(Long64_t entry)
{
// Set the environment to read one entry
   if(verbose>3) cout<<"loading tree for entry "<<entry<<endl;
   if (!fChain) return -5;
   if(verbose>3) cout<<"we have a valid chain"<<endl;
   Long64_t centry = fChain->LoadTree(entry);
   if(verbose>3) cout<<"centry is "<<centry<<endl;
   if (centry < 0) return centry;
   if(verbose>3) cout<<"which is positive"<<endl;
   if (fChain->GetTreeNumber() != fCurrent) {
      if(verbose>3) cout<<"this loaded a new tree; new tree number "<<fChain->GetTreeNumber()
          <<", old tree number "<<fCurrent<<endl;
      fCurrent = fChain->GetTreeNumber();
      if(verbose>3) cout<<"fCurrent is now "<<fCurrent<<endl;
      Notify();
   }
   if(verbose>3) cout<<"returning "<<centry<<endl;
   return centry;
}

void wcsimT::Init(TTree *tree, TTree* geotree=0, TTree* optstree=0)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   if(verbose>2) cout<<"calling WCSimT::Init() with tree="
                   <<tree<<", geotree="<<geotree<<", optstree="<<optstree<<endl;
   // Set branch addresses and branch pointers
   if (!tree){ cerr<<"no tree!"<<endl; return; }
   fChain = tree;
   fCurrent = -1;
   //fChain->SetMakeClass(1); //makes it stop working!
   int branchok=0;
   branchok = fChain->SetBranchAddress("wcsimrootevent",&wcsimrootevent, &b_wcsimrootevent);
   if(branchok<0) cerr<<"Failed to set branch address for wcsimrootevent"<<endl;
   branchok = fChain->SetBranchAddress("wcsimrootevent_mrd",&wcsimrootevent_mrd, &b_wcsimrootevent_mrd);
   if(branchok<0) cerr<<"Failed to set branch address for wcsimrootevent_mrd"<<endl;
   branchok = fChain->SetBranchAddress("wcsimrootevent_facc",&wcsimrootevent_facc, &b_wcsimrootevent_facc);
   if(branchok<0) cerr<<"Failed to set branch address for wcsimrootevent_facc"<<endl;
   // XXX need to figure out how to uniquely identify fUniqueID and fBits branches to do this  XXX
//   branchok = fChain->SetBranchAddress("fUniqueID", &fUniqueID, &b_wcsimrootevent_fUniqueID);
//   if(branchok<0) cerr<<"Failed to set branch address for fUniqueID"<<endl;
//   branchok = fChain->SetBranchAddress("fBits", &fBits, &b_wcsimrootevent_fBits);
//   if(branchok<0) cerr<<"Failed to set branch address for wcsimrootevent_facc"<<endl;
//   branchok = fChain->SetBranchAddress("fUniqueID", &mrd_fUniqueID, &b_wcsimrootevent_mrd_fUniqueID);
//   if(branchok<0) cerr<<"Failed to set branch address for wcsimrootevent_facc"<<endl;
//   branchok = fChain->SetBranchAddress("fBits", &mrd_fBits, &b_wcsimrootevent_mrd_fBits);
//   if(branchok<0) cerr<<"Failed to set branch address for wcsimrootevent_facc"<<endl;
//   branchok = fChain->SetBranchAddress("fUniqueID", &facc_fUniqueID, &b_wcsimrootevent_facc_fUniqueID);
//   if(branchok<0) cerr<<"Failed to set branch address for wcsimrootevent_facc"<<endl;
//   branchok = fChain->SetBranchAddress("fBits", &facc_fBits, &b_wcsimrootevent_facc_fBits);
//   if(branchok<0) cerr<<"Failed to set branch address for wcsimrootevent_facc"<<endl;
   
   if (wcsimrootgeom==0&&geotree!=0){
      branchok = geotree->SetBranchAddress("wcsimrootgeom", &wcsimrootgeom, &b_wcsimrootgeom);
      if(branchok<0){ cerr<<"Failed to set branch address for wcsimrootgeom"<<endl; return; }
      if(geotree->GetEntries()==0){ cerr<<"no geotree entries!"<<endl; return; }
      b_wcsimrootgeom->GetEntry(0);
      if(verbose>2) cout<<"got geometry"<<endl;
   } else if(wcsimrootgeom==0){
      cerr<<"Init called with null geotree with no existing geometry!"<<endl; return;
   }
   
   if (wcsimrootopts==0&&optstree!=0){
      branchok = optstree->SetBranchAddress("wcsimrootoptions",&wcsimrootopts, &b_wcsimrootopts);
      if(branchok<0){ cerr<<"Failed to set branch address for wcsimrootopts"<<endl; return; }
      if(optstree->GetEntries()==0){ cerr<<"no optstree entries!"<<endl; return; }
      b_wcsimrootopts->GetEntry(0);
      if(verbose>2) cout<<"got wcsim options"<<endl;
   } else if(wcsimrootopts==0){
     cerr<<"Init called with null optstree with no existing wcsim options!"<<endl; return;
   }
   
   Notify();
}

Bool_t wcsimT::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return kTRUE;
}

void wcsimT::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}

Int_t wcsimT::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
#endif // #ifdef wcsimT_cxx
