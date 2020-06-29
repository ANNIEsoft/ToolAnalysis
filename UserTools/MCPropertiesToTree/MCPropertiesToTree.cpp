#include "MCPropertiesToTree.h"

MCPropertiesToTree::MCPropertiesToTree():Tool(){}


bool MCPropertiesToTree::Initialise(std::string configfile, DataModel &data){

  if(configfile!="") m_variables.Initialise(configfile);
  m_data= &data;

  //Default variable settings
  outfile_name = "mcproperties.root";
  save_histograms = true;
  save_tree = true;
  has_genie = false;

  //Load in configuration variables
  m_variables.Get("verbose",verbosity);
  m_variables.Get("OutFile",outfile_name);
  m_variables.Get("SaveHistograms",save_histograms);
  m_variables.Get("SaveTree",save_tree);
  m_variables.Get("HasGENIE",has_genie);

  //Define ROOT file to write histograms and tree to
  f = new TFile(outfile_name.c_str(),"RECREATE");

  //Define histograms + tree
  if (save_histograms) this->DefineHistograms();
  if (save_tree) this->DefineTree();

  //Get geometry of tank --> tank center
  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geom);
  Position detector_center=geom->GetTankCentre();
  tank_center_x = detector_center.X();
  tank_center_y = detector_center.Y();
  tank_center_z = detector_center.Z();

  return true;
}


bool MCPropertiesToTree::Execute(){

  //Check if relevant BoostStores (ANNIEEvent/RecoEvent) exist
  int annieeventexists = m_data->Stores.count("ANNIEEvent");
  int recoeventexists = m_data->Stores.count("RecoEvent");
  
  if(!annieeventexists){ 
    Log("MCPropertiesToTree tool: No ANNIEEvent store! Exiting...",v_error,verbosity);
    return false;
  }
  if (!recoeventexists){
    Log("MCPropertiesToTree tool: No RecoEvent store! Exiting...",v_error,verbosity);
    return false;
  }
  
  //Get relevant objects from ANNIEEvent store
  bool get_ok;
  get_ok = m_data->Stores["ANNIEEvent"]->Get("MCParticles",mcparticles);
  if (!get_ok) {Log("MCPropertiesToTree tool: No MCParticles object in ANNIEEvent! Abort.",v_error,verbosity); return true;}
  get_ok = m_data->Stores["ANNIEEvent"]->Get("TriggerData",TriggerData);  
  if (!get_ok) {Log("MCPropertiesToTree tool: No TriggerData object in ANNIEEvent! Abort.",v_error,verbosity); return true;}
  get_ok = m_data->Stores["ANNIEEvent"]->Get("MCTriggernum",MCTriggernum);
  if (!get_ok) {Log("MCPropertiesToTree tool: No MCTriggernum object in ANNIEEvent! Abort.",v_error,verbosity); return true;}
  get_ok = m_data->Stores["ANNIEEvent"]->Get("EventNumber", evnum);
  if (!get_ok) {Log("MCPropertiesToTree tool: No EventNumber object in ANNIEEvent! Abort.",v_error,verbosity); return true;}
  get_ok = m_data->Stores["ANNIEEvent"]->Get("MCHits", MCHits);
  if (!get_ok) {Log("MCPropertiesToTree tool: No MCHits object in ANNIEEvent! Abort.",v_error,verbosity); return true;}
  get_ok = m_data->Stores["ANNIEEvent"]->Get("MCLAPPDHits",MCLAPPDHits);
  if (!get_ok) {Log("MCPropertiesToTree tool: No MCLAPPDHits object in ANNIEEvent! Abort.",v_error,verbosity); return true;}
  get_ok = m_data->Stores["ANNIEEvent"]->Get("TDCData",TDCData);  // a std::map<ChannelKey,vector<TDCHit>>
  if (!get_ok) {Log("MCPropertiesToTree tool: No TDCData object in ANNIEEvent! Abort.",v_error,verbosity); return true;}

  //Get relevant objects from RecoEvent store
  get_ok = m_data->Stores["RecoEvent"]->Get("NRings",nrings);	// need to execute MCRecoEventLoader before this tool to load the relevant information into the store
  if (!get_ok) {Log("MCPropertiesToTree tool: No NRings object in RecoEvent! Abort.",v_error,verbosity); return true;}
  get_ok = m_data->Stores["RecoEvent"]->Get("MCMRDStop",mrd_stop);	// need to execute EventSelector tool before this tool to load the relevant information
  if (!get_ok) {Log("MCPropertiesToTree tool: No MRDStop object in RecoEvent! Abort.",v_error,verbosity); return true;}
  get_ok = m_data->Stores["RecoEvent"]->Get("MCFV",event_fv);
  if (!get_ok) {Log("MCPropertiesToTree tool: No EventFV object in RecoEvent! Abort.",v_error,verbosity); return true;}
  get_ok = m_data->Stores["RecoEvent"]->Get("MCNoPiK",no_pik);
  if (!get_ok) {Log("MCPropertiesToTree tool: No NoPiK object in RecoEvent! Abort.",v_error,verbosity); return true;}
  get_ok = m_data->Stores["RecoEvent"]->Get("MCPMTVol",event_pmtvol);
  if (!get_ok) {Log("MCPropertiesToTree tool: No EventPMTVol object in RecoEvent! Abort.",v_error,verbosity); return true;}
  get_ok = m_data->Stores["RecoEvent"]->Get("MCSingleRingEvent",event_singlering);
  if (!get_ok) {Log("MCPropertiesToTree tool: No MCSingleRingEvent object in RecoEvent! Abort.",v_error,verbosity); return true;}
  get_ok = m_data->Stores["RecoEvent"]->Get("MCMultiRingEvent",event_multiring);
  if (!get_ok) {Log("MCPropertiesToTree tool: No MCMultiRingEvent object in RecoEvent! Abort.",v_error,verbosity); return true;}
  get_ok = m_data->Stores["RecoEvent"]->Get("PMTMRDCoinc",event_pmtmrdcoinc);
  if (!get_ok) {Log("MCPropertiesToTree tool: No PMTMRDCoinc object in RecoEvent! Abort.",v_error,verbosity); return true;}
  get_ok = m_data->Stores["RecoEvent"]->Get("NumPMTClusters",event_pmtclusters);
  if (!get_ok) {Log("MCPropertiesToTree tool: No NumPMTClusters object in RecoEvent! Abort.",v_error,verbosity); return true;}
  get_ok = m_data->Stores["RecoEvent"]->Get("PMTClustersCharge",event_pmtclusters_Q);
  if (!get_ok) {Log("MCPropertiesToTree tool: No PMTClustersCharge object in RecoEvent! Abort.",v_error,verbosity); return true;}
  get_ok = m_data->Stores["RecoEvent"]->Get("PMTClustersTime",event_pmtclusters_T);
  if (!get_ok) {Log("MCPropertiesToTree tool: No PMTClustersTime object in RecoEvent! Abort.",v_error,verbosity); return true;}
  get_ok = m_data->Stores["RecoEvent"]->Get("MRDClustersTime",event_mrdclusters_T);
  if (!get_ok) {Log("MCPropertiesToTree tool: No MRDClustersTime object in RecoEvent! Abort.",v_error,verbosity); return true;}


  //Get relevant objects from CStore
  get_ok = m_data->CStore.Get("NumMrdTimeClusters",mrdClusters);   // need to execute time clustering tool before accessing this variable
  if (!get_ok) {Log("MCPropertiesToTree tool: No NumMrdTimeClusters object in CStore! Abort.",v_error,verbosity); return true;}

  bool new_genieentry = false;
  m_data->CStore.Get("NewGENIEEntry",new_genieentry);

  //Clear vectors stored in tree
  if (save_tree) this->ClearVectors();

  fill_tree = false;

  //Evaluate the TriggerData object
  this->EvalTriggerData();

  //Evaluate the MCParticles object
  this->EvalMCParticles();

  //Evaluate the MCHits object
  this->EvalMCHits();

  //Evaluate the MCLAPPDHits object
  this->EvalMCLAPPDHits();

  //Evaluate the TDCData object
  this->EvalTDCData();

  //Evaluate GENIE information (if any)
  if (has_genie && new_genieentry) this->EvalGENIE();

  //Fill tree if there was data
  if (fill_tree && save_tree) {
    f->cd();
    t->Fill();
    if (has_genie && new_genieentry) t_genie->Fill();
  }

  return true;
}


