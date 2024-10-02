#include "NeutronMultiplicity.h"

NeutronMultiplicity::NeutronMultiplicity():Tool(){}


bool NeutronMultiplicity::Initialise(std::string configfile, DataModel &data){

  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  m_data= &data; //assigning transient data pointer

  bool get_ok = false;

  save_bs = true;
  save_root = true;
  filename = "NeutronMultiplicity";
  read_bs = "None";

  get_ok = m_variables.Get("verbosity",verbosity);
  get_ok = m_variables.Get("SaveROOT",save_root);
  get_ok = m_variables.Get("SaveBoostStore",save_bs);
  get_ok = m_variables.Get("Filename",filename);
  get_ok = m_variables.Get("ReadFromBoostStore",read_bs);
  get_ok = m_variables.Get("MRDTrackRestriction",mrdtrack_restriction);

  //Check if we are dealing with a MC file or not
  //isMC = m_data->Stores.at("ANNIEEvent")->Get("MCFile",MCFile);

  if (verbosity > 1){
    std::cout <<"Initialise NeutronMultiplicity tool"<<std::endl;
    std::cout <<std::endl;
    std::cout <<"####################################"<<std::endl;
    std::cout <<" ########### CONFIGURATION #########"<<std::endl;
    std::cout <<"SaveROOT: "<<save_root<<std::endl;
    std::cout <<"SaveBoostStore: "<<save_bs<<std::endl;
    std::cout <<"Filename: "<<filename<<std::endl;
    std::cout <<"ReadFromBoostStore: "<<read_bs<<std::endl;
    std::cout <<"MRDTrackRestriction: "<<mrdtrack_restriction<<std::endl;
  }

  if (save_root){
    //Initialise root specific objects (files, histograms, trees)
    this->InitialiseHistograms();
  }
  if (save_bs){
    //Initialise Boost Store
    store_neutronmult = new BoostStore(false,2);
  }
  if (read_bs != "None"){
    //Read ANNIEEvent from BoostStore file
    ifstream input_bs(read_bs.c_str());
    if (!input_bs.good()){
      Log("NeutronMultiplicity tool: Tried to read in filelist for BoostStores "+read_bs+", but file does not exist! Stop execution",v_error,verbosity);
      m_data->vars.Set("StopLoop",1);     
    } else {
      std::string temp_str;
      while (input_bs >> temp_str ) input_filenames_.push_back( temp_str );
      if (input_filenames_.size() ==0){
        Log("NeutronMultiplicity tool: Error! No input file names!", v_error, verbosity);
        m_data->vars.Set("StopLoop",1);
      }
    }
    //Check if ANNIEEvent is already loaded -> would interfere with loaded BoostStore file
    if (m_data->Stores.count("ANNIEEvent")>0) {
      Log("NeutronMultiplicity tool: m_data->Stores[\"ANNIEEvent\"] seems to be already loaded! Please remove LoadANNIEEvent/LoadWCSim/... tools from toolchain when using the option ReadFromBoostStore! Stop execution",v_error,verbosity);
      m_data->vars.Set("StopLoop",1);
    } else {
      m_data->Stores["ANNIEEvent"] = new BoostStore(false,2);
    }

    //Check if RecoEvent is already loaded -> would interfere with loaded BoostStore file
    if (m_data->Stores.count("RecoEvent")>0) {
      Log("NeutronMultiplicity tool: m_data->Stores[\"RecoEvent\"] seems to be already loaded! Please remove LoadANNIEEvent/LoadWCSim/... tools from toolchain when using the option ReadFromBoostStore! Stop execution",v_error,verbosity);
      m_data->vars.Set("StopLoop",1);
    } else {
      m_data->Stores["RecoEvent"] = new BoostStore(false,2);
    }

    //Check if MRDTracks store is already loaded -> would interfere with loaded BoostStore file
    if (m_data->Stores.count("MRDTracks")>0) {
      Log("NeutronMultiplicity tool: m_data->Stores[\"MRDTracks\"] seems to be already loaded! Please remove LoadANNIEEvent/LoadWCSim/... tools from toolchain when using the option ReadFromBoostStore! Stop execution",v_error,verbosity);
      m_data->vars.Set("StopLoop",1);
    } else {
      m_data->Stores["MRDTracks"] = new BoostStore(false,2);
    }

    //Check if GenieInfo store is already loaded -> would interfere with loaded BoostStore file
    if (m_data->Stores.count("GenieInfo")>0) {
      Log("NeutronMultiplicity tool: m_data->Stores[\"GenieInfo\"] seems to be already loaded! Please remove LoadANNIEEvent/LoadWCSim/LoadGenieEvent... tools from toolchain when using the option ReadFromBoostStore! Stop execution",v_error,verbosity);
      m_data->vars.Set("StopLoop",1);
    } else {
      m_data->Stores["GenieInfo"] = new BoostStore(false,2);
    }

  }

  need_new_file_ = true;
  current_entry_ = 0u;
  current_file_ = 0u;

  return true;
}


bool NeutronMultiplicity::Execute(){

  isMC = m_data->Stores.at("ANNIEEvent")->Get("MCFile",MCFile);
  if (isMC) std::cout << "NeutronMultiplicity Tool: MCFile found!" << std::endl;

  //Reset tree variables
  this->ResetVariables();

  //Set ANNIEEvent if reading from BoostStore file
  if (read_bs!= "None"){
    if (need_new_file_){
      need_new_file_ = false;
      //if (read_neutronmult) delete read_neutronmult;
      if (current_file_ != 0) {
		read_neutronmult->Close();
		read_neutronmult->Delete();
		delete read_neutronmult;
	}
      read_neutronmult = new BoostStore(false,2);
      read_neutronmult->Initialise(input_filenames_.at(current_file_));
      read_neutronmult->Print(false);
      read_neutronmult->Header->Get("TotalEntries",total_entries_in_file_);
      isMC = read_neutronmult->Get("MCFile",MCFile);
    } 
    if (current_entry_ != 0) {
      //Delete calls for stores is necessary for pointers to work correctly
      read_neutronmult->Delete();
    }
 
    Log("NeutronMultiplicity tool: ReadFromBoostStore mode: Get entry "+std::to_string(current_entry_) + " / " + std::to_string(total_entries_in_file_),v_message,verbosity);     
    read_neutronmult->GetEntry(current_entry_);
    ++current_entry_;

    if (current_entry_ >= total_entries_in_file_){
      ++current_file_;
      if (current_file_ >= input_filenames_.size() ){
        m_data->vars.Set("StopLoop",1);
      } else {
        current_entry_ = 0u;
        need_new_file_ = true;
      }
    }

    //Read current BoostStore entry
    this->ReadBoostStore();
  }

  //Get Particles variable 
  bool get_ok = m_data->Stores["ANNIEEvent"]->Get("Particles",Particles);

  m_data->Stores["ANNIEEvent"]->Get("EventNumber",EventNumber);
  m_data->Stores["ANNIEEvent"]->Get("RunNumber",RunNumber);

  NumberNeutrons = 0;
  if (get_ok){
    for (int i_particle=0; i_particle < (int) Particles.size(); i_particle++){
      Log("NeutronMultiplicity tool: Retrieved particle # "+std::to_string(i_particle)+"...",2,verbosity);
      if (verbosity > 2) Particles.at(i_particle).Print();
      if (Particles.at(i_particle).GetPdgCode() == 2112) NumberNeutrons++;
    }
  }

  //Get selection cut variables
  bool pass_selection = false;
  m_data->Stores["RecoEvent"]->Get("EventCutStatus",pass_selection);

  //Fill h_neutrons independent of reconstruction status if selection cuts are passed
  if (pass_selection) h_neutrons->Fill(NumberNeutrons);

  bool pass_reconstruction = false;
  m_data->Stores["RecoEvent"]->Get("SimpleRecoFlag",SimpleRecoFlag);
  if (SimpleRecoFlag != -9999) pass_reconstruction = true;

  //Get information about clusters
  this->GetClusterInformation();

  m_data->Stores["MRDTracks"]->Get("NumMrdTracks",numtracksinev);
  //If MRDTrackRestriction is enabled, check if there was only one MRD track
  if (mrdtrack_restriction){
    if (numtracksinev != 1) pass_reconstruction = false;
    if (pass_selection && numtracksinev == 1) {
      for (int i_n=0; i_n < (int) reco_NCandTime->size(); i_n++){
        h_time_neutrons->Fill(reco_NCandTime->at(i_n));
      }
    }
  } else if (pass_selection) {
    for (int i_n=0; i_n < (int) reco_NCandTime->size(); i_n++){
      h_time_neutrons->Fill(reco_NCandTime->at(i_n));
    }
  }

  //std::cout <<"EventNumber: "<<EventNumber<<", pass_selection: "<<pass_selection<<", pass_reconstruction: "<<pass_reconstruction<<", numtracksinev: "<<numtracksinev<<std::endl;

  //Fill histograms in case of passed selection cuts
  if (pass_selection && pass_reconstruction){

    //Get reconstructed information
    this->GetParticleInformation();

    //Get truth information (if MC)
    if (isMC) this->GetMCTruthInformation();

    //Fill ROOT histograms
    if (save_root) this->FillHistograms();

    //Save variables to BoostStore
    if (save_bs) this->SaveBoostStore();

  }

  return true;
}


bool NeutronMultiplicity::Finalise(){

  //Fill TGraphErrors
  this->FillTGraphs();

  if (save_root) {
    file_neutronmult->Write();
 
    dir_graph->cd();
    gr_neutrons_muonE->Write("gr_neutrons_muonE");
    gr_neutrons_muonE_fv->Write("gr_neutrons_muonE_fv");
    gr_neutrons_muonE_zoom->Write("gr_neutrons_muonE_zoom");
    gr_neutrons_muonE_fv_zoom->Write("gr_neutrons_muonE_fv_zoom");
    gr_neutrons_muonCosTheta->Write("gr_neutrons_muonCosTheta");
    gr_neutrons_muonCosTheta_fv->Write("gr_neutrons_muonCosTheta_fv");
    gr_neutrons_pT->Write("gr_neutrons_pT"); 
    gr_neutrons_pT_fv->Write("gr_neutrons_pT_fv"); 

    dir_graph_mc->cd();
    gr_primneutrons_muonE->Write("gr_primneutrons_muonE");
    gr_totalneutrons_muonE->Write("gr_totalneutrons_muonE");
    gr_pmtvolneutrons_muonE->Write("gr_pmtvolneutrons_muonE");
    gr_primneutrons_muonE_fv->Write("gr_primneutrons_muonE_fv");
    gr_totalneutrons_muonE_fv->Write("gr_totalneutrons_muonE_fv");
    gr_pmtvolneutrons_muonE_fv->Write("gr_pmtvolneutrons_muonE_fv");
    gr_primneutrons_muonE_zoom->Write("gr_primneutrons_muonE_zoom");
    gr_totalneutrons_muonE_zoom->Write("gr_totalneutrons_muonE_zoom");
    gr_pmtvolneutrons_muonE_zoom->Write("gr_pmtvolneutrons_muonE_zoom");
    gr_primneutrons_muonE_fv_zoom->Write("gr_primneutrons_muonE_fv_zoom");
    gr_totalneutrons_muonE_fv_zoom->Write("gr_totalneutrons_muonE_fv_zoom");
    gr_pmtvolneutrons_muonE_fv_zoom->Write("gr_pmtvolneutrons_muonE_fv_zoom");
    gr_primneutrons_muonCosTheta->Write("gr_primneutrons_muonCosTheta");
    gr_totalneutrons_muonCosTheta->Write("gr_totalneutrons_muonCosTheta");
    gr_pmtvolneutrons_muonCosTheta->Write("gr_pmtvolneutrons_muonCosTheta");
    gr_primneutrons_muonCosTheta_fv->Write("gr_primneutrons_muonCosTheta_fv");
    gr_totalneutrons_muonCosTheta_fv->Write("gr_totalneutrons_muonCosTheta_fv");
    gr_pmtvolneutrons_muonCosTheta_fv->Write("gr_pmtvolneutrons_muonCosTheta_fv");
    gr_primneutrons_pT->Write("gr_primneutrons_pT"); 
    gr_totalneutrons_pT->Write("gr_totalneutrons_pT"); 
    gr_pmtvolneutrons_pT->Write("gr_pmtvolneutrons_pT"); 
    gr_primneutrons_pT_fv->Write("gr_primneutrons_pT_fv"); 
    gr_totalneutrons_pT_fv->Write("gr_totalneutrons_pT_fv"); 
    gr_pmtvolneutrons_pT_fv->Write("gr_pmtvolneutrons_pT_fv"); 

    dir_graph_eff->cd();
    gr_eff_muonE->Write("gr_eff_muonE");
    gr_eff_muonE_fv->Write("gr_eff_muonE_fv");
    gr_eff_muonE_zoom->Write("gr_eff_muonE_zoom");
    gr_eff_muonE_fv_zoom->Write("gr_eff_muonE_fv_zoom");
    gr_eff_costheta->Write("gr_eff_costheta");
    gr_eff_costheta_fv->Write("gr_eff_costheta_fv");
    gr_eff_pT->Write("gr_eff_pT");
    gr_eff_pT_fv->Write("gr_eff_pT_fv");

    dir_graph_corr->cd();
    gr_neutrons_muonE_corr->Write("gr_neutrons_muonE_corr");
    gr_neutrons_muonE_corr_fv->Write("gr_neutrons_muonE_corr_fv");
    gr_neutrons_muonE_corr_zoom->Write("gr_neutrons_muonE_corr_zoom");
    gr_neutrons_muonE_corr_fv_zoom->Write("gr_neutrons_muonE_corr_fv_zoom");
    gr_neutrons_muonCosTheta_corr->Write("gr_neutrons_muonCosTheta_corr");
    gr_neutrons_muonCosTheta_corr_fv->Write("gr_neutrons_muonCosTheta_corr_fv");
    gr_neutrons_pT_corr->Write("gr_neutrons_pT_corr"); 
    gr_neutrons_pT_corr_fv->Write("gr_neutrons_pT_corr_fv"); 
    delete file_neutronmult;
  }

  store_neutronmult->Close();
  store_neutronmult->Delete();
  delete store_neutronmult;
  
  return true;
}

