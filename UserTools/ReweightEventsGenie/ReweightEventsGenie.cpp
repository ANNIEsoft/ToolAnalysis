#include "ReweightEventsGenie.h"
#include "GenieInfo.h"
#include "TMap.h"
#include "TArrayF.h"
#include "TString.h"


#define LOADED_GENIE 1

/**
 * \class ReweightEventsGenie
 * Loads GENIE events and handles event weights for flux and cross section systematics studies
 *
* Ported in parts from LoadGenieEvent
* $Author: B.Richards
* $Date: 2019/05/28 10:44:00
* Contact: b.richards@qmul.ac.uk
*
* UBGenieWeightCalc.cxx
* Ported from larsim back to uboonecode on Mar 13 2020
*   by Steven Gardiner <gardiner@fnal.gov>
* Heavily rewritten on Dec 9 2019
*   by Steven Gardiner <gardiner@fnal.gov>
* Updated by Marco Del Tutto on Feb 18 2017
* Ported from uboonecode to larsim on Feb 14 2017
*   by Marco Del Tutto <marco.deltutto@physics.ox.ac.uk>
*
* Ported and updated to ANNIE ToolChain Framework
* $Author: James Minock
* $Date: 2023/02/09
* Contact: jmm1018@physics.rutgers.edu

*/
ReweightEventsGenie::ReweightEventsGenie():Tool(){}