bool MCPropertiesToTree::Finalise(){

  //Explicitly switch to TFile to avoid conflicts with potential TFiles from other tools
  f->cd();

  //Write tree
  if (save_tree) {
    t->Write("",TObject::kOverwrite);
    if (has_genie) t_genie->Write("",TObject::kOverwrite);
  }

  //Write histograms
  if (save_histograms) WriteHistograms();
 
  //Closing and deleting f takes care of cleanup for histograms and trees
  f->Close();
  delete f;
  
  return true;
}

void MCPropertiesToTree::DefineHistograms(){

  //-----------------------------------------
  //--------Define all histograms------------
  //-----------------------------------------

  f->cd();

  hE = new TH1F("hE","Histogram True Energies",1000,0,5000);
  hPosX = new TH1F("hPosX","True X Start Position",200,-3.,3.);
  hPosY = new TH1F("hPosY","True Y Start Position",200,-2.5,2.5);
  hPosZ = new TH1F("hPosZ","True Z Start Position",200,-5.,5.);
  hPosStopX = new TH1F("hPosStopX","True X Stop Position",200,-3.,3.);
  hPosStopY = new TH1F("hPosStopY","True Y Stop Position",200,-2.5,2.5);
  hPosStopZ = new TH1F("hPosStopZ","True Z Stop Position",200,-5.,5.);
  hDirX = new TH1F("hDirX","True X Direction",200,-1.5,1.5);
  hDirY = new TH1F("hDirY","True Y Direction",200,-1.5,1.5);
  hDirZ = new TH1F("hDirZ","True Z Direction",200,-1.5,1.5);
  hNumPrimaries = new TH1F("hNumPrimaries","Number of Primaries",200,0,200);
  hNumSecondaries = new TH1F("hNumSecondaries","Number of Secondaries",200,0,200);
  hPDGPrimaries = new TH1F("hPDGPrimaries","PDG values (primaries)",10000,0,10000);
  hPDGSecondaries = new TH1F("hPDGSecondaries","PDG values (secondaries)",10000,0,10000);
  hRings = new TH1F("hRings","Number of rings",10,0,10);
  hNoPiK = new TH1F("hNoPiK","No Pions / Kaons",2,0,2);
  hMRDStop = new TH1F("hMRDStop","Events stopped in MRD",2,0,2);
  hFV = new TH1F("hFV","Events started in FV",2,0,2);
  hPMTVol = new TH1F("hPMTVol","Events started in PMTVol",2,0,2);
  hQ = new TH1F("hQ","Charge",200,0,200);
  hQtotal = new TH1F("hQtotal","Total Charge",1000,0,1000);
  hT = new TH1F("hT","Times",200,0,50);
  hQ_LAPPD = new TH1F("hQ_LAPPD","LAPPD Charge",200,0,200);
  hT_LAPPD = new TH1F("hT_LAPPD","LAPPD Times",200,0,50);
  hQtotal_LAPPD = new TH1F("hQtotal_LAPPD","LAPPD Total Charge",1000,0,1000);
  hPMTHits = new TH1F("hPMTHits","PMT Hits",200,0,200);
  hLAPPDHits = new TH1F("hLAPPDHits","LAPPD Hits",200,0,200);
  hMRDPaddles = new TH1F("hMRDPaddles","MRD Paddle Hits",100,0,100);
  hMRDLayers = new TH1F("hMRDLayers","MRD Layer Hits",11,0,11);
  hMRDClusters = new TH1F("hMRDClusters","MRD Clustered Hits",5,0,5);  
  hVetoHits = new TH1F("hVetoHits","Veto Hits",26,0,26);

  gROOT->cd();

}

