/* vim:set noexpandtab tabstop=2 wrap */
#include "LoadGenieEvent.h"

//using namespace genie;
//using namespace genie::constants;
//using namespace genie::flux;

/*
GENIE INPUTS
============

Robert's Files
--------------
1) gnumi files: /pnfs/annie/persistent/flux/bnb/bnb_annie_0000.root
- these are on parallel with the gsimple files but store less information
- they feed into genie and are used to generate neutrino rays

Zarko's Files
-------------
There are 2 stages of files:
1) ReDecay files: /annie/data/flux/redecay_bnb/beammc_annie_0000.root
- these are the topmost BNB flux files, containing all BNB simulation errors
- these files are used to generate gsimple ntuples
- events in this ntuple have weighting according to systematic errors on generation parameters
- there are 1000 weights for 7 systematics:
K+, K-, K0, pi+, pi-, beamunisims, and total (product of other 6)
beamunisims contains systematics due to horn current miscal, skin depth, variations in total, qe and inelastic cross section of pi on Be and Al
- you can use these to do systematic studies

2) gsimple files: /annie/data/flux/gsimple_bnb/gsimple_beammc_annie_0000.root
- the flux rom the re-decay files is propagated to a detector window
- weights are stripped as genie cannot propagate them
- the resulting gsimple files describe neutrino rays that feed into genie
- zarko's above files have a window of 20x20m, 20m upstream of detector origin at (0,0,100.35m)
  (the origin of detector coordinate system in beam coordinate system)

GENIE OUTPUTS
=============

Robert's Files
--------------
Genie 2.8.6 GNTP files: /pnfs/annie/persistent/users/vfischer/genie/BNB_Water_10k_22-05-17/gntp.1000.ghep.root
- these files contain genie::NtpMCEventRecord objects that contain details of the neutrino event
- input event information is passed into a genie::flux::GNuMIFluxPassThroughInfo object

Zarko's Files
-------------
Genie 2.12 GNTP files: /pnfs/annie/persistent/users/moflaher/genie/BNB_World_10k_11-03-18_gsimpleflux/gntp.1000.ghep.root
- these files also contain genie::NtpMCEventRecord objects to describe details of the neutrino event
- input event information is passed to a genie::flux::GSimpleNtpEntry and genie::flux::GSimpleNtpNuMI object

*/

LoadGenieEvent::LoadGenieEvent():Tool(){}

bool LoadGenieEvent::Initialise(std::string configfile, DataModel &data){

	/////////////////// Useful header ///////////////////////
	if(configfile!="") m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////
	
	m_variables.Get("verbosity",verbosity);
	m_variables.Get("FluxVersion",fluxver); // flux version: 0=rhatcher files, 1=zarko files
	m_variables.Get("FileDir",filedir);
	m_variables.Get("FilePattern",filepattern);
	
	// create a store for holding Genie information to pass to downstream Tools
	// will be a single entry BoostStore containing a vector of single entry BoostStores
	geniestore = new BoostStore(true,0); // enable type-checking, BoostStore type binary
	m_data->Stores.emplace("GenieInfo",geniestore);
	
	// Open the flux files
	///////////////////////
	Log("Tool LoadGenieEvent: Opening TChain",v_debug,verbosity);
	flux = new TChain("gtree");
	std::string inputfiles = filedir+"/"+filepattern;
	int numbytes = flux->Add(inputfiles.c_str());
	Log("Tool LoadGenieEvent: Read "+to_string(numbytes)+" bytes loading TChain "+inputfiles,v_debug,verbosity);
	Log("Tool LoadGenieEvent: Genie TChain has "+to_string(flux->GetEntries())+" entries",v_message,verbosity);
	Log("Tool LoadGenieEvent: Setting branch addresses",v_debug,verbosity);
	// neutrino event information
	flux->SetBranchAddress("gmcrec",&genieintx);
	flux->GetBranch("gmcrec")->SetAutoDelete(kTRUE);
	// input (BNB intx) event information
	if(fluxver==0){   // rhatcher files
//		flux->SetBranchAddress("flux",&gnumipassthruentry);
//		flux->GetBranch("flux")->SetAutoDelete(kTRUE);
	} else {          // zarko files
		flux->SetBranchAddress("numi",&gsimplenumientry);
		flux->GetBranch("numi")->SetAutoDelete(kTRUE);
		flux->SetBranchAddress("simple",&gsimpleentry);
		flux->GetBranch("simple")->SetAutoDelete(kTRUE);
		flux->SetBranchAddress("aux",&gsimpleauxinfo);
		flux->GetBranch("aux")->SetAutoDelete(kTRUE);
	}
	
	return true;
}

