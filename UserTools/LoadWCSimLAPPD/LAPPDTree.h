/* vim:set noexpandtab tabstop=4 wrap */

//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Thu Mar 22 21:17:14 2018 by ROOT version 6.06/08
// from TTree LAPPDTree/LAPPDTree
// found on file: /pnfs/annie/persistent/users/moflaher/wcsim_lappd_24-09-17_BNB_Water_10k_22-05-17/wcsim_lappd_0.0.0.root
//////////////////////////////////////////////////////////

#ifndef LAPPDTree_h
#define LAPPDTree_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>
// for adding files and directories
#include <sys/types.h> // for stat() test to see if file or folder
#include <sys/stat.h>
#include <unistd.h>
#include "TSystemFile.h"
#include "TSystemDirectory.h"
#include <TString.h>

#include <iostream>
// Header file for the classes stored in the TTree if any.
#include "vector"

#include <TStyle.h>
#include <MRDspecs.hh>

using namespace std;

#define SINGLE_TREE        // TODO TODO TODO TODO shouldn't need this define

class LAPPDTree {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain
   int             verbose=1;

// Fixed size dimensions of array or collections stored in the TTree if any.

   // Declaration of leaf types
   Int_t           lappdevt;                          // event number
   Int_t           lappd_numhits;                     // number of LAPPDs hit this evt
   Int_t           lappdhit_objnum[180];              //[lappd_numhits], hit LAPPD number
   Double_t        lappdhit_x[180];                   //[lappd_numhits], hit LAPPD x position, [mm]
   Double_t        lappdhit_y[180];                   //[lappd_numhits]
   Double_t        lappdhit_z[180];                   //[lappd_numhits]
   vector<double>  *lappdhit_stripcoorx;              // x pos within tile [mm]
   vector<double>  *lappdhit_stripcoory;              // y pos within tile
   vector<double>  *lappdhit_stripcoort;              // photon true time (EVENT absolute) [ns]
   Double_t        lappdhit_edep[180];                //[lappd_numhits], total # photon hits on each lappd in the event (raw: no triggering or digitization applied)
   vector<double>  *lappdhit_globalcoorx;             // photon hit position within global coords
   vector<double>  *lappdhit_globalcoory;             // only present in file vers > 3
   vector<double>  *lappdhit_globalcoorz;
   vector<double>  lappdhit_globalcoorxdummy, lappdhit_globalcoorydummy, lappdhit_globalcoorzdummy;
   vector<int>     *lappdhit_primaryParentID2;      // G4 internal trackID of photon parent
   
   /* DISABLED BRANCHES */
   //Int_t           lappdhit_totalpes_perevt;        // total photons on all LAPPDs in this event
   //Int_t           lappdhit_process[180];           //[lappd_numhits] not implemented at all
   //Int_t           lappdhit_particleID[180];        //[lappd_numhits] not implemented at all
   //Int_t           lappdhit_trackID[180];           //[lappd_numhits] G4 internal trackid of first photon hit on this lappd this event
   //vector<double>  *lappdhit_stripcoorz;            // z pos within tile? meaningless
   //vector<float>   *lappdhit_truetime2;             // same as lappdhit_stripcoort
   //vector<int>     *lappdhit_totalpes_perlappd2;    // same as lappdhit_edep
   //vector<int>     *lappdhit_stripnum;              // strip (anode) num of photon hit
   //vector<float>   *lappdhit_smeartime2;            // smeared hit time: incorrect timing res though
   //vector<int>     *lappdhit_NoOfneighstripsHit;    // size of below vectors
   //vector<int>     *lappdhit_neighstripnum;         // strip num of neighbour
   //vector<double>  *lappdhit_neighstrippeak;        // pulse amplitude on that strip
   //vector<double>  *lappdhit_neighstrip_time;       // smeared hit time on that hit (time 0 of below)
   //vector<double>  *lappdhit_neighstrip_lefttime;   // rel. pulse ETA @ LHS
   //vector<double>  *lappdhit_neighstrip_righttime;  // rel. pulse ETA @ RHS
   // note the vectors re neighbouring hits are concatenated; so you need to keep a running 
   // index within the vector to know the correct elements that correspond to a given LAPPD photon