void MCPropertiesToTree::DefineTree(){

  //----------------------------------------
  //-----------Define TTree-----------------
  //----------------------------------------

  //The Tree is meant to be used if one wants to look at more detailed information about the data
  //Enables a more flexible selection of data

  f->cd();

  t = new TTree("mcproperties","Tree MCProperties");

  particleE = new std::vector<double>;
  particlePDG = new std::vector<int>;
  particleParentPDG = new std::vector<int>;
  particleFlag = new std::vector<int>;
  sec_particleE = new std::vector<double>;
  sec_particlePDG = new std::vector<int>;
  sec_particleParentPDG = new std::vector<int>;
  sec_particleFlag = new std::vector<int>;
  particle_posX = new std::vector<double>;
  particle_posY = new std::vector<double>;
  particle_posZ = new std::vector<double>;
  particle_stopposX = new std::vector<double>;
  particle_stopposY = new std::vector<double>;
  particle_stopposZ = new std::vector<double>;
  particle_dirX = new std::vector<double>;
  particle_dirY = new std::vector<double>;
  particle_dirZ = new std::vector<double>;
  sec_particle_posX = new std::vector<double>;
  sec_particle_posY = new std::vector<double>;
  sec_particle_posZ = new std::vector<double>;
  sec_particle_stopposX = new std::vector<double>;
  sec_particle_stopposY = new std::vector<double>;
  sec_particle_stopposZ = new std::vector<double>;
  sec_particle_dirX = new std::vector<double>;
  sec_particle_dirY = new std::vector<double>;
  sec_particle_dirZ = new std::vector<double>;
  pmtQ = new std::vector<double>;
  pmtT = new std::vector<double>;
  pmtID = new std::vector<int>;
  lappdQ = new std::vector<double>;
  lappdT = new std::vector<double>;
  lappdID = new std::vector<int>;
  mrdT = new std::vector<double>;
  mrdID = new std::vector<int>;
  fmvT = new std::vector<double>;
  fmvID = new std::vector<int>;

  t->Branch("E_true",&particleE);
  t->Branch("PDG",&particlePDG);
  t->Branch("ParentPDG",&particleParentPDG);
  t->Branch("Flag",&particleFlag);
  t->Branch("SecE_true",&sec_particleE);
  t->Branch("SecPDG",&sec_particlePDG);
  t->Branch("SecParentPDG",&sec_particleParentPDG);
  t->Branch("SecFlag",&sec_particleFlag);
  t->Branch("NTriggers",&particleTriggers);
  t->Branch("EventNr",&evnum);
  t->Branch("ParticlePosX",&particle_posX);
  t->Branch("ParticlePosY",&particle_posY);
  t->Branch("ParticlePosZ",&particle_posZ);
  t->Branch("ParticleStopPosX",&particle_stopposX);
  t->Branch("ParticleStopPosY",&particle_stopposY);
  t->Branch("ParticleStopPosZ",&particle_stopposZ);
  t->Branch("ParticleDirX",&particle_dirX);
  t->Branch("ParticleDirY",&particle_dirY);
  t->Branch("ParticleDirZ",&particle_dirZ);
  t->Branch("SecParticlePosX",&sec_particle_posX);
  t->Branch("SecParticlePosY",&sec_particle_posY);
  t->Branch("SecParticlePosZ",&sec_particle_posZ);
  t->Branch("SecParticleStopPosX",&sec_particle_stopposX);
  t->Branch("SecParticleStopPosY",&sec_particle_stopposY);
  t->Branch("SecParticleStopPosZ",&sec_particle_stopposZ);
  t->Branch("SecParticleDirX",&sec_particle_dirX);
  t->Branch("SecParticleDirY",&sec_particle_dirY);
  t->Branch("SecParticleDirZ",&sec_particle_dirZ);
  t->Branch("NumPrimaries",&num_primaries);
  t->Branch("NumSecondaries",&num_secondaries);
  t->Branch("PMTQ",&pmtQ);
  t->Branch("PMTT",&pmtT);
  t->Branch("PMTID",&pmtID);
  t->Branch("LAPPDID",&lappdID);
  t->Branch("PMTQtotal",&pmtQtotal);
  t->Branch("LAPPDQ",&lappdQ);
  t->Branch("LAPPDT",&lappdT);
  t->Branch("LAPPDQtotal",&lappdQtotal);
  t->Branch("PMTHits",&pmtHits);
  t->Branch("LAPPDHits",&lappdHits);
  t->Branch("MRDPaddles",&mrdPaddles);
  t->Branch("MRDLayers",&mrdLayers);
  t->Branch("MRDT",&mrdT);
  t->Branch("MRDID",&mrdID);
  t->Branch("FMVHits",&num_veto_hits);
  t->Branch("FMVT",&fmvT);
  t->Branch("FMVID",&fmvID);
  t->Branch("Prompt",&is_prompt);
  t->Branch("MCTriggerNum",&mctriggernum);
  t->Branch("TriggerTime",&trigger_time);
  t->Branch("NRings",&nrings);
  t->Branch("NoPiK",&no_pik);
  t->Branch("MRDStop",&mrd_stop);
  t->Branch("FV",&event_fv);
  t->Branch("PMTVol",&event_pmtvol);
  t->Branch("SingleRing",&event_singlering);
  t->Branch("MultiRing",&event_multiring);
  t->Branch("PMTMRDCoinc",&event_pmtmrdcoinc);
  t->Branch("NumMRDClusters",&mrdClusters);
  t->Branch("NumPMTClusters",&event_pmtclusters);
  t->Branch("PMTClustersCharge",&event_pmtclusters_Q);
  t->Branch("PMTClustersTime",&event_pmtclusters_T);
  t->Branch("MRDClustersTime",&event_mrdclusters_T);

  if (has_genie) {

    t_genie = new TTree("genieproperties","Tree GENIE properties");

    /*genie_file_pointer = new std::string;  
    genie_parentdecaystring_pointer = new std::string;
    genie_parentpdgattgtexitstring_pointer = new std::string;
    genie_interactiontypestring_pointer = new std::string;
    genie_fsleptonname_pointer = new std::string;
*/
    t_genie->Branch("file",&genie_file);
    t_genie->Branch("fluxver",&genie_fluxver);
    t_genie->Branch("evtnum",&genie_evtnum);
    t_genie->Branch("ParentPdg",&genie_parentpdg);
    t_genie->Branch("ParentDecayMode",&genie_parentdecaymode);
    t_genie->Branch("ParentDecayString",&genie_parentdecaystring);
    t_genie->Branch("ParentDecayVtx_X",&genie_parentdecayvtxx);
    t_genie->Branch("ParentDecayVtx_Y",&genie_parentdecayvtxy);
    t_genie->Branch("ParentDecayVtx_Z",&genie_parentdecayvtxz);
    t_genie->Branch("ParentDecayMom_X",&genie_parentdecaymomx);
    t_genie->Branch("ParentDecayMom_Y",&genie_parentdecaymomy);
    t_genie->Branch("ParentDecayMom_Z",&genie_parentdecaymomz);
    t_genie->Branch("ParentProdMom_X",&genie_prodmomx);
    t_genie->Branch("ParentProdMom_Y",&genie_prodmomy);
    t_genie->Branch("ParentProdMom_Z",&genie_prodmomz);
    t_genie->Branch("ParentPdgAtTgtExit",&genie_parentpdgattgtexit);
    t_genie->Branch("ParentTypeAtTgtExitString",&genie_parenttypeattgtexitstring);
    t_genie->Branch("ParentTgtExitMom_X",&genie_parenttgtexitmomx);
    t_genie->Branch("ParentTgtExitMom_Y",&genie_parenttgtexitmomy);
    t_genie->Branch("ParentTgtExitMom_Z",&genie_parenttgtexitmomz);
    t_genie->Branch("ParentProdMedium",&genie_parentprodmedium);
    t_genie->Branch("ParentProdMediumString",&genie_parentprodmediumstring);
   
    t_genie->Branch("IsQuasiElastic",&genie_isquasielastic);
    t_genie->Branch("IsResonant",&genie_isresonant);
    t_genie->Branch("IsDeepInelastic",&genie_isdeepinelastic);
    t_genie->Branch("IsCoherent",&genie_iscoherent);
    t_genie->Branch("IsDiffractive",&genie_isdiffractive);
    t_genie->Branch("IsInverseMuDecay",&genie_isinversemudecay);
    t_genie->Branch("IsIMDAnnihilation",&genie_isimdannihilation);
    t_genie->Branch("IsSingleKaon",&genie_issinglekaon);
    t_genie->Branch("IsNuElectronElastic",&genie_isnuelectronelastic);
    t_genie->Branch("IsEM",&genie_isem);
    t_genie->Branch("IsWeakCC",&genie_isweakcc);
    t_genie->Branch("IsWeakNC",&genie_isweaknc);
    t_genie->Branch("IsMEC",&genie_ismec);
    t_genie->Branch("InteractionTypeString",&genie_interactiontypestring);
    t_genie->Branch("NeutCode",&genie_neutcode);
    t_genie->Branch("NuIntVtx_X",&genie_nuintvtxx);
    t_genie->Branch("NuIntVtx_Y",&genie_nuintvtxy);
    t_genie->Branch("NuIntVtx_Z",&genie_nuintvtxz);
    t_genie->Branch("NuIntVtx_T",&genie_nuintvtxt);
    t_genie->Branch("NuVtxInTank",&genie_nuvtxintank);
    t_genie->Branch("NuVtxInFidVol",&genie_nuvtxinfidvol);
    t_genie->Branch("EventQ2",&genie_eventQ2);
    t_genie->Branch("NeutrinoEnergy",&genie_neutrinoenergy);
    t_genie->Branch("NeutrinoPDG",&genie_neutrinopdg);
    t_genie->Branch("MuonEnergy",&genie_muonenergy);
    t_genie->Branch("MuonAngle",&genie_muonangle);
    t_genie->Branch("FSLeptonName",&genie_fsleptonname);
    t_genie->Branch("NumFSProtons",&genie_numfsp);
    t_genie->Branch("NumFSNeutrons",&genie_numfsn);
    t_genie->Branch("NumFSPi0",&genie_numfspi0);
    t_genie->Branch("NumFSPiPlus",&genie_numfspiplus);
    t_genie->Branch("NumFSPiMinus",&genie_numfspiminus);
    t_genie->Branch("NumFSKPlus",&genie_numfskplus);
    t_genie->Branch("NumFSKMinus",&genie_numfskminus);
  }

  gROOT->cd();

}