bool LoadGenieEvent::Execute(){
	
	Log("Tool LoadGenieEvent: Loading tchain entry "+to_string(tchainentrynum),v_debug,verbosity);
	local_entry = flux->LoadTree(tchainentrynum);
	Log("Tool LoadGenieEvent: localentry is "+to_string(local_entry),v_debug,verbosity);
	if(local_entry<0){
		Log("Tool LoadGenieEvent: Reached end of file, returning",v_message,verbosity);
		return -1;
	}
	flux->GetEntry(local_entry);
	curf = flux->GetCurrentFile();
	if(curf!=curflast || curflast==nullptr){
		TString curftstring = curf->GetName();
		currentfilestring = std::string(curftstring.Data());
		curflast=curf;
		Log("Tool LoadGenieEvent: Opening new file \""+currentfilestring+"\"",v_debug,verbosity);
	}
	tchainentrynum++;
	
	// Expand out the neutrino event info
	// =======================================================
	
	// header only contains the event number
	genie::NtpMCRecHeader hdr = genieintx->hdr;
	unsigned int genie_event_num = hdr.ievent;
	
	// all neutrino intx details are in the event record
	genie::EventRecord* gevtRec = genieintx->event;
	
	if(fluxver==0){
/*
		// FLUXVER 0 - genie::flux::GNuMIFluxPassThroughInfo
		// =================================================
		// extract the target intx details from the GNuMIFluxPassThroughInfo object
		parentpdg = gnumipassthruentry->ptype;
		parentdecaymode = gnumipassthruentry->ndecay;
		parentdecayvtx_x = gnumipassthruentry->vx;
		parentdecayvtx_y = gnumipassthruentry->vy;
		parentdecayvtx_z = gnumipassthruentry->vz;
		parentdecaymom_x = gnumipassthruentry->pdpx;
		parentdecaymom_y = gnumipassthruentry->pdpy;
		parentdecaymom_z = gnumipassthruentry->pdpz;
		parentprodmom_x = gnumipassthruentry->ppdxdz;
		parentprodmom_y = gnumipassthruentry->ppdydz;
		parentprodmom_z = gnumipassthruentry->pppz;
		//parentprodmedium = gnumipassthruentry->ppmedium;
		parentpdgattgtexit = gnumipassthruentry->tptype;
		parenttgtexitmom_x = gnumipassthruentry->tpx;
		parenttgtexitmom_y = gnumipassthruentry->tpy;
		parenttgtexitmom_z = gnumipassthruentry->tpz;
		
		// convenience type conversions
		parentdecayvtx = Position(parentdecayvtx_x,parentdecayvtx_y,parentdecayvtx_z);
		parentdecaymom = Position(parentdecaymom_x,parentdecaymom_y,parentdecaymom_z);
		parentprodmom = Position(parentprodmom_x,parentprodmom_y,parentprodmom_z);
		parenttgtexitmom = Position(parenttgtexitmom_x,parenttgtexitmom_y,parenttgtexitmom_z);
		parenttypestring = (fluxstage==0) ? GnumiToString(parentpdg) : PdgToString(parentpdg);
		parenttypestringattgtexit = (fluxstage==0) ? 
			GnumiToString(parentpdgattgtexit) : PdgToString(parentpdgattgtexit);
		parentdecaystring = DecayTypeToString(parentdecaymode);
		//parentprodmediumstring = MediumToString(parentprodmedium);
*/
		
	} else {
		// FLUXVER 1 - genie::flux::GSimpleNtpEntry
		// ========================================
		// extract the target intx details from the GSimpleNtpNuMI object
		Log("Tool LoadGenieEvent: Retrieving interaction info from GSimpleNtpNuMI object",v_debug,verbosity);
		parentpdg = gsimplenumientry->ptype;
		parentdecaymode = gsimplenumientry->ndecay;
		parentdecayvtx_x = gsimplenumientry->vx;
		parentdecayvtx_y = gsimplenumientry->vy;
		parentdecayvtx_z = gsimplenumientry->vz;
		parentdecaymom_x = gsimplenumientry->pdpx;
		parentdecaymom_y = gsimplenumientry->pdpy;
		parentdecaymom_z = gsimplenumientry->pdpz;
		parentprodmom_x = gsimplenumientry->pppx/gsimplenumientry->pppz; // ??? is this ppdxdz?
		parentprodmom_y = gsimplenumientry->pppy/gsimplenumientry->pppz;
		parentprodmom_z = gsimplenumientry->pppz;
		//parentprodmedium = gsimplenumientry->ppmedium;
		parentpdgattgtexit = gsimplenumientry->tptype;
		parenttgtexitmom_x = gsimplenumientry->tpx;
		parenttgtexitmom_y = gsimplenumientry->tpy;
		parenttgtexitmom_z = gsimplenumientry->tpz;
		
		// convenience type conversions
		parentdecayvtx = Position(parentdecayvtx_x,parentdecayvtx_y,parentdecayvtx_z);
		parentdecaymom = Position(parentdecaymom_x,parentdecaymom_y,parentdecaymom_z);
		parentprodmom = Position(parentprodmom_x,parentprodmom_y,parentprodmom_z);
		parenttgtexitmom = Position(parenttgtexitmom_x,parenttgtexitmom_y,parenttgtexitmom_z);
		parenttypestring = PdgToString(parentpdg);
		parenttypestringattgtexit = PdgToString(parentpdgattgtexit);
		parentdecaystring = DecayTypeToString(parentdecaymode);
		//parentprodmediumstring = MediumToString(parentprodmedium);
		
	}
	
	// neutrino interaction info
	genie::Interaction* genieint = gevtRec->Summary();
	//cout<<"scraping interaction info"<<endl;
	GenieInfo thegenieinfo;
	Log("Tool LoadGenieEvent: Filling GenieInfo struct",v_debug,verbosity);
	GetGenieEntryInfo(gevtRec, genieint, thegenieinfo, (verbosity>v_debug));
	
	// retrieve info from the struct
	IsQuasiElastic=thegenieinfo.eventtypes.at("IsQuasiElastic");
	IsResonant=thegenieinfo.eventtypes.at("IsResonant");
	IsDeepInelastic=thegenieinfo.eventtypes.at("IsDeepInelastic");
	IsCoherent=thegenieinfo.eventtypes.at("IsCoherent");
	IsDiffractive=thegenieinfo.eventtypes.at("IsDiffractive");
	IsInverseMuDecay=thegenieinfo.eventtypes.at("IsInverseMuDecay");
	IsIMDAnnihilation=thegenieinfo.eventtypes.at("IsIMDAnnihilation");
	IsSingleKaon=thegenieinfo.eventtypes.at("IsSingleKaon");
	IsNuElectronElastic=thegenieinfo.eventtypes.at("IsNuElectronElastic");
	IsEM=thegenieinfo.eventtypes.at("IsEM");
	IsWeakCC=thegenieinfo.eventtypes.at("IsWeakCC");
	IsWeakNC=thegenieinfo.eventtypes.at("IsWeakNC");
	IsMEC=thegenieinfo.eventtypes.at("IsMEC");
	interactiontypestring=thegenieinfo.interactiontypestring;
	neutcode=thegenieinfo.neutinteractioncode; // currently disabled to prevent excessive verbosity
	
	eventq2=thegenieinfo.Q2;
	eventEnu=thegenieinfo.probeenergy;
	neutrinopdg=thegenieinfo.probepdg;
	muonenergy=thegenieinfo.fsleptonenergy;
	muonangle=thegenieinfo.fslangle;
	
	nuIntxVtx_X=thegenieinfo.Intx_x; // cm
	nuIntxVtx_Y=thegenieinfo.Intx_y; // cm
	nuIntxVtx_Z=thegenieinfo.Intx_z; // cm
	nuIntxVtx_T=thegenieinfo.Intx_t; // ns
	// check in tank
	if( ( sqrt( pow(nuIntxVtx_X, 2) + pow(nuIntxVtx_Z-MRDSpecs::tank_start-MRDSpecs::tank_radius,2) )
		  < MRDSpecs::tank_radius ) && 
		  ( abs(nuIntxVtx_Y-MRDSpecs::tank_yoffset) < MRDSpecs::tank_halfheight) ){
		isintank=true;
	} else { isintank=false; }
	// check in fiducial volume
	if( isintank &&
	  ( sqrt (pow(nuIntxVtx_X, 2) + pow(nuIntxVtx_Z-MRDSpecs::tank_start-MRDSpecs::tank_radius,2)) 
	  < MRDSpecs::fidcutradius ) && 
	  ( abs(nuIntxVtx_Y-MRDSpecs::tank_yoffset) < MRDSpecs::fidcuty ) && 
	  ( (nuIntxVtx_Z-MRDSpecs::tank_start-MRDSpecs::tank_radius) < MRDSpecs::fidcutz) ){
		isinfiducialvol=true;
	} else { isinfiducialvol = false; }
	
	fsleptonname = std::string(thegenieinfo.fsleptonname.Data());
	// this data does not appear to be populated...
	numfsprotons = thegenieinfo.numfsprotons = genieint->ExclTag().NProtons();
	numfsneutrons = thegenieinfo.numfsneutrons = genieint->ExclTag().NNeutrons();
	numfspi0 = thegenieinfo.numfspi0 = genieint->ExclTag().NPi0();
	numfspiplus = thegenieinfo.numfspiplus = genieint->ExclTag().NPiPlus();
	numfspiminus = thegenieinfo.numfspiminus = genieint->ExclTag().NPiMinus();
	
	Log("Tool LoadGenieEvent: Passing information to the GenieEvent store",v_debug,verbosity);
	// Update the Store with all the current event information
	// =======================================================
	geniestore->Set("file",currentfilestring);
	geniestore->Set("fluxver",fluxver);
	geniestore->Set("evtnum",tchainentrynum);
	geniestore->Set("ParentPdg",parentpdg);
	geniestore->Set("ParentTypeString",parenttypestring);
	geniestore->Set("ParentDecayMode",parentdecaymode);
	geniestore->Set("ParentDecayString",parentdecaystring);
	geniestore->Set("ParentDecayVtx",parentdecayvtx);
	geniestore->Set("ParentDecayVtx_X",parentdecayvtx_x);
	geniestore->Set("ParentDecayVtx_Y",parentdecayvtx_y);
	geniestore->Set("ParentDecayVtx_Z",parentdecayvtx_z);
	geniestore->Set("ParentDecayMom",parentdecaymom);
	geniestore->Set("ParentDecayMom_X",parentdecaymom_x);
	geniestore->Set("ParentDecayMom_Y",parentdecaymom_y);
	geniestore->Set("ParentDecayMom_Z",parentdecaymom_z);
	geniestore->Set("ParentProdMom",parentprodmom);
	geniestore->Set("ParentProdMom_X",parentprodmom_x);
	geniestore->Set("ParentProdMom_Y",parentprodmom_y);
	geniestore->Set("ParentProdMom_Z",parentprodmom_z);
	//geniestore->Set("ParentProdMedium",parentprodmedium);
	//geniestore->Set("ParentProdMediumString",parentprodmediumstring);
	geniestore->Set("ParentPdgAtTgtExit",parentpdgattgtexit);
	geniestore->Set("ParentTypeAtTgtExitString",parenttypestringattgtexit);
	geniestore->Set("ParentTgtExitMom",parenttgtexitmom);
	geniestore->Set("ParentTgtExitMom_X",parenttgtexitmom_x);
	geniestore->Set("ParentTgtExitMom_Y",parenttgtexitmom_y);
	geniestore->Set("ParentTgtExitMom_Z",parenttgtexitmom_z);
	
	geniestore->Set("IsQuasiElastic",IsQuasiElastic);
	geniestore->Set("IsResonant",IsResonant);
	geniestore->Set("IsDeepInelastic",IsDeepInelastic);
	geniestore->Set("IsCoherent",IsCoherent);
	geniestore->Set("IsDiffractive",IsDiffractive);
	geniestore->Set("IsInverseMuDecay",IsInverseMuDecay);
	geniestore->Set("IsIMDAnnihilation",IsIMDAnnihilation);
	geniestore->Set("IsSingleKaon",IsSingleKaon);
	geniestore->Set("IsNuElectronElastic",IsNuElectronElastic);
	geniestore->Set("IsEM",IsEM);
	geniestore->Set("IsWeakCC",IsWeakCC);
	geniestore->Set("IsWeakNC",IsWeakNC);
	geniestore->Set("IsMEC",IsMEC);
	geniestore->Set("InteractionTypeString",interactiontypestring);
	geniestore->Set("NeutCode",neutcode);
	geniestore->Set("NuIntxVtx_X",nuIntxVtx_X);
	geniestore->Set("NuIntxVtx_Y",nuIntxVtx_Y);
	geniestore->Set("NuIntxVtx_Z",nuIntxVtx_Z);
	geniestore->Set("NuIntxVtx_T",nuIntxVtx_T);
	geniestore->Set("NuVtxInTank",isintank);
	geniestore->Set("NuVtxInFidVol",isinfiducialvol);
	geniestore->Set("EventQ2",eventq2);
	geniestore->Set("NeutrinoEnergy",eventEnu);
	geniestore->Set("NeutrinoPDG",neutrinopdg);
	geniestore->Set("MuonEnergy",muonenergy);
	geniestore->Set("MuonAngle",muonangle);
	geniestore->Set("FSLeptonName",fsleptonname);
	geniestore->Set("NumFSProtons",numfsprotons);
	geniestore->Set("NumFSNeutrons",numfsneutrons);
	geniestore->Set("NumFSPi0",numfspi0);
	geniestore->Set("NumFSPiPlus",numfspiplus);
	geniestore->Set("NumFSPiMinus",numfspiminus);
	geniestore->Set("TheGenieInfo",thegenieinfo);
	
	Log("Tool LoadGenieEvent: Clearing genieintx",v_debug,verbosity);
	genieintx->Clear(); // REQUIRED TO PREVENT MEMORY LEAK
	
	Log("Tool LoadGenieEvent: done",v_debug,verbosity);
	return true;
}