bool NeutronMultiplicity::InitialiseHistograms(){
 
  Log("NeutronMultiplicity tool: InitialiseHistograms",2,verbosity);
 
  //Initialise truevtx 
  truevtx = new RecoVertex();

  //output TFile
  std::stringstream ss_filename;
  ss_filename << filename << ".root";
  file_neutronmult = new TFile(ss_filename.str().c_str(),"RECREATE");

  dir_overall = (TDirectory*) gDirectory->CurrentDirectory();

  Log("NeutronMultiplicity tool: Define histograms",2,verbosity);

  dir_muon = (TDirectory*) file_neutronmult->mkdir("hist_muon");
  dir_neutron = (TDirectory*) file_neutronmult->mkdir("hist_neutron");
  dir_mc = (TDirectory*) file_neutronmult->mkdir("hist_mc");
  dir_eff = (TDirectory*) file_neutronmult->mkdir("hist_eff");
  dir_graph = (TDirectory*) file_neutronmult->mkdir("graph_neutron");
  dir_graph_mc = (TDirectory*) file_neutronmult->mkdir("graph_neutron_mc");
  dir_graph_eff = (TDirectory*) file_neutronmult->mkdir("graph_neutron_eff");
  dir_graph_corr = (TDirectory*) file_neutronmult->mkdir("graph_neutron_corr");

  //output histograms
  dir_muon->cd();
  h_muon_energy = new TH1F("h_muon_energy","Muon energy distribution",100,0,2000);
  h_muon_energy_fv = new TH1F("h_muon_energy_fv","Muon energy distribution (FV)",100,0,2000);
  h_muon_vtx_yz = new TH2F("h_muon_vtx_yz","Muon vertex (tank) Y-Z",200,-3,3,200,-3,3);
  h_muon_vtx_xz = new TH2F("h_muon_vtx_xz","Muon vertex (tank) X-Z",200,-2.5,2.5,200,-2.5,2.5);
  h_muon_costheta = new TH1F("h_muon_costheta","Muon cos(#theta) distribution",100,-1,1);
  h_muon_costheta_fv = new TH1F("h_muon_costheta_fv","Muon cos(#theta) distribution (FV)",100,-1,1);
  h_muon_vtx_x = new TH1F("h_muon_vtx_x","Muon vertex (x)",200,-3,3);
  h_muon_vtx_y = new TH1F("h_muon_vtx_y","Muon vertex (y)",200,-3,3);
  h_muon_vtx_z = new TH1F("h_muon_vtx_z","Muon vertex (z)",200,-3,3);

  dir_neutron->cd();
  h_neutrons = new TH1F("h_neutrons","Number of neutrons in beam events",10,0,10);
  h_neutrons_mrdstop = new TH1F("h_neutrons_mrdstop","Number of of neutrons in beam events (MRD stop)",10,0,10);
  h_neutrons_mrdstop_fv = new TH1F("h_neutrons_mrdstop_fv","Number of of neutrons in beam events (MRD stop, FV)",10,0,10);
  h_time_neutrons = new TH1F("h_time_neutrons","Cluster time beam neutrons",200,10000,70000);
  h_time_neutrons_mrdstop = new TH1F("h_time_neutrons_mrdstop","Cluster time beam neutrons",200,10000,70000);
  h_neutrons_energy = new TH2F("h_neutrons_energy","Neutron multiplicity vs muon energy",10,0,2000,20,0,20);
  h_neutrons_energy_fv = new TH2F("h_neutrons_energy_fv","Neutron multiplicity vs muon energy (FV)",10,0,2000,20,0,20);
  h_neutrons_energy_zoom = new TH2F("h_neutrons_energy_zoom","Neutron multiplicity vs muon energy",8,400,1200,20,0,20);
  h_neutrons_energy_fv_zoom = new TH2F("h_neutrons_energy_fv_zoom","Neutron multiplicity vs muon energy (FV)",6,600,1200,20,0,20);
  h_neutrons_energy_fv_down = new TH2F("h_neutrons_energy_fv_down","Neutron multiplicity vs muon energy (Downstream FV)",10,0,2000,20,0,20);
  h_neutrons_energy_fv_cyl = new TH2F("h_neutrons_energy_fv_cyl","Neutron multiplicity vs muon energy (Cylindrical FV)",10,0,2000,20,0,20);
  h_neutrons_q2 = new TH2F("h_neutrons_q2","Neutron multiplicity vs Q^{2}",100,0,2,20,0,20);
  h_neutrons_q2_fv = new TH2F("h_neutrons_q2_fv","Neutron multiplicity vs Q^{2} (FV)",100,0,2,20,0,20);
  h_neutrons_q2_fv_down = new TH2F("h_neutrons_q2_fv_down","Neutron multiplicity vs Q^{2} (Downstream FV)",100,0,2,20,0,20);
  h_neutrons_costheta = new TH2F("h_neutrons_costheta","Neutron multiplicity vs muon angle",6,0.7,1.0,20,0,20);
  h_neutrons_costheta_fv = new TH2F("h_neutrons_costheta_fv","Neutron multiplicity vs muon angle (FV)",6,0.7,1.0,20,0,20);
  h_neutrons_pT = new TH2F("h_neutrons_pT","Neutron multiplicity vs transverse muon momentum",10,0,1000,20,0,20);
  h_neutrons_pT_fv = new TH2F("h_neutrons_pT_fv","Neutron multiplicity vs transverse muon momentum (FV)",6,0,600,20,0,20);

  dir_mc->cd();
  h_primneutrons_energy = new TH2F("h_primneutrons_energy","Primary Neutron multiplicity vs muon energy",10,0,2000,20,0,20);
  h_primneutrons_energy_fv = new TH2F("h_primneutrons_energy_fv","Primary Neutron multiplicity vs muon energy (FV)",10,0,2000,20,0,20);
  h_primneutrons_energy_zoom = new TH2F("h_primneutrons_energy_zoom","Primary Neutron multiplicity vs muon energy",8,400,1200,20,0,20);
  h_primneutrons_energy_fv_zoom = new TH2F("h_primneutrons_energy_fv_zoom","Primary Neutron multiplicity vs muon energy (FV)",6,600,1200,20,0,20);
  h_totalneutrons_energy = new TH2F("h_totalneutrons_energy","Total Neutron multiplicity vs muon energy",10,0,2000,20,0,20);
  h_totalneutrons_energy_fv = new TH2F("h_totalneutrons_energy_fv","Total Neutron multiplicity vs muon energy (FV)",10,0,2000,20,0,20);
  h_totalneutrons_energy_zoom = new TH2F("h_totalneutrons_energy_zoom","Total Neutron multiplicity vs muon energy",8,400,1200,20,0,20);
  h_totalneutrons_energy_fv_zoom = new TH2F("h_totalneutrons_energy_fv_zoom","Total Neutron multiplicity vs muon energy (FV)",6,600,1200,20,0,20);
  h_pmtvolneutrons_energy = new TH2F("h_pmtvolneutrons_energy","PMTVol Neutron multiplicity vs muon energy",10,0,2000,20,0,20);
  h_pmtvolneutrons_energy_fv = new TH2F("h_pmtvolneutrons_energy_fv","PMTVol Neutron multiplicity vs muon energy (FV)",10,0,2000,20,0,20);
  h_pmtvolneutrons_energy_zoom = new TH2F("h_pmtvolneutrons_energy_zoom","PMTVol Neutron multiplicity vs muon energy",8,400,1200,20,0,20);
  h_pmtvolneutrons_energy_fv_zoom = new TH2F("h_pmtvolneutrons_energy_fv_zoom","PMTVol Neutron multiplicity vs muon energy (FV)",6,600,1200,20,0,20);
  h_primneutrons_costheta = new TH2F("h_primneutrons_costheta","Primary Neutron multiplicity vs muon angle",6,0.7,1.0,20,0,20);
  h_primneutrons_costheta_fv = new TH2F("h_primvolneutrons_costheta_fv","Primary Neutron multiplicity vs muon angle (FV)",6,0.7,1.0,20,0,20);
  h_totalneutrons_costheta = new TH2F("h_totalneutrons_costheta","Total Neutron multiplicity vs muon angle",6,0.7,1.0,20,0,20);
  h_totalneutrons_costheta_fv = new TH2F("h_totalneutrons_costheta_fv","Total Neutron multiplicity vs muon angle (FV)",6,0.7,1.0,20,0,20);
  h_pmtvolneutrons_costheta = new TH2F("h_pmtvolneutrons_costheta","PMTVol Neutron multiplicity vs muon angle",6,0.7,1.0,20,0,20);
  h_pmtvolneutrons_costheta_fv = new TH2F("h_pmtvolneutrons_costheta_fv","PMTVol Neutron multiplicity vs muon angle (FV)",6,0.7,1.0,20,0,20);
  h_primneutrons_pT = new TH2F("h_primneutrons_pT","Primary Neutron multiplicity vs transverse muon momentum",10,0,1000,20,0,20);
  h_primneutrons_pT_fv = new TH2F("h_primneutrons_pT_fv","Primary Neutron multiplicity vs transverse muon momentum (FV)",6,0,600,20,0,20);
  h_totalneutrons_pT = new TH2F("h_totalneutrons_pT","Total Neutron multiplicity vs transverse muon momentum",10,0,1000,20,0,20);
  h_totalneutrons_pT_fv = new TH2F("h_totalneutrons_pT_fv","Total Neutron multiplicity vs transverse muon momentum (FV)",6,0,600,20,0,20);
  h_pmtvolneutrons_pT = new TH2F("h_pmtvolneutrons_pT","PMTVol Neutron multiplicity vs transverse muon momentum",10,0,1000,20,0,20);
  h_pmtvolneutrons_pT_fv = new TH2F("h_pmtvolneutrons_pT_fv","PMTVol Neutron multiplicity vs transverse muon momentum (FV)",6,0,600,20,0,20);

  dir_eff->cd();
  hist_neutron_eff = new TH3F("hist_neutron_eff","Neutron efficiency",100,-3,3,100,-3,3,100,-1,5);
  h_eff_energy = new TH2F("h_eff_energy","Neutron detection efficiency vs muon energy",10,0,2000,100,0,1.0);
  h_eff_energy_fv = new TH2F("h_eff_energy_fv","Neutron detection efficiency vs muon energy (FV)",10,0,2000,100,0,1.0);
  h_eff_energy_zoom = new TH2F("h_eff_energy_zoom","Neutron detection efficiency vs muon energy",8,400,1200,100,0,1.0);
  h_eff_energy_fv_zoom = new TH2F("h_eff_energy_fv_zoom","Neutron detection efficiency vs muon energy (FV)",6,600,1200,100,0,1.0);
  h_eff_costheta = new TH2F("h_eff_costheta","Neutron detection efficiency vs muon angle",6,0.7,1.0,100,0,1.0);
  h_eff_costheta_fv = new TH2F("h_eff_costheta_fv","Neutron detection efficiency vs muon angle (FV)",6,0.7,1.0,100,0,1.0);
  h_eff_pT = new TH2F("h_eff_pT","Neutron detection efficiency vs transverse muon momentum",10,0,1000,100,0,1.0);
  h_eff_pT_fv = new TH2F("h_eff_pT_fv","Neutron detection efficiency vs transverse muon momentum (FV)",6,0,600,100,0,1.0);


  //Set properties of histograms (axes titles, etc)
  h_neutrons->SetLineWidth(2);
  h_neutrons->SetStats(0);
  h_neutrons->GetXaxis()->SetTitle("Number of neutrons");
  h_neutrons->GetYaxis()->SetTitle("#");

  h_neutrons_mrdstop->SetLineWidth(2);
  h_neutrons_mrdstop->SetLineColor(kBlack);
  h_neutrons_mrdstop->SetStats(0);
  h_neutrons_mrdstop->GetXaxis()->SetTitle("Number of neutrons");
  h_neutrons_mrdstop->GetYaxis()->SetTitle("#");

  h_neutrons_mrdstop_fv->SetLineWidth(2);
  h_neutrons_mrdstop_fv->SetStats(0);
  h_neutrons_mrdstop_fv->SetLineColor(kRed);
  h_neutrons_mrdstop_fv->GetXaxis()->SetTitle("Number of neutrons");
  h_neutrons_mrdstop_fv->GetYaxis()->SetTitle("#");

  h_neutrons_energy->SetStats(0);
  h_neutrons_energy->GetXaxis()->SetTitle("E_{#mu} [MeV]");
  h_neutrons_energy->GetYaxis()->SetTitle("Number of neutrons");

  h_neutrons_energy_zoom->SetStats(0);
  h_neutrons_energy_zoom->GetXaxis()->SetTitle("E_{#mu} [MeV]");
  h_neutrons_energy_zoom->GetYaxis()->SetTitle("Number of neutrons");

  h_neutrons_energy_fv->SetStats(0);
  h_neutrons_energy_fv->GetXaxis()->SetTitle("E_{#mu} [MeV]");
  h_neutrons_energy_fv->GetYaxis()->SetTitle("Number of neutrons");

  h_neutrons_energy_fv_zoom->SetStats(0);
  h_neutrons_energy_fv_zoom->GetXaxis()->SetTitle("E_{#mu} [MeV]");
  h_neutrons_energy_fv_zoom->GetYaxis()->SetTitle("Number of neutrons");

  h_neutrons_energy_fv_down->SetStats(0);
  h_neutrons_energy_fv_down->GetXaxis()->SetTitle("E_{#mu} [MeV]");
  h_neutrons_energy_fv_down->GetYaxis()->SetTitle("Number of neutrons");

  h_neutrons_energy_fv_cyl->SetStats(0);
  h_neutrons_energy_fv_cyl->GetXaxis()->SetTitle("E_{#mu} [MeV]");
  h_neutrons_energy_fv_cyl->GetYaxis()->SetTitle("Number of neutrons");

  h_neutrons_q2->SetStats(0);
  h_neutrons_q2->GetXaxis()->SetTitle("Q^{2} [(GeV/c)^{2}]");
  h_neutrons_q2->GetYaxis()->SetTitle("Number of neutrons");

  h_neutrons_q2_fv->SetStats(0);
  h_neutrons_q2_fv->GetXaxis()->SetTitle("Q^{2} [(GeV/c)^{2}]");
  h_neutrons_q2_fv->GetYaxis()->SetTitle("Number of neutrons");

  h_neutrons_q2_fv_down->SetStats(0);
  h_neutrons_q2_fv_down->GetXaxis()->SetTitle("Q^{2} [(GeV/c)^{2}]");
  h_neutrons_q2_fv_down->GetYaxis()->SetTitle("Number of neutrons");

  h_neutrons_costheta->SetStats(0);
  h_neutrons_costheta->GetXaxis()->SetTitle("cos(#theta)");
  h_neutrons_costheta->GetYaxis()->SetTitle("Number of neutrons");

  h_neutrons_costheta_fv->SetStats(0);
  h_neutrons_costheta_fv->GetXaxis()->SetTitle("cos(#theta)");
  h_neutrons_costheta_fv->GetYaxis()->SetTitle("Number of neutrons");

  h_neutrons_pT->SetStats(0);
  h_neutrons_pT->GetXaxis()->SetTitle("p_{T} [MeV]");
  h_neutrons_pT->GetYaxis()->SetTitle("Number of neutrons");

  h_neutrons_pT_fv->SetStats(0);
  h_neutrons_pT_fv->GetXaxis()->SetTitle("p_{T} [MeV]");
  h_neutrons_pT_fv->GetYaxis()->SetTitle("Number of neutrons");

  h_muon_energy->GetXaxis()->SetTitle("E_{#mu} [MeV]");
  h_muon_energy->GetYaxis()->SetTitle("#");
  h_muon_energy->SetStats(0);
  h_muon_energy->SetLineWidth(2);
  h_muon_energy_fv->SetStats(0);
  h_muon_energy_fv->SetLineColor(kRed);
  h_muon_energy_fv->SetLineWidth(2);

  h_muon_costheta->GetXaxis()->SetTitle("cos(#theta)");
  h_muon_costheta->GetYaxis()->SetTitle("#");
  h_muon_costheta->SetStats(0);
  h_muon_costheta->SetLineWidth(2);

  h_muon_costheta_fv->GetXaxis()->SetTitle("cos(#theta)");
  h_muon_costheta_fv->GetYaxis()->SetTitle("#");
  h_muon_costheta_fv->SetStats(0);
  h_muon_costheta_fv->SetLineWidth(2);
  h_muon_costheta_fv->SetLineColor(kRed);


  h_muon_vtx_yz->GetXaxis()->SetTitle("Vertex Z [m]");
  h_muon_vtx_yz->GetYaxis()->SetTitle("Vertex Y [m]");
  h_muon_vtx_yz->SetStats(0);
  h_muon_vtx_xz->GetXaxis()->SetTitle("Vertex Z [m]");
  h_muon_vtx_xz->GetYaxis()->SetTitle("Vertex X [m]");
  h_muon_vtx_xz->SetStats(0);

  h_muon_vtx_x->GetXaxis()->SetTitle("Vertex X [m]");
  h_muon_vtx_x->GetYaxis()->SetTitle("#");
  h_muon_vtx_y->GetXaxis()->SetTitle("Vertex Y [m]");
  h_muon_vtx_y->GetYaxis()->SetTitle("#");
  h_muon_vtx_z->GetXaxis()->SetTitle("Vertex Z [m]");
  h_muon_vtx_z->GetYaxis()->SetTitle("#");

  //Define TGraphs
  gr_neutrons_muonE = new TGraphErrors();
  gr_neutrons_muonE_fv = new TGraphErrors();
  gr_neutrons_muonE_zoom = new TGraphErrors();
  gr_neutrons_muonE_fv_zoom = new TGraphErrors();
  gr_neutrons_muonCosTheta = new TGraphErrors();
  gr_neutrons_muonCosTheta_fv = new TGraphErrors();
  gr_neutrons_pT = new TGraphErrors();
  gr_neutrons_pT_fv = new TGraphErrors();

  gr_primneutrons_muonE = new TGraphErrors();
  gr_totalneutrons_muonE = new TGraphErrors();
  gr_pmtvolneutrons_muonE = new TGraphErrors();
  gr_primneutrons_muonE_fv = new TGraphErrors();
  gr_totalneutrons_muonE_fv = new TGraphErrors();
  gr_pmtvolneutrons_muonE_fv = new TGraphErrors();
  gr_primneutrons_muonE_zoom = new TGraphErrors();
  gr_totalneutrons_muonE_zoom = new TGraphErrors();
  gr_pmtvolneutrons_muonE_zoom = new TGraphErrors();
  gr_primneutrons_muonE_fv_zoom = new TGraphErrors();
  gr_totalneutrons_muonE_fv_zoom = new TGraphErrors();
  gr_pmtvolneutrons_muonE_fv_zoom = new TGraphErrors();
  gr_primneutrons_muonCosTheta = new TGraphErrors();
  gr_totalneutrons_muonCosTheta = new TGraphErrors();
  gr_pmtvolneutrons_muonCosTheta = new TGraphErrors();
  gr_primneutrons_muonCosTheta_fv = new TGraphErrors();
  gr_totalneutrons_muonCosTheta_fv = new TGraphErrors();
  gr_pmtvolneutrons_muonCosTheta_fv = new TGraphErrors();
  gr_primneutrons_pT = new TGraphErrors();
  gr_totalneutrons_pT = new TGraphErrors();
  gr_pmtvolneutrons_pT = new TGraphErrors();
  gr_primneutrons_pT_fv = new TGraphErrors();
  gr_totalneutrons_pT_fv = new TGraphErrors();
  gr_pmtvolneutrons_pT_fv = new TGraphErrors();

  gr_eff_muonE = new TGraphErrors();
  gr_eff_muonE_fv = new TGraphErrors();
  gr_eff_muonE_zoom = new TGraphErrors();
  gr_eff_muonE_fv_zoom = new TGraphErrors();
  gr_eff_costheta = new TGraphErrors();
  gr_eff_costheta_fv = new TGraphErrors();
  gr_eff_pT = new TGraphErrors();
  gr_eff_pT_fv = new TGraphErrors();

  gr_neutrons_muonE_corr = new TGraphErrors();
  gr_neutrons_muonE_corr_fv = new TGraphErrors();
  gr_neutrons_muonE_corr_zoom = new TGraphErrors();
  gr_neutrons_muonE_corr_fv_zoom = new TGraphErrors();
  gr_neutrons_muonCosTheta_corr = new TGraphErrors();
  gr_neutrons_muonCosTheta_corr_fv = new TGraphErrors();
  gr_neutrons_pT_corr = new TGraphErrors();
  gr_neutrons_pT_corr_fv = new TGraphErrors();

  //Define tree properties and variables
  //

  dir_overall->cd();

  true_NeutVtxX = new std::vector<double>;
  true_NeutVtxY = new std::vector<double>;
  true_NeutVtxZ = new std::vector<double>;
  true_NeutCapNucl = new std::vector<double>;
  true_NeutCapTime = new std::vector<double>;
  true_NeutCapETotal = new std::vector<double>;
  true_NeutCapNGamma = new std::vector<double>;
  true_NeutCapPrimary = new std::vector<int>;
  reco_ClusterCB = new std::vector<double>;
  reco_ClusterTime = new std::vector<double>;
  reco_ClusterPE = new std::vector<double>;
  reco_NCandCB = new std::vector<double>;
  reco_NCandTime = new std::vector<double>;
  reco_NCandPE = new std::vector<double>;
  true_PrimaryPdgs = new std::vector<int>;

  neutron_tree = new TTree("neutron_tree","Neutron tree");
  neutron_tree->Branch("RunNumber",&run_nr);
  neutron_tree->Branch("EventNumber",&ev_nr);
  neutron_tree->Branch("RecoTankMRDCoinc",&reco_TankMRDCoinc);
  neutron_tree->Branch("RecoNCandidates",&reco_NCandidates);
  neutron_tree->Branch("RecoEmu",&reco_Emu);
  neutron_tree->Branch("RecoEnu",&reco_Enu);
  neutron_tree->Branch("RecoQ2",&reco_Q2);
  neutron_tree->Branch("RecoPt",&reco_pT);
  neutron_tree->Branch("RecoVtxX",&reco_VtxX);
  neutron_tree->Branch("RecoVtxY",&reco_VtxY);
  neutron_tree->Branch("RecoVtxZ",&reco_VtxZ);
  neutron_tree->Branch("RecoFV",&reco_FV);
  neutron_tree->Branch("RecoCosTheta",&reco_CosTheta);
  neutron_tree->Branch("RecoClusters",&reco_Clusters);
  neutron_tree->Branch("RecoClusterCB",&reco_ClusterCB);
  neutron_tree->Branch("RecoClusterTime",&reco_ClusterTime);
  neutron_tree->Branch("RecoClusterPE",&reco_ClusterPE);
  neutron_tree->Branch("RecoNCandCB",&reco_NCandCB);
  neutron_tree->Branch("RecoNCandTime",&reco_NCandTime);
  neutron_tree->Branch("RecoNCandPE",&reco_NCandPE);
  neutron_tree->Branch("RecoMrdEnergyLoss",&reco_MrdEnergyLoss);
  neutron_tree->Branch("RecoTrackLengthInMRD",&reco_TrackLengthInMRD);
  neutron_tree->Branch("RecoMrdStartX",&reco_MrdStartX);
  neutron_tree->Branch("RecoMrdStartY",&reco_MrdStartY);
  neutron_tree->Branch("RecoMrdStartZ",&reco_MrdStartZ);
  neutron_tree->Branch("RecoMrdStopX",&reco_MrdStopX);
  neutron_tree->Branch("RecoMrdStopY",&reco_MrdStopY);
  neutron_tree->Branch("RecoMrdStopZ",&reco_MrdStopZ);
  neutron_tree->Branch("TrueEmu",&true_Emu);
  neutron_tree->Branch("TrueEnu",&true_Enu);
  neutron_tree->Branch("TrueQ2",&true_Q2);
  neutron_tree->Branch("TruePt",&true_pT);
  neutron_tree->Branch("TrueVtxX",&true_VtxX);
  neutron_tree->Branch("TrueVtxY",&true_VtxY);
  neutron_tree->Branch("TrueVtxZ",&true_VtxZ);
  neutron_tree->Branch("TrueVtxTime",&true_VtxTime);
  neutron_tree->Branch("TrueDirX",&true_DirX);
  neutron_tree->Branch("TrueDirY",&true_DirY);
  neutron_tree->Branch("TrueDirZ",&true_DirZ);
  neutron_tree->Branch("TrueCosTheta",&true_CosTheta);
  neutron_tree->Branch("TrueFV",&true_FV);
  neutron_tree->Branch("TrueTrackLengthInWater",&true_TrackLengthInWater);
  neutron_tree->Branch("TrueTrackLengthInMRD",&true_TrackLengthInMRD);
  neutron_tree->Branch("TruePrimNeut",&true_PrimNeut);
  neutron_tree->Branch("TruePrimProt",&true_PrimProt);
  neutron_tree->Branch("TrueNCaptures",&true_NCaptures);
  neutron_tree->Branch("TrueNCapturesPMTVol",&true_NCapturesPMTVol);
  neutron_tree->Branch("TrueNeutVtxX",&true_NeutVtxX);
  neutron_tree->Branch("TrueNeutVtxY",&true_NeutVtxY);
  neutron_tree->Branch("TrueNeutVtxZ",&true_NeutVtxZ);
  neutron_tree->Branch("TrueNeutCapNucl",&true_NeutCapNucl);
  neutron_tree->Branch("TrueNeutCapTime",&true_NeutCapTime);
  neutron_tree->Branch("TrueNeutCapETotal",&true_NeutCapETotal);
  neutron_tree->Branch("TrueNeutCapNGamma",&true_NeutCapNGamma);
  neutron_tree->Branch("TrueNeutCapPrimary",&true_NeutCapPrimary);
  neutron_tree->Branch("TrueCC",&true_CC);
  neutron_tree->Branch("TrueQEL",&true_QEL);
  neutron_tree->Branch("TrueDIS",&true_DIS);
  neutron_tree->Branch("TrueRES",&true_RES);
  neutron_tree->Branch("TrueMEC",&true_MEC);
  neutron_tree->Branch("TrueCOH",&true_COH);
  neutron_tree->Branch("TrueMultiRing",&true_MultiRing);
  neutron_tree->Branch("TruePrimaryPdgs",&true_PrimaryPdgs);
  neutron_tree->Branch("TruePdgPrimary",&true_PdgPrimary);

  return true;

}