void MCPropertiesToTree::WriteHistograms(){

  f->cd();

  hE->Write();
  hPosX->Write();
  hPosY->Write();
  hPosZ->Write();
  hPosStopX->Write();
  hPosStopY->Write();
  hPosStopZ->Write();
  hDirX->Write();
  hDirY->Write();
  hDirZ->Write();
  hQ->Write();
  hT->Write();
  hQtotal->Write();
  hQ_LAPPD->Write();
  hT_LAPPD->Write();
  hQtotal_LAPPD->Write();
  hMRDLayers->Write();
  hMRDPaddles->Write();
  hMRDClusters->Write();
  hVetoHits->Write();
  hPMTHits->Write();
  hLAPPDHits->Write();
  hNumPrimaries->Write();
  hNumSecondaries->Write();
  hPDGPrimaries->Write();
  hPDGSecondaries->Write();
  hRings->Write();
  hNoPiK->Write();
  hMRDStop->Write();
  hFV->Write();
  hPMTVol->Write();

  gROOT->cd();

}

void MCPropertiesToTree::ClearVectors(){

  particleE->clear();
  particlePDG->clear();
  particleParentPDG->clear();
  particleFlag->clear();
  sec_particleE->clear();
  sec_particlePDG->clear();
  sec_particleParentPDG->clear();
  sec_particleFlag->clear();
  particle_posX->clear();
  particle_posY->clear();
  particle_posZ->clear();
  particle_stopposX->clear();
  particle_stopposY->clear();
  particle_stopposZ->clear();
  particle_dirX->clear();
  particle_dirY->clear();
  particle_dirZ->clear();
  sec_particle_posX->clear();
  sec_particle_posY->clear();
  sec_particle_posZ->clear();
  sec_particle_stopposX->clear();
  sec_particle_stopposY->clear();
  sec_particle_stopposZ->clear();
  sec_particle_dirX->clear();
  sec_particle_dirY->clear();
  sec_particle_dirZ->clear();
  pmtQ->clear();
  pmtT->clear();
  pmtID->clear();
  lappdQ->clear();
  lappdT->clear();
  lappdID->clear();
  mrdT->clear();
  mrdID->clear();
  fmvT->clear();
  fmvID->clear();

}

