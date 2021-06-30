#ifndef LoadGenieEvent_H
#define LoadGenieEvent_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "GenieInfo.h"

// legacy
#define LOADED_GENIE 1

#if LOADED_GENIE==1                       // disable this tool unless Genie is loaded (FNAL)
//GENIE
#include <Tools/Flux/GSimpleNtpFlux.h>
#include <Tools/Flux/GNuMIFlux.h>
#include <GHEP/GHepUtils.h>               // neut reaction codes
#include <ParticleData/PDGLibrary.h>
#include <ParticleData/PDGCodes.h>
#include <Ntuple/NtpMCEventRecord.h>
#include <Conventions/Constants.h>
#include <GHEP/GHepParticle.h>
#include <GHEP/GHepStatus.h>
#include <EventGen/EventRecord.h>
#include <TParticlePDG.h>
#include <Interaction/Interaction.h>
// other
#endif  // LOADED_GENIE==1

class LoadGenieEvent: public Tool {
	
	public:
	
	LoadGenieEvent();
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
	
#if LOADED_GENIE==1
	// function to load the branch addresses
	void SetBranchAddresses();

	// function to fill the info into the handy genieinfostruct
	void GetGenieEntryInfo(genie::EventRecord* gevtRec, genie::Interaction* genieint,
	  GenieInfo& thegenieinfo, bool printneutrinoevent=false);
	// type conversion functions:
	std::map<int,std::string> pdgcodetoname;
	std::map<int,std::string> decaymap;
	std::map<int,std::string> gnumicodetoname;
	std::map<int,std::string> mediummap;
	std::map<int,std::string>* GenerateGnumiMap();
	std::map<int,std::string>* GeneratePdgMap();
	std::map<int,std::string>* GenerateDecayMap();
	std::map<int,std::string>* GenerateMediumMap();
	std::string GnumiToString(int code);
	std::string PdgToString(int code);
	std::string DecayTypeToString(int code);
	std::string MediumToString(int code);
	
	BoostStore* geniestore = nullptr;
	int fluxstage;
	std::string filedir, filepattern;
	bool loadwcsimsource;
	TChain* flux = nullptr;
	TFile* curf = nullptr;       // keep track of file changes
	TFile* curflast = nullptr;
	genie::NtpMCEventRecord* genieintx = nullptr; // = new genie::NtpMCEventRecord;
//	// for fluxver 0 files
	genie::flux::GNuMIFluxPassThroughInfo* gnumipassthruentry  = nullptr;
	// for fluxver 1 files
	genie::flux::GSimpleNtpEntry* gsimpleentry = nullptr;
	genie::flux::GSimpleNtpAux* gsimpleauxinfo = nullptr;
	genie::flux::GSimpleNtpNuMI* gsimplenumientry = nullptr;
	
	// genie file variables
	int fluxver;                         // 0 = old flux, 1 = new flux
	std::string currentfilestring;
	unsigned long local_entry=0;           // 
	unsigned int tchainentrynum=0;         // 
	bool manualmatch=1;			//to be used when GENIE information is not stored properly in file	

	// common input/output variables to both Robert/Zarko filesets
	int parentpdg;
	std::string parenttypestring;
	int parentdecaymode;                 // some arbitrary number that maps to a decay mode string.
	std::string parentdecaystring;       // descriptive string. Should we store a map of the translation?
	float parentdecayvtx_x, parentdecayvtx_y, parentdecayvtx_z;
	Position parentdecayvtx;
	float parentdecaymom_x, parentdecaymom_y, parentdecaymom_z;
	Position parentdecaymom;
	float parentprodmom_x, parentprodmom_y, parentprodmom_z;
	Position parentprodmom;
	int parentprodmedium;                // they're all 0
	std::string parentprodmediumstring;  // do we even have this mapping? --> There seems to be a mapping here: https://minos-docdb.fnal.gov/cgi-bin/sso/RetrieveFile?docid=6316&filename=flugg_doc.pdf&version=10
	int parentpdgattgtexit;
	std::string parenttypestringattgtexit;
	Position parenttgtexitmom;
	float parenttgtexitmom_x, parenttgtexitmom_y, parenttgtexitmom_z;
	int pcodes;			// Needed to evaluate whether the particle codes are stored in GEANT format or in PDG format

	// Additional zarko-only information
	// TODO fillme
	
	// store the neutrino info from gntp files
	// a load of variables to specify interaction type
	bool IsQuasiElastic=false;
	bool IsResonant=false;
	bool IsDeepInelastic=false;
	bool IsCoherent=false;
	bool IsDiffractive=false;
	bool IsInverseMuDecay=false;
	bool IsIMDAnnihilation=false;
	bool IsSingleKaon=false;
	bool IsNuElectronElastic=false;
	bool IsEM=false;
	bool IsWeakCC=false;
	bool IsWeakNC=false;
	bool IsMEC=false;
	std::string interactiontypestring="";
	int neutcode=-1;
	// ok, moving on
	double nuIntxVtx_X; // cm
	double nuIntxVtx_Y; // cm
	double nuIntxVtx_Z; // cm
	double nuIntxVtx_T; // ns
	bool isintank=false;
	bool isinfiducialvol=false;
	double eventq2=-1;
	double eventEnu=-1;
	int neutrinopdg=-1;
	double muonenergy=-1;
	double muonangle=-1;
	std::string fsleptonname; // assumed to be muon, but we should confirm
	// these may not be properly copied... --> temp fix applied that seems to be working
	int numfsprotons;
	int numfsneutrons;
	int numfspi0;
	int numfspiplus;
	int numfspiminus;
	int numfskplus;
	int numfskminus;	

#endif   // LOADED_GENIE==1
	
};

#endif