bool LoadGenieEvent::Finalise(){
	
	return true;
}

void LoadGenieEvent::GetGenieEntryInfo(genie::EventRecord* gevtRec, genie::Interaction* genieint, GenieInfo &thegenieinfo, bool printneutrinoevent){
	// process information:
	/*TString*/ thegenieinfo.procinfostring = genieint->ProcInfo().AsString();
	/*TString*/ thegenieinfo.scatteringtypestring = genieint->ProcInfo().ScatteringTypeAsString();
	/*TString*/ thegenieinfo.interactiontypestring = genieint->ProcInfo().InteractionTypeAsString();
	thegenieinfo.eventtypes.at("IsQuasiElastic") = genieint->ProcInfo().IsQuasiElastic();
	thegenieinfo.eventtypes.at("IsResonant") = genieint->ProcInfo().IsResonant();
	thegenieinfo.eventtypes.at("IsDeepInelastic") = genieint->ProcInfo().IsDeepInelastic();
	thegenieinfo.eventtypes.at("IsCoherent") = genieint->ProcInfo().IsCoherent();
	thegenieinfo.eventtypes.at("IsDiffractive") = genieint->ProcInfo().IsDiffractive();
	thegenieinfo.eventtypes.at("IsInverseMuDecay") = genieint->ProcInfo().IsInverseMuDecay();
	thegenieinfo.eventtypes.at("IsIMDAnnihilation") = genieint->ProcInfo().IsIMDAnnihilation();
	thegenieinfo.eventtypes.at("IsSingleKaon") = genieint->ProcInfo().IsSingleKaon();
	thegenieinfo.eventtypes.at("IsNuElectronElastic") = genieint->ProcInfo().IsNuElectronElastic();
	thegenieinfo.eventtypes.at("IsEM") = genieint->ProcInfo().IsEM();
	thegenieinfo.eventtypes.at("IsWeakCC") = genieint->ProcInfo().IsWeakCC();
	thegenieinfo.eventtypes.at("IsWeakNC") = genieint->ProcInfo().IsWeakNC();
	thegenieinfo.eventtypes.at("IsMEC") = genieint->ProcInfo().IsMEC();
	/*
	getting the neut reaction code results in the printing of a bunch of surplus info, e.g:
1501283211 NOTICE GHepUtils : [n] <GHepUtils.cxx::NeutReactionCode (106)> : Current event is RES or DIS with W<2
1501283211 NOTICE GHepUtils : [n] <GHepUtils.cxx::NeutReactionCode (153)> : Num of primary particles: 
 p = 1, n = 0, pi+ = 0, pi- = 1, pi0 = 0, eta = 0, K+ = 0, K- = 0, K0 = 0, Lambda's = 0, gamma's = 0
	if we could redirect and capture this (rather than printing it to stdout) it might actually be useful,
	as extracting number of other hadrons doesn't work! but for now, just turn it off to reduce verbosity.
	*/
	/*Int_t*/ //thegenieinfo.neutinteractioncode = genie::utils::ghep::NeutReactionCode(gevtRec);
	/*Int_t*/ thegenieinfo.nuanceinteractioncode	= genie::utils::ghep::NuanceReactionCode(gevtRec);
	/*TLorentzVector**/ thegenieinfo.IntxVtx = gevtRec->Vertex();
	/*Double_t*/ thegenieinfo.Intx_x = thegenieinfo.IntxVtx->X() * 100.;	 // same info as nuvtx in g4dirt file
	/*Double_t*/ thegenieinfo.Intx_y = thegenieinfo.IntxVtx->Y() * 100.;	 // GENIE uses meters
	/*Double_t*/ thegenieinfo.Intx_z = thegenieinfo.IntxVtx->Z() * 100.;	 // GENIE uses meters
	/*Double_t*/ thegenieinfo.Intx_t = thegenieinfo.IntxVtx->T() * 1000000000; // GENIE uses seconds
	
	// neutrino information:
	/*Double_t*/ thegenieinfo.probeenergy = genieint->InitState().ProbeE(genie::kRfLab);	// GeV
	/*Int_t*/ thegenieinfo.probepdg = genieint->InitState().Probe()->PdgCode();
	/*TString*/ thegenieinfo.probepartname = genieint->InitState().Probe()->GetName();
	/*TLorentzVector**/ thegenieinfo.probemomentum = gevtRec->Probe()->P4();
	/*TVector3*/ thegenieinfo.probethreemomentum = thegenieinfo.probemomentum->Vect();
	/*TVector3*/ thegenieinfo.probemomentumdir = thegenieinfo.probethreemomentum.Unit();
	/*Double_t*/ thegenieinfo.probeanglex = 
		TMath::ATan(thegenieinfo.probethreemomentum.X()/thegenieinfo.probethreemomentum.Z());
	/*Double_t*/ thegenieinfo.probeangley = 
		TMath::ATan(thegenieinfo.probethreemomentum.Y()/thegenieinfo.probethreemomentum.Z());
	/*Double_t*/ thegenieinfo.probeangle = TMath::Max(thegenieinfo.probeanglex,thegenieinfo.probeangley);
	// n.b.	genieint->InitState().Probe != gevtRec->Probe()
	
	// target nucleon:
	/*genie::GHepParticle**/ thegenieinfo.targetnucleon = gevtRec->HitNucleon();
	/*int*/ thegenieinfo.targetnucleonpdg = genieint->InitState().Tgt().HitNucPdg();
	/*TString*/ thegenieinfo.targetnucleonname="";
	if ( genie::pdg::IsNeutronOrProton(thegenieinfo.targetnucleonpdg) ) {
		TParticlePDG * p = genie::PDGLibrary::Instance()->Find(thegenieinfo.targetnucleonpdg);
		thegenieinfo.targetnucleonname = p->GetName();
	} else {
		thegenieinfo.targetnucleonname = std::to_string(thegenieinfo.targetnucleonpdg);
	}
	/*TVector3*/ thegenieinfo.targetnucleonthreemomentum=TVector3(0.,0.,0.);
	/*Double_t*/ thegenieinfo.targetnucleonenergy=0.;
	if(thegenieinfo.targetnucleon){
		TLorentzVector* targetnucleonmomentum = thegenieinfo.targetnucleon->P4();
		thegenieinfo.targetnucleonthreemomentum = targetnucleonmomentum->Vect();
		thegenieinfo.targetnucleonenergy = targetnucleonmomentum->Energy(); //GeV
	}
	
	// target nucleus:
	/*Int_t*/ thegenieinfo.targetnucleuspdg = genieint->InitState().Tgt().Pdg();
	/*TParticlePDG**/ thegenieinfo.targetnucleus = 
		genie::PDGLibrary::Instance()->Find(thegenieinfo.targetnucleuspdg);
	/*TString*/ thegenieinfo.targetnucleusname = "unknown";
	if(thegenieinfo.targetnucleus){ thegenieinfo.targetnucleusname = thegenieinfo.targetnucleus->GetName(); }
	/*Int_t*/ thegenieinfo.targetnucleusZ = genieint->InitState().Tgt().Z();
	/*Int_t*/ thegenieinfo.targetnucleusA = genieint->InitState().Tgt().A();
	
	// remnant nucleus:
	int remnucpos = gevtRec->RemnantNucleusPosition(); 
	/*TString*/ thegenieinfo.remnantnucleusname="n/a";
	/*Double_t*/ thegenieinfo.remnantnucleusenergy=-1.;
	if(remnucpos>-1){
		thegenieinfo.remnantnucleusname = gevtRec->Particle(remnucpos)->Name();
		thegenieinfo.remnantnucleusenergy = gevtRec->Particle(remnucpos)->Energy(); //GeV
	}
	
	// final state lepton:
	int fsleppos = gevtRec->FinalStatePrimaryLeptonPosition();
	/*TString*/ thegenieinfo.fsleptonname="n/a";
	/*Double_t*/ thegenieinfo.fsleptonenergy=-1.;
	if(fsleppos>-1){
		thegenieinfo.fsleptonname = gevtRec->Particle(fsleppos)->Name();
		thegenieinfo.fsleptonenergy = gevtRec->Particle(fsleppos)->Energy();
	}
	
	// other remnants: TODO: this information is NOT being correctly read in
	/*Int_t*/ thegenieinfo.numfsprotons = genieint->ExclTag().NProtons();
	/*Int_t*/ thegenieinfo.numfsneutrons = genieint->ExclTag().NNeutrons();
	/*Int_t*/ thegenieinfo.numfspi0 = genieint->ExclTag().NPi0();
	/*Int_t*/ thegenieinfo.numfspiplus = genieint->ExclTag().NPiPlus();
	/*Int_t*/ thegenieinfo.numfspiminus = genieint->ExclTag().NPiMinus();
	
	// kinematic information
	Double_t NucleonM	= genie::constants::kNucleonMass; 
	// Calculate kinematic variables "as an experimentalist would measure them; 
	// neglecting fermi momentum and off-shellness of bound nucleons"
	/*TLorentzVector**/ thegenieinfo.k1 = gevtRec->Probe()->P4();
	/*TLorentzVector**/ thegenieinfo.k2 = gevtRec->FinalStatePrimaryLepton()->P4();
	/*Double_t*/ thegenieinfo.costhfsl = TMath::Cos( thegenieinfo.k2->Vect().Angle(thegenieinfo.k1->Vect()) );
	/*Double_t*/ thegenieinfo.fslangle = thegenieinfo.k2->Vect().Angle(thegenieinfo.k1->Vect());
	// q=k1-k2, 4-p transfer
	/*TLorentzVector*/ thegenieinfo.q	= (*(thegenieinfo.k1))-(*(thegenieinfo.k2));
//	/*Double_t*/ thegenieinfo.Q2 = genieint->Kine().Q2();	// not set in our GENIE files!
	// momemtum transfer
	/*Double_t*/ thegenieinfo.Q2 = -1 * thegenieinfo.q.M2();
	// E transfer to the nucleus
	/*Double_t*/ thegenieinfo.Etransf	= (thegenieinfo.targetnucleon) ? thegenieinfo.q.Energy() : -1;
	// Bjorken x
	/*Double_t*/ thegenieinfo.x	= 
		(thegenieinfo.targetnucleon) ? 0.5*thegenieinfo.Q2/(NucleonM*thegenieinfo.Etransf) : -1;
	// Inelasticity, y = q*P1/k1*P1
	/*Double_t*/ thegenieinfo.y	= 
		(thegenieinfo.targetnucleon) ? thegenieinfo.Etransf/thegenieinfo.k1->Energy() : -1;
	// Hadronic Invariant mass ^ 2
	/*Double_t*/ thegenieinfo.W2 = 
	(thegenieinfo.targetnucleon) ? (NucleonM*NucleonM + 2*NucleonM*thegenieinfo.Etransf - thegenieinfo.Q2) : -1;
	
	if(printneutrinoevent){
		cout<<"This was a "<< thegenieinfo.procinfostring <<" (neut code "<<thegenieinfo.neutinteractioncode
			<<") interaction of a "
			<<thegenieinfo.probeenergy<<"GeV " << thegenieinfo.probepartname << " on a "; 
		
		if( thegenieinfo.targetnucleonpdg==2212 || thegenieinfo.targetnucleonpdg==2122 ){
			cout<<thegenieinfo.targetnucleonname<<" in a ";
		} else {
			cout<<"PDG-Code " << thegenieinfo.targetnucleonpdg<<" in a ";
		}
		
		if( thegenieinfo.targetnucleusname!="unknown"){ cout<<thegenieinfo.targetnucleusname<<" nucleus, "; }
		else { cout<<"Z=["<<thegenieinfo.targetnucleusZ<<","<<thegenieinfo.targetnucleusA<<"] nucleus, "; }
		
		if(remnucpos>-1){
			cout<<"producing a "<<thegenieinfo.remnantnucleusenergy<<"GeV "<<thegenieinfo.remnantnucleusname;
		} else { cout<<"with no remnant nucleus"; }	// DIS on 16O produces no remnant nucleus?!
		
		if(fsleppos>-1){
			cout<<" and a "<<thegenieinfo.fsleptonenergy<<"GeV "<<thegenieinfo.fsleptonname<<endl;
		} else{ cout<<" and no final state leptons"<<endl; }
		
		cout<<endl<<"Q^2 was "<<thegenieinfo.Q2<<"(GeV/c)^2, with final state lepton"
			<<" ejected at Cos(θ)="<<thegenieinfo.costhfsl<<endl;
		cout<<"Additional final state particles included "<<endl;
		cout<< " N(p) = "	 << thegenieinfo.numfsprotons
			<< " N(n) = "	 << thegenieinfo.numfsneutrons
			<< endl
			<< " N(pi^0) = "	<< thegenieinfo.numfspi0
			<< " N(pi^+) = "	<< thegenieinfo.numfspiplus
			<< " N(pi^-) = "	<< thegenieinfo.numfspiminus
			<<endl;
	}
}