   // List of branches
   TBranch        *b_lappdevt;                        //!
   TBranch        *b_lappd_numhits;                   //!
   TBranch        *b_lappdhit_totalpes_perevt;        //!
   TBranch        *b_lappdhit_totalpes_perlappd2;     //!
   TBranch        *b_lappdhit_x;                      //!
   TBranch        *b_lappdhit_y;                      //!
   TBranch        *b_lappdhit_z;                      //!
   TBranch        *b_lappdhit_stripcoorx;             //!
   TBranch        *b_lappdhit_stripcoory;             //!
   TBranch        *b_lappdhit_stripcoorz;             //!
   TBranch        *b_lappdhit_stripcoort;             //!
   TBranch        *b_lappdhit_globalcoorx;            //!
   TBranch        *b_lappdhit_globalcoory;            //!
   TBranch        *b_lappdhit_globalcoorz;            //!
   TBranch        *b_lappdhit_process;                //!
   TBranch        *b_lappdhit_particleID;             //!
   TBranch        *b_lappdhit_trackID;                //!
   TBranch        *b_lappdhit_edep;                   //!
   TBranch        *b_lappdhit_objnum;                 //!
   TBranch        *b_lappdhit_stripnum;               //!
   TBranch        *b_lappdhit_truetime2;              //!
   TBranch        *b_lappdhit_smeartime2;             //!
   TBranch        *b_lappdhit_primaryParentID2;       //!
   TBranch        *b_lappdhit_NoOfneighstripsHit;     //!
   TBranch        *b_lappdhit_neighstripnum;          //!
   TBranch        *b_lappdhit_neighstrippeak;         //!
   TBranch        *b_lappdhit_neighstrip_time;        //!
   TBranch        *b_lappdhit_neighstrip_lefttime;    //!
   TBranch        *b_lappdhit_neighstrip_righttime;   //!

   LAPPDTree(TTree *tree=0);
   LAPPDTree(const char* filepath, bool addsubdirs=false);
   virtual ~LAPPDTree();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);
   TChain* AddFiles(const char* inputdir, bool addsubfolders);
};

#endif

#ifdef LAPPDTree_cxx
LAPPDTree::LAPPDTree(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
#ifdef SINGLE_TREE
   // The following code should be used if you want this class to access
   // a single tree instead of a chain
   // TODO implement a verbosity arg
   if (tree == 0){
      cerr<<"LAPPDTree Treee mode constructor with no LAPPDTree!"<<endl; return;
   }
#else // not SINGLE_TREE
   // The following code should be used if you want this class to access a chain of trees.
   if(tree == 0) {
      cerr<<"LAPPDTree Chain mode constructor with no TChain!"<<endl; return;
   }
#endif // SINGLE_TREE
   Init(tree);
   if(verbose){
      TFile* f = tree->GetCurrentFile();
      cout<<"LAPPDTree is from "<<f->GetName()<<endl;
   }
}

LAPPDTree::LAPPDTree(const char* filepath, bool addsubdirs)
{
   fChain = AddFiles(filepath, addsubdirs);
   if(!fChain) return;
   Init(fChain);
}

LAPPDTree::~LAPPDTree()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t LAPPDTree::GetEntry(Long64_t entry)
{
   if(verbose>2) cout<<"getting LAPPDTree entry "<<entry<<endl;
   Long64_t ientry = LoadTree(entry);
   if(verbose>3) cout<<"corresponding localentry is "<<ientry<<endl;
   if (ientry < 0) return -4;
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);  // FIXME ientry?
}

Long64_t LAPPDTree::LoadTree(Long64_t entry)
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