bool ReweightEventsGenie::Initialise(std::string configfile, DataModel &data){
//#if LOADED_GENIE==1
  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();
  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("FluxVersion",fluxver); // flux version: 0=rhatcher files, 1=zarko files(gsimple)
  m_variables.Get("FromWCSim",fromwcsim);
  m_variables.Get("OnGrid",on_grid);
  m_variables.Get("genie_module_label",fGenieModuleWeight);

  // Open the GHEP files
  ///////////////////////
  std::cout << "Tool ReweightEventsGenie: Opening TChain" << std::endl;

  int annieeventexists = m_data->Stores.count("ANNIEEvent");
  if(annieeventexists==0) m_data->Stores["ANNIEEvent"] = new BoostStore(false,2);

  //opens dummy file to initialize GENIE
  flux = new TChain("gtree");
  int dummy_check = flux->Add("./UserTools/ReweightEventsGenie/beamData/gntp.dummy.ghep.root");
  if(dummy_check == 0){ throw std::invalid_argument("ERROR: cannot find gntp.dummy.ghep.root"); }
  SetBranchAddresses();
 
  if(!fromwcsim){
    m_variables.Get("FileDir",filedir);
    m_variables.Get("FilePattern",filepattern);
    m_variables.Get("OutputDir",outdir);
    m_variables.Get("OutputFile",outpattern);
    std::string inputfiles = filedir+"/"+filepattern;
    if(on_grid) inputfiles = filepattern;
    tchainentrynum=0;
    flux = new TChain("gtree");
    int numbytes = flux->Add(inputfiles.c_str());
    std::cout << "Tool ReweightEventsGenie: Read " << to_string(numbytes) << " bytes loading TChain " << inputfiles << std::endl;
    std::cout << "Tool ReweightEventsGenie: Genie TChain has " << to_string(flux->GetEntries()) << " entries" << std::endl;
    SetBranchAddresses();
  }

  m_variables.Get("weight_functions_genie",weight_options);
  m_variables.Get("weight_functions_flux",fweight_options);
  std::string central_values;
  m_variables.Get("genie_central_values",central_values);

  //parse and tokenize array of strings that list weights
  std::stringstream weights_in(weight_options);
  std::stringstream fweights_in(fweight_options);
  std::string temp;
  while (weights_in.good()){
    std::getline(weights_in, temp, ',');
    weight_names.push_back(temp);
  }
  while (fweights_in.good()){
    std::getline(fweights_in, temp, ',');
    fweight_names.push_back(temp);
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
    evwgh::xsecconfig xs_configs;
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
//-------------------------------------------------------------------------------------
  //intitalize variables for flux weight configuration
  std::string flux_function = "";
  tokens = "";
  keys =   "";
  values = "";
  sig = 0.0;
  //Get each parameter
  for(unsigned int i = 0; i < fweight_names.size(); i++){
    m_variables.Get(fweight_names[i],flux_function);
    evwgh::fluxconfig temp_configs;
    temp_configs.title = fweight_names[i];
    //Separate key-value pairs from each other
    std::stringstream param_in(flux_function);
    while (param_in.good()){
      std::getline(param_in, tokens, '|');
      //Separate and save values from keys
      std::stringstream pair_in(tokens);
      while (pair_in.good()){
        std::getline(pair_in, keys, ':');
        std::getline(pair_in, values, ':');
        if(keys == "type") temp_configs.type = values;
        else if(keys == "CentralValue_hist_file"){
          values.erase(0,1);
          values.erase(values.size()-1);
          temp_configs.CentralValue_hist_file = values;
        }
        else if(keys == "PositiveSystematicVariation_hist_file"){
          values.erase(0,1);
          values.erase(values.size()-1);
          temp_configs.PositiveSystematicVariation_hist_file = values;
        }
        else if(keys == "NegativeSystematicVariation_hist_file"){
          values.erase(0,1);
          values.erase(values.size()-1);
          temp_configs.NegativeSystematicVariation_hist_file = values;
        }
        else if(keys == "cv_hist_file"){
          values.erase(0,1);
          values.erase(values.size()-1);
          temp_configs.cv_hist_file = values;
        }
        else if(keys == "rw_hist_file"){
          values.erase(0,1);
          values.erase(values.size()-1);
          temp_configs.rw_hist_file = values;
        }
        else if(keys == "ExternalData"){
          values.erase(0,1);
          values.erase(values.size()-1);
          temp_configs.ExternalData = values;
        }
        else if(keys == "ExternalFit"){
          values.erase(0,1);
          values.erase(values.size()-1);
          temp_configs.ExternalFit = values;
        }
        else if(keys == "parameter_list"){
          //strip out brackets
          values.erase(0,1);
          values.erase(values.size()-1);
          std::string temp_pars;
          std::stringstream pars_in(values);
          while(pars_in.good()){
            std::getline(pars_in, temp_pars, ',');
            //strip out quotations
            temp_pars.erase(0,1);
            temp_pars.erase(temp_pars.size()-1);
            temp_configs.parameter_list.push_back(temp_pars);
          }
        }
        else if(keys == "weight_calculator"){
          values.erase(0,1);
          values.erase(values.size()-1);
          temp_configs.weight_calculator = values;
        }
        else if(keys == "mode") temp_configs.mode = values;
        else if(keys == "random_seed"){
          std::stringstream to_int(values);
          int rand;
          to_int >> rand;
          temp_configs.random_seed = rand;
        }
        else if(keys == "number_of_multisims"){
          std::stringstream to_int(values);
          int nom;
          to_int >> nom;
          temp_configs.number_of_multisims = nom;
        }
        else if(keys == "scale_factor_pos"){
          std::stringstream to_double(values);
          double sfp;
          to_double >> sfp;
          temp_configs.scale_factor_pos= sfp;
        }
        else if(keys == "scale_factor_neg"){
          std::stringstream to_double(values);
          double sfn;
          to_double >> sfn;
          temp_configs.scale_factor_neg = sfn;
        }
        else if(keys == "parameter_sigma"){
          std::stringstream to_int(values);
          int ps;
          to_int >> ps;
          temp_configs.parameter_sigma = ps;
        }
        else if(keys == "scale_factor"){
          std::stringstream to_int(values);
          int sf;
          to_int >> sf;
          temp_configs.scale_factor = sf;
        }
        else if(keys == "PrimaryHadronGeantCode"){
          if(temp_configs.type == "PrimaryHadronSanfordWang"){
            //strip out brackets
            values.erase(0,1);
            values.erase(values.size()-1);
            std::string temp_pars;
            std::stringstream pars_in(values);
            while(pars_in.good()){
              std::getline(pars_in, temp_pars, ',');
              //strip out quotations
              temp_pars.erase(0,1);
              temp_pars.erase(temp_pars.size()-1);
              temp_configs.parameter_list.push_back(temp_pars);
            }
          }
          else{
            std::stringstream to_int(values);
            int phgc;
            to_int >> phgc;
            temp_configs.PrimaryHadronGeantCode = phgc;//array for Sanford Wang
          }
        }
        else if(keys == "use_MiniBooNE_random_numbers"){
          temp_configs.use_MiniBooNE_random_numbers = (values == "false") ? false : true;
        }
      }
    }
    fconfig_funcs.push_back(temp_configs);
  }
  //put in Weight Manager
  wm.Configure(fconfig_funcs);
  return true;
}


bool ReweightEventsGenie::Execute(){
  if(fromwcsim){
    // retrieve the genie file and entry number from the LoadWCSim tool
    std::string inputfiles;
    get_ok = m_data->CStore.Get("GenieFile",inputfiles);
    if(!get_ok){
      std::cout << "Tool ReweightEventsGenie: Failed to find GenieFile in CStore" << std::endl;
      return false;
    }
    get_ok = m_data->CStore.Get("GenieEntry",tchainentrynum);
    if(!get_ok){
      std::cout << "Tool ReweightEventsGenie: Failed to find GenieEntry in CStore" << std::endl;
      return false;
    }
    std::string curfname = ((curf) ? curf->GetName() : "");
    // check if this is a new file
    if(inputfiles!=curfname){
      // we need to load the new file
      if(flux) flux->ResetBranchAddresses();
      if(curf) curf->Close();
      std::cout << "Tool ReweightEventsGenie: Loading new file " << inputfiles << std::endl;
      curf=TFile::Open(inputfiles.c_str());
      flux=(TChain*)curf->Get("gtree");
      SetBranchAddresses();
    }
  }
  std::cout << "Tool ReweightEventsGenie: Loading tchain entry " << to_string(tchainentrynum) << std::endl;
  local_entry = flux->LoadTree(tchainentrynum);
  std::cout << "Tool ReweightEventsGenie: localentry is " << to_string(local_entry) << std::endl;
  if((local_entry<0) || (local_entry!=tchainentrynum)){
    std::cout << "Tool ReweightEventsGenie: Reached end of file, returning" << std::endl;
    m_data->vars.Set("StopLoop",1);
    return true;
  }
  flux->GetEntry(local_entry);
  curf = flux->GetCurrentFile();
  geniehdr=dynamic_cast<NtpMCTreeHeader*> (curf->Get("header")); //used to get run number
  if(curf!=curflast || curflast==nullptr){
    TString curftstring = curf->GetName();
    currentfilestring = std::string(curftstring.Data());
    curflast=curf;
    std::cout << "Tool ReweightEventsGenie: Opening new file \"" << currentfilestring << "\"" << std::endl;
  }
  tchainentrynum++;

  // Expand out the neutrino event and flux info
  // =======================================================
  // header only contains the event number
  genie::NtpMCRecHeader hdr = genieintx->hdr;
  unsigned int genie_event_num = hdr.ievent;
  // ======== flux info ========
  evwgh::event e;
  e.entryno = gsimplenumientry->entryno;
  e.run = gsimplenumientry->run;
  double energy = gsimpleentry->E;
  e.nenergyn = energy;
  e.nenergyf = energy;
  e.evtno = gsimplenumientry->evtno;
  e.ntype = gsimpleentry->pdg;

  e.tpx=gsimplenumientry->tpx;
  e.tpy=gsimplenumientry->tpy;
  e.tpz=gsimplenumientry->tpz;
  e.tptype=gsimplenumientry->tptype;

  e.vx       = gsimplenumientry->vx;
  e.vy       = gsimplenumientry->vy;
  e.vz       = gsimplenumientry->vz;
  e.pdpx     = gsimplenumientry->pdpx;
  e.pdpy     = gsimplenumientry->pdpy;
  e.pdpz     = gsimplenumientry->pdpz;

  double apppz   = gsimplenumientry->pppz;
  e.ppdxdz   = (gsimplenumientry->pppx)/apppz;
  e.ppdydz   = (gsimplenumientry->pppy)/apppz;
  e.pppz     = apppz;
  e.ppmedium = gsimplenumientry->ppmedium;
  e.ptype    = gsimplenumientry->ptype;

  e.nimpwt = gsimpleentry->wgt;

  //Fill Ndecay (check parent type, neutrino type and if it is a 2 or 3 body decay)
  e.ndecay = gsimplenumientry->ndecay;

  //Run flux reweighting
  evwgh::MCEventWeight wght=wm.Run(e,0);

  // all neutrino intx details are in the event record
  genie::EventRecord* gevtRec = genieintx->event;

  // neutrino interaction info
  genie::Interaction* genieint = gevtRec->Summary();

  //assume 1 neutrino per event, no pile-up
  std::vector<std::vector<double>> weights(weight_names.size());

  for(unsigned int i = 0; i < weight_names.size(); i++){
    unsigned int num_knobs = reweightVector[i].size();
    double nuE = genieint->InitState().ProbeE(genie::kRfLab);

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

    double lep_px = p4l.Px();
    double lep_py = p4l.Py();
    double lep_pz = p4l.Pz();

    TIter partitr(gevtRec);
    genie::GHepParticle *part = 0;
    bool isCC = true;
    bool has0pi0 = true;
    bool has0piPMgt160 = true;

    if(genieint->ProcInfo().IsWeakNC()) isCC = false;

    // Pion flags
    while((part = dynamic_cast<genie::GHepParticle *>(partitr.Next()))){
      int piPDG = part->Pdg();
      if(piPDG == 111) has0pi0 = false;
      if(std::abs(piPDG) == 211) {
        double piPx = part->Px();
        double piPy = part->Py();
        double piPz = part->Pz();
        if(std::sqrt(std::pow(piPx,2)+std::pow(piPy,2)+std::pow(piPz,2)) > 0.160) has0piPMgt160 = false;
      }
    }

    // All right, the event record is fully ready. Now ask the GReWeight
    // objects to compute the weights.
    weights[i].resize( num_knobs );
    for (unsigned int k = 0; k < num_knobs; ++k ) {
      weights[i][k] = reweightVector[i].at(k).CalcWeight( *gevtRec );
    }

    m_data->Stores.at("ANNIEEvent")->Set("flux_weights",wght.fWeight);
    m_data->Stores.at("ANNIEEvent")->Set("xsec_weights",weights);
    m_data->Stores.at("ANNIEEvent")->Set("MCisCC",isCC); 
    m_data->Stores.at("ANNIEEvent")->Set("MCQsquared",genieint->KinePtr()->Q2());
    m_data->Stores.at("ANNIEEvent")->Set("MCNuPx",genieint->InitState().GetProbeP4()->Px());
    m_data->Stores.at("ANNIEEvent")->Set("MCNuPy",genieint->InitState().GetProbeP4()->Py());
    m_data->Stores.at("ANNIEEvent")->Set("MCNuPz",genieint->InitState().GetProbeP4()->Pz());
    m_data->Stores.at("ANNIEEvent")->Set("MCNuE",nuE);
    m_data->Stores.at("ANNIEEvent")->Set("MCTgtPx",genieint->InitState().GetTgtP4()->Px());
    m_data->Stores.at("ANNIEEvent")->Set("MCTgtPy",genieint->InitState().GetTgtP4()->Py());
    m_data->Stores.at("ANNIEEvent")->Set("MCTgtPz",genieint->InitState().GetTgtP4()->Pz());
    m_data->Stores.at("ANNIEEvent")->Set("MCTgtE",genieint->InitState().GetTgtP4()->E());
    m_data->Stores.at("ANNIEEvent")->Set("MCNuPDG",genieint->InitState().ProbePdg());
    m_data->Stores.at("ANNIEEvent")->Set("MCTgtPDG",genieint->InitState().TgtPdg());
    m_data->Stores.at("ANNIEEvent")->Set("MCTgtisP",genieint->InitState().TgtPtr()->IsProton());
    m_data->Stores.at("ANNIEEvent")->Set("MCTgtisN",genieint->InitState().TgtPtr()->IsNeutron());
    m_data->Stores.at("ANNIEEvent")->Set("MCFSLm",ml);
    m_data->Stores.at("ANNIEEvent")->Set("MCFSLPx",lep_px);
    m_data->Stores.at("ANNIEEvent")->Set("MCFSLPy",lep_py);
    m_data->Stores.at("ANNIEEvent")->Set("MCFSLPz",lep_pz);
    m_data->Stores.at("ANNIEEvent")->Set("MCFSLE",p4l.E());
    m_data->Stores.at("ANNIEEvent")->Set("MC0pi0",has0pi0);
    m_data->Stores.at("ANNIEEvent")->Set("MC0piPMgt160",has0piPMgt160);

  }

  std::cout << "Tool ReweightEventsGenie: Clearing genieintx" << std::endl;
  genieintx->Clear(); // REQUIRED TO PREVENT MEMORY LEAK

  return true;
}


bool ReweightEventsGenie::Finalise(){
  std::cout << "Completed reweighting" << std::endl;
  return true;
}

void ReweightEventsGenie::SetBranchAddresses(){
  std::cout << "Tool ReweightEventsGenie: Setting branch addresses" << std::endl;
  // neutrino event information
  flux->SetBranchAddress("gmcrec",&genieintx);
  flux->GetBranch("gmcrec")->SetAutoDelete(kTRUE);

  // input (BNB intx) event information
  if(fluxver==0){   // rhatcher files
    flux->SetBranchAddress("flux",&gnumipassthruentry);
    flux->GetBranch("flux")->SetAutoDelete(kTRUE);
  } else if(fluxver==1){          // zarko files
//    flux->Print();
    flux->SetBranchAddress("numi",&gsimplenumientry);
    flux->GetBranch("numi")->SetAutoDelete(kTRUE);
    flux->SetBranchAddress("simple",&gsimpleentry);
    flux->GetBranch("simple")->SetAutoDelete(kTRUE);
    flux->SetBranchAddress("aux",&gsimpleauxinfo);
    flux->GetBranch("aux")->SetAutoDelete(kTRUE);
  } else {
    flux->Print();
  }
}

bool ReweightEventsGenie::valid_knob_name( const std::string& knob_name, genie::rew::GSyst_t& knob ) {
  std::set< genie::rew::GSyst_t > UNIMPLEMENTED_GENIE_KNOBS = {
    kXSecTwkDial_RnubarnuCC,  // tweak the ratio of \sigma(\bar\nu CC) / \sigma(\nu CC)
    kXSecTwkDial_NormCCQEenu, // tweak CCQE normalization (maintains dependence on neutrino energy)
    kXSecTwkDial_NormDISCC,   // tweak the inclusive DIS CC normalization
    kXSecTwkDial_DISNuclMod   // unclear intent, does anyone else know? - S. Gardiner
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

void ReweightEventsGenie::SetupWeightCalculators(genie::rew::GReWeight& rw, const std::map<std::string, int>& modes_to_use){
  // Based on the list from the GENIE command-line tool grwght1p

  rw.AdoptWghtCalc( "xsec_ncel",       new GReWeightNuXSecNCEL      );
  rw.AdoptWghtCalc( "xsec_ccqe",       new GReWeightNuXSecCCQE      );
  rw.AdoptWghtCalc( "xsec_ccqe_axial", new GReWeightNuXSecCCQEaxial );
  rw.AdoptWghtCalc( "xsec_ccqe_vec",   new GReWeightNuXSecCCQEvec   );
  rw.AdoptWghtCalc( "xsec_ccres",      new GReWeightNuXSecCCRES     );
  rw.AdoptWghtCalc( "xsec_ncres",      new GReWeightNuXSecNCRES     );
  rw.AdoptWghtCalc( "xsec_nonresbkg",  new GReWeightNonResonanceBkg );
  rw.AdoptWghtCalc( "xsec_coh",        new GReWeightNuXSecCOH       );
  rw.AdoptWghtCalc( "xsec_dis",        new GReWeightNuXSecDIS       );
  rw.AdoptWghtCalc( "nuclear_qe",      new GReWeightFGM             );
  rw.AdoptWghtCalc( "hadro_res_decay", new GReWeightResonanceDecay  );
  rw.AdoptWghtCalc( "hadro_fzone",     new GReWeightFZone           );
  rw.AdoptWghtCalc( "hadro_intranuke", new GReWeightINuke           );
  rw.AdoptWghtCalc( "hadro_agky",      new GReWeightAGKY            );
  rw.AdoptWghtCalc( "xsec_nc",         new GReWeightNuXSecNC        );
  rw.AdoptWghtCalc( "res_dk",          new GReWeightResonanceDecay  );
  rw.AdoptWghtCalc( "xsec_empmec",     new GReWeightXSecEmpiricalMEC);
  // GReWeightDISNuclMod::CalcWeight() is not implemented, so we won't
  // bother to use it here. - S. Gardiner, 9 Dec 2019
  //rw.AdoptWghtCalc( "nuclear_dis",   new GReWeightDISNuclMod );
  // Set the modes for the weight calculators that need them to be specified
  //UBoone Patch
  rw.AdoptWghtCalc( "xsec_mec",        new GReWeightXSecMEC );
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

std::map< std::string, int > ReweightEventsGenie::CheckForIncompatibleSystematics(const std::vector<genie::rew::GSyst_t>& knob_vec){
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
          kXSecTwkDial_ZNormCCQE,  // tweak Z-expansion CCQE normalization (energy independent)
          kXSecTwkDial_ZExpA1CCQE, // tweak Z-expansion coefficient 1, affects dsigma(CCQE)/dQ2 both in shape and normalization
          kXSecTwkDial_ZExpA2CCQE, // tweak Z-expansion coefficient 2, affects dsigma(CCQE)/dQ2 both in shape and normalization
          kXSecTwkDial_ZExpA3CCQE, // tweak Z-expansion coefficient 3, affects dsigma(CCQE)/dQ2 both in shape and normalization
          kXSecTwkDial_ZExpA4CCQE  // tweak Z-expansion coefficient 4, affects dsigma(CCQE)/dQ2 both in shape and normalization
        }
      },
    } },

    // CCRES (genie::rew::GReWeightNuXSecCCRES)
    { "xsec_ccres", {
      { genie::rew::GReWeightNuXSecCCRES::kModeNormAndMaMvShape,
        {
          // Norm + shape
          kXSecTwkDial_NormCCRES,    /// tweak CCRES normalization
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