void MCPropertiesToTree::EvalTriggerData(){
 
 //Get TriggerTime in ns, MCTriggerNum
 particleTriggers = TriggerData->size();   	//there will be always 1 trigger entry for 1 WCSim event
  
 for (unsigned int i_trigger = 0; i_trigger< TriggerData->size(); i_trigger++){
    TriggerTime = TriggerData->at(i_trigger).GetTime();

    if (verbosity >= v_message){
	std::cout <<"ParticleTriggers = "<<particleTriggers<<", ";
  	std::cout <<"MCTriggerNum = "<<MCTriggernum<<", ";
  	std::cout <<"EventNr = "<<evnum<<", ";
	std::cout <<"TriggerTime = "<<TriggerTime.GetNs()<<std::endl;
    }

    trigger_time = TriggerTime.GetNs();
  }

  Log("MCPropertiesToTree tool: Trigger time = "+std::to_string(trigger_time)+", FV = "+std::to_string(event_fv)+", PMTVol = "+std::to_string(event_pmtvol)+", MRDStop = "+std::to_string(mrd_stop),v_message,verbosity);

  mctriggernum = MCTriggernum;

  //Prompt event or delayed?
  if (MCTriggernum==0) is_prompt = 1;
  else is_prompt = 0;
}

void MCPropertiesToTree::EvalMCParticles(){

  //Get information about true particle behavior
  num_primaries = 0;
  num_secondaries = 0;

  for (unsigned int i_particle = 0; i_particle < mcparticles->size(); i_particle++){

    MCParticle aparticle = mcparticles->at(i_particle);
    double particle_energy = aparticle.GetStartEnergy();
    int particle_pdg = aparticle.GetPdgCode();
    int particle_parentpdg = aparticle.GetParentPdg();
    int particle_flag = aparticle.GetFlag();

    if (particle_parentpdg == 0 && particle_flag == 0) {
      num_primaries++;
      Position pos = aparticle.GetStartVertex();
      Position Stoppos = aparticle.GetStopVertex();
      Direction dir = aparticle.GetStartDirection();
      double particleposX = pos.X() - tank_center_x;
      double particleposY = pos.Y() - tank_center_y;
      double particleposZ = pos.Z() - tank_center_z;
      double particlestopposX = Stoppos.X() - tank_center_x;
      double particlestopposY = Stoppos.Y() - tank_center_y;
      double particlestopposZ = Stoppos.Z() - tank_center_z;
      double particledirX = dir.X();
      double particledirY = dir.Y();
      double particledirZ = dir.Z();
      
      if (save_histograms){
        hE->Fill(particle_energy);
        hPosX->Fill(particleposX);
        hPosY->Fill(particleposY);
        hPosZ->Fill(particleposZ);
        hPosStopX->Fill(particlestopposX);
        hPosStopY->Fill(particlestopposY);
        hPosStopZ->Fill(particlestopposZ);
        hDirX->Fill(particledirX);
        hDirY->Fill(particledirY);
        hDirZ->Fill(particledirZ);
      }
      if (save_tree){
        particleE->push_back(particle_energy);
        particlePDG->push_back(particle_pdg);
        particleParentPDG->push_back(particle_parentpdg);
        particleFlag->push_back(particle_flag);
        particle_posX->push_back(particleposX);
        particle_posY->push_back(particleposY);
        particle_posZ->push_back(particleposZ);
        particle_stopposX->push_back(particlestopposX);
        particle_stopposY->push_back(particlestopposY);
        particle_stopposZ->push_back(particlestopposZ);
        particle_dirX->push_back(particledirX);
        particle_dirY->push_back(particledirY);
        particle_dirZ->push_back(particledirZ); 
      }
    }

    else {
      
      num_secondaries++;
      Position pos = aparticle.GetStartVertex();
      Position Stoppos = aparticle.GetStopVertex();
      Direction dir = aparticle.GetStartDirection();
      double secparticleposX = pos.X() - tank_center_x;
      double secparticleposY = pos.Y() - tank_center_y;
      double secparticleposZ = pos.Z() - tank_center_z;
      double secparticlestopposX = Stoppos.X() - tank_center_x;
      double secparticlestopposY = Stoppos.Y() - tank_center_y;
      double secparticlestopposZ = Stoppos.Z() - tank_center_z;
      double secparticledirX = dir.X();
      double secparticledirY = dir.Y();
      double secparticledirZ = dir.Z();

      if (save_tree){
        
        sec_particleE->push_back(particle_energy);
        sec_particlePDG->push_back(particle_pdg);
        sec_particleParentPDG->push_back(particle_parentpdg);
        sec_particleFlag->push_back(particle_flag);
        sec_particle_posX->push_back(secparticleposX);
        sec_particle_posY->push_back(secparticleposY);
        sec_particle_posZ->push_back(secparticleposZ);
        sec_particle_stopposX->push_back(secparticlestopposX);
        sec_particle_stopposY->push_back(secparticlestopposY);
        sec_particle_stopposZ->push_back(secparticlestopposZ);
        sec_particle_dirX->push_back(secparticledirX);
        sec_particle_dirY->push_back(secparticledirY);
        sec_particle_dirZ->push_back(secparticledirZ); 

      }
    }
  } 

  if (save_histograms){

    hNumPrimaries->Fill(num_primaries);
    hNumSecondaries->Fill(num_secondaries);
    for (unsigned int i_primary = 0; i_primary < particlePDG->size(); i_primary++){
      hPDGPrimaries->Fill(particlePDG->at(i_primary));
    }
    for (unsigned int i_secondary = 0; i_secondary < sec_particlePDG->size(); i_secondary++){
      hPDGSecondaries->Fill(sec_particlePDG->at(i_secondary));
    }

    hRings->Fill(nrings);
    hNoPiK->Fill(no_pik);
    hMRDStop->Fill(mrd_stop);
    hFV->Fill(event_fv);
    hPMTVol->Fill(event_pmtvol);
    hMRDClusters->Fill(mrdClusters);

  }

  fill_tree=true;
  	
}