bool NeutronMultiplicity::FillHistograms(){
 
  h_neutrons_mrdstop->Fill(NumberNeutrons);
  h_neutrons_energy->Fill(SimpleRecoEnergy,NumberNeutrons);
  h_neutrons_energy_zoom->Fill(SimpleRecoEnergy,NumberNeutrons);
  h_neutrons_q2->Fill(SimpleRecoQ2/1.e6,NumberNeutrons);
  h_neutrons_costheta->Fill(SimpleRecoCosTheta,NumberNeutrons);
  h_neutrons_pT->Fill(SimpleRecoPt,NumberNeutrons);
  h_muon_energy->Fill(SimpleRecoEnergy);
  h_muon_costheta->Fill(SimpleRecoCosTheta);
  if (isMC){
    h_primneutrons_energy->Fill(SimpleRecoEnergy,true_PrimNeut);
    h_primneutrons_energy_zoom->Fill(SimpleRecoEnergy,true_PrimNeut);
    h_totalneutrons_energy->Fill(SimpleRecoEnergy,true_NCaptures);
    h_totalneutrons_energy_zoom->Fill(SimpleRecoEnergy,true_NCaptures);
    h_pmtvolneutrons_energy->Fill(SimpleRecoEnergy,true_NCapturesPMTVol);
    h_pmtvolneutrons_energy_zoom->Fill(SimpleRecoEnergy,true_NCapturesPMTVol);
    h_primneutrons_costheta->Fill(SimpleRecoCosTheta,true_PrimNeut);
    h_totalneutrons_costheta->Fill(SimpleRecoCosTheta,true_NCaptures);
    h_pmtvolneutrons_costheta->Fill(SimpleRecoCosTheta,true_NCapturesPMTVol);
    h_primneutrons_pT->Fill(SimpleRecoPt,true_PrimNeut);
    h_totalneutrons_pT->Fill(SimpleRecoPt,true_NCaptures);
    h_pmtvolneutrons_pT->Fill(SimpleRecoPt,true_NCapturesPMTVol);
  }

  if (SimpleRecoFV) {
    h_muon_energy_fv->Fill(SimpleRecoEnergy);
    h_muon_costheta_fv->Fill(SimpleRecoCosTheta);
    h_neutrons_energy_fv->Fill(SimpleRecoEnergy,NumberNeutrons);
    h_neutrons_energy_fv_zoom->Fill(SimpleRecoEnergy,NumberNeutrons);
    h_neutrons_q2_fv->Fill(SimpleRecoQ2/1.e6,NumberNeutrons);
    h_neutrons_costheta_fv->Fill(SimpleRecoCosTheta,NumberNeutrons);
    h_neutrons_pT_fv->Fill(SimpleRecoPt,NumberNeutrons);
    h_neutrons_mrdstop_fv->Fill(NumberNeutrons);
    if (isMC){
      h_primneutrons_energy_fv->Fill(SimpleRecoEnergy,true_PrimNeut);
      h_primneutrons_energy_fv_zoom->Fill(SimpleRecoEnergy,true_PrimNeut);
      h_totalneutrons_energy_fv->Fill(SimpleRecoEnergy,true_NCaptures);
      h_totalneutrons_energy_fv_zoom->Fill(SimpleRecoEnergy,true_NCaptures);
      h_pmtvolneutrons_energy_fv->Fill(SimpleRecoEnergy,true_NCapturesPMTVol);
      h_pmtvolneutrons_energy_fv_zoom->Fill(SimpleRecoEnergy,true_NCapturesPMTVol);
      h_primneutrons_costheta_fv->Fill(SimpleRecoCosTheta,true_PrimNeut);
      h_totalneutrons_costheta_fv->Fill(SimpleRecoCosTheta,true_NCaptures);
      h_pmtvolneutrons_costheta_fv->Fill(SimpleRecoCosTheta,true_NCapturesPMTVol);
      h_primneutrons_pT_fv->Fill(SimpleRecoPt,true_PrimNeut);
      h_totalneutrons_pT_fv->Fill(SimpleRecoPt,true_NCaptures);
      h_pmtvolneutrons_pT_fv->Fill(SimpleRecoPt,true_NCapturesPMTVol);
    }
  }
  std::cout << " NM [debug] DownstreamFV/FullCylFV: " << DownstreamFV << "/" << FullCylFV << std::endl;
  if (DownstreamFV == 1)
  {
    h_neutrons_energy_fv_down->Fill(SimpleRecoEnergy,NumberNeutrons);
    h_neutrons_q2_fv_down->Fill(SimpleRecoQ2/1.e6,NumberNeutrons);
  }
  if (FullCylFV == 1) h_neutrons_energy_fv_cyl->Fill(SimpleRecoEnergy,NumberNeutrons);


  h_muon_vtx_x->Fill(SimpleRecoVtx.X());
  h_muon_vtx_y->Fill(SimpleRecoVtx.Y());
  h_muon_vtx_z->Fill(SimpleRecoVtx.Z());
  //h_muon_vtx_yz->Fill(SimpleRecoVtx.Z()-1.681,SimpleRecoVtx.Y()-0.144);
  //h_muon_vtx_xz->Fill(SimpleRecoVtx.Z()-1.681,SimpleRecoVtx.X());
  h_muon_vtx_yz->Fill(SimpleRecoVtx.Z(),SimpleRecoVtx.Y());
  h_muon_vtx_xz->Fill(SimpleRecoVtx.Z(),SimpleRecoVtx.X());

  reco_VtxX = SimpleRecoVtx.X();
  reco_VtxY = SimpleRecoVtx.Y();
  reco_VtxZ = SimpleRecoVtx.Z();
  reco_Emu = SimpleRecoEnergy;
  reco_CosTheta = SimpleRecoCosTheta;
  reco_pT = SimpleRecoPt;
  reco_NCandidates = NumberNeutrons;
  reco_FV = SimpleRecoFV;
  reco_MrdEnergyLoss = SimpleRecoMrdEnergyLoss;
  reco_TrackLengthInMRD = SimpleRecoTrackLengthInMRD;
  reco_MrdStartX = SimpleRecoMRDStart.X();
  reco_MrdStartY = SimpleRecoMRDStart.Y();
  reco_MrdStartZ = SimpleRecoMRDStart.Z();
  reco_MrdStopX = SimpleRecoMRDStop.X();
  reco_MrdStopY = SimpleRecoMRDStop.Y();
  reco_MrdStopZ = SimpleRecoMRDStop.Z();

  for (int i_n=0; i_n < (int) reco_NCandTime->size(); i_n++){
    h_time_neutrons_mrdstop->Fill(reco_NCandTime->at(i_n));
  }

  //Fill efficiency histogram
  //Get efficiency for current vertex position

  //Unfortunately interpolation only works for a full rectangular grid of efficiency points, not unstructed data -> fix in future
  //double current_eff = hist_neutron_eff->Interpolate(reco_VtxX,reco_VtxY,reco_VtxZ);

  //For now check which AmBe calibration point is closest
  double min_dist = 99999999;
  double min_eff = 0.;
  for (std::map<std::vector<double>,double>::iterator it = neutron_eff_map.begin(); it!= neutron_eff_map.end(); it++)
  {
    std::vector<double> pos = it->first;
    double distX = reco_VtxX - pos.at(0);
    double distY = reco_VtxY - pos.at(1);
    double distZ = reco_VtxZ - pos.at(2);
    double dist = sqrt(distX*distX+distY*distY+distZ*distZ);
    if (dist < min_dist) {
      min_dist = dist;
      min_eff = it->second;
    }
  }
  double current_eff = min_eff;
  if (verbosity > 1){
    std::cout <<"NeutronMultiplicity tool: vertex: ("<<reco_VtxX<<", "<<reco_VtxY<<", "<<reco_VtxZ<<"), detection eff: "<<current_eff<<std::endl;
  }

  h_eff_energy->Fill(SimpleRecoEnergy,current_eff);
  h_eff_energy_zoom->Fill(SimpleRecoEnergy,current_eff);
  h_eff_costheta->Fill(SimpleRecoCosTheta,current_eff);
  h_eff_pT->Fill(SimpleRecoPt,current_eff);
  if (SimpleRecoFV) {
    h_eff_energy_fv->Fill(SimpleRecoEnergy,current_eff);
    h_eff_energy_fv_zoom->Fill(SimpleRecoEnergy,current_eff);
    h_eff_costheta_fv->Fill(SimpleRecoCosTheta,current_eff);
    h_eff_pT_fv->Fill(SimpleRecoPt,current_eff);
  }

  neutron_tree->Fill();

  return true;

}

