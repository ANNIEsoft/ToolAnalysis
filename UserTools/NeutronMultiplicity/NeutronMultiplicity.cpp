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
  }

  need_new_file_ = true;
  current_entry_ = 0u;
  current_file_ = 0u;

  return true;
}


bool NeutronMultiplicity::Execute(){

  //Reset tree variables
  this->ResetVariables();

  //Set ANNIEEvent if reading from BoostStore file
  if (read_bs!= "None"){
    if (need_new_file_){
      need_new_file_ = false;
      if (read_neutronmult) delete read_neutronmult;
      read_neutronmult = new BoostStore(false,2);
      read_neutronmult->Initialise(input_filenames_.at(current_file_));
      read_neutronmult->Print(false);
      read_neutronmult->Header->Get("TotalEntries",total_entries_in_file_);
      std::cout <<"total_entries_in_file_: "<<total_entries_in_file_<<std::endl;
    } 
    if (current_entry_ != 0) read_neutronmult->Delete();
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
      Log("NeutronMultiplicity tool: Retrieved particle # "+std::to_string(i_particle)+"...",v_message,verbosity);
      if (verbosity > 2) Particles.at(i_particle).Print();
      if (Particles.at(i_particle).GetPdgCode() == 2112) NumberNeutrons++;
    }
  }

  //Get selection cut variables
  bool pass_selection = false;
  m_data->Stores["ANNIEEvent"]->Get("EventCutStatus",pass_selection);

  //Fill h_neutrons independent of reconstruction status if selection cuts are passed
  if (pass_selection) h_neutrons->Fill(NumberNeutrons);

  bool pass_reconstruction = false;
  m_data->Stores["ANNIEEvent"]->Get("SimpleRecoFlag",SimpleRecoFlag);
  if (SimpleRecoFlag != -9999) pass_reconstruction = true;

  //If MRDTrackRestriction is enabled, check if there was only one MRD track
  if (mrdtrack_restriction){
    m_data->Stores["MRDTracks"]->Get("NumMrdTracks",numtracksinev);
    if (numtracksinev != 1) pass_reconstruction = false;
  }

  //std::cout <<"EventNumber: "<<EventNumber<<", pass_selection: "<<pass_selection<<", pass_reconstruction: "<<pass_reconstruction<<std::endl;

  //Fill histograms in case of passed selection cuts
  if (pass_selection && pass_reconstruction){

    //Get reconstructed information
    this->GetParticleInformation();

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
    gr_neutrons_muonCosTheta_zoom->Write("gr_neutrons_muonCosTheta_zoom");
    gr_neutrons_muonCosTheta_fv_zoom->Write("gr_neutrons_muonCosTheta_fv_zoom");
    delete file_neutronmult;
  }

  store_neutronmult->Close();
  store_neutronmult->Delete();
  delete store_neutronmult;
  
  return true;
}

bool NeutronMultiplicity::InitialiseHistograms(){
 
  Log("NeutronMultiplicity tool: InitialiseHistograms",v_message,verbosity);
 
  //output TFile
  std::stringstream ss_filename;
  ss_filename << filename << ".root";
  file_neutronmult = new TFile(ss_filename.str().c_str(),"RECREATE");

  dir_overall = (TDirectory*) gDirectory->CurrentDirectory();

  Log("NeutronMultiplicity tool: Define histograms",v_message,verbosity);

  dir_muon = (TDirectory*) file_neutronmult->mkdir("hist_muon");
  dir_neutron = (TDirectory*) file_neutronmult->mkdir("hist_neutron");
  dir_mc = (TDirectory*) file_neutronmult->mkdir("hist_mc");
  dir_graph = (TDirectory*) file_neutronmult->mkdir("graph_neutron");

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
  h_neutrons_costheta = new TH2F("h_neutrons_costheta","Neutron multiplicity vs muon angle",6,0.7,1.0,20,0,20);
  h_neutrons_costheta_fv = new TH2F("h_neutrons_costheta_fv","Neutron multiplicity vs muon angle (FV)",6,0.7,1.0,20,0,20);

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

  h_neutrons_costheta->SetStats(0);
  h_neutrons_costheta->GetXaxis()->SetTitle("cos(#theta)");
  h_neutrons_costheta->GetYaxis()->SetTitle("Number of neutrons");

  h_neutrons_costheta_fv->SetStats(0);
  h_neutrons_costheta_fv->GetXaxis()->SetTitle("cos(#theta)");
  h_neutrons_costheta_fv->GetYaxis()->SetTitle("Number of neutrons");

  h_muon_energy->GetXaxis()->SetTitle("E_{#mu} [MeV]");
  h_muon_energy->GetYaxis()->SetTitle("#");
  h_muon_energy->SetStats(0);
  h_muon_energy->SetLineWidth(2);
  h_muon_energy_fv->SetStats(0);
  h_muon_energy_fv->SetLineColor(kRed);
  h_muon_energy_fv->SetLineWidth(2);

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
  gr_neutrons_muonCosTheta_zoom = new TGraphErrors();
  gr_neutrons_muonCosTheta_fv_zoom = new TGraphErrors();

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
  neutron_tree->Branch("TrueEmu",&true_Emu);
  neutron_tree->Branch("TrueEnu",&true_Enu);
  neutron_tree->Branch("TrueQ2",&true_Q2);
  neutron_tree->Branch("TrueVtxX",&true_VtxX);
  neutron_tree->Branch("TrueVtxY",&true_VtxY);
  neutron_tree->Branch("TrueVtxZ",&true_VtxZ);
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


  return true;

}

