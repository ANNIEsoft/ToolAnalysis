#include "NeutronMultiplicity.h"

NeutronMultiplicity::NeutronMultiplicity():Tool(){}


bool NeutronMultiplicity::Initialise(std::string configfile, DataModel &data){

  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  m_data= &data; //assigning transient data pointer

  bool get_ok = false;

  save_bs = true;
  save_root = true;
  filename = "NeutronMultiplicity";

  get_ok = m_variables.Get("verbosity",verbosity);

  get_ok = m_variables.Get("SaveROOT",save_root);

  get_ok = m_variables.Get("SaveBoostStore",save_bs);

  get_ok = m_variables.Get("Filename",filename);

  get_ok = m_variables.Get("MRDTrackRestriction",mrdtrack_restriction);

  if (save_root){
    //Initialise root specific objects (files, histograms, trees)
    this->InitialiseHistograms();
  }


  return true;
}


bool NeutronMultiplicity::Execute(){

  //Reset tree variables
  this->ResetVariables();

  //Get Particles variable
 
  bool get_ok = m_data->Stores["ANNIEEvent"]->Get("Particles",Particles);

  int EventNumber = -1;
  m_data->Stores["ANNIEEvent"]->Get("EventNumber",EventNumber);

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
    int numtracksinev=-1;
    m_data->Stores["MRDTracks"]->Get("NumMrdTracks",numtracksinev);
    if (numtracksinev != 1) pass_reconstruction = false;
  }

  std::cout <<"EventNumber: "<<EventNumber<<", pass_selection: "<<pass_selection<<", pass_reconstruction: "<<pass_reconstruction<<std::endl;

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

  if (save_root) file_neutronmult->Write();
  delete file_neutronmult;

  return true;
}

bool NeutronMultiplicity::InitialiseHistograms(){
 
  Log("NeutronMultiplicity tool: InitialiseHistograms",v_message,verbosity);
 
  //output TFile
  std::stringstream ss_filename;
  ss_filename << filename << ".root";
  file_neutronmult = new TFile(ss_filename.str().c_str(),"RECREATE");

  Log("NeutronMultiplicity tool: Define histograms",v_message,verbosity);

  //output histograms
  h_time_neutrons = new TH1F("h_time_neutrons","Cluster time beam neutrons",200,10000,70000);
  h_time_neutrons_mrdstop = new TH1F("h_time_neutrons_mrdstop","Cluster time beam neutrons",200,10000,70000);
  h_neutrons = new TH1F("h_neutrons","Number of neutrons in beam events",10,0,10);
  h_neutrons_mrdstop = new TH1F("h_neutrons_mrdstop","Number of of neutrons in beam events (MRD stop)",10,0,10);
  h_neutrons_mrdstop_fv = new TH1F("h_neutrons_mrdstop_fv","Number of of neutrons in beam events (MRD stop, FV)",10,0,10);
  h_neutrons_energy = new TH2F("h_neutrons_energy","Neutron multiplicity vs muon energy",10,0,2000,20,0,20);
  h_neutrons_energy_fv = new TH2F("h_neutrons_energy_fv","Neutron multiplicity vs muon energy (FV)",10,0,2000,20,0,20);
  h_neutrons_energy_zoom = new TH2F("h_neutrons_energy_zoom","Neutron multiplicity vs muon energy",8,400,1200,20,0,20);
  h_neutrons_energy_fv_zoom = new TH2F("h_neutrons_energy_fv_zoom","Neutron multiplicity vs muon energy (FV)",6,600,1200,20,0,20);
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
  h_neutrons_costheta = new TH2F("h_neutrons_costheta","Neutron multiplicity vs muon angle",6,0.7,1.0,20,0,20);
  h_neutrons_costheta_fv = new TH2F("h_neutrons_costheta_fv","Neutron multiplicity vs muon angle (FV)",6,0.7,1.0,20,0,20);
  h_primneutrons_costheta = new TH2F("h_primneutrons_costheta","Primary Neutron multiplicity vs muon angle",6,0.7,1.0,20,0,20);
  h_primneutrons_costheta_fv = new TH2F("h_primvolneutrons_costheta_fv","Primary Neutron multiplicity vs muon angle (FV)",6,0.7,1.0,20,0,20);
  h_totalneutrons_costheta = new TH2F("h_totalneutrons_costheta","Total Neutron multiplicity vs muon angle",6,0.7,1.0,20,0,20);
  h_totalneutrons_costheta_fv = new TH2F("h_totalneutrons_costheta_fv","Total Neutron multiplicity vs muon angle (FV)",6,0.7,1.0,20,0,20);
  h_pmtvolneutrons_costheta = new TH2F("h_pmtvolneutrons_costheta","PMTVol Neutron multiplicity vs muon angle",6,0.7,1.0,20,0,20);
  h_pmtvolneutrons_costheta_fv = new TH2F("h_pmtvolneutrons_costheta_fv","PMTVol Neutron multiplicity vs muon angle (FV)",6,0.7,1.0,20,0,20);

  h_muon_energy = new TH1F("h_muon_energy","Muon energy distribution",100,0,2000);
  h_muon_energy_fv = new TH1F("h_muon_energy_fv","Muon energy distribution (FV)",100,0,2000);
  h_muon_vtx_yz = new TH2F("h_muon_vtx_yz","Muon vertex (tank) Y-Z",200,-3,3,200,-3,3);
  h_muon_vtx_xz = new TH2F("h_muon_vtx_xz","Muon vertex (tank) X-Z",200,-2.5,2.5,200,-2.5,2.5);
  h_muon_costheta = new TH1F("h_muon_costheta","Muon cos(#theta) distribution",100,-1,1);
  h_muon_costheta_fv = new TH1F("h_muon_costheta_fv","Muon cos(#theta) distribution (FV)",100,-1,1);
  h_muon_vtx_x = new TH1F("h_muon_vtx_x","Muon vertex (x)",200,-3,3);
  h_muon_vtx_y = new TH1F("h_muon_vtx_y","Muon vertex (y)",200,-3,3);
  h_muon_vtx_z = new TH1F("h_muon_vtx_z","Muon vertex (z)",200,-3,3);

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

  //Define tree properties and variables
  //

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
  store_neutronmult.Set("EventCutStatus",true);
  store_neutronmult.Set("Particles",Particles);
  store_neutronmult.Set("SimpleRecoFlag",SimpleRecoFlag);
  store_neutronmult.Set("SimpleRecoEnergy",SimpleRecoFlag);
  store_neutronmult.Set("SimpleRecoVtx",SimpleRecoVtx); 
  store_neutronmult.Set("SimpleRecoStopVtx",SimpleRecoStopVtx); 
   
  //Construct BoostStore filename
  std::stringstream ss_filename;
  ss_filename << filename << ".bs";
  Log("NeutronMultiplicity tool: Saving BoostStore file "+ss_filename.str(),v_message,verbosity);
    
  //Save BoostStore
  store_neutronmult.Save(ss_filename.str().c_str()); 
    
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

  std::vector<int> cluster_neutron;
  std::vector<double> cluster_times_neutron;
  std::vector<double> cluster_charges_neutron;
  std::vector<double> cluster_cb_neutron;
  std::vector<double> cluster_times;
  std::vector<double> cluster_charges;
  std::vector<double> cluster_cb;

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

  bool passPMTMRDCoincCut; 
  return_value = m_data->Stores.at("RecoEvent")->Get("PMTMRDCoinc",passPMTMRDCoincCut);
  reco_TankMRDCoinc = (passPMTMRDCoincCut)? 1 : 0;

 
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

  return true;
}
