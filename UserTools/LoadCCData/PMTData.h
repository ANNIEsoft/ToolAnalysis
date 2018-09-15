/////////////////////////////////////////////////////////
// This class has been automatically generated on
// Thu Sep 15 10:08:12 2016 by ROOT version 5.34/34
// from TTree PMTData/PMTData
// found on file: DataR248S1p0.root
//////////////////////////////////////////////////////////

#ifndef PMTData_h
#define PMTData_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

#include <string>

// Header file for the classes stored in the TTree if any.

// Fixed size dimensions of array or collections stored in the TTree if any.
/*
class SplitTree{

 public:
  
  SplitTree(std::string name){

    tree=new TTree(name.c_str(),name.c_str());
    TBranch* br;
    br=tree->Branch("LastSync",&LastSync,"LastSync/l");                                                      
    br->SetCompressionLevel(4);   
    br=tree->Branch("SequenceID", &SequenceID, "SequenceID/I");                                                  
    br->SetCompressionLevel(4);  
    br=tree->Branch("StartTimeSec", &StartTimeSec, "StartTimeSec/I");                                            
    br->SetCompressionLevel(4);   
    br=tree->Branch("StartTimeNSec", &StartTimeNSec, "StartTimeNSec/I");
    br->SetCompressionLevel(4);  
    br=tree->Branch("StartCount", &StartCount, "StartCount/l");                                                  
    br->SetCompressionLevel(4);   
    br=tree->Branch("TriggerNumber", &TriggerNumber, "TriggerNumber/I");                                         
    br->SetCompressionLevel(4);
    br=tree->Branch("TriggerCount", &TriggerCount, "TriggerCount/l");                                          
    br->SetCompressionLevel(4);
    br=tree->Branch("CardID", &CardID, "CardID/I");                                                              
    br->SetCompressionLevel(4);
    br=tree->Branch("Channel", &Channel, "Channel/I");                                                        
    br->SetCompressionLevel(4);
    br=tree->Branch("Rate", &Rate, "Rate/i");                                                                  
    br->SetCompressionLevel(4);
    br=tree->Branch("BufferSize", &BufferSize, "BufferSize/I");                                                  
    br->SetCompressionLevel(4);
    br=tree->Branch("Trigger", &Trigger, "Trigger/I");
    br->SetCompressionLevel(4);
    //    br=tree->Branch("PMTf", &PMTf, "PMTf/I");
    //br->SetCompressionLevel(4);
    //br=tree->Branch("PMTx", &PMTx, "PMTx/I");
    //br->SetCompressionLevel(4);
    //br=tree->Branch("PMTy", &PMTy, "PMTy/I");
    //br->SetCompressionLevel(4);
    //br=tree->Branch("PMTz", &PMTz, "PMTz/I");
    //br->SetCompressionLevel(4);
    //    br=tree->Branch("Data", Data, "Data[BufferSize]/F");          
    //br->SetCompressionLevel(4);
    br=tree->Branch("UnCalData", UnCalData, "UnCalData[BufferSize]/s");
    br->SetCompressionLevel(4);
  }
  

  void Fill(){tree->Fill();}  
  void GetEntry(long entry){tree->GetEntry(entry);}
  TTree *tree;

  ULong64_t LastSync;
  Int_t SequenceID;
  Int_t StartTimeSec;
  Int_t StartTimeNSec;
  ULong64_t StartCount;
  Int_t TriggerNumber;
  ULong64_t TriggerCount; //[TriggerNumber]
  UInt_t Rate; // [TriggerNumber]
  Int_t CardID;
  Int_t Channel;
  Int_t PMTID;
  Int_t BufferSize;
  Int_t Trigger;
  Int_t PMTf;
  Int_t PMTx;
  Int_t PMTy;
  Int_t PMTz;
  Float_t Data[160000];
  UShort_t UnCalData[160000];
};

*/

class PMTData {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

   // Declaration of leaf types
   ULong64_t       LastSync;
   Int_t           SequenceID;
   Int_t           StartTimeSec;
   Int_t           StartTimeNSec;
   ULong64_t       StartCount;
   Int_t           TriggerNumber;
   ULong64_t       TriggerCounts[1000];   //[TriggerNumber]
   Int_t           CardID;
   Int_t           Channels;
   UInt_t          Rates[4];   //[Channels]
   Int_t           BufferSize;
   Int_t           Eventsize;
   Int_t           FullBufferSize;
   UShort_t        Data[160000];   //[FullBufferSize]

   // List of branches
   TBranch        *b_LastSync;   //!
   TBranch        *b_SequenceID;   //!
   TBranch        *b_StartTimeSec;   //!
   TBranch        *b_StartTimeNSec;   //!
   TBranch        *b_StartCount;   //!
   TBranch        *b_TriggerNumber;   //!
   TBranch        *b_TriggerCounts;   //!
   TBranch        *b_CardID;   //!
   TBranch        *b_Channels;   //!
   TBranch        *b_Rates;   //!
   TBranch        *b_BufferSize;   //!
   TBranch        *b_Eventsize;   //!
   TBranch        *b_FullBufferSize;   //!
   TBranch        *b_Data;   //!

   PMTData(TTree *tree=0);
   virtual ~PMTData();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);
};

#endif

#ifdef PMTData_cxx
PMTData::PMTData(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("DataR248S1p0.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("DataR248S1p0.root");
      }
      f->GetObject("PMTData",tree);

   }
   Init(tree);
}

PMTData::~PMTData()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t PMTData::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t PMTData::LoadTree(Long64_t entry)
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

void PMTData::Init(TTree *tree)
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

   fChain->SetBranchAddress("LastSync", &LastSync, &b_LastSync);
   fChain->SetBranchAddress("SequenceID", &SequenceID, &b_SequenceID);
   fChain->SetBranchAddress("StartTimeSec", &StartTimeSec, &b_StartTimeSec);
   fChain->SetBranchAddress("StartTimeNSec", &StartTimeNSec, &b_StartTimeNSec);
   fChain->SetBranchAddress("StartCount", &StartCount, &b_StartCount);
   fChain->SetBranchAddress("TriggerNumber", &TriggerNumber, &b_TriggerNumber);
   fChain->SetBranchAddress("TriggerCounts", TriggerCounts, &b_TriggerCounts);
   fChain->SetBranchAddress("CardID", &CardID, &b_CardID);
   fChain->SetBranchAddress("Channels", &Channels, &b_Channels);
   fChain->SetBranchAddress("Rates", Rates, &b_Rates);
   fChain->SetBranchAddress("BufferSize", &BufferSize, &b_BufferSize);
   fChain->SetBranchAddress("Eventsize", &Eventsize, &b_Eventsize);
   fChain->SetBranchAddress("FullBufferSize", &FullBufferSize, &b_FullBufferSize);
   fChain->SetBranchAddress("Data", Data, &b_Data);
   Notify();
}

Bool_t PMTData::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return kTRUE;
}

void PMTData::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t PMTData::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
#endif // #ifdef PMTData_cxx
