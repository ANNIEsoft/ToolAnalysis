/* vim:set noexpandtab tabstop=2 wrap */
#include "LoadReweightGenieEvent.h"

#include "TChain.h"
#include "TFile.h"
#include "TVector3.h"
#include "TLorentzVector.h"

#include "MRDspecs.hh"
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

2) gsimple files: /pnfs/annie/persistent/flux/gsimple_bnb/gsimple_beammc_annie_0000.root
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

James's Files
-------------
Genie 3.0.4.ub3 GNTP files: /pnfs/annie/persistent/simulation/genie3/G1810a0211a/standard/tank/gntp.1000.ghep.root
- these files contain genie::NtpMCEventRecord objects to describe details of the neutrino event
- input event information is passed to a genie::flux::GSimpleNtpEntry and genie::flux::GSimpleNtpNuMI object
*/

LoadReweightGenieEvent::LoadReweightGenieEvent():Tool(){}

Position TVector3ToPositionRW(TVector3 tvecin);
Direction TVector3ToDirectionRW(TVector3 tvecin);
FourVector TLorentzVectorToFourVectorRW(TLorentzVector tlvecin);

bool LoadReweightGenieEvent::Initialise(std::string configfile, DataModel &data){

	/////////////////// Useful header ///////////////////////
	if(configfile!="") m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////
	
	
	int evoffset;

	m_variables.Get("verbosity",verbosity);
	m_variables.Get("FluxVersion",fluxver); // flux version: 0=rhatcher files, 1=zarko files
	m_variables.Get("FileDir",filedir);
	m_variables.Get("FilePattern",filepattern);
	m_variables.Get("ManualFileMatching",manualmatch);
	m_variables.Get("EventOffset",evoffset);
	m_variables.Get("FileEvents",fileevents);
	m_variables.Get("OnGrid",on_grid);
	m_variables.Get("genie_module_label",fGenieModuleWeight);


	// create a store for holding Genie information to pass to downstream Tools
	// will be a single entry BoostStore containing a vector of single entry BoostStores
	geniestore = new BoostStore(true,0); // enable type-checking, BoostStore type binary
	m_data->Stores.emplace("GenieInfo",geniestore);

	// Open the flux files
	///////////////////////
	Log("Tool LoadReweightGenieEvent: Opening TChain",v_debug,verbosity);
	loadwcsimsource = (filepattern=="LoadWCSimTool");
	if(not loadwcsimsource && not manualmatch){
		// construct a new TChain and add all the files at once
		// this is for use of looking at stand alone Genie file, with no WCSim
		std::string inputfiles = filedir+"/"+filepattern;
		tchainentrynum=0;
		flux = new TChain("gtree");
		int numbytes = flux->Add(inputfiles.c_str());
		Log("Tool LoadReweightGenieEvent: Read "+to_string(numbytes)+" bytes loading TChain "+inputfiles,v_debug,verbosity);
		Log("Tool LoadReweightGenieEvent: Genie TChain has "+to_string(flux->GetEntries())+" entries",v_message,verbosity);
		SetBranchAddresses();
		tchainentrynum = evoffset;
		Log("LoadReweightGenieEvent tool: # of flux entries: "+std::to_string(flux->GetEntries()),v_message,verbosity);
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

		std::string inputfile = filedir+"/gntp."+wcsimfile+".ghep.root";
		curf=TFile::Open(inputfile.c_str());
                flux=(TChain*)curf->Get("gtree");
                SetBranchAddresses();
		tchainentrynum = wcsimevnumber*fileevents;
	}
 
	m_variables.Get("weight_functions_xsec",weight_options);
	std::string central_values;
	m_variables.Get("genie_central_values",central_values);

	//parse and tokenize array of strings that list weights
	std::stringstream weights_in(weight_options);
	std::string temp;
	while (weights_in.good()){
		std::getline(weights_in, temp, ',');
		weight_names.push_back(temp);
	}

	//Parse Genie central values configuration
	std::stringstream CV_in(central_values);
	std::string temp_token;
	float val;
	vector<string> CV_knob_names;
	vector<float> CV_knob_value;
	while (CV_in.good()){
		std::getline(CV_in, temp, '|');
		std::stringstream token_in(temp);
		while (token_in.good()){
			std::getline(token_in, temp_token, ':');
			CV_knob_names.push_back(temp_token);
			std::getline(token_in, temp_token, ':');
			std::stringstream to_val(temp_token);
			to_val >> val;
			CV_knob_value.push_back(val);
		}
	}
	// Map to store the CV knob settings
	std::map< genie::rew::GSyst_t, float > gsyst_to_cv_map;
	genie::rew::GSyst_t temp_knob;
	for (unsigned int i = 0; i < CV_knob_names.size(); i++ ) {
		if ( valid_knob_name(CV_knob_names[i], temp_knob) ) {
			if ( gsyst_to_cv_map.count( temp_knob ) ) {
				std::cout << "ERROR: Duplicate central values were configured for the " << CV_knob_names[i] << " GENIE knob.";
			}
			gsyst_to_cv_map[ temp_knob ] = CV_knob_value[i];
		}
	}

	reweightVector.resize(weight_names.size());

	//intitalize variables for Genie weight configurations
	std::string parameter, tokens, keys, values;
	std::string temp_pars, temp_sigs, temp_mins, temp_maxs;
	float sig;
	vector<string> str_par;
	vector<string> str_sig;
	vector<string> str_min;
	vector<string> str_max;
	//Get each parameter
	for(unsigned int i = 0; i < weight_names.size(); i++){
		xsecevwgh::xsecconfig xs_configs;
		m_variables.Get(weight_names[i],parameter);
		//Separate key-value pairs from each other
		std::stringstream param_in(parameter);
		while (param_in.good()){
			std::getline(param_in, tokens, '|');
			//Separate and save values from keys
			std::stringstream pair_in(tokens);
			while (pair_in.good()){
				std::getline(pair_in, keys, ':');
				std::getline(pair_in, values, ':');
				if(keys == "parameter_list"){
					values.erase(0,1);//strip out brackets
					values.erase(values.size()-1);
					std::stringstream pars_in(values);
					while(pars_in.good()){
						std::getline(pars_in, temp_pars, ',');//separate tokens
						temp_pars.erase(0,1);//strip out quotations
						temp_pars.erase(temp_pars.size()-1);
						xs_configs.parameter_list.push_back(temp_pars);
					}
				}
				else if(keys == "parameter_sigma"){
					values.erase(0,1);//strip out brackets
					values.erase(values.size()-1);
					std::stringstream sigs_in(values);
					while(sigs_in.good()){
						std::getline(sigs_in, temp_sigs, ',');//separate tokens
						std::stringstream to_float(temp_sigs);
						to_float >> sig;
						xs_configs.parameter_sigma.push_back(sig);
					}
				}
				else if(keys == "parameter_min"){
					values.erase(0,1);//strip out brackets
					values.erase(values.size()-1);
					std::stringstream mins_in(values);
					while(mins_in.good()){
						std::getline(mins_in, temp_mins, ',');//separate tokens
						std::stringstream to_float(temp_mins);
						to_float >> sig;
						xs_configs.parameter_min.push_back(sig);
					}
				}
				else if(keys == "parameter_max"){
					values.erase(0,1);//strip out brackets
					values.erase(values.size()-1);
					std::stringstream maxs_in(values);
					while(maxs_in.good()){
						std::getline(maxs_in, temp_maxs, ',');//separate tokens
						std::stringstream to_float(temp_maxs);
						to_float >> sig;
						xs_configs.parameter_max.push_back(sig);
					}
				}
				else if(keys == "type") xs_configs.type = values;
				else if(keys == "mode") xs_configs.mode = values;
				else if(keys == "random_seed"){
					std::stringstream to_int(values);
					int rand;
					to_int >> rand;
					xs_configs.random_seed = rand;
				}
				else if(keys == "number_of_multisims"){
					std::stringstream to_int(values);
					int nom;
					to_int >> nom;
					xs_configs.number_of_multisims = nom;
				}
			}
		}

		vector< genie::rew::GSyst_t > knobs_to_use;
		for ( const auto& knob_name : xs_configs.parameter_list ) {
			if ( valid_knob_name(knob_name, temp_knob) ) knobs_to_use.push_back( temp_knob );
		}

		// We need to add all of the tuned CV knobs to the list when configuring
		// the weight calculators and checking for incompatibilities. Maybe all of
		// the systematic variation knobs are fine to use together, but they also
		// need to be compatible with the tuned CV. To perform the check, copy the
		// list of systematic variation knobs to use and add all the CV knobs that
		// aren't already present.
		std::vector< genie::rew::GSyst_t > all_knobs_vec = knobs_to_use;
		for ( const auto& pair : gsyst_to_cv_map ) {
			genie::rew::GSyst_t cv_knob = pair.first;
			auto begin = all_knobs_vec.cbegin();
			auto end = all_knobs_vec.cend();
			if ( !std::count(begin, end, cv_knob) ) all_knobs_vec.push_back( cv_knob );
		}

		// Check that the enabled knobs (both systematic variations and knobs used
		// for the CV tune) are all compatible with each other. The std::map
		// returned by this function call provides information needed to fine-tune
		// the configuration of the GENIE weight calculators.
		std::map< std::string, int > modes_to_use = this->CheckForIncompatibleSystematics( all_knobs_vec );

		//check for same number of parameters as sigmas =========================

		if(xs_configs.mode == "pm1sigma" || xs_configs.mode == "minmax") xs_configs.number_of_multisims = 2; 
		else if(xs_configs.mode == "central_value") xs_configs.number_of_multisims = 1;

		CLHEP::HepJamesRandom engine(xs_configs.random_seed);

		reweightVector[i].resize(xs_configs.number_of_multisims);

		// Set up the weight calculators for each universe
		for ( auto& rwght : reweightVector[i] ) {
			this->SetupWeightCalculators( rwght, modes_to_use );
		}

		//prepare sigmas
		size_t num_usable_knobs = knobs_to_use.size();
		std::vector< std::vector<double> > reweightingSigmas( num_usable_knobs );

		for ( size_t k = 0; k < num_usable_knobs; ++k ) {
			reweightingSigmas[k].resize( xs_configs.number_of_multisims );
			genie::rew::GSyst_t current_knob = knobs_to_use.at( k );
			for ( size_t u = 0; u < xs_configs.number_of_multisims; ++u ) {
				if (xs_configs.mode == "multisim") {
					reweightingSigmas[k][u] = xs_configs.parameter_sigma[k] * CLHEP::RandGaussQ::shoot(&engine, 0., 1.);
				}
				else if (xs_configs.mode == "pm1sigma") {
					reweightingSigmas[k][u] = ( u == 0 ? 1. : -1. ); // u == 0 => 1; u == 1 => -1 if pm1sigma is specified
				}
				else if (xs_configs.mode == "minmax") {
					reweightingSigmas[k][u] = ( u == 0 ? xs_configs.parameter_max.at(k) : xs_configs.parameter_min.at(k) ); // u == 0 => max; u == 1 => min if minmax is specified
				}
				else if (xs_configs.mode == "central_value") {
					reweightingSigmas[k][u] = 0.; // we'll correct for a modified CV below if needed
				}
				else {
					reweightingSigmas[k][u] = xs_configs.parameter_sigma[k];
				}
				std::cout << "Set sigma for the " << genie::rew::GSyst::AsString( current_knob ) << " knob in universe #" << u << ". sigma = " << reweightingSigmas[k][u] << std::endl;
				// Add an offset if the central value for the current knob has been
				// configured (and is thus probably nonzero). Ignore this for minmax mode
				// (the limits should be chosen to respect a modified central value)
				if (xs_configs.mode != "minmax") {
					auto iter = gsyst_to_cv_map.find( current_knob );
					if ( iter != gsyst_to_cv_map.end() ) {
						reweightingSigmas[k][u] += iter->second;
						std::cout << "CV offset added to the " << genie::rew::GSyst::AsString( current_knob ) << " knob. New sigma for universe #" << u << " is " << reweightingSigmas[k][u] << std::endl;
					}
				}
			}
		}

		// Set of FHiCL weight calculator labels for which the tuned CV will be
		// ignored. If the name of the weight calculator doesn't appear in this set,
		// then variation weights will be thrown around the tuned CV.
		std::set< std::string > CALC_NAMES_THAT_IGNORE_TUNED_CV = { "RootinoFix" };

		// Don't adjust knobs to reflect the tuned CV if this weight calculator
		// should ignore those (as determined by whether it has one of the special
		// FHiCL names)
		if ( !CALC_NAMES_THAT_IGNORE_TUNED_CV.count(weight_names[i]) ) {
			// Add tuned CV knobs which have not been tweaked, and set them to their
			// modified central values. This ensures that weights are always thrown
			// around the modified CV.
			for ( const auto& pair : gsyst_to_cv_map ) {
				genie::rew::GSyst_t cv_knob = pair.first;
				float cv_value = pair.second;
				// If the current tuned CV knob is not present in the list of tweaked
				// knobs, then add it to the list with its tuned central value
				if ( !std::count(knobs_to_use.cbegin(), knobs_to_use.cend(), cv_knob) ) {
					++num_usable_knobs;
					knobs_to_use.push_back( cv_knob );
					// The tuned CV knob will take the same value in every universe
					reweightingSigmas.emplace_back(std::vector<double>(xs_configs.number_of_multisims, cv_value) );
				}
			}
		}

		// TODO: deal with parameters that have a priori bounds (e.g., FFCCQEVec,
		// which can vary on the interval [0,1])
		// Set up the knob values for each universe
		for ( size_t u = 0; u < reweightVector[i].size(); ++u ) {
			auto& rwght = reweightVector[i].at( u );
			genie::rew::GSystSet& syst = rwght.Systematics();
			for ( unsigned int k = 0; k < knobs_to_use.size(); ++k ) {
				genie::rew::GSyst_t knob = knobs_to_use.at( k );
				float twk_dial_value = reweightingSigmas.at( k ).at( u );
				syst.Set( knob, twk_dial_value );
			} // loop over tweaked knobs
			rwght.Reconfigure();
			rwght.Print();
		} // loop over universes
	}

	return true;

}