bool NeutronMultiplicity::FillHistograms(){
 
  h_neutrons_mrdstop->Fill(NumberNeutrons);
  h_neutrons_energy->Fill(SimpleRecoEnergy,NumberNeutrons);
  h_neutrons_energy_zoom->Fill(SimpleRecoEnergy,NumberNeutrons);
  h_neutrons_costheta->Fill(SimpleRecoCosTheta,NumberNeutrons);
  h_muon_energy->Fill(SimpleRecoEnergy);
  h_muon_costheta->Fill(SimpleRecoCosTheta);

  if (SimpleRecoFV) {
    h_muon_energy_fv->Fill(SimpleRecoEnergy);
    h_muon_costheta_fv->Fill(SimpleRecoCosTheta);
    h_neutrons_energy_fv->Fill(SimpleRecoEnergy,NumberNeutrons);
    h_neutrons_energy_fv_zoom->Fill(SimpleRecoEnergy,NumberNeutrons);
    h_neutrons_costheta_fv->Fill(SimpleRecoCosTheta,NumberNeutrons);
    h_neutrons_mrdstop_fv->Fill(NumberNeutrons);
  }

  h_muon_vtx_x->Fill(SimpleRecoVtx.X());
  h_muon_vtx_y->Fill(SimpleRecoVtx.Y());
  h_muon_vtx_z->Fill(SimpleRecoVtx.Z());
  h_muon_vtx_yz->Fill(SimpleRecoVtx.Z(),SimpleRecoVtx.Y());
  h_muon_vtx_xz->Fill(SimpleRecoVtx.Z(),SimpleRecoVtx.X());

  reco_VtxX = SimpleRecoVtx.X();
  reco_VtxY = SimpleRecoVtx.Y();
  reco_VtxZ = SimpleRecoVtx.Z();
  reco_Emu = SimpleRecoEnergy;
  reco_CosTheta = SimpleRecoCosTheta;
  reco_NCandidates = NumberNeutrons;
  reco_FV = SimpleRecoFV;
  reco_MrdEnergyLoss = SimpleRecoMrdEnergyLoss;

  for (int i_n=0; i_n < (int) reco_NCandTime->size(); i_n++){
    h_time_neutrons_mrdstop->Fill(reco_NCandTime->at(i_n));
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
  store_neutronmult->Set("SimpleRecoFV",SimpleRecoFV);
  store_neutronmult->Set("SimpleRecoMrdEnergyLoss",SimpleRecoMrdEnergyLoss);
  store_neutronmult->Set("ClusterIndicesNeutron",cluster_neutron);
  store_neutronmult->Set("ClusterTimesNeutron",cluster_times_neutron);
  store_neutronmult->Set("ClusterChargesNeutron",cluster_charges_neutron);
  store_neutronmult->Set("ClusterCBNeutron",cluster_cb_neutron);
  store_neutronmult->Set("ClusterTimes",cluster_times);
  store_neutronmult->Set("ClusterCharges",cluster_charges);
  store_neutronmult->Set("ClusterCB",cluster_cb);
  store_neutronmult->Set("PMTMRDCoinc",passPMTMRDCoincCut);
  store_neutronmult->Set("NumMRDTracks",numtracksinev);

  //Construct BoostStore filename
  std::stringstream ss_filename;
  ss_filename << filename << ".bs";
  Log("NeutronMultiplicity tool: Saving BoostStore file "+ss_filename.str(),v_message,verbosity);
    
  //Save BoostStore
  store_neutronmult->Save(ss_filename.str().c_str()); 
  store_neutronmult->Delete();
   
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
  read_neutronmult->Get("SimpleRecoFV",SimpleRecoFV);
  read_neutronmult->Get("SimpleRecoMrdEnergyLoss",SimpleRecoMrdEnergyLoss);
  read_neutronmult->Get("ClusterIndicesNeutron",cluster_neutron);
  read_neutronmult->Get("ClusterTimesNeutron",cluster_times_neutron);
  read_neutronmult->Get("ClusterChargesNeutron",cluster_charges_neutron);
  read_neutronmult->Get("ClusterCBNeutron",cluster_cb_neutron);
  read_neutronmult->Get("ClusterTimes",cluster_times);
  read_neutronmult->Get("ClusterCharges",cluster_charges);
  read_neutronmult->Get("ClusterCB",cluster_cb);
  read_neutronmult->Get("PMTMRDCoinc",passPMTMRDCoincCut);
  read_neutronmult->Get("NumMRDTracks",numtracksinev);

  m_data->Stores["ANNIEEvent"]->Set("RunNumber",RunNumber);
  m_data->Stores["ANNIEEvent"]->Set("EventNumber",EventNumber);
  m_data->Stores["ANNIEEvent"]->Set("EventCutStatus",EventCutStatus);
  m_data->Stores["ANNIEEvent"]->Set("Particles",Particles);
  m_data->Stores["ANNIEEvent"]->Set("SimpleRecoEnergy",SimpleRecoEnergy);
  m_data->Stores["ANNIEEvent"]->Set("SimpleRecoVtx",SimpleRecoVtx);
  m_data->Stores["ANNIEEvent"]->Set("SimpleRecoStopVtx",SimpleRecoStopVtx);
  m_data->Stores["ANNIEEvent"]->Set("SimpleRecoCosTheta",SimpleRecoCosTheta);
  m_data->Stores["ANNIEEvent"]->Set("SimpleRecoFV",SimpleRecoFV);
  m_data->Stores["ANNIEEvent"]->Set("SimpleRecoMrdEnergyLoss",SimpleRecoMrdEnergyLoss);
  m_data->Stores["ANNIEEvent"]->Set("ClusterIndicesNeutron",cluster_neutron);
  m_data->Stores["ANNIEEvent"]->Set("ClusterTimesNeutron",cluster_times_neutron);
  m_data->Stores["ANNIEEvent"]->Set("ClusterChargesNeutron",cluster_charges_neutron);
  m_data->Stores["ANNIEEvent"]->Set("ClusterCBNeutron",cluster_cb_neutron);
  m_data->Stores["ANNIEEvent"]->Set("ClusterTimes",cluster_times);
  m_data->Stores["ANNIEEvent"]->Set("ClusterCharges",cluster_charges);
  m_data->Stores["ANNIEEvent"]->Set("ClusterCB",cluster_cb);
  m_data->Stores.at("RecoEvent")->Set("PMTMRDCoinc",passPMTMRDCoincCut);
  m_data->Stores["MRDTracks"]->Set("NumMrdTracks",numtracksinev);

  return true;
}

bool NeutronMultiplicity::GetParticleInformation(){

  bool return_value = true;

  return_value = m_data->Stores["ANNIEEvent"]->Get("SimpleRecoEnergy",SimpleRecoEnergy);
  return_value = m_data->Stores["ANNIEEvent"]->Get("SimpleRecoVtx",SimpleRecoVtx);
  return_value = m_data->Stores["ANNIEEvent"]->Get("SimpleRecoStopVtx",SimpleRecoStopVtx);
  return_value = m_data->Stores["ANNIEEvent"]->Get("SimpleRecoCosTheta",SimpleRecoCosTheta);
  return_value = m_data->Stores["ANNIEEvent"]->Get("SimpleRecoFV",SimpleRecoFV);
  return_value = m_data->Stores["ANNIEEvent"]->Get("SimpleRecoMrdEnergyLoss",SimpleRecoMrdEnergyLoss);

  return_value = m_data->Stores["ANNIEEvent"]->Get("ClusterIndicesNeutron",cluster_neutron);
  return_value = m_data->Stores["ANNIEEvent"]->Get("ClusterTimesNeutron",cluster_times_neutron);
  return_value = m_data->Stores["ANNIEEvent"]->Get("ClusterChargesNeutron",cluster_charges_neutron);
  return_value = m_data->Stores["ANNIEEvent"]->Get("ClusterCBNeutron",cluster_cb_neutron);
  return_value = m_data->Stores["ANNIEEvent"]->Get("ClusterTimes",cluster_times);
  return_value = m_data->Stores["ANNIEEvent"]->Get("ClusterCharges",cluster_charges);
  return_value = m_data->Stores["ANNIEEvent"]->Get("ClusterCB",cluster_cb);

  (*reco_NCandCB) = cluster_cb_neutron;
  (*reco_NCandTime) = cluster_times_neutron;
  (*reco_NCandPE) = cluster_charges_neutron;
  (*reco_ClusterCB) = cluster_cb;
  (*reco_ClusterTime) = cluster_times;
  (*reco_ClusterPE) = cluster_charges;
  reco_Clusters = reco_ClusterCB->size();

  return_value = m_data->Stores.at("RecoEvent")->Get("PMTMRDCoinc",passPMTMRDCoincCut);
  reco_TankMRDCoinc = (passPMTMRDCoincCut)? 1 : 0;

  run_nr = RunNumber;
  ev_nr = (int) EventNumber;
 
  return return_value;

}

bool NeutronMultiplicity::ResetVariables(){

  SimpleRecoFlag = -9999;
  SimpleRecoVtx = Position(-9999,-9999,-9999);
  SimpleRecoStopVtx = Position(-9999,-9999,-9999);
  SimpleRecoEnergy = -9999;
  SimpleRecoCosTheta = -9999;
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

  //ROOT tree variables
  true_PrimNeut = -9999;
  true_PrimProt = -9999;
  true_NCaptures = -9999;
  true_NCapturesPMTVol = -9999;
  true_VtxX = -9999;
  true_VtxY = -9999;
  true_VtxZ = -9999;
  true_Emu = -9999;
  true_Enu = -9999;
  true_Q2 = -9999;
  true_FV = -9999;
  true_CosTheta = -9999;
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

  reco_Emu = -9999;
  reco_Enu = -9999;
  reco_Q2 = -9999;
  reco_ClusterCB->clear();
  reco_ClusterTime->clear();
  reco_ClusterPE->clear();
  reco_NCandCB->clear();
  reco_NCandTime->clear();
  reco_NCandPE->clear();
  reco_MrdEnergyLoss = -9999;
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

  this->FillSingleTGraph(gr_neutrons_muonE,h_neutrons_energy,labels_muon_E);
  this->FillSingleTGraph(gr_neutrons_muonE_zoom,h_neutrons_energy_zoom,labels_muon_E);
  this->FillSingleTGraph(gr_neutrons_muonE_fv,h_neutrons_energy_fv,labels_muon_E_fv);
  this->FillSingleTGraph(gr_neutrons_muonE_fv_zoom,h_neutrons_energy_fv_zoom,labels_muon_E_fv);
  this->FillSingleTGraph(gr_neutrons_muonCosTheta,h_neutrons_costheta,labels_muon_CosTheta);
  this->FillSingleTGraph(gr_neutrons_muonCosTheta_fv,h_neutrons_costheta_fv,labels_muon_CosTheta_fv);
  this->FillSingleTGraph(gr_neutrons_muonCosTheta_zoom,h_neutrons_costheta,labels_muon_CosTheta);
  this->FillSingleTGraph(gr_neutrons_muonCosTheta_fv_zoom,h_neutrons_costheta_fv,labels_muon_CosTheta_fv);

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
  //Get lower end of xaxis -> use first bin
  double lower_range = xaxis->GetBinLowEdge(1);
  //Get bin width -> any bin possible, but first is always possible
  double bin_width = xaxis->GetBinWidth(1);

  std::cout <<"lower_range: "<<lower_range<<", bin_width: "<<bin_width<<std::endl;

  for (int i=0; i<n_bins_x; i++){
    double mean_n = 0;
    int n_n = 0;
    for (int j=0; j<n_bins_y; j++){
      int nentries = h2d->GetBinContent(i+1,j+1);
      mean_n+= (j*nentries);
      n_n += nentries;
    }
    if (n_n>0){
      mean_n /= n_n;
      gr->SetPoint(i_graph,lower_range+(i+0.5)*bin_width,mean_n);
      std::cout <<"Setting point (X/Y): "<<lower_range+(i+0.5)*bin_width<<" / "<<mean_n<<std::endl;
      double std_dev = 0;
      for (int j=0; j<n_bins_y; j++){
        int nentries = h2d->GetBinContent(i+1,j+1);
        std_dev+=(nentries*(j-mean_n)*(j-mean_n));
      }
      if (n_n>1) std_dev/=(n_n-1);
      std_dev = sqrt(std_dev);
      if (n_n>0) std_dev/=sqrt(n_n);
      gr->SetPointError(i_graph,bin_width/2.,std_dev);
      std::cout <<"Setting error (X/Y): "<<bin_width/2.<<" / "<<std_dev<<std::endl;
      i_graph++;
    }
  }

  std::string title_string = labels.at(2)+";"+labels.at(0)+";"+labels.at(1);
  gr->SetTitle(title_string.c_str());
  gr->SetDrawOption("ALP");
  gr->SetLineWidth(2);

  return true;
}
