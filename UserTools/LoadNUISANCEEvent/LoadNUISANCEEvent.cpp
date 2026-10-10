/* vim:set noexpandtab tabstop=2 wrap */
#include "LoadNUISANCEEvent.h"

#include "TChain.h"
#include "TFile.h"
#include "TVector3.h"
#include "TLorentzVector.h"

#include "MRDspecs.hh"

LoadNUISANCEEvent::LoadNUISANCEEvent():Tool(){}

bool LoadNUISANCEEvent::Initialise(std::string configfile, DataModel &data){

	/////////////////// Useful header ///////////////////////
	if(configfile!="") m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////
	
	
	int evoffset;

	m_variables.Get("verbosity",verbosity);
	m_variables.Get("FileDir",filedir);
	m_variables.Get("FilePattern",filepattern);
	m_variables.Get("ManualFileMatching",manualmatch);
	m_variables.Get("EventOffset",evoffset);
	m_variables.Get("FileEvents",fileevents);

	// create a store for holding NUISANCE information to pass to downstream Tools
	// will be a single entry BoostStore containing a vector of single entry BoostStores
	nuisancestore = new BoostStore(true,0); // enable type-checking, BoostStore type binary
	m_data->Stores.emplace("NuisanceInfo",nuisancestore);
	
	// Open the flux files
	///////////////////////
	Log("Tool LoadNUISANCEEvent: Opening TChain",v_debug,verbosity);
	loadwcsimsource = (filepattern=="LoadWCSimTool");
	if(not loadwcsimsource && not manualmatch){
		// construct a new TChain and add all the files at once
		// this is for use of looking at stand alone NUISANCE file, with no WCSim
		std::string inputfiles = filedir+"/"+filepattern;
		tchainentrynum=0;
		flux = new TChain("FlatTree_VARS");
		int numbytes = flux->Add(inputfiles.c_str());
		Log("Tool LoadNUISANCEEvent: Read "+to_string(numbytes)+" bytes loading TChain "+inputfiles,v_debug,verbosity);
		Log("Tool LoadNUISANCEEvent: NUISANCE TChain has "+to_string(flux->GetEntries())+" entries",v_message,verbosity);
		SetBranchAddresses();
		tchainentrynum = evoffset;
		Log("LoadNUISANCEEvent tool: # of flux entries: "+std::to_string(flux->GetEntries()),v_message,verbosity);
	}

	if(manualmatch){
		// Manually create path to matching GENIE File for WCSim file
		std::string wcsimfile;
		m_data->Stores.at("ANNIEEvent")->Get("MCFile",wcsimfile);
		//Strip WCSim file name of its prefix path
		std::string wcsim_prefix = "wcsim_0.";
		wcsimfile.erase(0,wcsimfile.find(wcsim_prefix)+wcsim_prefix.length());
		wcsimfile.erase(wcsimfile.find(".root"),wcsimfile.find(".root")+5);
		std::string wcsimev = wcsimfile;
		wcsimfile.erase(wcsimfile.find("."),wcsimfile.length());
		wcsimev.erase(0,wcsimev.find(".")+1);

		std::cout <<"wcsimfile: "<<wcsimfile<<", wcsimev: "<<wcsimev<<std::endl;
		std::string::size_type sz;
		int wcsimfilenumber = std::stoi(wcsimfile,&sz);
		int wcsimevnumber = std::stoi(wcsimev,&sz);
		std::cout <<"wcsimfilenumber: "<<wcsimfilenumber<<", wcsimevnumber: "<<wcsimevnumber<<std::endl;

		std::string inputfile = filedir+"/nui."+wcsimfile+".root";
		curf=TFile::Open(inputfile.c_str());
                flux=(TChain*)curf->Get("FlatTree_VARS");
		SetBranchAddresses();
		tchainentrynum = wcsimevnumber*fileevents;
	}

	return true;

}