bool LoadReweightGenieEvent::Execute(){
	
	if(loadwcsimsource && !manualmatch){
                // retrieve the genie file and entry number from the LoadWCSim tool
                std::string inputfiles;
                get_ok = m_data->CStore.Get("GenieFile",inputfiles);
                if(!get_ok){
                        if(verbosity) std::cout << "Tool LoadReweightGenieEvent: Failed to find GenieFile in CStore" << std::endl;
                        return false;
                }
                if(filedir!="NA"){
                        std::string genie_prefix = "gntp.";
                        inputfiles.erase(0, inputfiles.find(genie_prefix));
                        inputfiles = filedir+"/"+inputfiles;
                        if(verbosity) std::cout << "Tool LoadReweightGenieEvent: Loading Genie file: " << inputfiles << std::endl;
                }
		get_ok = m_data->CStore.Get("GenieEntry",tchainentrynum);
                if(!get_ok){
                        Log("Tool LoadReweightGenieEvent: Failed to find GenieEntry in CStore",v_error,verbosity);
                        return false;
                }
                std::string curfname = ((curf) ? curf->GetName() : "");
                // check if this is a new file
                if(inputfiles!=curfname){
                        // we need to load the new file
                        if(flux) flux->ResetBranchAddresses();
                        if(curf) curf->Close();
                        Log("Tool LoadReweightGenieEvent: Loading new file "+inputfiles,v_debug,verbosity);
                        curf=TFile::Open(inputfiles.c_str());
                        flux=(TChain*)curf->Get("gtree");
                        SetBranchAddresses();
                }
        }
	
	Log("Tool LoadReweightGenieEvent: Loading tchain entry "+to_string(tchainentrynum),v_message,verbosity);
	local_entry = flux->LoadTree(tchainentrynum);
	Log("Tool LoadReweightGenieEvent: localentry is "+to_string(local_entry),v_debug,verbosity);
	if(local_entry<0||local_entry!=tchainentrynum){
		Log("Tool LoadReweightGenieEvent: Reached end of file, returning",v_message,verbosity);
		m_data->vars.Set("StopLoop",1);
		return true;
	}
	flux->GetEntry(local_entry);
	curf = flux->GetCurrentFile();
	if(curf!=curflast || curflast==nullptr){
		TString curftstring = curf->GetName();
		currentfilestring = std::string(curftstring.Data());
		curflast=curf;
		Log("Tool LoadReweightGenieEvent: Opening new file \""+currentfilestring+"\"",v_debug,verbosity);
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
	
	// header only contains the event number
	genie::NtpMCRecHeader hdr = genieintx->hdr;
	unsigned int genie_event_num = hdr.ievent;
	
	// all neutrino intx details are in the event record
	genie::EventRecord* gevtRec = genieintx->event;
	
	if(fluxver==0){
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
		parentprodmedium = gnumipassthruentry->ppmedium; //Seems to be 0 all the time? Not registered in the materials table, numbers start at 5...
		parentpdgattgtexit = gnumipassthruentry->tptype;
		parenttgtexitmom_x = gnumipassthruentry->tpx;
		parenttgtexitmom_y = gnumipassthruentry->tpy;
		parenttgtexitmom_z = gnumipassthruentry->tpz;
		pcodes = gnumipassthruentry->pcodes;	//pcodes = 0--> GEANT particle codes, 1--> converted PDG particle codes		

		// convenience type conversions
		parentdecayvtx = Position(parentdecayvtx_x,parentdecayvtx_y,parentdecayvtx_z);
		parentdecaymom = Direction(parentdecaymom_x,parentdecaymom_y,parentdecaymom_z);
		parentprodmom = Direction(parentprodmom_x,parentprodmom_y,parentprodmom_z);
		parenttgtexitmom = Direction(parenttgtexitmom_x,parenttgtexitmom_y,parenttgtexitmom_z);
		//parenttypestring = (fluxstage==0) ? GnumiToString(parentpdg) : PdgToString(parentpdg);
		//parenttypestringattgtexit = (fluxstage==0) ? 
		parenttypestring = (pcodes==0) ? GnumiToString(parentpdg) : PdgToString(parentpdg);
		parenttypestringattgtexit = (pcodes==0) ? 
			GnumiToString(parentpdgattgtexit) : PdgToString(parentpdgattgtexit);
		parentdecaystring = DecayTypeToString(parentdecaymode);
		parentprodmediumstring = MediumToString(parentprodmedium);
		
	} else {
		// FLUXVER 1 - genie::flux::GSimpleNtpEntry
		// ========================================
		// extract the target intx details from the GSimpleNtpNuMI object
		Log("Tool LoadReweightGenieEvent: Retrieving interaction info from GSimpleNtpNuMI object",v_debug,verbosity);
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
		parentprodmedium = gsimplenumientry->ppmedium;
		parentpdgattgtexit = gsimplenumientry->tptype;
		parenttgtexitmom_x = gsimplenumientry->tpx;
		parenttgtexitmom_y = gsimplenumientry->tpy;
		parenttgtexitmom_z = gsimplenumientry->tpz;

                fluxentryno = gsimplenumientry->entryno;
                fluxrun = gsimplenumientry->run;
                double flux_energy = gsimpleentry->E;
                fluxnenergyn = flux_energy;
                fluxnenergyf = flux_energy;
                fluxevtno = gsimplenumientry->evtno;
                fluxntype = gsimpleentry->pdg;
                fluxnimpwt = gsimpleentry->wgt;
		
		// convenience type conversions
		parentdecayvtx = Position(parentdecayvtx_x,parentdecayvtx_y,parentdecayvtx_z);
		parentdecaymom = Direction(parentdecaymom_x,parentdecaymom_y,parentdecaymom_z);
		parentprodmom = Direction(parentprodmom_x,parentprodmom_y,parentprodmom_z);
		parenttgtexitmom = Direction(parenttgtexitmom_x,parenttgtexitmom_y,parenttgtexitmom_z);
		parenttypestring = PdgToString(parentpdg);
		parenttypestringattgtexit = PdgToString(parentpdgattgtexit);
		parentdecaystring = DecayTypeToString(parentdecaymode);
		parentprodmediumstring = MediumToString(parentprodmedium);
		
	}
	
	// neutrino interaction info
	genie::Interaction* genieint = gevtRec->Summary();
	//cout<<"scraping interaction info"<<endl;
	GenieInfo thegenieinfo;
	Log("Tool LoadReweightGenieEvent: Filling GenieInfo struct",v_debug,verbosity);
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
	
	eventq2=thegenieinfo.Q2;                  //MeV
	eventEnu=thegenieinfo.probeenergy;        //MeV
	eventPnu=thegenieinfo.probethreemomentum; //MeV
	neutrinopdg=thegenieinfo.probepdg;
	muonenergy=thegenieinfo.fsleptonenergy;   //MeV
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
	
	fsleptonname = std::string(thegenieinfo.fsleptonname);
        fsleptonenergy = thegenieinfo.fsleptonenergy;     //MeV
        fsleptonpdg = thegenieinfo.fsleptonpdg;
        fsleptonm = thegenieinfo.fsleptonm;               //MeV
        fsleptonmomentum = thegenieinfo.fsleptonmomentum; //MeV
//        fsleptonmomentumdir = thegenieinfo.fsleptonmomentumdir;
        fsleptonvtx = thegenieinfo.fsleptonvtx; // cm
        fsleptont = thegenieinfo.fsleptont;     // ns

	// this data does not appear to be populated...
	// Edit: Maybe due to the numbers for the exclusive tag being evaluated before Final State Interactions
	// Compare e.g. documentation here: https://internal.dunescience.org/doxygen/classgenie_1_1XclsTag.html
	/*
	numfsprotons = thegenieinfo.numfsprotons = genieint->ExclTag().NProtons();
	numfsneutrons = thegenieinfo.numfsneutrons = genieint->ExclTag().NNeutrons();
	numfspi0 = thegenieinfo.numfspi0 = genieint->ExclTag().NPi0();
	numfspiplus = thegenieinfo.numfspiplus = genieint->ExclTag().NPiPlus();
	numfspiminus = thegenieinfo.numfspiminus = genieint->ExclTag().NPiMinus();
	*/
	//The following is more cumbersome, but seems to work (we count the number of final state particles by hand)
        numfsprotons = thegenieinfo.numfsprotons;
        numfsneutrons =  thegenieinfo.numfsneutrons;
        numfspi0 =  thegenieinfo.numfspi0;
        numfspiplus=  thegenieinfo.numfspiplus;
        numfspipluscher = thegenieinfo.numfspipluscher;
        numfspiminus = thegenieinfo.numfspiminus;
        numfspiminuscher = thegenieinfo.numfspiminuscher;
        numfskplus = thegenieinfo.numfskplus;
        numfskpluscher = thegenieinfo.numfskpluscher;
        numfskminus = thegenieinfo.numfskminus;
        numfskminuscher = thegenieinfo.numfskminuscher;

	// Expand out the neutrino event and flux info
	// =======================================================
	// header only contains the event number

	//assume 1 neutrino per event, no pile-up
	std::vector<std::vector<double>> weights(weight_names.size());
	std::map<std::string, std::vector<double>> reweights;

	for(unsigned int i = 0; i < weight_names.size(); i++){
		unsigned int num_knobs = reweightVector[i].size();

		genie::Kinematics* kine_ptr = genieint->KinePtr();
		// Final lepton mass
		double ml = genieint->FSPrimLepton()->Mass();
		// Final lepton 4-momentum
		const TLorentzVector& p4l = kine_ptr->FSLeptonP4();
		// Final lepton kinetic energy
		double Tl = p4l.E() - ml;
		// Final lepton scattering cosine
		double ctl = p4l.CosTheta();

		kine_ptr->SetKV( kKVTl, Tl );
		kine_ptr->SetKV( kKVctl, ctl );

		// All right, the event record is fully ready. Now ask the GReWeight
		// objects to compute the weights.
		weights[i].resize( num_knobs );
		for (unsigned int k = 0; k < num_knobs; ++k ) {
			weights[i][k] = reweightVector[i].at(k).CalcWeight( *gevtRec );
		}

		reweights.insert(std::pair<std::string,std::vector<double>>(weight_names[i],weights[i]));
	}

	Log("Tool LoadReweightGenieEvent: Passing information to the GenieEvent store",v_debug,verbosity);
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
	geniestore->Set("ParentProdMedium",parentprodmedium);
	geniestore->Set("ParentProdMediumString",parentprodmediumstring);
	geniestore->Set("ParentPdgAtTgtExit",parentpdgattgtexit);
	geniestore->Set("ParentTypeAtTgtExitString",parenttypestringattgtexit);
	geniestore->Set("ParentTgtExitMom",parenttgtexitmom);
	geniestore->Set("ParentTgtExitMom_X",parenttgtexitmom_x);
	geniestore->Set("ParentTgtExitMom_Y",parenttgtexitmom_y);
	geniestore->Set("ParentTgtExitMom_Z",parenttgtexitmom_z);
        geniestore->Set("ParentEntryNo",fluxentryno);
        geniestore->Set("ParentRunNo",fluxrun);
        geniestore->Set("ParentNEnergyN",fluxnenergyn);
        geniestore->Set("ParentNEnergyF",fluxnenergyf);
        geniestore->Set("ParentEventNo",fluxevtno);
        geniestore->Set("ParentNType",fluxntype);
        geniestore->Set("ParentWgt",fluxnimpwt);
	
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
	geniestore->Set("NeutrinoMomentum",eventPnu);
	geniestore->Set("NeutrinoPDG",neutrinopdg);
	geniestore->Set("MuonEnergy",muonenergy);
	geniestore->Set("MuonAngle",muonangle);
	geniestore->Set("FSLeptonName",fsleptonname);
        geniestore->Set("FSLeptonPdg",fsleptonpdg);
        geniestore->Set("FSLeptonMass",fsleptonm);
        geniestore->Set("FSLeptonEnergy",fsleptonenergy);
        geniestore->Set("FSLeptonMomentum",fsleptonmomentum);
//        geniestore->Set("FSLeptonMomentumDir",fsleptonmomentumdir);
        geniestore->Set("FSLeptonVertex",fsleptonvtx);
        geniestore->Set("FSLeptonTime",fsleptont);
        geniestore->Set("NumFSProtons",numfsprotons);
        geniestore->Set("NumFSNeutrons",numfsneutrons);
        geniestore->Set("NumFSPi0",numfspi0);
        geniestore->Set("NumFSPiPlus",numfspiplus);
        geniestore->Set("NumFSPiPlusCher",numfspipluscher);
        geniestore->Set("NumFSPiMinus",numfspiminus);
        geniestore->Set("NumFSPiMinusCher",numfspiminuscher);
        geniestore->Set("NumFSKPlus",numfskplus);
        geniestore->Set("NumFSKPlusCher",numfskpluscher);
        geniestore->Set("NumFSKMinus",numfskminus);
        geniestore->Set("NumFSKMinusCher",numfskminuscher);
	geniestore->Set("GenieInfo",thegenieinfo);
	m_data->Stores.at("ANNIEEvent")->Set("xsec_weights",reweights);
	//geniestore->Set("TheGenieInfoPtr",&thegenieinfo,false);
	//intptr_t thegenieinfoptr = reinterpret_cast<intptr_t>(&thegenieinfo);
	//m_data->CStore.Set("TheGenieInfoPtr2",thegenieinfoptr);
	tchainentrynum++;
	
	Log("Tool LoadReweightGenieEvent: Clearing genieintx",v_debug,verbosity);
	genieintx->Clear(); // REQUIRED TO PREVENT MEMORY LEAK
	
	Log("Tool LoadReweightGenieEvent: done",v_debug,verbosity);
	return true;
}

bool LoadReweightGenieEvent::Finalise(){
	
	if(flux){
		flux->ResetBranchAddresses();
		if (not loadwcsimsource) delete flux;	//only need to delete in case it was created with "new" --> only in not-loadwcsimource case. Otherwise double-free corruption
		flux=nullptr;
	}
	Log("Tool LoadReweightGenieEvent: exiting",v_debug,verbosity);
	return true;
}


void LoadReweightGenieEvent::SetBranchAddresses(){
	Log("Tool LoadReweightGenieEvent: Setting branch addresses",v_debug,verbosity);
	// neutrino event information
	flux->SetBranchAddress("gmcrec",&genieintx);
	flux->GetBranch("gmcrec")->SetAutoDelete(kTRUE);
	// input (BNB intx) event information
	if(fluxver==0){   // rhatcher files
		flux->SetBranchAddress("flux",&gnumipassthruentry);
		flux->GetBranch("flux")->SetAutoDelete(kTRUE);
	} else {          // zarko files
	//	flux->Print();
		flux->SetBranchAddress("numi",&gsimplenumientry);
		flux->GetBranch("numi")->SetAutoDelete(kTRUE);
		flux->SetBranchAddress("simple",&gsimpleentry);
		flux->GetBranch("simple")->SetAutoDelete(kTRUE);
		flux->SetBranchAddress("aux",&gsimpleauxinfo);
		flux->GetBranch("aux")->SetAutoDelete(kTRUE);
	}
}

void LoadReweightGenieEvent::GetGenieEntryInfo(genie::EventRecord* gevtRec, genie::Interaction* genieint, GenieInfo &thegenieinfo, bool printneutrinoevent){
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
	/*Int_t*/ thegenieinfo.nuanceinteractioncode = genie::utils::ghep::NuanceReactionCode(gevtRec);
	TLorentzVector* IntxVtx = gevtRec->Vertex();
	/*Double_t*/ thegenieinfo.Intx_x = IntxVtx->X() * 100.;   // same info as nuvtx in g4dirt file
	/*Double_t*/ thegenieinfo.Intx_y = IntxVtx->Y() * 100.;   // GENIE uses meters
	/*Double_t*/ thegenieinfo.Intx_z = IntxVtx->Z() * 100.;   // GENIE uses meters
	/*Double_t*/ thegenieinfo.Intx_t = IntxVtx->T() * 1000000000.; // GENIE uses seconds
	
	// neutrino information:
	/*Double_t*/ thegenieinfo.probeenergy = genieint->InitState().ProbeE(genie::kRfLab)*1000.;  //GeV->MeV
	/*Int_t*/ thegenieinfo.probepdg = genieint->InitState().Probe()->PdgCode();
	/*TString*/ thegenieinfo.probepartname = genieint->InitState().Probe()->GetName();
	TLorentzVector* probemomentum = new TLorentzVector(gevtRec->Probe()->P4()->Px()*1000.,
			gevtRec->Probe()->P4()->Py()*1000.,
			gevtRec->Probe()->P4()->Pz()*1000.,
			gevtRec->Probe()->P4()->E()*1000.); //GeV->MeV
	if(probemomentum->E()!=thegenieinfo.probeenergy){
		logmessage = "LoadReweightGenieEvent Tool: WARNING, Probe energy from probemomentum.E and ProbeE differ!";
		logmessage+= "ProbeE = "+to_string(thegenieinfo.probeenergy);
		logmessage+= ", ProbeMomentum[0] = "+to_string(probemomentum->E());
		Log(logmessage,v_warning,verbosity);
	}
	/*Direction*/ thegenieinfo.probethreemomentum = TVector3ToDirectionRW(probemomentum->Vect());
	/*TVector3*/ thegenieinfo.probemomentumdir = thegenieinfo.probethreemomentum.Unit();
	/*Double_t*/ thegenieinfo.probeanglex = 
		TMath::ATan(thegenieinfo.probethreemomentum.X()/thegenieinfo.probethreemomentum.Z());
	/*Double_t*/ thegenieinfo.probeangley = 
		TMath::ATan(thegenieinfo.probethreemomentum.Y()/thegenieinfo.probethreemomentum.Z());
	/*Double_t*/ thegenieinfo.probeangle = TMath::Max(thegenieinfo.probeanglex,thegenieinfo.probeangley);
	// n.b. genieint->InitState().Probe != gevtRec->Probe()
	
	// target nucleon:
	genie::GHepParticle* targetnucleon = gevtRec->HitNucleon();
	/*int*/ thegenieinfo.targetnucleonpdg = genieint->InitState().Tgt().HitNucPdg();
	/*TString*/ thegenieinfo.targetnucleonname="";
	if ( genie::pdg::IsNeutronOrProton(thegenieinfo.targetnucleonpdg) ) {
		TParticlePDG * p = genie::PDGLibrary::Instance()->Find(thegenieinfo.targetnucleonpdg);
		thegenieinfo.targetnucleonname = p->GetName();
	} else {
		thegenieinfo.targetnucleonname = std::to_string(thegenieinfo.targetnucleonpdg);
	}
	/*TVector3*/ thegenieinfo.targetnucleonthreemomentum=Direction(0.,0.,0.);
	/*Double_t*/ thegenieinfo.targetnucleonenergy=0.;
	if(targetnucleon){
		TLorentzVector* targetnucleonmomentum = new TLorentzVector(targetnucleon->P4()->Px()*1000.,
				targetnucleon->P4()->Py()*1000.,
				targetnucleon->P4()->Pz()*1000.,
				targetnucleon->P4()->E()*1000.); //GeV->MeV
		thegenieinfo.targetnucleonthreemomentum = TVector3ToDirectionRW(targetnucleonmomentum->Vect());
		thegenieinfo.targetnucleonenergy = targetnucleonmomentum->Energy();
	}
	
	// target nucleus:
	/*Int_t*/ thegenieinfo.targetnucleuspdg = genieint->InitState().Tgt().Pdg();
	TParticlePDG* targetnucleus = 
		genie::PDGLibrary::Instance()->Find(thegenieinfo.targetnucleuspdg);
	/*TString*/ thegenieinfo.targetnucleusname = "unknown";
	if(targetnucleus){ thegenieinfo.targetnucleusname = targetnucleus->GetName(); }
	/*Int_t*/ thegenieinfo.targetnucleusZ = genieint->InitState().Tgt().Z();
	/*Int_t*/ thegenieinfo.targetnucleusA = genieint->InitState().Tgt().A();
	
	// remnant nucleus:
	int remnucpos = gevtRec->RemnantNucleusPosition(); 
	/*TString*/ thegenieinfo.remnantnucleusname="n/a";
	/*Double_t*/ thegenieinfo.remnantnucleusenergy=-1.;
	if(remnucpos>-1){
		thegenieinfo.remnantnucleusname = gevtRec->Particle(remnucpos)->Name();
		thegenieinfo.remnantnucleusenergy = gevtRec->Particle(remnucpos)->Energy()*1000.; //GeV->MeV
	}
	
	// final state lepton:
	       int fsleppos = gevtRec->FinalStatePrimaryLeptonPosition();
        /*TString*/ thegenieinfo.fsleptonname="n/a";
        /*Double_t*/ thegenieinfo.fsleptonenergy=-1.;
        /*Int_t*/ thegenieinfo.fsleptonpdg=-1;
        /*Double_t*/ thegenieinfo.fsleptonm=-1.;
        /*Double_t*/ thegenieinfo.fsleptont=-1.;
        /*TVector3*/ thegenieinfo.fsleptonmomentum=Direction(0.,0.,0.);
        /*TVector3*/ thegenieinfo.fsleptonmomentumdir=Direction(0.,0.,0.);
        /*TVector3*/ thegenieinfo.fsleptonvtx=Position(0.,0.,0.);
        if(fsleppos>-1){
                thegenieinfo.fsleptonname = gevtRec->Particle(fsleppos)->Name();
                thegenieinfo.fsleptonenergy = gevtRec->Particle(fsleppos)->Energy()*1000.; //GeV->MeV
                thegenieinfo.fsleptonpdg = gevtRec->Particle(fsleppos)->Pdg();
                thegenieinfo.fsleptonm = gevtRec->Particle(fsleppos)->Mass()*1000.; //GeV->MeV
                TLorentzVector* fsleptonmom = new TLorentzVector(gevtRec->Particle(fsleppos)->Px()*1000.,
                                gevtRec->Particle(fsleppos)->Py()*1000.,
                                gevtRec->Particle(fsleppos)->Pz()*1000.,
				gevtRec->Particle(fsleppos)->E()*1000.); //GeV->MeV
		thegenieinfo.fsleptonmomentum = TVector3ToDirectionRW(fsleptonmom->Vect());
                thegenieinfo.fsleptonmomentumdir = thegenieinfo.fsleptonmomentum.Unit();
                thegenieinfo.fsleptonvtx = Position(gevtRec->Particle(fsleppos)->Vx()*100.,
                                gevtRec->Particle(fsleppos)->Vy()*100.,
                                gevtRec->Particle(fsleppos)->Vz()*100.); //meters -> cm
                thegenieinfo.fsleptont = gevtRec->Particle(fsleppos)->Vt()*1000000000.; //sec -> ns
        }

	// other remnants: TEMP FIX: this information is NOT being correctly read in
	// Edit: Maybe due to the numbers for the exclusive tag being evaluated before Final State Interactions
	// Compare e.g. documentation here: https://internal.dunescience.org/doxygen/classgenie_1_1XclsTag.html
	/*Int_t*/ //thegenieinfo.numfsprotons = genieint->ExclTag().NProtons();
	/*Int_t*/ //thegenieinfo.numfsneutrons = genieint->ExclTag().NNeutrons();
	/*Int_t*/ //thegenieinfo.numfspi0 = genieint->ExclTag().NPi0();
	/*Int_t*/ //thegenieinfo.numfspiplus = genieint->ExclTag().NPiPlus();
	/*Int_t*/ //thegenieinfo.numfspiminus = genieint->ExclTag().NPiMinus();
	
	//The following is more cumbersome, but seems to work (we count the number of final state particles by hand)
        thegenieinfo.numfsprotons = 0;
        thegenieinfo.numfsneutrons = 0;
        thegenieinfo.numfspi0 = 0;
        thegenieinfo.numfspiplus= 0;
        thegenieinfo.numfspipluscher = 0;
        thegenieinfo.numfspiminus = 0;
        thegenieinfo.numfspiminuscher = 0;
        thegenieinfo.numfskplus = 0;
        thegenieinfo.numfskpluscher = 0;
        thegenieinfo.numfskminus = 0;
        thegenieinfo.numfskminuscher = 0;

	TObjArrayIter iter(gevtRec);
	genie::GHepParticle * p = 0;

	//Loop over event particles
        while ((p = dynamic_cast<genie::GHepParticle *>(iter.Next()))) {

                int pdgc = p->Pdg();
                int status = p->Status();
                double wclimit = 0.160; //water Cherenkov momentum threshold GeV

                if (status != genie::kIStStableFinalState) continue;

                if (pdgc == genie::kPdgNeutron) thegenieinfo.numfsneutrons++;
                else if (pdgc == genie::kPdgProton) thegenieinfo.numfsprotons++;
                else if (pdgc == genie::kPdgPiP) {
                        thegenieinfo.numfspiplus++;
                        double pipx = p->Px();
                        double pipy = p->Py();
                        double pipz = p->Pz();
                        if (std::sqrt(pipx*pipx + pipy*pipy + pipz*pipz) > wclimit) thegenieinfo.numfspipluscher++;
                }
                else if (pdgc == genie::kPdgPiM) {
                        thegenieinfo.numfspiminus++;
                        double pipx = p->Px();
                        double pipy = p->Py();
                        double pipz = p->Pz();
                        if (std::sqrt(pipx*pipx + pipy*pipy + pipz*pipz) > wclimit) thegenieinfo.numfspiminuscher++;
                }
                else if (pdgc == genie::kPdgPi0) thegenieinfo.numfspi0++;
                else if (pdgc == genie::kPdgKP) {
                        thegenieinfo.numfskplus++;
                        double pipx = p->Px();
                        double pipy = p->Py();
                        double pipz = p->Pz();
                        if (std::sqrt(pipx*pipx + pipy*pipy + pipz*pipz) > wclimit) thegenieinfo.numfskpluscher++;
                }
                else if (pdgc == genie::kPdgKM) {
                        thegenieinfo.numfskminus++;
                        double pipx = p->Px();
                        double pipy = p->Py();
                        double pipz = p->Pz();
                        if (std::sqrt(pipx*pipx + pipy*pipy + pipz*pipz) > wclimit) thegenieinfo.numfskminuscher++;
                }
        }
	
	// kinematic information
	Double_t NucleonM = genie::constants::kNucleonMass*1000.; //GeV->MeV
	// Calculate kinematic variables "as an experimentalist would measure them; 
	// neglecting fermi momentum and off-shellness of bound nucleons"
	TLorentzVector* k1 = new TLorentzVector(gevtRec->Probe()->P4()->Px()*1000.,
			gevtRec->Probe()->P4()->Py()*1000.,
			gevtRec->Probe()->P4()->Pz()*1000.,
			gevtRec->Probe()->P4()->E()*1000.); //GeV->MeV
	TLorentzVector* k2 = new TLorentzVector(gevtRec->FinalStatePrimaryLepton()->P4()->Px()*1000.,
			gevtRec->FinalStatePrimaryLepton()->P4()->Py()*1000.,
			gevtRec->FinalStatePrimaryLepton()->P4()->Pz()*1000.,
			gevtRec->FinalStatePrimaryLepton()->P4()->E()*1000.); //GeV->MeV
	/*Double_t*/ thegenieinfo.costhfsl = TMath::Cos( k2->Vect().Angle(k1->Vect()) );
	/*Double_t*/ thegenieinfo.fslangle = k2->Vect().Angle(k1->Vect());
	// q=k1-k2, 4-p transfer
	/*TLorentzVector*/ thegenieinfo.q = TLorentzVectorToFourVectorRW((*k1)-(*k2));
//	/*Double_t*/ thegenieinfo.Q2 = genieint->Kine().Q2();  // not set in our GENIE files!
	// momemtum transfer
	/*Double_t*/ thegenieinfo.Q2 = -1 * thegenieinfo.q.M2();
	// E transfer to the nucleus
	/*Double_t*/ thegenieinfo.Etransf = (targetnucleon) ? thegenieinfo.q.E() : -1;
	// Bjorken x
	/*Double_t*/ thegenieinfo.x = 
		(targetnucleon) ? 0.5*thegenieinfo.Q2/(NucleonM*thegenieinfo.Etransf) : -1;
	// Inelasticity, y = q*P1/k1*P1
	/*Double_t*/ thegenieinfo.y = 
		(targetnucleon) ? thegenieinfo.Etransf/k1->Energy() : -1;
	// Hadronic Invariant mass ^ 2
	/*Double_t*/ thegenieinfo.W2 = 
	(targetnucleon) ? (NucleonM*NucleonM + 2*NucleonM*thegenieinfo.Etransf - thegenieinfo.Q2) : -1;
	
	if(printneutrinoevent){
		cout<<"This was a "<< thegenieinfo.procinfostring <<" (neut code "<<thegenieinfo.neutinteractioncode
			<<") interaction of a "
			<<thegenieinfo.probeenergy<<"MeV " << thegenieinfo.probepartname << " on a "; 
		
		if( thegenieinfo.targetnucleonpdg==2212 || thegenieinfo.targetnucleonpdg==2122 ){
			cout<<thegenieinfo.targetnucleonname<<" in a ";
		} else {
			cout<<"PDG-Code " << thegenieinfo.targetnucleonpdg<<" in a ";
		}
		
		if( thegenieinfo.targetnucleusname!="unknown"){ cout<<thegenieinfo.targetnucleusname<<" nucleus, "; }
		else { cout<<"Z=["<<thegenieinfo.targetnucleusZ<<","<<thegenieinfo.targetnucleusA<<"] nucleus, "; }
		
		if(remnucpos>-1){
			cout<<"producing a "<<thegenieinfo.remnantnucleusenergy<<"MeV "<<thegenieinfo.remnantnucleusname;
		} else { cout<<"with no remnant nucleus"; }  // DIS on 16O produces no remnant nucleus?!
		
		if(fsleppos>-1){
			cout<<" and a "<<thegenieinfo.fsleptonenergy<<"MeV "<<thegenieinfo.fsleptonname<<endl;
		} else{ cout<<" and no final state leptons"<<endl; }
		
		cout<<endl<<"Q^2 was "<<thegenieinfo.Q2<<"(MeV/c)^2, with final state lepton"
			<<" ejected at Cos(θ)="<<thegenieinfo.costhfsl<<endl;
		cout<<"Additional final state particles included "<<endl;
		cout<< " N(p) = "  << thegenieinfo.numfsprotons
			<< " N(n) = "    << thegenieinfo.numfsneutrons
			<< endl
			<< " N(pi^0) = " << thegenieinfo.numfspi0
                        << " N(pi^+)(over CT) = " << thegenieinfo.numfspiplus << "(" << thegenieinfo.numfspipluscher << ")"
                        << " N(pi^-)(over CT) = " << thegenieinfo.numfspiminus << "(" << thegenieinfo.numfspiminuscher << ")"
                        << " N(K^+)(over CT) = " << thegenieinfo.numfskplus << "(" << thegenieinfo.numfskpluscher << ")"
                        << " N(K^-)(over CT) = " << thegenieinfo.numfskminus << "(" << thegenieinfo.numfskminuscher << ")"
			<<endl;
	}
}

// type conversion functions:
std::string LoadReweightGenieEvent::GnumiToString(int code){
	if(gnumicodetoname.size()==0) GenerateGnumiMap();
	if(gnumicodetoname.count(code)!=0){
		return gnumicodetoname.at(code);
	} else {
		cerr<<"unknown gnumi code "<<code<<endl;
		return std::to_string(code);
	}
}

std::string LoadReweightGenieEvent::PdgToString(int code){
	if(pdgcodetoname.size()==0) GeneratePdgMap();
	if(pdgcodetoname.count(code)!=0){
		return pdgcodetoname.at(code);
	} else {
		cerr<<"unknown pdg code "<<code<<endl;
		return std::to_string(code);
	}
}

std::string LoadReweightGenieEvent::DecayTypeToString(int code){
	if(decaymap.size()==0) GenerateDecayMap();
	if(decaymap.count(code)!=0){
		return decaymap.at(code);
	} else {
		cerr<<"unknown decay code "<<code<<endl;
		return std::to_string(code);
	}
}

std::string LoadReweightGenieEvent::MediumToString(int code){
	if(mediummap.size()==0) GenerateMediumMap();
	if(mediummap.count(code)!=0){
		return mediummap.at(code);
	} else {
		Log("LoadReweightGenieEvent: unknown medium "+std::to_string(code),v_warning,verbosity);
		return std::to_string(code);
	}
}

std::map<int,std::string>* LoadReweightGenieEvent::GenerateGnumiMap(){
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

std::map<int,std::string>* LoadReweightGenieEvent::GeneratePdgMap(){
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

std::map<int,std::string>* LoadReweightGenieEvent::GenerateDecayMap(){
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

std::map<int,std::string>* LoadReweightGenieEvent::GenerateMediumMap(){
	//gnumi material mapping taken from table 10 in https://minos-docdb.fnal.gov/cgi-bin/sso/RetrieveFile?docid=6316&filename=flugg_doc.pdf&version=10
	if(mediummap.size()!=0) return &mediummap;
	mediummap.emplace(5,"Beryllium");
	mediummap.emplace(6,"Carbon");
	mediummap.emplace(9,"Aluminum");
	mediummap.emplace(10,"Iron");
	mediummap.emplace(11,"Slab Steel");
	mediummap.emplace(12,"Blu Steel");
	mediummap.emplace(15,"Air");
	mediummap.emplace(16,"Vacuum");
	mediummap.emplace(17,"Concrete");
	mediummap.emplace(18,"Target");
	mediummap.emplace(19,"Rebar Concrete");
	mediummap.emplace(20,"Shotcrete");
	mediummap.emplace(21,"Variable Density Aluminum");
	mediummap.emplace(22,"Variable Density Steel");
	mediummap.emplace(23,"1018 Steel");
	mediummap.emplace(24,"A500 Steel");
	mediummap.emplace(25,"Water");
	mediummap.emplace(26,"M1018 Steel");
	mediummap.emplace(28,"Decay Pipe Vacuum");
	mediummap.emplace(31,"CT852");
	return &mediummap;
}


Position TVector3ToPositionRW(TVector3 tvecin){
	Position pos(0,0,0);
	pos.SetX(tvecin.X());
	pos.SetY(tvecin.Y());
	pos.SetZ(tvecin.Z());
	return pos;
}

Direction TVector3ToDirectionRW(TVector3 tvecin){
	Direction dir(0,0,0);
	dir.SetX(tvecin.X());
	dir.SetY(tvecin.Y());
	dir.SetZ(tvecin.Z());
	return dir;
}

FourVector TLorentzVectorToFourVectorRW(TLorentzVector tlvecin){
	FourVector vec(0,0,0,0);
	vec.SetT(tlvecin.T());
	vec.SetX(tlvecin.X());
	vec.SetY(tlvecin.Y());
	vec.SetZ(tlvecin.Z());
	return vec;
}

bool LoadReweightGenieEvent::valid_knob_name( const std::string& knob_name, genie::rew::GSyst_t& knob ) {
	std::set< genie::rew::GSyst_t > UNIMPLEMENTED_GENIE_KNOBS = {
		kXSecTwkDial_RnubarnuCC,  // tweak the ratio of \sigma(\bar\nu CC) / \sigma(\nu CC)
		kXSecTwkDial_NormCCQEenu, // tweak CCQE normalization (maintains dependence on neutrino energy)
		kXSecTwkDial_NormDISCC,	 // tweak the inclusive DIS CC normalization
		kXSecTwkDial_DISNuclMod	 // unclear intent, does anyone else know? - S. Gardiner
	}; 
	knob = genie::rew::GSyst::FromString( knob_name );
	if ( knob != kNullSystematic && knob != kNTwkDials ) {
		if ( UNIMPLEMENTED_GENIE_KNOBS.count(knob) ) {
			return false;
		}
	}
	else {
		return false;
	}
	return true;
}

void LoadReweightGenieEvent::SetupWeightCalculators(genie::rew::GReWeight& rw, const std::map<std::string, int>& modes_to_use){
	// Based on the list from the GENIE command-line tool grwght1p

	rw.AdoptWghtCalc( "xsec_ncel",       new GReWeightNuXSecNCEL   );
	rw.AdoptWghtCalc( "xsec_ccqe",       new GReWeightNuXSecCCQE   );
	rw.AdoptWghtCalc( "xsec_ccqe_axial", new GReWeightNuXSecCCQEaxial);
	rw.AdoptWghtCalc( "xsec_ccqe_vec",  new GReWeightNuXSecCCQEvec );
	rw.AdoptWghtCalc( "xsec_ccres",	    new GReWeightNuXSecCCRES   );
	rw.AdoptWghtCalc( "xsec_ncres",	    new GReWeightNuXSecNCRES   );
	rw.AdoptWghtCalc( "xsec_nonresbkg", new GReWeightNonResonanceBkg);
	rw.AdoptWghtCalc( "xsec_coh",       new GReWeightNuXSecCOH     );
	rw.AdoptWghtCalc( "xsec_dis",       new GReWeightNuXSecDIS     );
	rw.AdoptWghtCalc( "nuclear_qe",	    new GReWeightFGM           );
	rw.AdoptWghtCalc( "hadro_res_decay", new GReWeightResonanceDecay);
	rw.AdoptWghtCalc( "hadro_fzone",     new GReWeightFZone	       );
	rw.AdoptWghtCalc( "hadro_intranuke", new GReWeightINuke	       );
	rw.AdoptWghtCalc( "hadro_agky",	     new GReWeightAGKY         );
	rw.AdoptWghtCalc( "xsec_nc",     new GReWeightNuXSecNC         );
	rw.AdoptWghtCalc( "xsec_empmec", new GReWeightXSecEmpiricalMEC );
	// GReWeightDISNuclMod::CalcWeight() is not implemented, so we won't
	// bother to use it here. - S. Gardiner, 9 Dec 2019
	//rw.AdoptWghtCalc( "nuclear_dis",	 new GReWeightDISNuclMod );
	// Set the modes for the weight calculators that need them to be specified
	//UBoone Patch
	rw.AdoptWghtCalc( "xsec_mec",     new GReWeightXSecMEC );
	rw.AdoptWghtCalc( "deltarad_angle",  new GReWeightDeltaradAngle );
	rw.AdoptWghtCalc( "xsec_coh_ub",  new GReWeightNuXSecCOHuB );
	rw.AdoptWghtCalc( "res_bug_fix",  new GReWeightRESBugFix );

	for ( const auto& pair : modes_to_use ) {
		std::string calc_name = pair.first;
		int mode = pair.second;
		genie::rew::GReWeightI* calc = rw.WghtCalc( calc_name );
		// The GReWeightI base class doesn't have a SetMode(int) function,
		// so we'll just try dynamic casting until we get the right one.
		// If none work, then throw an exception.
		// TODO: Add a virtual function GReWeightI::SetMode( int ) in GENIE's
		// Reweight framework. Then we can avoid the hacky dynamic casts here.
		auto* calc_ccqe = dynamic_cast< genie::rew::GReWeightNuXSecCCQE* >( calc );
		auto* calc_ccres = dynamic_cast< genie::rew::GReWeightNuXSecCCRES* >( calc );
		auto* calc_ncres = dynamic_cast< genie::rew::GReWeightNuXSecNCRES* >( calc );
		auto* calc_dis = dynamic_cast< genie::rew::GReWeightNuXSecDIS* >( calc );
		if ( calc_ccqe ) calc_ccqe->SetMode( mode );
		else if ( calc_ccres ) calc_ccres->SetMode( mode );
		else if ( calc_ncres ) calc_ncres->SetMode( mode );
		else if ( calc_dis ) calc_dis->SetMode( mode );
	}
}

std::map< std::string, int > LoadReweightGenieEvent::CheckForIncompatibleSystematics(const std::vector<genie::rew::GSyst_t>& knob_vec){
	std::map< std::string, int > modes_to_use;
	std::map< std::string, std::map<int, std::set<genie::rew::GSyst_t> > > INCOMPATIBLE_GENIE_KNOBS = {
		// CCQE (genie::rew::GReWeightNuXSecCCQE)
		{ "xsec_ccqe", {
			{ genie::rew::GReWeightNuXSecCCQE::kModeNormAndMaShape,
				{
					// Norm + shape
					kXSecTwkDial_NormCCQE,    // tweak CCQE normalization (energy independent)
					kXSecTwkDial_MaCCQEshape, // tweak Ma CCQE, affects dsigma(CCQE)/dQ2 in shape only (normalized to constant integral)
					kXSecTwkDial_E0CCQEshape  // tweak E0 CCQE RunningMA, affects dsigma(CCQE)/dQ2 in shape only (normalized to constant integral)
				}
			},
			{ genie::rew::GReWeightNuXSecCCQE::kModeMa,
				{
					// Ma
					kXSecTwkDial_MaCCQE, // tweak Ma CCQE, affects dsigma(CCQE)/dQ2 both in shape and normalization
					kXSecTwkDial_E0CCQE, // tweak E0 CCQE RunningMA, affects dsigma(CCQE)/dQ2 both in shape and normalization
				}
			},
			{ genie::rew::GReWeightNuXSecCCQE::kModeZExp,
				{
					// Z-expansion
					kXSecTwkDial_ZNormCCQE,	// tweak Z-expansion CCQE normalization (energy independent)
					kXSecTwkDial_ZExpA1CCQE, // tweak Z-expansion coefficient 1, affects dsigma(CCQE)/dQ2 both in shape and normalization
					kXSecTwkDial_ZExpA2CCQE, // tweak Z-expansion coefficient 2, affects dsigma(CCQE)/dQ2 both in shape and normalization
					kXSecTwkDial_ZExpA3CCQE, // tweak Z-expansion coefficient 3, affects dsigma(CCQE)/dQ2 both in shape and normalization
					kXSecTwkDial_ZExpA4CCQE	// tweak Z-expansion coefficient 4, affects dsigma(CCQE)/dQ2 both in shape and normalization
				}
			},
		} },

		// CCRES (genie::rew::GReWeightNuXSecCCRES)
		{ "xsec_ccres", {
			{ genie::rew::GReWeightNuXSecCCRES::kModeNormAndMaMvShape,
				{
					// Norm + shape
					kXSecTwkDial_NormCCRES,	   /// tweak CCRES normalization
					kXSecTwkDial_MaCCRESshape, /// tweak Ma CCRES, affects d2sigma(CCRES)/dWdQ2 in shape only (normalized to constant integral)
					kXSecTwkDial_MvCCRESshape  /// tweak Mv CCRES, affects d2sigma(CCRES)/dWdQ2 in shape only (normalized to constant integral)
				}
			},
			{ genie::rew::GReWeightNuXSecCCRES::kModeMaMv,
				{
					// Ma + Mv
					kXSecTwkDial_MaCCRES, // tweak Ma CCRES, affects d2sigma(CCRES)/dWdQ2 both in shape and normalization
					kXSecTwkDial_MvCCRES  // tweak Mv CCRES, affects d2sigma(CCRES)/dWdQ2 both in shape and normalization
				}
			}
		} },

		// NCRES (genie::rew::GReWeightNuXSecNCRES)
		{ "xsec_ncres", {
			{ genie::rew::GReWeightNuXSecNCRES::kModeNormAndMaMvShape,
				{
					// Norm + shape
					kXSecTwkDial_NormNCRES,    /// tweak NCRES normalization
					kXSecTwkDial_MaNCRESshape, /// tweak Ma NCRES, affects d2sigma(NCRES)/dWdQ2 in shape only (normalized to constant integral)
					kXSecTwkDial_MvNCRESshape  /// tweak Mv NCRES, affects d2sigma(NCRES)/dWdQ2 in shape only (normalized to constant integral)
				} 
			},
			{ genie::rew::GReWeightNuXSecNCRES::kModeMaMv,
				{
					// Ma + Mv
					kXSecTwkDial_MaNCRES, // tweak Ma NCRES, affects d2sigma(NCRES)/dWdQ2 both in shape and normalization
					kXSecTwkDial_MvNCRES  // tweak Mv NCRES, affects d2sigma(NCRES)/dWdQ2 both in shape and normalization
				}
			}
		} },

		// DIS (genie::rew::GReWeightNuXSecDIS)
		{ "xsec_dis", {
			{ genie::rew::GReWeightNuXSecDIS::kModeABCV12u,
				{
					kXSecTwkDial_AhtBY,  // tweak the Bodek-Yang model parameter A_{ht} - incl. both shape and normalization effect
					kXSecTwkDial_BhtBY,  // tweak the Bodek-Yang model parameter B_{ht} - incl. both shape and normalization effect
					kXSecTwkDial_CV1uBY, // tweak the Bodek-Yang model parameter CV1u - incl. both shape and normalization effect
					kXSecTwkDial_CV2uBY  // tweak the Bodek-Yang model parameter CV2u - incl. both shape and normalization effect
				}
			},
			{ genie::rew::GReWeightNuXSecDIS::kModeABCV12uShape,
				{
					kXSecTwkDial_AhtBYshape,  // tweak the Bodek-Yang model parameter A_{ht} - shape only effect to d2sigma(DIS)/dxdy
					kXSecTwkDial_BhtBYshape,  // tweak the Bodek-Yang model parameter B_{ht} - shape only effect to d2sigma(DIS)/dxdy
					kXSecTwkDial_CV1uBYshape, // tweak the Bodek-Yang model parameter CV1u - shape only effect to d2sigma(DIS)/dxdy
					kXSecTwkDial_CV2uBYshape  // tweak the Bodek-Yang model parameter CV2u - shape only effect to d2sigma(DIS)/dxdy
				}
			}
		} }
	};
	for ( const auto& knob : knob_vec ) {
		for ( const auto& pair1 : INCOMPATIBLE_GENIE_KNOBS ) {
			std::string calc_name = pair1.first;
			const auto& mode_map = pair1.second;
			for ( const auto& pair2 : mode_map ) {
				int mode = pair2.first;
				std::set<genie::rew::GSyst_t> knob_set = pair2.second;
				if ( knob_set.count(knob) ) {
					auto search = modes_to_use.find( calc_name );
					if ( search != modes_to_use.end() ) {
						if ( search->second != mode ) {
							auto knob_str = genie::rew::GSyst::AsString( knob );
							std::cout << "ERROR: The GENIE knob " << knob_str << " is incompatible with others that are already configured" << std::endl;
						}
					}
					else modes_to_use[ calc_name ] = mode;
				}
			}
		}
	}
	return modes_to_use;
}