bool NeutronMultiplicity::SaveBoostStore(){

  //Set BoostStore variables
  store_neutronmult->Set("RunNumber",RunNumber);
  store_neutronmult->Set("EventNumber",EventNumber);
  store_neutronmult->Set("EventCutStatus",true);
  store_neutronmult->Set("Particles",Particles);
  store_neutronmult->Set("SimpleRecoFlag",SimpleRecoFlag);
  store_neutronmult->Set("SimpleRecoEnergy",SimpleRecoEnergy);
  store_neutronmult->Set("SimpleRecoVtx",SimpleRecoVtx); 
  store_neutronmult->Set("SimpleRecoStopVtx",SimpleRecoStopVtx); 
  store_neutronmult->Set("SimpleRecoCosTheta",SimpleRecoCosTheta);
  store_neutronmult->Set("SimpleRecoPt",SimpleRecoPt);
  store_neutronmult->Set("SimpleRecoFV",SimpleRecoFV);
  store_neutronmult->Set("SimpleRecoMrdEnergyLoss",SimpleRecoMrdEnergyLoss);
  store_neutronmult->Set("SimpleRecoTrackLengthInMRD",SimpleRecoTrackLengthInMRD);
  store_neutronmult->Set("SimpleRecoMRDStart",SimpleRecoMRDStart);
  store_neutronmult->Set("SimpleRecoMRDStop",SimpleRecoMRDStop);
  store_neutronmult->Set("ClusterIndicesNeutron",cluster_neutron);
  store_neutronmult->Set("ClusterTimesNeutron",cluster_times_neutron);
  store_neutronmult->Set("ClusterChargesNeutron",cluster_charges_neutron);
  store_neutronmult->Set("ClusterCBNeutron",cluster_cb_neutron);
  store_neutronmult->Set("ClusterTimes",cluster_times);
  store_neutronmult->Set("ClusterCharges",cluster_charges);
  store_neutronmult->Set("ClusterCB",cluster_cb);
  store_neutronmult->Set("NeutronEffMap",neutron_eff_map);
  store_neutronmult->Set("PMTMRDCoinc",passPMTMRDCoincCut);
  store_neutronmult->Set("NumMRDTracks",numtracksinev);
  store_neutronmult->Set("DownstreamFV",DownstreamFV);
  store_neutronmult->Set("FullCylFV",FullCylFV);
  store_neutronmult->Set("SimpleRecoNeutrinoEnergy",SimpleRecoNeutrinoEnergy);
  store_neutronmult->Set("SimpleRecoQ2",SimpleRecoQ2);

  if (isMC){
    store_neutronmult->Set("NeutrinoEnergy",true_Enu);
    store_neutronmult->Set("EventQ2",true_Q2);
    store_neutronmult->Set("IsWeakCC",true_CC);
    store_neutronmult->Set("IsQuasiElastic",true_QEL);
    store_neutronmult->Set("IsResonant",true_RES);
    store_neutronmult->Set("IsDeepInelastic",true_DIS);
    store_neutronmult->Set("IsCoherent",true_COH);
    store_neutronmult->Set("IsMEC",true_MEC);
    store_neutronmult->Set("NumFSNeutrons",true_PrimNeut);
    store_neutronmult->Set("NumFSProtons",true_PrimProt);

    double temp_vtx_x = true_VtxX;
    double temp_vtx_y = true_VtxY;
    double temp_vtx_z = true_VtxZ;
    double temp_vtx_t = true_VtxTime;
    Log("NeutronMultiplicity tool: Set vertex with "+std::to_string(true_VtxX*100.)+", "+std::to_string((true_VtxY+0.144)*100.)+", "+std::to_string((true_VtxZ-1.681)*100.)+", "+std::to_string(true_VtxTime),v_message,verbosity);
    store_neutronmult->Set("TrueVertex_X",temp_vtx_x*100.);
    store_neutronmult->Set("TrueVertex_Y",(temp_vtx_y+0.144)*100.);
    store_neutronmult->Set("TrueVertex_Z",(temp_vtx_z-1.681)*100.);
    store_neutronmult->Set("TrueVertex_T",temp_vtx_t);
    store_neutronmult->Set("TrueVertex_DirX",true_DirX);
    store_neutronmult->Set("TrueVertex_DirY",true_DirY);
    store_neutronmult->Set("TrueVertex_DirZ",true_DirZ);
    
    store_neutronmult->Set("TrueMuonEnergy",true_Emu+105.66);
    store_neutronmult->Set("PrimaryPdgs",vec_true_PrimaryPdgs);
    store_neutronmult->Set("PdgPrimary",true_PdgPrimary);
    store_neutronmult->Set("TrueTrackLengthInWater",true_TrackLengthInWater);
    store_neutronmult->Set("TrueTrackLengthInMRD",true_TrackLengthInMRD);
    store_neutronmult->Set("MCMultiRingEvent",IsMultiRing);
    store_neutronmult->Set("MCNeutCap",MCNeutCap);
    store_neutronmult->Set("MCNeutCapGammas",MCNeutCapGammas);
    store_neutronmult->Set("MCFile",MCFile);
  }

  //Construct BoostStore filename
  std::stringstream ss_filename;
  ss_filename << filename << ".bs";
  Log("NeutronMultiplicity tool: Saving BoostStore file "+ss_filename.str(),2,verbosity);
    
  //Save BoostStore
  store_neutronmult->Save(ss_filename.str().c_str()); 
   
  return true;
}