// type conversion functions:
std::string LoadGenieEvent::GnumiToString(int code){
	if(gnumicodetoname.size()==0) GenerateGnumiMap();
	if(gnumicodetoname.count(code)!=0){
		return gnumicodetoname.at(code);
	} else {
		cerr<<"unknown gnumi code "<<code<<endl;
		return std::to_string(code);
	}
}

std::string LoadGenieEvent::PdgToString(int code){
	if(pdgcodetoname.size()==0) GeneratePdgMap();
	if(pdgcodetoname.count(code)!=0){
		return pdgcodetoname.at(code);
	} else {
		cerr<<"unknown pdg code "<<code<<endl;
		return std::to_string(code);
	}
}

std::string LoadGenieEvent::DecayTypeToString(int code){
	if(decaymap.size()==0) GenerateDecayMap();
	if(decaymap.count(code)!=0){
		return decaymap.at(code);
	} else {
		cerr<<"unknown decay code "<<code<<endl;
		return std::to_string(code);
	}
}

std::string LoadGenieEvent::MediumToString(int code){
	return std::to_string(code); // TODO fill this out
}

std::map<int,std::string>* LoadGenieEvent::GenerateGnumiMap(){
	if(gnumicodetoname.size()!=0) return &gnumicodetoname;
	gnumicodetoname.emplace(14 ,"Proton");
	gnumicodetoname.emplace(15 ,"Anti Proton");
	gnumicodetoname.emplace(3 ,"Electron");
	gnumicodetoname.emplace(2 ,"Positron");
	gnumicodetoname.emplace(53 ,"Electron Neutrino");
	gnumicodetoname.emplace(52 ,"Anti Electron Neutrino");
	gnumicodetoname.emplace(1 ,"Photon");
	gnumicodetoname.emplace(13 ,"Neutron");
	gnumicodetoname.emplace(25 ,"Anti Neutron");
	gnumicodetoname.emplace(5 ,"Muon+");
	gnumicodetoname.emplace(6 ,"Muon-");
	gnumicodetoname.emplace(10 ,"Kaonlong");
	gnumicodetoname.emplace(8 ,"Pion+");
	gnumicodetoname.emplace(9 ,"Pion-");
	gnumicodetoname.emplace(11 ,"Kaon+");
	gnumicodetoname.emplace(12 ,"Kaon-");
	gnumicodetoname.emplace(18 ,"Lambda");
	gnumicodetoname.emplace(26 ,"Antilambda");
	gnumicodetoname.emplace(16 ,"Kaonshort");
	gnumicodetoname.emplace(21 ,"Sigma-");
	gnumicodetoname.emplace(19 ,"Sigma+");
	gnumicodetoname.emplace(20 ,"Sigma0");
	gnumicodetoname.emplace(7 ,"Pion0");
	gnumicodetoname.emplace(99,"Kaon0");  // gnumi particle code for Kaon0 and Antikaon0
	gnumicodetoname.emplace(98,"Antikaon0");  // are both listed as "10 & 16" ... 
	gnumicodetoname.emplace(56 ,"Muon Neutrino");
	gnumicodetoname.emplace(55 ,"Anti Muon Neutrino");
	gnumicodetoname.emplace(27 ,"Anti Sigma-");
	gnumicodetoname.emplace(28 ,"Anti Sigma0");
	gnumicodetoname.emplace(29 ,"Anti Sigma+");
	gnumicodetoname.emplace(22 ,"Xsi0");
	gnumicodetoname.emplace(30 ,"Anti Xsi0");
	gnumicodetoname.emplace(23 ,"Xsi-");
	gnumicodetoname.emplace(31 ,"Xsi+");
	gnumicodetoname.emplace(24 ,"Omega-");
	gnumicodetoname.emplace(32 ,"Omega+");
	gnumicodetoname.emplace(33 ,"Tau+");
	gnumicodetoname.emplace(34 ,"Tau-");
	return &gnumicodetoname;
}