bool LoadNUISANCEEvent::Execute(){
	
        if(loadwcsimsource && !manualmatch){
                // retrieve the nuisance file and entry number from the LoadWCSim tool
                std::string inputfiles;
                get_ok = m_data->CStore.Get("GenieFile",inputfiles);
                if(!get_ok){
                        if(verbosity) std::cout << "Tool LoadNUISANCEEvent: Failed to find NuisanceFile in CStore" << std::endl;
                        return false;
                }
                if(filedir!="NA"){
                        std::string nuisance_prefix = "nui.";
                        inputfiles.erase(0, inputfiles.find(nuisance_prefix));
                        inputfiles = filedir+"/"+inputfiles;
                        if(verbosity) std::cout << "Tool LoadNUISANCEEvent: Loading NUISANCE file: " << inputfiles << std::endl;
                }
		get_ok = m_data->CStore.Get("GenieEntry",tchainentrynum);
                if(!get_ok){
                        Log("Tool LoadNUISANCEEvent: Failed to find NuisanceEntry in CStore",v_error,verbosity);
                        return false;
                }
                std::string curfname = ((curf) ? curf->GetName() : "");
                // check if this is a new file
                if(inputfiles!=curfname){
                        // we need to load the new file
                        if(flux) flux->ResetBranchAddresses();
                        if(curf) curf->Close();
                        Log("Tool LoadNUISANCEEvent: Loading new file "+inputfiles,v_debug,verbosity);
                        curf=TFile::Open(inputfiles.c_str());
                        flux=(TChain*)curf->Get("FlatTree_VARS");
                        SetBranchAddresses();
                }
        }
	
	Log("Tool LoadNUISANCEEvent: Loading tchain entry "+to_string(tchainentrynum),v_message,verbosity);
	local_entry = flux->LoadTree(tchainentrynum);
	Log("Tool LoadNUISANCEEvent: localentry is "+to_string(local_entry),v_debug,verbosity);
	if(local_entry<0||local_entry!=tchainentrynum){
		Log("Tool LoadNUISANCEEvent: Reached end of file, returning",v_message,verbosity);
		m_data->vars.Set("StopLoop",1);
		return true;
	}
	flux->GetEntry(local_entry);
	curf = flux->GetCurrentFile();
	if(curf!=curflast || curflast==nullptr){
		TString curftstring = curf->GetName();
		currentfilestring = std::string(curftstring.Data());
		curflast=curf;
		Log("Tool LoadNUISANCEEvent: Opening new file \""+currentfilestring+"\"",v_debug,verbosity);
	}
	if (manualmatch){
		uint16_t MCTriggernum;
		m_data->Stores["ANNIEEvent"]->Get("MCTriggernum",MCTriggernum);
		if (MCTriggernum != 0){
			m_data->CStore.Set("NewGENIEEntry",false);
			return true;	//Don't evaluate new GENIE event for dealyed WCSim triggers
		} else {
			m_data->CStore.Set("NewGENIEEntry",true);
		}
	}
	
	// Expand out the neutrino event info
	// =======================================================
	// neutrino and nucleon information:
	bool iscc = (IsCC == '1');
	TLorentzVector* probemomentum = new TLorentzVector(-9999.,-9999.,-9999.,-9999.);
	TLorentzVector* targetnucleonmomentum = new TLorentzVector(-9999.,-9999.,-9999.,-9999.);
	for(int i = 0; i < ninitp; i++){
		if(neutrinopdg == pdg_init[i]){
			probemomentum->SetPxPyPzE(((double)px_init[i])*1000.,
				((double)py_init[i])*1000.,
				((double)pz_init[i])*1000.,
				((double)E_init[i])*1000.); //GeV->MeV
		}
		else if(pdg_init[i] == 2212 || pdg_init[i] == 2112){
			targetnucleonmomentum->SetPxPyPzE(((double)px_init[i])*1000.,
				((double)py_init[i])*1000.,
				((double)pz_init[i])*1000.,
				((double)E_init[i])*1000.); //GeV->MeV
			break;
		}
	}
	
        int numfsprotons  = 0;
        int numfsneutrons = 0;
        int numfspi0      = 0;
        int numfspiplus   = 0;
        int numfspipluscher  = 0; // reach Cherenkov threshold
        int numfspiminus     = 0;
        int numfspiminuscher = 0; // reach Cherenkov threshold
        int numfskplus       = 0;
        int numfskpluscher   = 0; // reach Cherenkov threshold
        int numfskminus      = 0;
        int numfskminuscher  = 0; // reach Cherenkov threshold

	//Loop over event particles
	double wclimit_pion  = 0.212; //water Cherenkov energy threshold GeV 
	double wclimit_proton  = 1.4; //water Cherenkov energy threshold GeV 
	double wclimit_kaon  = .74; //water Cherenkov energy threshold GeV **THe the charged and neutral kaons thresholds are the same to 2 sigfigs 
	for(int i = 0; i < nfsp; i++){
		if (pdg[i] == 2112) numfsneutrons++;
                else if (pdg[i] == 2212) { 
                        numfsprotons++;
                        if (E[i] > wclimit_proton) {    
            	                pdgs.push_back(pdg[i]); 
            	                Emag.push_back(((double)E[i])*1000.);
                        }
                }
                else if (pdg[i] == 211) {
                        numfspiplus++;
                        if (E[i] > wclimit_pion){    
                                pdgs.push_back(pdg[i]); 
                                Emag.push_back(((double)E[i])*1000.);
                                numfspipluscher++;
                        }
                }
                else if (pdg[i] == -211) {
                        numfspiminus++;
                        if (E[i] > wclimit_pion) {    
                                pdgs.push_back(pdg[i]); 
                                Emag.push_back(((double)E[i])*1000.);
                                numfspiminuscher++;
                        }
                }
                else if (pdg[i] == 111) numfspi0++;
                else if (pdg[i] == 321) {
                        numfskplus++;
                        if (E[i] > wclimit_kaon) {    
                                pdgs.push_back(pdg[i]); 
                                Emag.push_back(((double)E[i])*1000.);
                                numfskpluscher++;
			}
                }
                else if (pdg[i] == -321) {
                        numfskminus++;
                        if (E[i] > wclimit_kaon){    
                                pdgs.push_back(pdg[i]); 
                                Emag.push_back(((double)E[i])*1000.);
                                numfskminuscher++;
                        }
                }
        }

	eventPnu.SetX(probemomentum->Px()); //MeV
	eventPnu.SetY(probemomentum->Py()); //MeV
	eventPnu.SetZ(probemomentum->Pz()); //MeV
	
	for(int i = 0; i < nfsp; i++){
		if(pdg[i] == fsleptonpdg){
			fsleptonmomentum.SetX(((double)px[i])*1000.);
			fsleptonmomentum.SetY(((double)py[i])*1000.);
			fsleptonmomentum.SetZ(((double)pz[i])*1000.);
			break;
		}
	}
	nuIntxVtx_X = (double)fnuIntxVtx_X;
	nuIntxVtx_Y = (double)fnuIntxVtx_Y;
	nuIntxVtx_Z = (double)fnuIntxVtx_Z;
	eventq2     = (double)feventq2;
	eventq2qe   = (double)feventq2qe;
	eventw      = (double)feventw;
	eventbj_x   = (double)feventbj_x;
	eventelastic_y = (double)feventelastic_y;
	eventq0        = (double)feventq0;
	eventq3        = (double)feventq3;
	eventEnu       = ((double)feventEnu)*1000.;
        fsleptonenergy = ((double)ffsleptonenergy)*1000.;

	Log("Tool LoadNUISANCEEvent: Passing information to the NUISANCEEvent store",v_debug,verbosity);
	// Update the Store with all the current event information
	// =======================================================
	nuisancestore->Set("file",currentfilestring);
	nuisancestore->Set("evtnum",tchainentrynum);

	nuisancestore->Set("IsCC",iscc);	
	nuisancestore->Set("IsCCINC",IsCCINC);
	nuisancestore->Set("IsNCINC",IsNCINC);
	nuisancestore->Set("IsCCQE",IsCCQE);
	nuisancestore->Set("IsCC0pi",IsCC0pi);
	nuisancestore->Set("IsCCQELike",IsCCQELike);
	nuisancestore->Set("IsNCEL",IsNCEL);
	nuisancestore->Set("IsNC0pi",IsNC0pi);
	nuisancestore->Set("IsCCcoh",IsCCcoh);
	nuisancestore->Set("IsNCcoh",IsNCcoh);
	nuisancestore->Set("IsCC1pip",IsCC1pip);
	nuisancestore->Set("IsNC1pip",IsNC1pip);
	nuisancestore->Set("IsCC1pim",IsCC1pim);
	nuisancestore->Set("IsNC1pim",IsNC1pim);
	nuisancestore->Set("IsCC1pi0",IsCC1pi0);
	nuisancestore->Set("IsNC1pi0",IsNC1pi0);
	nuisancestore->Set("IsCC0piMINERvA",IsCC0piMINERvA);
	nuisancestore->Set("IsCC0Pi_T2K_AnaI",IsCC0Pi_T2K_AnaI);
	nuisancestore->Set("IsCC0Pi_T2K_AnaII",IsCC0Pi_T2K_AnaII);

	nuisancestore->Set("NeutCode",neutcode);
	nuisancestore->Set("NuIntxVtx_X",nuIntxVtx_X); //cm
	nuisancestore->Set("NuIntxVtx_Y",nuIntxVtx_Y); //cm
	nuisancestore->Set("NuIntxVtx_Z",nuIntxVtx_Z); //cm
	nuisancestore->Set("EventQ2",eventq2);
	nuisancestore->Set("EventQ2QE",eventq2qe);
	nuisancestore->Set("EventW2",eventw);
	nuisancestore->Set("EventBjx",eventbj_x);
	nuisancestore->Set("Eventy",eventelastic_y);
	nuisancestore->Set("TargetZ",TrueTargetZ);
	nuisancestore->Set("Eventq0",eventq0);
	nuisancestore->Set("Eventq3",eventq3);
	nuisancestore->Set("pdg_vector", pdgs);
	nuisancestore->Set("Emag_vector",Emag); //MeV
	nuisancestore->Set("NeutrinoEnergy",eventEnu); //MeV
	nuisancestore->Set("NeutrinoMomentum",eventPnu); //MeV/c
	nuisancestore->Set("NeutrinoPDG",neutrinopdg);
        nuisancestore->Set("FSLeptonPdg",fsleptonpdg);
        nuisancestore->Set("FSLeptonEnergy",fsleptonenergy); //MeV
        nuisancestore->Set("FSLeptonMomentum",fsleptonmomentum); //MeV/c
//        nuisancestore->Set("FSLeptonMomentumDir",fsleptonmomentumdir);
        nuisancestore->Set("NumFSProtons",numfsprotons);
        nuisancestore->Set("NumFSNeutrons",numfsneutrons);
        nuisancestore->Set("NumFSPi0",numfspi0);
        nuisancestore->Set("NumFSPiPlus",numfspiplus);
        nuisancestore->Set("NumFSPiPlusCher",numfspipluscher);
        nuisancestore->Set("NumFSPiMinus",numfspiminus);
        nuisancestore->Set("NumFSPiMinusCher",numfspiminuscher);
        nuisancestore->Set("NumFSKPlus",numfskplus);
        nuisancestore->Set("NumFSKPlusCher",numfskpluscher);
        nuisancestore->Set("NumFSKMinus",numfskminus);
        nuisancestore->Set("NumFSKMinusCher",numfskminuscher);
        nuisancestore->Set("ScaleFactor",scale_factor);
	tchainentrynum++;
	
	pdgs.clear();
	Emag.clear();
	
	Log("Tool LoadNUISANCEEvent: done",v_debug,verbosity);
	return true;

}