bool NeutronMultiplicity::ReadBoostStore(){

  bool EventCutStatus = false;

  //Get BoostStore variables
  read_neutronmult->Get("RunNumber",RunNumber);
  read_neutronmult->Get("EventNumber",EventNumber);
  read_neutronmult->Get("EventCutStatus",EventCutStatus);
  read_neutronmult->Get("Particles",Particles);
  read_neutronmult->Get("SimpleRecoFlag",SimpleRecoFlag);
  read_neutronmult->Get("SimpleRecoEnergy",SimpleRecoEnergy);
  read_neutronmult->Get("SimpleRecoVtx",SimpleRecoVtx);
  read_neutronmult->Get("SimpleRecoStopVtx",SimpleRecoStopVtx);
  read_neutronmult->Get("SimpleRecoCosTheta",SimpleRecoCosTheta);
  read_neutronmult->Get("SimpleRecoPt",SimpleRecoPt);
  read_neutronmult->Get("SimpleRecoFV",SimpleRecoFV);
  read_neutronmult->Get("SimpleRecoMrdEnergyLoss",SimpleRecoMrdEnergyLoss);
  read_neutronmult->Get("SimpleRecoTrackLengthInMRD",SimpleRecoTrackLengthInMRD);
  read_neutronmult->Get("SimpleRecoMRDStart",SimpleRecoMRDStart);
  read_neutronmult->Get("SimpleRecoMRDStop",SimpleRecoMRDStop);
  read_neutronmult->Get("ClusterIndicesNeutron",cluster_neutron);
  read_neutronmult->Get("ClusterTimesNeutron",cluster_times_neutron);
  read_neutronmult->Get("ClusterChargesNeutron",cluster_charges_neutron);
  read_neutronmult->Get("ClusterCBNeutron",cluster_cb_neutron);
  read_neutronmult->Get("ClusterTimes",cluster_times);
  read_neutronmult->Get("ClusterCharges",cluster_charges);
  read_neutronmult->Get("ClusterCB",cluster_cb);
  read_neutronmult->Get("NeutronEffMap",neutron_eff_map);
  read_neutronmult->Get("PMTMRDCoinc",passPMTMRDCoincCut);
  read_neutronmult->Get("NumMRDTracks",numtracksinev);
  read_neutronmult->Get("DownstreamFV",DownstreamFV);
  read_neutronmult->Get("FullCylFV",FullCylFV);
  read_neutronmult->Get("SimpleRecoNeutrinoEnergy",SimpleRecoNeutrinoEnergy);
  read_neutronmult->Get("SimpleRecoQ2",SimpleRecoQ2);

  m_data->Stores["ANNIEEvent"]->Set("RunNumber",RunNumber);
  m_data->Stores["ANNIEEvent"]->Set("EventNumber",EventNumber);
  m_data->Stores["RecoEvent"]->Set("EventCutStatus",EventCutStatus);
  m_data->Stores["ANNIEEvent"]->Set("Particles",Particles);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoEnergy",SimpleRecoEnergy);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoVtx",SimpleRecoVtx);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoStopVtx",SimpleRecoStopVtx);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoCosTheta",SimpleRecoCosTheta);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoPt",SimpleRecoPt);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoFV",SimpleRecoFV);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoMrdEnergyLoss",SimpleRecoMrdEnergyLoss);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoTrackLengthInMRD",SimpleRecoTrackLengthInMRD);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoMRDStart",SimpleRecoMRDStart);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoMRDStop",SimpleRecoMRDStop);
  m_data->Stores["RecoEvent"]->Set("ClusterIndicesNeutron",cluster_neutron);
  m_data->Stores["RecoEvent"]->Set("ClusterTimesNeutron",cluster_times_neutron);
  m_data->Stores["RecoEvent"]->Set("ClusterChargesNeutron",cluster_charges_neutron);
  m_data->Stores["RecoEvent"]->Set("ClusterCBNeutron",cluster_cb_neutron);
  m_data->Stores["RecoEvent"]->Set("ClusterTimes",cluster_times);
  m_data->Stores["RecoEvent"]->Set("ClusterCharges",cluster_charges);
  m_data->Stores["RecoEvent"]->Set("ClusterCB",cluster_cb);
  m_data->Stores["RecoEvent"]->Set("NeutronEffMap",neutron_eff_map);
  m_data->Stores.at("RecoEvent")->Set("PMTMRDCoinc",passPMTMRDCoincCut);
  m_data->Stores["MRDTracks"]->Set("NumMrdTracks",numtracksinev);
  m_data->Stores["RecoEvent"]->Set("DownstreamFV",DownstreamFV);
  m_data->Stores["RecoEvent"]->Set("FullCylFV",FullCylFV);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoNeutrinoEnergy",SimpleRecoNeutrinoEnergy);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoQ2",SimpleRecoQ2);
  
  if (isMC){
    read_neutronmult->Get("NeutrinoEnergy",true_Enu);
    read_neutronmult->Get("EventQ2",true_Q2);
    read_neutronmult->Get("IsWeakCC",true_CC);
    read_neutronmult->Get("IsQuasiElastic",true_QEL);
    read_neutronmult->Get("IsResonant",true_RES);
    read_neutronmult->Get("IsDeepInelastic",true_DIS);
    read_neutronmult->Get("IsCoherent",true_COH);
    read_neutronmult->Get("IsMEC",true_MEC);
    read_neutronmult->Get("NumFSNeutrons",true_PrimNeut);
    read_neutronmult->Get("NumFSProtons",true_PrimProt);
    double temp_vtx_x, temp_vtx_y, temp_vtx_z, temp_vtx_t;
    read_neutronmult->Get("TrueVertex_X",temp_vtx_x);
    read_neutronmult->Get("TrueVertex_Y",temp_vtx_y);
    read_neutronmult->Get("TrueVertex_Z",temp_vtx_z);
    read_neutronmult->Get("TrueVertex_T",temp_vtx_t);
    read_neutronmult->Get("TrueVertex_DirX",true_DirX);
    read_neutronmult->Get("TrueVertex_DirY",true_DirY);
    read_neutronmult->Get("TrueVertex_DirZ",true_DirZ);
    read_neutronmult->Get("TrueVertex",true_vertex);
    read_neutronmult->Get("TrueMuonEnergy",true_Emu);
    read_neutronmult->Get("PrimaryPdgs",vec_true_PrimaryPdgs);
    read_neutronmult->Get("PdgPrimary",true_PdgPrimary);
    read_neutronmult->Get("TrueTrackLengthInWater",true_TrackLengthInWater);
    read_neutronmult->Get("TrueTrackLengthInMRD",true_TrackLengthInMRD);
    read_neutronmult->Get("MCMultiRingEvent",IsMultiRing);
    read_neutronmult->Get("MCNeutCap",MCNeutCap);
    read_neutronmult->Get("MCNeutCapGammas",MCNeutCapGammas);
    read_neutronmult->Get("MCFile",MCFile);

    Log("NeutronMultiplicity tool: Got TrueVertex from file with properties: "+std::to_string(temp_vtx_x)+", "+std::to_string(temp_vtx_y)+", "+std::to_string(temp_vtx_z)+", "+std::to_string(temp_vtx_t),v_message,verbosity);

    truevtx->SetVertex(temp_vtx_x,temp_vtx_y,temp_vtx_z,temp_vtx_t);
    truevtx->SetDirection(true_DirX,true_DirY,true_DirZ);

    m_data->Stores["GenieInfo"]->Set("NeutrinoEnergy",true_Enu/1000.);
    m_data->Stores["GenieInfo"]->Set("EventQ2",true_Q2/1000.);
    m_data->Stores["GenieInfo"]->Set("IsWeakCC",bool(true_CC));
    m_data->Stores["GenieInfo"]->Set("IsQuasiElastic",true_QEL);
    m_data->Stores["GenieInfo"]->Set("IsResonant",true_RES);
    m_data->Stores["GenieInfo"]->Set("IsDeepInelastic",true_DIS);
    m_data->Stores["GenieInfo"]->Set("IsCoherent",true_COH);
    m_data->Stores["GenieInfo"]->Set("IsMEC",true_MEC);
    m_data->Stores["GenieInfo"]->Set("NumFSNeutrons",true_PrimNeut);
    m_data->Stores["GenieInfo"]->Set("NumFSProtons",true_PrimProt);
    m_data->Stores.at("RecoEvent")->Set("TrueVertex",truevtx,true);
    m_data->Stores.at("RecoEvent")->Set("TrueMuonEnergy",true_Emu);
    m_data->Stores.at("RecoEvent")->Set("PrimaryPdgs",vec_true_PrimaryPdgs);
    m_data->Stores.at("RecoEvent")->Set("PdgPrimary",true_PdgPrimary);
    m_data->Stores.at("RecoEvent")->Set("TrueTrackLengthInWater",true_TrackLengthInWater);
    m_data->Stores.at("RecoEvent")->Set("TrueTrackLengthInMRD",true_TrackLengthInMRD*100.);
    m_data->Stores["RecoEvent"]->Set("MCMultiRingEvent",IsMultiRing);
    m_data->Stores.at("ANNIEEvent")->Set("MCNeutCap",MCNeutCap);
    m_data->Stores.at("ANNIEEvent")->Set("MCNeutCapGammas",MCNeutCapGammas);
    m_data->Stores.at("ANNIEEvent")->Set("MCFile",MCFile);
  }

  return true;
}