void MCPropertiesToTree::EvalMCHits(){

  //Get information associated to PMT hits

  pmtHits=0;
  pmtQtotal=0;
  if (MCHits){
    for(std::pair<unsigned long, std::vector<MCHit>>&& apair : *MCHits){
      unsigned long chankey = apair.first;
      Detector* thistube = geom->ChannelToDetector(chankey);
      unsigned long detkey = thistube->GetDetectorID();
      if (thistube->GetDetectorElement()=="Tank"){
        std::vector<MCHit>& Hits = apair.second;
        double q=0;
        double t=0;
        int singlepmtHits=0;
        for (MCHit &ahit : Hits){
          q += ahit.GetCharge();
          t += ahit.GetTime();
          singlepmtHits++;
        }
        t/=singlepmtHits;         //use mean time of all hits on one PMT
        if (save_histograms){
          hQ->Fill(q);
          hT->Fill(t);
        }
        if (save_tree){
          pmtT->push_back(t);
          pmtQ->push_back(q);
          pmtID->push_back(chankey);
        }
        pmtHits++;
	pmtQtotal+=q;
      }
    }
  }

  if (save_histograms){
    hPMTHits->Fill(pmtHits);
    hQtotal->Fill(pmtQtotal);
  }
}

void MCPropertiesToTree::EvalMCLAPPDHits(){
    
  //Get information associated to LAPPD hits

  lappdHits = 0;
  lappdQtotal = 0;
  int num_lappds_hit=0;
  
  if(MCLAPPDHits){
    num_lappds_hit = MCLAPPDHits->size();
    for (std::pair<unsigned long, std::vector<MCLAPPDHit>>&& apair : *MCLAPPDHits){
      unsigned long chankey = apair.first;
      Detector *det = geom->ChannelToDetector(chankey);
      if(det==nullptr){
        if (verbosity > 0) std::cout <<"MCPropertiesToTree Tool: LAPPD Detector not found! "<<std::endl;;
        continue;
      }
      int detkey = det->GetDetectorID();
      std::vector<MCLAPPDHit>& hits = apair.second;
      for (MCLAPPDHit& ahit : hits){
  	if (save_tree){
          lappdQ->push_back(1.0);
          lappdT->push_back(ahit.GetTime());
          lappdID->push_back(chankey);
        }
        if (save_histograms){
          hT_LAPPD->Fill(ahit.GetTime());
          hQ_LAPPD->Fill(1.0);
        }
	lappdQtotal++;
      }
      lappdHits++;
    }
  } else {
    Log("MCPropertiesToTree tool: No MCLAPPDHits!", v_warning, verbosity);
    num_lappds_hit = 0;
  }
 
  if (save_histograms){   
    hLAPPDHits->Fill(lappdHits);
    hQtotal_LAPPD->Fill(lappdQtotal);
  }

}