void LAPPDTree::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).
   
   // Set object pointer
   lappdhit_stripcoorx = 0;
   lappdhit_stripcoory = 0;
   lappdhit_stripcoort = 0;
   lappdhit_globalcoorx = 0;
   lappdhit_globalcoory = 0;
   lappdhit_globalcoorz = 0;
   lappdhit_primaryParentID2 = 0;
   //lappdhit_stripcoorz = 0;
   //lappdhit_totalpes_perlappd2 = 0;
   //lappdhit_stripnum = 0;
   //lappdhit_truetime2 = 0;
   //lappdhit_smeartime2 = 0;
   //lappdhit_NoOfneighstripsHit = 0;
   //lappdhit_neighstripnum = 0;
   //lappdhit_neighstrippeak = 0;
   //lappdhit_neighstrip_time = 0;
   //lappdhit_neighstrip_lefttime = 0;
   //lappdhit_neighstrip_righttime = 0;
   
   // Set branch addresses and branch pointers
   if (!tree){ cerr<<"Init() called with no tree!"<<endl; return; }
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1); // Do we need to disable this like for WCSimT? no classes, so prob not

   fChain->SetBranchAddress("lappdevt", &lappdevt, &b_lappdevt);
   fChain->SetBranchAddress("lappd_numhits", &lappd_numhits, &b_lappd_numhits);
   fChain->SetBranchAddress("lappdhit_x", lappdhit_x, &b_lappdhit_x);
   fChain->SetBranchAddress("lappdhit_y", lappdhit_y, &b_lappdhit_y);
   fChain->SetBranchAddress("lappdhit_z", lappdhit_z, &b_lappdhit_z);
   fChain->SetBranchAddress("lappdhit_stripcoorx", &lappdhit_stripcoorx, &b_lappdhit_stripcoorx);
   fChain->SetBranchAddress("lappdhit_stripcoory", &lappdhit_stripcoory, &b_lappdhit_stripcoory);
   fChain->SetBranchAddress("lappdhit_stripcoort", &lappdhit_stripcoort, &b_lappdhit_stripcoort);
   TBranch* br = (TBranch*)fChain->GetListOfBranches()->FindObject("lappdhit_globalcoorx"); // disable by test
   if(br){
     fChain->SetBranchAddress("lappdhit_globalcoorx", &lappdhit_globalcoorx, &b_lappdhit_globalcoorx);
     fChain->SetBranchAddress("lappdhit_globalcoory", &lappdhit_globalcoory, &b_lappdhit_globalcoory);
     fChain->SetBranchAddress("lappdhit_globalcoorz", &lappdhit_globalcoorz, &b_lappdhit_globalcoorz);
   } else {
     fChain->SetBranchStatus("lappdhit_globalcoorx", 0);
     fChain->SetBranchStatus("lappdhit_globalcoory", 0);
     fChain->SetBranchStatus("lappdhit_globalcoorz", 0);
     lappdhit_globalcoorx = &lappdhit_globalcoorxdummy;
     lappdhit_globalcoorx = &lappdhit_globalcoorydummy;
     lappdhit_globalcoorx = &lappdhit_globalcoorzdummy;
   }
   fChain->SetBranchAddress("lappdhit_edep", lappdhit_edep, &b_lappdhit_edep);
   fChain->SetBranchAddress("lappdhit_objnum", lappdhit_objnum, &b_lappdhit_objnum);
   fChain->SetBranchAddress("lappdhit_primaryParentID2", &lappdhit_primaryParentID2, &b_lappdhit_primaryParentID2);
   
   //fChain->SetBranchAddress("lappdhit_stripcoorz", &lappdhit_stripcoorz, &b_lappdhit_stripcoorz);
   //fChain->SetBranchAddress("lappdhit_process", lappdhit_process, &b_lappdhit_process);
   //fChain->SetBranchAddress("lappdhit_particleID", lappdhit_particleID, &b_lappdhit_particleID);
   //fChain->SetBranchAddress("lappdhit_trackID", lappdhit_trackID, &b_lappdhit_trackID);
   //fChain->SetBranchAddress("lappdhit_totalpes_perevt", &lappdhit_totalpes_perevt, &b_lappdhit_totalpes_perevt);
   //fChain->SetBranchAddress("lappdhit_totalpes_perlappd2", &lappdhit_totalpes_perlappd2, &b_lappdhit_totalpes_perlappd2);
   //fChain->SetBranchAddress("lappdhit_stripnum", &lappdhit_stripnum, &b_lappdhit_stripnum);
   //fChain->SetBranchAddress("lappdhit_truetime2", &lappdhit_truetime2, &b_lappdhit_truetime2);
   //fChain->SetBranchAddress("lappdhit_smeartime2", &lappdhit_smeartime2, &b_lappdhit_smeartime2);
   //fChain->SetBranchAddress("lappdhit_NoOfneighstripsHit", &lappdhit_NoOfneighstripsHit, &b_lappdhit_NoOfneighstripsHit);
   //fChain->SetBranchAddress("lappdhit_neighstripnum", &lappdhit_neighstripnum, &b_lappdhit_neighstripnum);
   //fChain->SetBranchAddress("lappdhit_neighstrippeak", &lappdhit_neighstrippeak, &b_lappdhit_neighstrippeak);
   //fChain->SetBranchAddress("lappdhit_neighstrip_time", &lappdhit_neighstrip_time, &b_lappdhit_neighstrip_time);
   //fChain->SetBranchAddress("lappdhit_neighstrip_lefttime", &lappdhit_neighstrip_lefttime, &b_lappdhit_neighstrip_lefttime);
   //fChain->SetBranchAddress("lappdhit_neighstrip_righttime", &lappdhit_neighstrip_righttime, &b_lappdhit_neighstrip_righttime);
   
   fChain->SetBranchStatus("lappdhit_stripcoorz",0);
   fChain->SetBranchStatus("lappdhit_totalpes_perevt",0);
   fChain->SetBranchStatus("lappdhit_process",0);
   fChain->SetBranchStatus("lappdhit_particleID",0);
   fChain->SetBranchStatus("lappdhit_trackID",0);
   fChain->SetBranchStatus("lappdhit_stripnum",0);
   fChain->SetBranchStatus("lappdhit_truetime2",0);
   fChain->SetBranchStatus("lappdhit_smeartime2",0);