bool NeutronMultiplicity::GetClusterInformation(){

  bool return_value = true;

  bool get_cluster_idxN = m_data->Stores["RecoEvent"]->Get("ClusterIndicesNeutron",cluster_neutron);
  if (!get_cluster_idxN) Log("NeutronMultiplicity tool: No ClusterIndicesNeutron In RecoEvent!",v_error,verbosity); 
  bool get_cluster_tN = m_data->Stores["RecoEvent"]->Get("ClusterTimesNeutron",cluster_times_neutron);
  if (!get_cluster_tN) Log("NeutronMultiplicity tool: No ClusterTimesNeutron In RecoEvent!",v_error,verbosity); 
  bool get_cluster_qN = m_data->Stores["RecoEvent"]->Get("ClusterChargesNeutron",cluster_charges_neutron);
  if (!get_cluster_qN) Log("NeutronMultiplicity tool: No ClusterChargesNeutron In RecoEvent!",v_error,verbosity); 
  bool get_cluster_cbN = m_data->Stores["RecoEvent"]->Get("ClusterCBNeutron",cluster_cb_neutron);
  if (!get_cluster_cbN) Log("NeutronMultiplicity tool: No ClusterCBNeutron In RecoEvent!",v_error,verbosity); 
  bool get_cluster_t = m_data->Stores["RecoEvent"]->Get("ClusterTimes",cluster_times);
  if (!get_cluster_t) Log("NeutronMultiplicity tool: No ClusterTimes In RecoEvent!",v_error,verbosity); 
  bool get_cluster_q = m_data->Stores["RecoEvent"]->Get("ClusterCharges",cluster_charges);
  if (!get_cluster_q) Log("NeutronMultiplicity tool: No ClusterCharges In RecoEvent!",v_error,verbosity); 
  bool get_cluster_cb = m_data->Stores["RecoEvent"]->Get("ClusterCB",cluster_cb);
  if (!get_cluster_cb) Log("NeutronMultiplicity tool: No ClusterCB In RecoEvent!",v_error,verbosity); 

  return_value = (get_cluster_idxN && get_cluster_tN && get_cluster_qN && get_cluster_cbN && get_cluster_t && get_cluster_q && get_cluster_cb);

  (*reco_NCandCB) = cluster_cb_neutron;
  (*reco_NCandTime) = cluster_times_neutron;
  (*reco_NCandPE) = cluster_charges_neutron;
  (*reco_ClusterCB) = cluster_cb;
  (*reco_ClusterTime) = cluster_times;
  (*reco_ClusterPE) = cluster_charges;
  reco_Clusters = reco_ClusterCB->size();

  return return_value;

}

bool NeutronMultiplicity::GetParticleInformation(){

  bool return_value = true;

  bool get_simple_e = m_data->Stores["RecoEvent"]->Get("SimpleRecoEnergy",SimpleRecoEnergy);
  if (!get_simple_e) Log("NeutronMultiplicity tool: No SimpleRecoEnergy In RecoEvent!",v_error,verbosity); 
  bool get_simple_vtx = m_data->Stores["RecoEvent"]->Get("SimpleRecoVtx",SimpleRecoVtx);
  if (!get_simple_vtx) Log("NeutronMultiplicity tool: No SimpleRecoVtx In RecoEvent!",v_error,verbosity); 
  bool get_simple_stop = m_data->Stores["RecoEvent"]->Get("SimpleRecoStopVtx",SimpleRecoStopVtx);
  if (!get_simple_stop) Log("NeutronMultiplicity tool: No SimpleRecoStopVtx In RecoEvent!",v_error,verbosity); 
  bool get_simple_cos = m_data->Stores["RecoEvent"]->Get("SimpleRecoCosTheta",SimpleRecoCosTheta);
  if (!get_simple_cos) Log("NeutronMultiplicity tool: No SimpleRecoCosTheta In RecoEvent!",v_error,verbosity); 
  bool get_simple_pT = m_data->Stores["RecoEvent"]->Get("SimpleRecoPt",SimpleRecoPt);
  if (!get_simple_pT) Log("NeutronMultiplicity tool: No SimpleRecoPt In RecoEvent!",v_error,verbosity); 
  bool get_simple_fv = m_data->Stores["RecoEvent"]->Get("SimpleRecoFV",SimpleRecoFV);
  if (!get_simple_fv) Log("NeutronMultiplicity tool: No SimpleRecoFV in RecoEvent!",v_error,verbosity);
  bool get_simple_mrdloss = m_data->Stores["RecoEvent"]->Get("SimpleRecoMrdEnergyLoss",SimpleRecoMrdEnergyLoss);
  if (!get_simple_mrdloss) Log("NeutronMultiplicity tool: No SimpleRecoMrdEnergyLoss in RecoEvent!",v_error,verbosity);
  bool get_simple_mrdlength = m_data->Stores["RecoEvent"]->Get("SimpleRecoTrackLengthInMRD",SimpleRecoTrackLengthInMRD);
  if (!get_simple_mrdlength) Log("NeutronMultiplicity tool: No SimpleRecoTrackLength in RecoEvent!",v_error,verbosity);
  bool get_simple_mrdstart = m_data->Stores["RecoEvent"]->Get("SimpleRecoMRDStart",SimpleRecoMRDStart);
  if (!get_simple_mrdstart) Log("NeutronMultiplicity tool: No SimpleRecoMRDStart in RecoEvent!",v_error,verbosity);
  bool get_simple_mrdstop = m_data->Stores["RecoEvent"]->Get("SimpleRecoMRDStop",SimpleRecoMRDStop);
  if (!get_simple_mrdstop) Log("NeutronMultiplicity tool: No SimpleRecoMRDStop in RecoEvent!",v_error,verbosity);

  bool get_simple_fvregion = m_data->Stores["RecoEvent"]->Get("DownstreamFV",DownstreamFV);
  if (!get_simple_fvregion) Log("NeutronMultiplicity tool: No DownstreamFV in RecoEvent!",v_error,verbosity);
  get_simple_fvregion = m_data->Stores["RecoEvent"]->Get("FullCylFV",FullCylFV);
  if (!get_simple_fvregion) Log("NeutronMultiplicity tool: No FullCylFV in RecoEvent!",v_error,verbosity);
  bool get_simple_nuenergy = m_data->Stores["RecoEvent"]->Get("SimpleRecoNeutrinoEnergy",SimpleRecoNeutrinoEnergy);
  if (!get_simple_nuenergy) Log("NeutronMultiplicity tool: No SimpleRecoNeutrinoEnergy in RecoEvent!",v_error,verbosity);
  bool get_simple_q2 = m_data->Stores["RecoEvent"]->Get("SimpleRecoQ2",SimpleRecoQ2);
  if (!get_simple_q2) Log("NeutronMultiplicity tool: No SimpleRecoQ2 in RecoEvent!",v_error,verbosity);


  bool get_pmtmrdcoinc = m_data->Stores.at("RecoEvent")->Get("PMTMRDCoinc",passPMTMRDCoincCut);
  if (!get_pmtmrdcoinc) Log("NeutronMultiplicity tool: No PMTMRDCoinc in RecoEvent!",v_error,verbosity);
  reco_TankMRDCoinc = (passPMTMRDCoincCut)? 1 : 0;

  bool get_neutroneff = m_data->Stores.at("RecoEvent")->Get("NeutronEffMap",neutron_eff_map);
  if (!get_neutroneff) Log("NeutronMultiplicity tool: No NeutronEffMap in RecoEvent!",v_error,verbosity);
  else {
    if (!neutron_hist_filled){	//Just fill it once
      for (std::map<std::vector<double>,double>::iterator it = neutron_eff_map.begin(); it!= neutron_eff_map.end(); it++){
        std::vector<double> position = it->first;
        double eff = it->second;
        int bin_nr = hist_neutron_eff->FindBin(position.at(0),position.at(1),position.at(2));
        hist_neutron_eff->SetBinContent(bin_nr,eff);
      }
      neutron_hist_filled = true;
    }
  }

  return_value = (get_simple_e && get_simple_vtx && get_simple_stop && get_simple_cos && get_simple_pT && get_simple_fv && get_simple_mrdloss && get_pmtmrdcoinc && get_simple_mrdlength && get_simple_mrdstart && get_simple_mrdstop && get_neutroneff);

  run_nr = RunNumber;
  ev_nr = (int) EventNumber;
 
  return return_value;

}