void MCPropertiesToTree::EvalTDCData(){
 
  //Get information associated to MRD hits

  if(!TDCData){
    Log("MCPropertiesToTree tool: No TDC data to process!",v_warning,verbosity);
  } else {
    if(TDCData->size()==0){
      Log("MCPropertiesToTree tool: No TDC hits.",v_message,verbosity);
      if (save_histograms){
        hMRDPaddles->Fill(0);
        hMRDLayers->Fill(0);
	hVetoHits->Fill(0);
      }
      mrdPaddles = 0;
      mrdLayers = 0;
      num_veto_hits = 0;
    } else {
      mrdPaddles=0;
      mrdLayers=0;
      num_veto_hits=0;
      bool layer_occupied[11] = {0};
      for(auto&& anmrdpmt : (*TDCData)){
        unsigned long chankey = anmrdpmt.first;
        Detector *thedetector = geom->ChannelToDetector(chankey);
        double mrdtimes=0.;
        int mrddigits=0;
        for(auto&& hitsonthismrdpmt : anmrdpmt.second){
          mrdtimes+=hitsonthismrdpmt.GetTime();
          mrddigits++;
        }
        if (mrddigits > 0) mrdtimes/=mrddigits;
        if(thedetector->GetDetectorElement()!="MRD") {
          num_veto_hits++;
    	  fmvT->push_back(mrdtimes);
          fmvID->push_back(chankey);
          continue;  // this is a veto hit, not an MRD hit.
        }
	mrdT->push_back(mrdtimes);
        mrdID->push_back(chankey);
        mrdPaddles++;
        int detkey = thedetector->GetDetectorID();
        Paddle *apaddle = geom->GetDetectorPaddle(detkey);
        int layer = apaddle->GetLayer();
        layer_occupied[layer]=true;
      }
 
      if (save_histograms) {
        hMRDPaddles->Fill(mrdPaddles);
        hVetoHits->Fill(num_veto_hits);
      }
          
      if (mrdPaddles > 0) {
        for (int i_layer=0;i_layer<11;i_layer++){
          if (layer_occupied[i_layer]==true) {
            mrdLayers++;
          }
        }
      }     
    }
  }
 
  if (save_histograms) hMRDLayers->Fill(mrdLayers);

}

