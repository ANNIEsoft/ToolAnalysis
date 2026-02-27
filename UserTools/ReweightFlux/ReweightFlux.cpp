#include "ReweightFlux.h"

ReweightFlux::ReweightFlux():Tool(){}


bool ReweightFlux::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  m_variables.Get("Verbosity",verbosity);
  m_variables.Get("weight_functions_flux",weight_options);

  //parse and tokenize array of strings that list weights
  std::stringstream weights_in(weight_options);
  std::string temp;
  while (weights_in.good()){
    std::getline(weights_in, temp, ',');
    fweight_names.push_back(temp);
  }


//-------------------------------------------------------------------------------------
  //intitalize variables for flux weight configuration
  std::string flux_function = "";
  std::string tokens = "";
  std::string keys =   "";
  std::string values = "";
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


bool ReweightFlux::Execute(){
  // Get the flux info
  // =======================================================

  int parentpdg, parentdecaymode, parentprodmedium, parentpdgattgtexit;
  float parentdecayvtx_x, parentdecayvtx_y, parentdecayvtx_z;
  float parentdecaymom_x, parentdecaymom_y, parentdecaymom_z;
  float parentprodmom_x, parentprodmom_y, parentprodmom_z;
  float parenttgtexitmom_x, parenttgtexitmom_y, parenttgtexitmom_z;
  int fluxrun, fluxentryno, fluxevtno, fluxntype;
  double fluxnimpwt, fluxnenergyn, fluxnenergyf;

  bool get_pdg = m_data->Stores["GenieInfo"]->Get("ParentPdg",parentpdg);
  bool get_decay_mode = m_data->Stores["GenieInfo"]->Get("ParentDecayMode",parentdecaymode);
  bool get_decay_vx = m_data->Stores["GenieInfo"]->Get("ParentDecayVtx_X",parentdecayvtx_x);
  bool get_decay_vy = m_data->Stores["GenieInfo"]->Get("ParentDecayVtx_Y",parentdecayvtx_y);
  bool get_decay_vz = m_data->Stores["GenieInfo"]->Get("ParentDecayVtx_Z",parentdecayvtx_z);
  bool get_decay_px = m_data->Stores["GenieInfo"]->Get("ParentDecayMom_X",parentdecaymom_x);
  bool get_decay_py = m_data->Stores["GenieInfo"]->Get("ParentDecayMom_Y",parentdecaymom_y);
  bool get_decay_pz = m_data->Stores["GenieInfo"]->Get("ParentDecayMom_Z",parentdecaymom_z);
  bool get_prod_px = m_data->Stores["GenieInfo"]->Get("ParentProdMom_X",parentprodmom_x);
  bool get_prod_py = m_data->Stores["GenieInfo"]->Get("ParentProdMom_Y",parentprodmom_y);
  bool get_prod_pz = m_data->Stores["GenieInfo"]->Get("ParentProdMom_Z",parentprodmom_z);
  bool get_medium = m_data->Stores["GenieInfo"]->Get("ParentProdMedium",parentprodmedium);
  bool get_tgt_pdg = m_data->Stores["GenieInfo"]->Get("ParentPdgAtTgtExit",parentpdgattgtexit);
  bool get_tgt_px = m_data->Stores["GenieInfo"]->Get("ParentTgtExitMom_X",parenttgtexitmom_x);
  bool get_tgt_py = m_data->Stores["GenieInfo"]->Get("ParentTgtExitMom_Y",parenttgtexitmom_y);
  bool get_tgt_pz = m_data->Stores["GenieInfo"]->Get("ParentTgtExitMom_Z",parenttgtexitmom_z);
  bool get_entryno = m_data->Stores["GenieInfo"]->Get("ParentEntryNo",fluxentryno);
  bool get_run = m_data->Stores["GenieInfo"]->Get("ParentRunNo",fluxrun);
  bool get_energyn = m_data->Stores["GenieInfo"]->Get("ParentNEnergyN",fluxnenergyn);
  bool get_energyf = m_data->Stores["GenieInfo"]->Get("ParentNEnergyF",fluxnenergyf);
  bool get_evtno = m_data->Stores["GenieInfo"]->Get("ParentEventNo",fluxevtno);
  bool get_ntype = m_data->Stores["GenieInfo"]->Get("ParentNType",fluxntype);
  bool get_wgt = m_data->Stores["GenieInfo"]->Get("ParentWgt",fluxnimpwt);
	

  // ======== flux info ========
  evwgh::event e;
  e.entryno = fluxentryno;
  e.run = fluxrun;
  e.nenergyn = fluxnenergyn;
  e.nenergyf = fluxnenergyf;
  e.evtno = fluxevtno;
  e.ntype = fluxntype;

  e.tpx=parenttgtexitmom_x;
  e.tpy=parenttgtexitmom_y;
  e.tpz=parenttgtexitmom_z;
  e.tptype=parentpdgattgtexit;

  e.vx       = parentdecayvtx_x;
  e.vy       = parentdecayvtx_y;
  e.vz       = parentdecayvtx_z;
  e.pdpx     = parentdecaymom_x;
  e.pdpy     = parentdecaymom_y;
  e.pdpz     = parentdecaymom_z;

  e.ppdxdz   = parentprodmom_x;
  e.ppdydz   = parentprodmom_y;
  e.pppz     = parentprodmom_z;
  e.ppmedium = parentprodmedium;
  e.ptype    = parentpdg;

  e.nimpwt = fluxnimpwt;

  //Fill Ndecay (check parent type, neutrino type and if it is a 2 or 3 body decay)
  e.ndecay = parentdecaymode;

  //Run flux reweighting
  evwgh::MCEventWeight wght=wm.Run(e,0);

  m_data->Stores.at("ANNIEEvent")->Set("flux_weights",wght.fWeight);

  return true;
}


bool ReweightFlux::Finalise(){
  if(verbosity > 0) std::cout << "Completed Flux Reweighting" << std::endl;
  return true;
}