bool NeutronMultiplicity::GetMCTruthInformation(){

  bool return_value = true;

  //Get information from GENIE store
/*  bool get_neutrino_energy = m_data->Stores["GenieInfo"]->Get("NeutrinoEnergy",true_Enu);
  if (!get_neutrino_energy) Log("NeutronMultiplicity tool: No NeutrinoEnergy In GenieInfo!",v_error,verbosity); 
  true_Enu *= 1000; //convert to MeV for consistency
  bool get_q2 = m_data->Stores["GenieInfo"]->Get("EventQ2",true_Q2);
  if (!get_q2) Log("NeutronMultiplicity tool: No EventQ2 In GenieInfo!",v_error,verbosity); 
  true_Q2 *= 1000; //convert to MeV for consistency
  bool bool_true_CC=false;
  bool get_cc = m_data->Stores["GenieInfo"]->Get("IsWeakCC",bool_true_CC);
  true_CC = (bool_true_CC)? 1 : 0;
  if (!get_cc) Log("NeutronMultiplicity tool: No IsWeakCC In GenieInfo!",v_error,verbosity); 
  bool bool_true_QEL=false;
  bool get_qel = m_data->Stores["GenieInfo"]->Get("IsQuasiElastic",bool_true_QEL);
  true_QEL = (bool_true_QEL)? 1 : 0;
  if (!get_qel) Log("NeutronMultiplicity tool: No IsQuasiElastic In GenieInfo!",v_error,verbosity); 
  bool bool_true_RES=false;
  bool get_res = m_data->Stores["GenieInfo"]->Get("IsResonant",bool_true_RES);
  true_RES = (bool_true_RES)? 1 : 0;
  if (!get_res) Log("NeutronMultiplicity tool: No IsReonant In GenieInfo!",v_error,verbosity); 
  bool bool_true_DIS=false;
  bool get_dis = m_data->Stores["GenieInfo"]->Get("IsDeepInelastic",bool_true_DIS);
  true_DIS = (bool_true_DIS)? 1 : 0;
  if (!get_dis) Log("NeutronMultiplicity tool: No IsDeepInelastic In GenieInfo!",v_error,verbosity); 
  bool bool_true_COH=false;
  bool get_coh = m_data->Stores["GenieInfo"]->Get("IsCoherent",bool_true_COH);
  true_COH = (bool_true_COH)? 1 : 0;
  if (!get_coh) Log("NeutronMultiplicity tool: No IsCoherent In GenieInfo!",v_error,verbosity); 
  bool bool_true_MEC=false;
  bool get_mec = m_data->Stores["GenieInfo"]->Get("IsMEC",bool_true_MEC);
  true_MEC = (bool_true_MEC)? 1 : 0;
  if (!get_mec) Log("NeutronMultiplicity tool: No IsMEC In GenieInfo!",v_error,verbosity); 
  bool get_n = m_data->Stores["GenieInfo"]->Get("NumFSNeutrons",true_PrimNeut);
  if (!get_n) Log("NeutronMultiplicity tool: No NumFSNeutrons In GenieInfo!",v_error,verbosity); 
  bool get_p = m_data->Stores["GenieInfo"]->Get("NumFSProtons",true_PrimProt);
  if (!get_p) Log("NeutronMultiplicity tool: No NumFSProtons In GenieInfo!",v_error,verbosity); 

  return_value = (get_neutrino_energy && get_q2 && get_cc && get_qel && get_res && get_dis && get_coh && get_mec && get_n && get_p);*/
  //Get information from RecoEvent store
  RecoVertex* TrueVtx = nullptr;
  auto get_muonMC = m_data->Stores.at("RecoEvent")->Get("TrueVertex",TrueVtx);
  if (!get_muonMC) Log("NeutronMultiplicity tool: No TrueVertex In RecoEvent!",v_error,verbosity); 
  else {
    truevtx->SetVertex(TrueVtx->GetPosition().X(),TrueVtx->GetPosition().Y(),TrueVtx->GetPosition().Z(),TrueVtx->GetTime());
    truevtx->SetDirection(TrueVtx->GetDirection().X(),TrueVtx->GetDirection().Y(),TrueVtx->GetDirection().Z());
  }
  auto get_muonMCEnergy = m_data->Stores.at("RecoEvent")->Get("TrueMuonEnergy",true_Emu);
  if (!get_muonMCEnergy) Log("NeutronMultiplicity tool: No TrueMuonEnergy In RecoEvent!",v_error,verbosity); 
  auto get_pdg = m_data->Stores.at("RecoEvent")->Get("PdgPrimary",true_PdgPrimary);
  if (!get_pdg) Log("NeutronMultiplicity tool: No PdgPrimary In RecoEvent!",v_error,verbosity);
  bool has_primaries = m_data->Stores.at("RecoEvent")->Get("PrimaryPdgs",vec_true_PrimaryPdgs);
  if (has_primaries){
    for (int i_part=0; i_part < (int) vec_true_PrimaryPdgs.size(); i_part++){
      true_PrimaryPdgs->push_back(vec_true_PrimaryPdgs.at(i_part));
    }
  }
  true_Emu -= 105.66;    //Correct true muon energy by rest mass to get true kinetic energy
  std::cout <<"get_muonMC && get_muonMCEnergy: "<<(get_muonMC && get_muonMCEnergy)<<std::endl;
  if(get_muonMC && get_muonMCEnergy){ 
    true_VtxX = truevtx->GetPosition().X()/100.; //convert to meters
    true_VtxY = truevtx->GetPosition().Y()/100.-0.144; //convert to meters, offset
    true_VtxZ = truevtx->GetPosition().Z()/100.+1.681; //convert to meters, offset
    true_VtxTime = truevtx->GetTime();
    true_DirX = truevtx->GetDirection().X();
    true_DirY = truevtx->GetDirection().Y();
    true_DirZ = truevtx->GetDirection().Z();
    true_CosTheta = true_DirZ;
    double true_Emu_total = true_Emu + 105.66;
    true_pT = sqrt((1-true_CosTheta*true_CosTheta)*(true_Emu_total*true_Emu_total-105.6*105.6));
    if (sqrt(true_VtxX*true_VtxX+(true_VtxZ-1.681)*(true_VtxZ-1.681))<1.0 && fabs(true_VtxY)<0.5 && (true_VtxZ < 1.681)) true_FV = 1;
    else true_FV = 0;
  }
  return_value = (return_value && get_muonMC && get_muonMCEnergy && get_pdg);

  auto get_tankTrackLength = m_data->Stores.at("RecoEvent")->Get("TrueTrackLengthInWater",true_TrackLengthInWater); 
  if (!get_tankTrackLength) Log("NeutronMultiplicity tool: No TrueTrackLengthInWater in RecoEvent!",v_error,verbosity);
  auto get_MRDTrackLength = m_data->Stores.at("RecoEvent")->Get("TrueTrackLengthInMRD",true_TrackLengthInMRD); 
  if (!get_MRDTrackLength) Log("NeutronMultiplicity tool: No TrueTrackLengthInMRD In RecoEvent!",v_error,verbosity);
   true_TrackLengthInMRD /= 100; //convert to meters
  return_value = (return_value && get_tankTrackLength && get_MRDTrackLength);

  bool get_multi = m_data->Stores["RecoEvent"]->Get("MCMultiRingEvent",IsMultiRing);
  if (get_multi) { true_MultiRing = (IsMultiRing)? 1 : 0;}
  else Log("NeutronMultiplicity tool: No MCMultiRingEvent information in RecoEvent!",v_error,verbosity);

  return_value = (return_value && get_multi);

  bool get_neutcap = m_data->Stores.at("ANNIEEvent")->Get("MCNeutCap",MCNeutCap); 
  if (!get_neutcap){
    Log("NeutronMultiplicity tool: Did not find MCNeutCap in ANNIEEvent Store!",v_warning,verbosity);
  } 

  bool get_neutcap_gammas = m_data->Stores.at("ANNIEEvent")->Get("MCNeutCapGammas",MCNeutCapGammas);
  if (!get_neutcap_gammas){
    Log("NeutronMultiplicity tool: Did not find MCNeutCapGammas in ANNIEEvent Store!",v_warning,verbosity);
  }

  return_value = (return_value && get_neutcap && get_neutcap_gammas);
  
  if (MCNeutCap.count("CaptVtxX")>0){
    std::vector<double> n_vtxx = MCNeutCap["CaptVtxX"];
    std::vector<double> n_vtxy = MCNeutCap["CaptVtxY"];
    std::vector<double> n_vtxz = MCNeutCap["CaptVtxZ"];
    std::vector<double> n_parent = MCNeutCap["CaptParent"];
    std::vector<double> n_ngamma = MCNeutCap["CaptNGamma"];
    std::vector<double> n_totale = MCNeutCap["CaptTotalE"];
    std::vector<double> n_time = MCNeutCap["CaptTime"];
    std::vector<double> n_nuc = MCNeutCap["CaptNucleus"];
    std::vector<double> n_primary = MCNeutCap["CaptPrimary"];    

    true_NCapturesPMTVol = 0;
 
    for (int i_cap=0; i_cap < (int) n_vtxx.size(); i_cap++){
      true_NeutVtxX->push_back(n_vtxx.at(i_cap)/100.);  //convert to meters for consistency
      true_NeutVtxY->push_back(n_vtxy.at(i_cap)/100.); //convert to meters for consistency
      true_NeutVtxZ->push_back(n_vtxz.at(i_cap)/100.); //convert to meters for consistency
      true_NeutCapNucl->push_back(n_nuc.at(i_cap));
      true_NeutCapTime->push_back(n_time.at(i_cap));
      true_NeutCapNGamma->push_back(n_ngamma.at(i_cap));
      true_NeutCapETotal->push_back(n_totale.at(i_cap));
      true_NeutCapPrimary->push_back(n_primary.at(i_cap));
      double n_VtxX = n_vtxx.at(i_cap)/100.;
      double n_VtxY = n_vtxy.at(i_cap)/100.;
      double n_VtxZ = n_vtxz.at(i_cap)/100.;
      if (sqrt(n_VtxX*n_VtxX+(n_VtxZ-1.681)*(n_VtxZ-1.681))<1.5 && fabs(n_VtxY)<2.0) {
	  true_NCapturesPMTVol++;
      }
    }
    true_NCaptures = int(n_vtxx.size());
  }

  return return_value;

}

bool NeutronMultiplicity::ResetVariables(){

  DownstreamFV = -9999;
  FullCylFV = -9999;
  SimpleRecoNeutrinoEnergy = -9999;
  SimpleRecoQ2 = -9999;

  SimpleRecoFlag = -9999;
  SimpleRecoVtx = Position(-9999,-9999,-9999);
  SimpleRecoStopVtx = Position(-9999,-9999,-9999);
  SimpleRecoEnergy = -9999;
  SimpleRecoCosTheta = -9999;
  SimpleRecoPt = -9999;
  SimpleRecoFV = false;
  NumberNeutrons = -9999;
  SimpleRecoMrdEnergyLoss = -9999;
  Particles.clear();
  passPMTMRDCoincCut = false; 
  cluster_neutron.clear();
  cluster_times_neutron.clear();
  cluster_charges_neutron.clear();
  cluster_cb_neutron.clear();
  cluster_times.clear();
  cluster_charges.clear();
  cluster_cb.clear();
  numtracksinev = -9999;
  MCNeutCapGammas.clear();
  MCNeutCap.clear();
  IsMultiRing = false;
  truevtx->SetVertex(-9999,-9999,-9999,-9999);
  true_vertex.SetVertex(-9999,-9999,-9999,-9999);
  true_vertex.SetDirection(-9999,-9999,-9999);

  //ROOT tree variables
  true_PrimNeut = -9999;
  true_PrimProt = -9999;
  true_NCaptures = 0;
  true_NCapturesPMTVol = 0;
  true_VtxX = -9999;
  true_VtxY = -9999;
  true_VtxZ = -9999;
  true_VtxTime = -9999;
  true_DirX = -9999;
  true_DirY = -9999;
  true_DirZ = -9999;
  true_Emu = -9999;
  true_Enu = -9999;
  true_Q2 = -9999;
  true_pT = -9999;
  true_FV = -9999;
  true_CosTheta = -9999;
  true_TrackLengthInWater = -9999;
  true_TrackLengthInMRD = -9999;
  true_NeutVtxX->clear();
  true_NeutVtxY->clear();
  true_NeutVtxZ->clear();
  true_NeutCapNucl->clear();
  true_NeutCapTime->clear();
  true_NeutCapETotal->clear();
  true_NeutCapNGamma->clear();
  true_NeutCapPrimary->clear();
  true_CC = -9999;
  true_QEL = -9999;
  true_DIS = -9999;
  true_RES = -9999;
  true_COH = -9999;
  true_MEC = -9999;
  true_MultiRing = -9999;
  true_PrimaryPdgs->clear();
  true_PdgPrimary = -9999;
  vec_true_PrimaryPdgs.clear();

  reco_Emu = -9999;
  reco_Enu = -9999;
  reco_Q2 = -9999;
  reco_pT = -9999;
  reco_ClusterCB->clear();
  reco_ClusterTime->clear();
  reco_ClusterPE->clear();
  reco_NCandCB->clear();
  reco_NCandTime->clear();
  reco_NCandPE->clear();
  reco_MrdEnergyLoss = -9999;
  reco_TrackLengthInMRD = -9999;
  reco_MrdStartX = -9999;
  reco_MrdStartY = -9999;
  reco_MrdStartZ = -9999;
  reco_MrdStopX = -9999;
  reco_MrdStopY = -9999;
  reco_MrdStopZ = -9999;
  reco_TankMRDCoinc = -9999;
  reco_Clusters = -9999;
  reco_NCandidates = -9999;
  reco_VtxX = -9999;
  reco_VtxY = -9999;
  reco_VtxZ = -9999;
  reco_FV = -9999;
  reco_CosTheta = -9999;

  run_nr = -9999;
  ev_nr = -9999;

  return true;
}