void MCPropertiesToTree::EvalGENIE(){

  // More general GENIE variables
  m_data->Stores["GenieInfo"]->Get("file",genie_file);
  m_data->Stores["GenieInfo"]->Get("fluxver",genie_fluxver);
  m_data->Stores["GenieInfo"]->Get("evtnum",genie_evtnum);
  m_data->Stores["GenieInfo"]->Get("ParentPdg",genie_parentpdg);
  m_data->Stores["GenieInfo"]->Get("ParentDecayMode",genie_parentdecaymode);
  m_data->Stores["GenieInfo"]->Get("ParentDecayString",genie_parentdecaystring);
  m_data->Stores["GenieInfo"]->Get("ParentDecayVtx",genie_parentdecayvtx);
  m_data->Stores["GenieInfo"]->Get("ParentDecayVtx_X",genie_parentdecayvtxx);
  m_data->Stores["GenieInfo"]->Get("ParentDecayVtx_Y",genie_parentdecayvtxy);
  m_data->Stores["GenieInfo"]->Get("ParentDecayVtx_Z",genie_parentdecayvtxz);
  m_data->Stores["GenieInfo"]->Get("ParentDecayMom",genie_parentdecaymom);
  m_data->Stores["GenieInfo"]->Get("ParentDecayMom_X",genie_parentdecaymomx);
  m_data->Stores["GenieInfo"]->Get("ParentDecayMom_Y",genie_parentdecaymomy);
  m_data->Stores["GenieInfo"]->Get("ParentDecayMom_Z",genie_parentdecaymomz);
  m_data->Stores["GenieInfo"]->Get("ParentProdMom",genie_parentprodmom);
  m_data->Stores["GenieInfo"]->Get("ParentProdMom_X",genie_prodmomx);
  m_data->Stores["GenieInfo"]->Get("ParentProdMom_Y",genie_prodmomy);
  m_data->Stores["GenieInfo"]->Get("ParentProdMom_Z",genie_prodmomz);
  m_data->Stores["GenieInfo"]->Get("ParentProdMedium",genie_parentprodmedium);
  m_data->Stores["GenieInfo"]->Get("ParentProdMediumString",genie_parentprodmediumstring);
  m_data->Stores["GenieInfo"]->Get("ParentPdgAtTgtExit",genie_parentpdgattgtexit);
  m_data->Stores["GenieInfo"]->Get("ParentTypeAtTgtExitString",genie_parenttypeattgtexitstring);
  m_data->Stores["GenieInfo"]->Get("ParentTgtExitMom",genie_parenttgtexitmom);
  m_data->Stores["GenieInfo"]->Get("ParentTgtExitMom_X",genie_parenttgtexitmomx);
  m_data->Stores["GenieInfo"]->Get("ParentTgtExitMom_Y",genie_parenttgtexitmomy);
  m_data->Stores["GenieInfo"]->Get("ParentTgtExitMom_Z",genie_parenttgtexitmomz);

  // Neutrino-interaction related GENIE variables
  m_data->Stores["GenieInfo"]->Get("IsQuasiElastic",genie_isquasielastic);
  m_data->Stores["GenieInfo"]->Get("IsResonant",genie_isresonant);
  m_data->Stores["GenieInfo"]->Get("IsDeepInelastic",genie_isdeepinelastic);
  m_data->Stores["GenieInfo"]->Get("IsCoherent",genie_iscoherent);
  m_data->Stores["GenieInfo"]->Get("IsDiffractive",genie_isdiffractive);
  m_data->Stores["GenieInfo"]->Get("IsInverseMuDecay",genie_isinversemudecay);
  m_data->Stores["GenieInfo"]->Get("IsIMDAnnihilation",genie_isimdannihilation);
  m_data->Stores["GenieInfo"]->Get("IsSingleKaon",genie_issinglekaon);
  m_data->Stores["GenieInfo"]->Get("IsNuElectronElastic",genie_isnuelectronelastic);
  m_data->Stores["GenieInfo"]->Get("IsEM",genie_isem);
  m_data->Stores["GenieInfo"]->Get("IsWeakCC",genie_isweakcc);
  m_data->Stores["GenieInfo"]->Get("IsWeakNC",genie_isweaknc);
  m_data->Stores["GenieInfo"]->Get("IsMEC",genie_ismec);
  m_data->Stores["GenieInfo"]->Get("InteractionTypeString",genie_interactiontypestring);
  m_data->Stores["GenieInfo"]->Get("NeutCode",genie_neutcode);
  m_data->Stores["GenieInfo"]->Get("NuIntxVtx_X",genie_nuintvtxx);
  m_data->Stores["GenieInfo"]->Get("NuIntxVtx_Y",genie_nuintvtxy);
  m_data->Stores["GenieInfo"]->Get("NuIntxVtx_Z",genie_nuintvtxz);
  m_data->Stores["GenieInfo"]->Get("NuIntxVtx_T",genie_nuintvtxt);
  m_data->Stores["GenieInfo"]->Get("NuVtxInTank",genie_nuvtxintank);
  m_data->Stores["GenieInfo"]->Get("NuVtxInFidVol",genie_nuvtxinfidvol);
  m_data->Stores["GenieInfo"]->Get("EventQ2",genie_eventQ2);
  m_data->Stores["GenieInfo"]->Get("NeutrinoEnergy",genie_neutrinoenergy);
  m_data->Stores["GenieInfo"]->Get("NeutrinoPDG",genie_neutrinopdg);
  m_data->Stores["GenieInfo"]->Get("MuonEnergy",genie_muonenergy);
  m_data->Stores["GenieInfo"]->Get("MuonAngle",genie_muonangle);
  m_data->Stores["GenieInfo"]->Get("FSLeptonName",genie_fsleptonname);
  m_data->Stores["GenieInfo"]->Get("NumFSProtons",genie_numfsp);
  m_data->Stores["GenieInfo"]->Get("NumFSNeutrons",genie_numfsn);
  m_data->Stores["GenieInfo"]->Get("NumFSPi0",genie_numfspi0);
  m_data->Stores["GenieInfo"]->Get("NumFSPiPlus",genie_numfspiplus);
  m_data->Stores["GenieInfo"]->Get("NumFSPiMinus",genie_numfspiminus);
  m_data->Stores["GenieInfo"]->Get("NumFSKPlus",genie_numfskplus);
  m_data->Stores["GenieInfo"]->Get("NumFSKMinus",genie_numfskminus);
  
}
