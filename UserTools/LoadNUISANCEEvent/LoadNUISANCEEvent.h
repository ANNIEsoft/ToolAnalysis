#ifndef LoadNUISANCEEvent_H
#define LoadNUISANCEEvent_H

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <utility>
#include <memory>
#include <cmath>
#include <set>
#include <vector>

#include "Tool.h"
#include "CLHEP/Random/RandGaussQ.h"
#include "CLHEP/Random/JamesRandom.h"
#include <TParticlePDG.h>
#include "TChain.h"
#include "TFile.h"
#include "TTree.h"
#include "TVector3.h"
#include "TLorentzVector.h"

#include "MRDspecs.hh"

class LoadNUISANCEEvent: public Tool {
	
	public:
	
	LoadNUISANCEEvent();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();
	
	// verbosity levels: if 'verbosity' < this level, the message type will be logged.
	int verbosity;
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	int get_ok;
	
	private:

	// function to load the branch addresses
	void SetBranchAddresses();

	BoostStore* nuisancestore = nullptr;
	std::string filedir, filepattern;
	bool loadwcsimsource;
	TChain* flux = nullptr;
	TFile* curf = nullptr;       // keep track of file changes
	TFile* curflast = nullptr;
	
	// nuisance file variables
	std::string currentfilestring;
	unsigned long local_entry=0;           // 
	int tchainentrynum=0;         // 
	bool manualmatch=0;			//to be used when GENIE information is not stored properly in file
	int fileevents=0;
	
	// store the neutrino info from gntp files
	// a load of variables to specify interaction type
	char IsCC='0';
	bool IsCCINC=false;
	bool IsNCINC=false;
	bool IsCCQE=false;
	bool IsCC0pi=false;
	bool IsCCQELike=false;
	bool IsNCEL=false;
	bool IsNC0pi=false;
	bool IsCCcoh=false;
	bool IsNCcoh=false;
	bool IsCC1pip=false;
	bool IsNC1pip=false;
	bool IsCC1pim=false;
	bool IsNC1pim=false;
	bool IsCC1pi0=false;
	bool IsNC1pi0=false;
	bool IsCC0piMINERvA=false;
	bool IsCC0Pi_T2K_AnaI=false;
	bool IsCC0Pi_T2K_AnaII=false;

	int neutcode=-1;
	int ninitp=-1;
	int nfsp=-1;
	// ok, moving on
	int pdg_init[200];
	float px_init[200];
	float py_init[200];
	float pz_init[200];
	float E_init[200];
	int pdg[200];
	float px[200];
	float py[200];
	float pz[200];
	float E[200];

	float fnuIntxVtx_X; // cm
	float fnuIntxVtx_Y; // cm
	float fnuIntxVtx_Z; // cm
	float feventq2=-1.;
	float feventq2qe=-1.;
	float feventw=-1.;
	float feventbj_x=-1.;
	float feventelastic_y=-1.;
	float feventq0=-1.;
	float feventq3=-1.;
	float feventEnu=-1.;
        float ffsleptonenergy;

	double nuIntxVtx_X; // cm
	double nuIntxVtx_Y; // cm
	double nuIntxVtx_Z; // cm
	double eventq2=-1.;
	double eventq2qe=-1.;
	double eventw=-1.;
	int TrueTargetZ = -1; 
	double eventbj_x=-1.;
	double eventelastic_y=-1.;
	double eventq0=-1.;
	double eventq3=-1.;
	std::vector<int>pdgs;
	std::vector<double>Emag;
	double eventEnu=-1.;
	Direction eventPnu;
	int neutrinopdg=-1;
        double fsleptonenergy;
        int fsleptonpdg;
        Direction fsleptonmomentum;
	double scale_factor;
};

#endif