bool LoadNUISANCEEvent::Finalise(){
	
	if(flux){
		flux->ResetBranchAddresses();
		if (not loadwcsimsource) delete flux;	//only need to delete in case it was created with "new" --> only in not-loadwcsimource case. Otherwise double-free corruption
		flux=nullptr;
	}
	Log("Tool LoadNUISANCEEvent: exiting",v_debug,verbosity);
	return true;
}

void LoadNUISANCEEvent::SetBranchAddresses(){
	Log("Tool LoadNUISANCEEvent: Setting branch addresses",v_debug,verbosity);
	// neutrino event information
	flux->SetBranchAddress("ninitp",&ninitp);
	flux->SetBranchAddress("cc",&IsCC);
	flux->SetBranchAddress("PDGnu",&neutrinopdg);
	flux->SetBranchAddress("pdg_init",&pdg_init);
	flux->SetBranchAddress("px_init",&px_init);
	flux->SetBranchAddress("py_init",&py_init);
	flux->SetBranchAddress("pz_init",&pz_init);
	flux->SetBranchAddress("E_init",&E_init);
	flux->SetBranchAddress("nfsp",&nfsp);
	flux->SetBranchAddress("pdg",&pdg);
	flux->SetBranchAddress("px",&px);
	flux->SetBranchAddress("py",&py);
	flux->SetBranchAddress("pz",&pz);
	flux->SetBranchAddress("E",&E);
	flux->SetBranchAddress("q0",&feventq0);
	flux->SetBranchAddress("q3",&feventq3);
	flux->SetBranchAddress("Q2",&feventq2);
	flux->SetBranchAddress("Q2_QE",&feventq2qe);
	flux->SetBranchAddress("W",&feventw);
	flux->SetBranchAddress("x",&feventbj_x);
	flux->SetBranchAddress("y",&feventelastic_y);
	flux->SetBranchAddress("tgtz",&TrueTargetZ);
	flux->SetBranchAddress("Enu_true",&feventEnu);
	flux->SetBranchAddress("vtxx",&fnuIntxVtx_X);
	flux->SetBranchAddress("vtxy",&fnuIntxVtx_Y);
	flux->SetBranchAddress("vtxz",&fnuIntxVtx_Z);
	flux->SetBranchAddress("ELep",&ffsleptonenergy);
	flux->SetBranchAddress("PDGLep",&fsleptonpdg);
	flux->SetBranchAddress("Mode",&neutcode);
	flux->SetBranchAddress("fScaleFactor",&scale_factor);
	flux->SetBranchAddress("flagCCINC",&IsCCINC);
	flux->SetBranchAddress("flagNCINC",&IsNCINC);
	flux->SetBranchAddress("flagCCQE",&IsCCQE);
	flux->SetBranchAddress("flagCC0pi",&IsCC0pi);
	flux->SetBranchAddress("flagCCQELike",&IsCCQELike);
	flux->SetBranchAddress("flagNCEL",&IsNCEL);
	flux->SetBranchAddress("flagNC0pi",&IsNC0pi);
	flux->SetBranchAddress("flagCCcoh",&IsCCcoh);
	flux->SetBranchAddress("flagNCcoh",&IsNCcoh);
	flux->SetBranchAddress("flagCC1pip",&IsCC1pip);
	flux->SetBranchAddress("flagNC1pip",&IsNC1pip);
	flux->SetBranchAddress("flagCC1pim",&IsCC1pim);
	flux->SetBranchAddress("flagNC1pim",&IsNC1pim);
	flux->SetBranchAddress("flagCC1pi0",&IsCC1pi0);
	flux->SetBranchAddress("flagNC1pi0",&IsNC1pi0);
	flux->SetBranchAddress("flagCC0piMINERvA",&IsCC0piMINERvA);
	flux->SetBranchAddress("flagCC0Pi_T2K_AnaI",&IsCC0Pi_T2K_AnaI);
	flux->SetBranchAddress("flagCC0Pi_T2K_AnaII",&IsCC0Pi_T2K_AnaII);
//	flux->GetBranch("gmcrec")->SetAutoDelete(kTRUE);
}