std::map<int,std::string>* LoadGenieEvent::GeneratePdgMap(){
	if(pdgcodetoname.size()!=0) return &pdgcodetoname;
	pdgcodetoname.emplace(2212,"Proton");
	pdgcodetoname.emplace(-2212,"Anti Proton");
	pdgcodetoname.emplace(11,"Electron");
	pdgcodetoname.emplace(-11,"Positron");
	pdgcodetoname.emplace(12,"Electron Neutrino");
	pdgcodetoname.emplace(-12,"Anti Electron Neutrino");
	pdgcodetoname.emplace(22,"Gamma");
	pdgcodetoname.emplace(2112,"Neutron");
	pdgcodetoname.emplace(-2112,"Anti Neutron");
	pdgcodetoname.emplace(-13,"Muon+");
	pdgcodetoname.emplace(13,"Muon-");
	pdgcodetoname.emplace(130,"Kaonlong");
	pdgcodetoname.emplace(211,"Pion+");
	pdgcodetoname.emplace(-211,"Pion-");
	pdgcodetoname.emplace(321,"Kaon+");
	pdgcodetoname.emplace(-321,"Kaon-");
	pdgcodetoname.emplace(3122,"Lambda");
	pdgcodetoname.emplace(-3122,"Antilambda");
	pdgcodetoname.emplace(310,"Kaonshort");
	pdgcodetoname.emplace(3112,"Sigma-");
	pdgcodetoname.emplace(3222,"Sigma+");
	pdgcodetoname.emplace(3212,"Sigma0");
	pdgcodetoname.emplace(111,"Pion0");
	pdgcodetoname.emplace(311,"Kaon0");
	pdgcodetoname.emplace(-311,"Antikaon0");
	pdgcodetoname.emplace(14,"Muon Neutrino");
	pdgcodetoname.emplace(-14,"Anti Muon Neutrino");
	pdgcodetoname.emplace(-3222,"Anti Sigma-");
	pdgcodetoname.emplace(-3212,"Anti Sigma0");
	pdgcodetoname.emplace(-3112,"Anti Sigma+");
	pdgcodetoname.emplace(3322,"Xsi0");
	pdgcodetoname.emplace(-3322,"Anti Xsi0");
	pdgcodetoname.emplace(3312,"Xsi-");
	pdgcodetoname.emplace(-3312,"Xsi+");
	pdgcodetoname.emplace(3334,"Omega-");
	pdgcodetoname.emplace(-3334,"Omega+");
	pdgcodetoname.emplace(-15,"Tau+");
	pdgcodetoname.emplace(15,"Tau-");
	pdgcodetoname.emplace(100,"OpticalPhoton");
	pdgcodetoname.emplace(3328,"Alpha");
	pdgcodetoname.emplace(3329,"Deuteron");
	pdgcodetoname.emplace(3330,"Triton");
	pdgcodetoname.emplace(3351,"Li7");
	pdgcodetoname.emplace(3331,"C10");
	pdgcodetoname.emplace(3345,"B11");
	pdgcodetoname.emplace(3332,"C12");
	pdgcodetoname.emplace(3350,"C13");
	pdgcodetoname.emplace(3349,"N13");
	pdgcodetoname.emplace(3340,"N14");
	pdgcodetoname.emplace(3333,"N15");
	pdgcodetoname.emplace(3334,"N16");
	pdgcodetoname.emplace(3335,"O16");
	pdgcodetoname.emplace(3346,"Al27");
	pdgcodetoname.emplace(3341,"Fe54");
	pdgcodetoname.emplace(3348,"Mn54");
	pdgcodetoname.emplace(3342,"Mn55");
	pdgcodetoname.emplace(3352,"Mn56");
	pdgcodetoname.emplace(3343,"Fe56");
	pdgcodetoname.emplace(3344,"Fe57");
	pdgcodetoname.emplace(3347,"Fe58");
	pdgcodetoname.emplace(3353,"Eu154");
	pdgcodetoname.emplace(3336,"Gd158");
	pdgcodetoname.emplace(3337,"Gd156");
	pdgcodetoname.emplace(3338,"Gd157");
	pdgcodetoname.emplace(3339,"Gd155");
	return &pdgcodetoname;
}

std::map<int,std::string>* LoadGenieEvent::GenerateDecayMap(){
	if(decaymap.size()!=0) return &decaymap;
	decaymap.emplace(1,"K0L -> nue, pi-, e+");  //→
	decaymap.emplace(2,"K0L -> nuebar, pi+, e-");
	decaymap.emplace(3,"K0L -> numu, pi-, mu+");
	decaymap.emplace(4,"K0L -> numubar, pi+, mu-");
	decaymap.emplace(5,"K+  -> numu, mu+");
	decaymap.emplace(6,"K+  -> nue, pi0, e+");
	decaymap.emplace(7,"K+  -> numu, pi0, mu+");
	decaymap.emplace(8,"K-  -> numubar, mu-");
	decaymap.emplace(9,"K-  -> nuebar, pi0, e-");
	decaymap.emplace(10,"K-  -> numubar, pi0, mu-");
	decaymap.emplace(11,"mu+ -> numubar, nue, e+");
	decaymap.emplace(12,"mu- -> numu, nuebar, e-");
	decaymap.emplace(13,"pi+ -> numu, mu+");
	decaymap.emplace(14,"pi- -> numubar, mu-");
	return &decaymap;
}