bool NeutronMultiplicity::FillTGraphs(){

  std::vector<std::string> labels_muon_E = {"E_{#mu} [MeV]","Neutron multiplicity","ANNIE Neutron multiplicity"};
  std::vector<std::string> labels_muon_E_fv= {"E_{#mu} [MeV]","Neutron multiplicity","ANNIE Neutron multiplicity (FV)"};
  std::vector<std::string> labels_muon_CosTheta = {"cos(#theta)","Neutron multiplicity","ANNIE Neutron multiplicity"};
  std::vector<std::string> labels_muon_CosTheta_fv = {"cos(#theta)","Neutron multiplicity","ANNIE Neutron multiplicity (FV) "};
  std::vector<std::string> labels_muon_pT = {"p_{T} [MeV]","Neutron multiplicity","ANNIE Neutron multiplicity"};
  std::vector<std::string> labels_muon_pT_fv = {"p_{T} [MeV]","Neutron multiplicity","ANNIE Neutron multiplicity (FV) "};

  this->FillSingleTGraph(gr_neutrons_muonE,h_neutrons_energy,labels_muon_E);
  this->FillSingleTGraph(gr_neutrons_muonE_zoom,h_neutrons_energy_zoom,labels_muon_E);
  this->FillSingleTGraph(gr_neutrons_muonE_fv,h_neutrons_energy_fv,labels_muon_E_fv);
  this->FillSingleTGraph(gr_neutrons_muonE_fv_zoom,h_neutrons_energy_fv_zoom,labels_muon_E_fv);
  this->FillSingleTGraph(gr_neutrons_muonCosTheta,h_neutrons_costheta,labels_muon_CosTheta);
  this->FillSingleTGraph(gr_neutrons_muonCosTheta_fv,h_neutrons_costheta_fv,labels_muon_CosTheta_fv);
  this->FillSingleTGraph(gr_neutrons_pT,h_neutrons_pT,labels_muon_pT);
  this->FillSingleTGraph(gr_neutrons_pT_fv,h_neutrons_pT_fv,labels_muon_pT_fv);
  
  std::vector<std::string> labels_eff_E = {"E_{#mu} [MeV]","Efficiency #varepsilon","ANNIE Neutron efficiency"};
  std::vector<std::string> labels_eff_E_fv= {"E_{#mu} [MeV]","Efficiency #varepsilon","ANNIE Neutron efficiency (FV)"};
  std::vector<std::string> labels_eff_CosTheta = {"cos(#theta)","Efficiency #varepsilon","ANNIE Neutron efficiency"};
  std::vector<std::string> labels_eff_CosTheta_fv = {"cos(#theta)","Efficiency #varepsilon","ANNIE Neutron efficiency (FV) "};
  std::vector<std::string> labels_eff_pT = {"p_{T} [MeV]","Efficiency #varepsilon","ANNIE Neutron efficiency"};
  std::vector<std::string> labels_eff_pT_fv = {"p_{T} [MeV]","Efficiency #varepsilon","ANNIE Neutron efficiency (FV) "};

  this->FillSingleTGraph(gr_eff_muonE,h_eff_energy,labels_eff_E);
  this->FillSingleTGraph(gr_eff_muonE_zoom,h_eff_energy_zoom,labels_eff_E);
  this->FillSingleTGraph(gr_eff_muonE_fv,h_eff_energy_fv,labels_eff_E_fv);
  this->FillSingleTGraph(gr_eff_muonE_fv_zoom,h_eff_energy_fv_zoom,labels_eff_E_fv);
  this->FillSingleTGraph(gr_eff_costheta,h_eff_costheta,labels_eff_CosTheta);
  this->FillSingleTGraph(gr_eff_costheta_fv,h_eff_costheta_fv,labels_eff_CosTheta_fv);
  this->FillSingleTGraph(gr_eff_pT,h_eff_pT,labels_eff_pT);
  this->FillSingleTGraph(gr_eff_pT_fv,h_eff_pT_fv,labels_eff_pT_fv);

  std::vector<std::string> labels_muon_E_prim = {"E_{#mu} [MeV]","Primary Neutron multiplicity","ANNIE Neutron multiplicity"};
  std::vector<std::string> labels_muon_E_fv_prim= {"E_{#mu} [MeV]","Primary Neutron multiplicity","ANNIE Neutron multiplicity (FV)"};
  std::vector<std::string> labels_muon_CosTheta_prim = {"cos(#theta)","Primary Neutron multiplicity","ANNIE Neutron multiplicity"};
  std::vector<std::string> labels_muon_CosTheta_fv_prim = {"cos(#theta)","Primary Neutron multiplicity","ANNIE Neutron multiplicity (FV) "};
  std::vector<std::string> labels_muon_pT_prim = {"p_{T} [MeV]","Primary Neutron multiplicity","ANNIE Neutron multiplicity"};
  std::vector<std::string> labels_muon_pT_fv_prim = {"p_{T} [MeV]","Primary Neutron multiplicity","ANNIE Neutron multiplicity (FV) "};
  this->FillSingleTGraph(gr_primneutrons_muonE,h_primneutrons_energy,labels_muon_E_prim);
  this->FillSingleTGraph(gr_primneutrons_muonE_zoom,h_primneutrons_energy_zoom,labels_muon_E_prim);
  this->FillSingleTGraph(gr_primneutrons_muonE_fv,h_primneutrons_energy_fv,labels_muon_E_fv_prim);
  this->FillSingleTGraph(gr_primneutrons_muonE_fv_zoom,h_primneutrons_energy_fv_zoom,labels_muon_E_fv_prim);
  this->FillSingleTGraph(gr_primneutrons_muonCosTheta,h_primneutrons_costheta,labels_muon_CosTheta_prim);
  this->FillSingleTGraph(gr_primneutrons_muonCosTheta_fv,h_primneutrons_costheta_fv,labels_muon_CosTheta_fv_prim);
  this->FillSingleTGraph(gr_primneutrons_pT,h_primneutrons_pT,labels_muon_pT_prim);
  this->FillSingleTGraph(gr_primneutrons_pT_fv,h_primneutrons_pT_fv,labels_muon_pT_fv_prim);
  
  std::vector<std::string> labels_muon_E_total = {"E_{#mu} [MeV]","Total Neutron multiplicity","ANNIE Neutron multiplicity"};
  std::vector<std::string> labels_muon_E_fv_total= {"E_{#mu} [MeV]","Total Neutron multiplicity","ANNIE Neutron multiplicity (FV)"};
  std::vector<std::string> labels_muon_CosTheta_total = {"cos(#theta)","Total Neutron multiplicity","ANNIE Neutron multiplicity"};
  std::vector<std::string> labels_muon_CosTheta_fv_total = {"cos(#theta)","Total Neutron multiplicity","ANNIE Neutron multiplicity (FV) "};
  std::vector<std::string> labels_muon_pT_total = {"p_{T} [MeV]","Total Neutron multiplicity","ANNIE Neutron multiplicity"};
  std::vector<std::string> labels_muon_pT_fv_total = {"p_{T} [MeV]","Total Neutron multiplicity","ANNIE Neutron multiplicity (FV) "};
  this->FillSingleTGraph(gr_totalneutrons_muonE,h_totalneutrons_energy,labels_muon_E_total);
  this->FillSingleTGraph(gr_totalneutrons_muonE_zoom,h_totalneutrons_energy_zoom,labels_muon_E_total);
  this->FillSingleTGraph(gr_totalneutrons_muonE_fv,h_totalneutrons_energy_fv,labels_muon_E_fv_total);
  this->FillSingleTGraph(gr_totalneutrons_muonE_fv_zoom,h_totalneutrons_energy_fv_zoom,labels_muon_E_fv_total);
  this->FillSingleTGraph(gr_totalneutrons_muonCosTheta,h_totalneutrons_costheta,labels_muon_CosTheta_total);
  this->FillSingleTGraph(gr_totalneutrons_muonCosTheta_fv,h_totalneutrons_costheta_fv,labels_muon_CosTheta_fv_total);
  this->FillSingleTGraph(gr_totalneutrons_pT,h_totalneutrons_pT,labels_muon_pT_total);
  this->FillSingleTGraph(gr_totalneutrons_pT_fv,h_totalneutrons_pT_fv,labels_muon_pT_fv_total);

  std::vector<std::string> labels_muon_E_pmtvol = {"E_{#mu} [MeV]","PMTVolume Neutron multiplicity","ANNIE Neutron multiplicity"};
  std::vector<std::string> labels_muon_E_fv_pmtvol= {"E_{#mu} [MeV]","PMTVolume Neutron multiplicity","ANNIE Neutron multiplicity (FV)"};
  std::vector<std::string> labels_muon_CosTheta_pmtvol = {"cos(#theta)","PMTVolume Neutron multiplicity","ANNIE Neutron multiplicity"};
  std::vector<std::string> labels_muon_CosTheta_fv_pmtvol = {"cos(#theta)","PMTVolume Neutron multiplicity","ANNIE Neutron multiplicity (FV) "};
  std::vector<std::string> labels_muon_pT_pmtvol = {"p_{T} [MeV]","PMTVolume Neutron multiplicity","ANNIE Neutron multiplicity"};
  std::vector<std::string> labels_muon_pT_fv_pmtvol = {"p_{T} [MeV]","PMTVolume Neutron multiplicity","ANNIE Neutron multiplicity (FV) "};
  this->FillSingleTGraph(gr_pmtvolneutrons_muonE,h_pmtvolneutrons_energy,labels_muon_E_pmtvol);
  this->FillSingleTGraph(gr_pmtvolneutrons_muonE_zoom,h_pmtvolneutrons_energy_zoom,labels_muon_E_pmtvol);
  this->FillSingleTGraph(gr_pmtvolneutrons_muonE_fv,h_pmtvolneutrons_energy_fv,labels_muon_E_fv_pmtvol);
  this->FillSingleTGraph(gr_pmtvolneutrons_muonE_fv_zoom,h_pmtvolneutrons_energy_fv_zoom,labels_muon_E_fv_pmtvol);
  this->FillSingleTGraph(gr_pmtvolneutrons_muonCosTheta,h_pmtvolneutrons_costheta,labels_muon_CosTheta_pmtvol);
  this->FillSingleTGraph(gr_pmtvolneutrons_muonCosTheta_fv,h_pmtvolneutrons_costheta_fv,labels_muon_CosTheta_fv_pmtvol);
  this->FillSingleTGraph(gr_pmtvolneutrons_pT,h_pmtvolneutrons_pT,labels_muon_pT_pmtvol);
  this->FillSingleTGraph(gr_pmtvolneutrons_pT_fv,h_pmtvolneutrons_pT_fv,labels_muon_pT_fv_pmtvol);


  //Correct graphs with efficiencies:
  this->CorrectEfficiency(gr_neutrons_muonE,gr_eff_muonE,gr_neutrons_muonE_corr,labels_muon_E);
  this->CorrectEfficiency(gr_neutrons_muonE_fv,gr_eff_muonE_fv,gr_neutrons_muonE_corr_fv,labels_muon_E_fv);
  this->CorrectEfficiency(gr_neutrons_muonE_zoom,gr_eff_muonE_zoom,gr_neutrons_muonE_corr_zoom,labels_muon_E);
  this->CorrectEfficiency(gr_neutrons_muonE_fv_zoom,gr_eff_muonE_fv_zoom,gr_neutrons_muonE_corr_fv_zoom,labels_muon_E_fv);
  this->CorrectEfficiency(gr_neutrons_muonCosTheta,gr_eff_costheta,gr_neutrons_muonCosTheta_corr,labels_muon_CosTheta);
  this->CorrectEfficiency(gr_neutrons_muonCosTheta_fv,gr_eff_costheta_fv,gr_neutrons_muonCosTheta_corr_fv,labels_muon_CosTheta_fv);
  this->CorrectEfficiency(gr_neutrons_pT,gr_eff_pT,gr_neutrons_pT_corr,labels_muon_pT);
  this->CorrectEfficiency(gr_neutrons_pT_fv,gr_eff_pT_fv,gr_neutrons_pT_corr_fv,labels_muon_pT_fv);

  return true;
}

bool NeutronMultiplicity::FillSingleTGraph(TGraphErrors *gr, TH2F *h2d, std::vector<std::string> labels){

  if (labels.size() != 3) {
    Log("NeutronMultiplicity::FillSingleTGraph: Encountered wrong size for labels vector! Check that the object is defined correctly!",v_error,verbosity);
    return false;
  }

  int i_graph=0;
  int n_bins_x = h2d->GetNbinsX();
  int n_bins_y = h2d->GetNbinsY();

  TAxis *xaxis = h2d->GetXaxis();
  TAxis *yaxis = h2d->GetYaxis();
  //Get lower end of xaxis -> use first bin
  double lower_range = xaxis->GetBinLowEdge(1);
  //Get bin width -> any bin possible, but first is always possible
  double bin_width = xaxis->GetBinWidth(1);
  double bin_width_y = yaxis->GetBinWidth(1);

  for (int i=0; i<n_bins_x; i++){
    double mean_n = 0;
    int n_n = 0;
    for (int j=0; j<n_bins_y; j++){
      int nentries = h2d->GetBinContent(i+1,j+1);
      mean_n+= (j*bin_width_y*nentries);
      n_n += nentries;
    }
    if (n_n>0){
      mean_n /= n_n;
      gr->SetPoint(i_graph,lower_range+(i+0.5)*bin_width,mean_n);
      double std_dev = 0;
      for (int j=0; j<n_bins_y; j++){
        int nentries = h2d->GetBinContent(i+1,j+1);
        std_dev+=(nentries*(j*bin_width_y-mean_n)*(j*bin_width_y-mean_n));
      }
      if (n_n>1) std_dev/=(n_n-1);
      std_dev = sqrt(std_dev);
      if (n_n>0) std_dev/=sqrt(n_n);
      gr->SetPointError(i_graph,bin_width/2.,std_dev);
      i_graph++;
    }
  }

  std::string title_string = labels.at(2)+";"+labels.at(0)+";"+labels.at(1);
  gr->SetTitle(title_string.c_str());
  gr->SetDrawOption("ALP");
  gr->SetLineWidth(2);

  return true;
}

bool NeutronMultiplicity::CorrectEfficiency(TGraphErrors *gr_original, TGraphErrors *gr_eff, TGraphErrors *gr_new,std::vector<std::string> labels){

  //Loop over points in original graph, grab x-values
  int n_points = gr_eff->GetN();
  for (int i_point=0; i_point < n_points; i_point++){
    double point_x;
    double original_value;
    double eff_value;
    double original_errorx, original_errory;
    double errory;

    gr_eff->GetPoint(i_point,point_x,eff_value);
    gr_original->GetPoint(i_point,point_x,original_value);
    original_errorx = gr_original->GetErrorX(i_point);
    original_errory = gr_original->GetErrorY(i_point);
    double new_value = (eff_value > 0)? original_value/eff_value : original_value;
    errory = (eff_value > 0)? original_errory/eff_value : original_errory;
    gr_new->SetPoint(i_point,point_x,new_value);
    gr_new->SetPointError(i_point,original_errorx,errory);
  }

  std::string title_string = labels.at(2)+";"+labels.at(0)+";"+labels.at(1);
  gr_new->SetTitle(title_string.c_str());
  gr_new->SetDrawOption("ALP");
  gr_new->SetLineWidth(2);

  return true;

}