//   fChain->SetBranchStatus("lappdhit_primaryParentID2",0);
   fChain->SetBranchStatus("lappdhit_NoOfneighstripsHit",0);
   fChain->SetBranchStatus("lappdhit_neighstripnum",0);
   fChain->SetBranchStatus("lappdhit_neighstrippeak",0);
   fChain->SetBranchStatus("lappdhit_neighstrip_time",0);
   fChain->SetBranchStatus("lappdhit_neighstrip_lefttime",0);
   fChain->SetBranchStatus("lappdhit_neighstrip_righttime",0);
   fChain->SetBranchStatus("lappdhit_totalpes_perlappd2",0);
   
   Notify();
}

Bool_t LAPPDTree::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return kTRUE;
}

void LAPPDTree::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}

Int_t LAPPDTree::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}

TChain* LAPPDTree::AddFiles(const char* inputdir, bool addsubfolders){
	// Add files by pattern, directory. Optionally check subdirectories, but only 1 level deep. 
	if(strcmp(inputdir,"")==0){ cerr<<"no wcsim_lappd file to load"<<endl; return nullptr; }
	if(verbose){
		cout<<"getting trees from file "<<inputdir;
		addsubfolders ? cout<<", including first level subdirectories"<<endl : cout<<endl;
	}
	
	// first check if we've been passed a file, directory or a pattern
	bool isdir;
	struct stat s;
	if(stat(inputdir,&s)==0){
		if(s.st_mode & S_IFDIR){        // mask to extract if it's a directory
			isdir=true;                 //it's a directory
		} else if(s.st_mode & S_IFREG){ // mask to check if it's a file
			isdir=false;                //it's a file
		} else {
			cerr<<"Check input path: stat says LAPPD path is neither file nor directory..?"<<endl;
			return nullptr;
		}
	} else {
		// errors could be because this is a file pattern: e.g. wcsim_0.4*.root Treat as a file.
		isdir=false;
	}
	
	TChain * t = new TChain("LAPPDTree","");
	// if it's a directory
	if(isdir){
		const char* ext = ".root";
		TString nextfilepattern;
		TSystemDirectory dir(inputdir, inputdir);
		TList *subfolders = dir.GetListOfFiles();  // despite name returns files and folders
		
		// Add subfolders if requested
		if(addsubfolders&&subfolders&&(subfolders->GetEntries()>2)){ // always has '.' and '..'
			if(verbose>2) cout<<"looping over subfolders"<<endl;
			TSystemDirectory *subfolder;
			TIter nextsf(subfolders);
			while ((subfolder=(TSystemDirectory*)nextsf())) {
				if (subfolder->IsDirectory()){
					TString sfname = subfolder->GetName();
					if(sfname=="."||sfname==".."){ continue; }
					if(verbose>2) cout<<"found subfolder '"<<sfname<<"'"<<endl;
					nextfilepattern = TString::Format("%s/%s/%s",inputdir,sfname.Data(),"wcsim_lappd_*.root"); // XXX
					if(verbose>2) cout<<"adding "<<nextfilepattern<<endl;
					t->Add(nextfilepattern.Data());
				} // end if item in directory was a subdirectory
			}     // end loop over subdirectories
		}         // end if subfolders are to be added 
		
		// add files in parent directory
		if(1 /*t->GetEntriesFast()==0*/) {   // add files in parent directory
			nextfilepattern = std::string(inputdir) + std::string("/wcsim_lappd_*.root"); // XXX
			if(verbose>2) cout<<"adding "<<nextfilepattern<<endl;
			t->Add(nextfilepattern.Data());
		}
		if(verbose) cout<<"constructed LAPPDTree class with directory "<<inputdir<<endl;
		if(verbose>1) cout<<"Loaded "<<t->GetEntries()<<" LAPPD entries"<<endl;
		return t;
	
	// if it's a file or pattern
	} else {
		t->Add(inputdir);
		if(verbose) cout<<"constructed LAPPDTree class with file/pattern "<<inputdir<<endl;
		if(verbose>1) cout<<"Loaded "<<t->GetEntries()<<" LAPPD entries"<<endl;
		return t;
	}
}

#endif // #ifdef LAPPDTree_cxx
