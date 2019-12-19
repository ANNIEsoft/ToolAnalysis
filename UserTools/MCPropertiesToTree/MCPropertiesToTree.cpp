#include "MCPropertiesToTree.h"

MCPropertiesToTree::MCPropertiesToTree():Tool(){}


bool MCPropertiesToTree::Initialise(std::string configfile, DataModel &data){

  if(configfile!="") m_variables.Initialise(configfile);
  m_data= &data;

  outfile_name = "explore_mcproperties.root";

  //load in configuration variables
  m_variables.Get("verbose",verbosity);
  m_variables.Get("OutFile",outfile_name);

  f = new TFile(outfile_name.c_str(),"RECREATE");
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

  //The Tree is meant to be used if one wants to look at more detailed information about the data
  t = new TTree("mcproperties","Tree MCProperties");

  particleE = new std::vector<double>;
  particlePDG = new std::vector<int>;
  particleParentPDG = new std::vector<int>;
  particleFlag = new std::vector<int>;
  particlePDG_primaries = new std::vector<int>;
  particlePDG_secondaries = new std::vector<int>;
  particle_posX = new std::vector<double>;
  particle_posY = new std::vector<double>;
  particle_posZ = new std::vector<double>;
  particle_stopposX = new std::vector<double>;
  particle_stopposY = new std::vector<double>;
  particle_stopposZ = new std::vector<double>;
  particle_dirX = new std::vector<double>;
  particle_dirY = new std::vector<double>;
  particle_dirZ = new std::vector<double>;
  pmtQ = new std::vector<double>;
  pmtT = new std::vector<double>;
  lappdQ = new std::vector<double>;
  lappdT = new std::vector<double>;

  t->Branch("E_true",&particleE);
  t->Branch("PDG",&particlePDG);
  t->Branch("ParentPDG",&particleParentPDG);
  t->Branch("Flag",&particleFlag);
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
  t->Branch("PDGPrimaries",&particlePDG_primaries);
  t->Branch("PDGSecondaries",&particlePDG_secondaries);
  t->Branch("NumPrimaries",&num_primaries);
  t->Branch("NumSecondaries",&num_secondaries);
  t->Branch("PMTQ",&pmtQ);
  t->Branch("PMTT",&pmtT);
  t->Branch("PMTQtotal",&pmtQtotal);
  t->Branch("LAPPDQ",&lappdQ);
  t->Branch("LAPPDT",&lappdT);
  t->Branch("LAPPDQtotal",&lappdQtotal);
  t->Branch("PMTHits",&pmtHits);
  t->Branch("LAPPDHits",&lappdHits);
  t->Branch("MRDPaddles",&mrdPaddles);
  t->Branch("MRDLayers",&mrdLayers);
  t->Branch("MRDClusters",&mrdClusters);
  t->Branch("Prompt",&is_prompt);
  t->Branch("TriggerTime",&trigger_time);
  t->Branch("NRings",&nrings);
  t->Branch("NoPiK",&no_pik);
  t->Branch("MRDStop",&mrd_stop);
  t->Branch("FV",&event_fv);
  t->Branch("PMTVol",&event_pmtvol);

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geom);

  Position detector_center=geom->GetTankCentre();
  tank_center_x = detector_center.X();
  tank_center_y = detector_center.Y();
  tank_center_z = detector_center.Z();

  return true;
}


bool MCPropertiesToTree::Execute(){

  int annieeventexists = m_data->Stores.count("ANNIEEvent");
  if(!annieeventexists){ cerr<<"no ANNIEEvent store!"<<endl;}

  m_data->Stores["ANNIEEvent"]->Get("MCParticles",mcparticles);
  m_data->Stores["ANNIEEvent"]->Get("TriggerData",TriggerData);  
  m_data->Stores["ANNIEEvent"]->Get("MCTriggernum",MCTriggernum);
  m_data->Stores["ANNIEEvent"]->Get("EventNumber", evnum);
  m_data->Stores["ANNIEEvent"]->Get("MCHits", MCHits);
  m_data->Stores["ANNIEEvent"]->Get("MCLAPPDHits",MCLAPPDHits);
  m_data->Stores["ANNIEEvent"]->Get("TDCData",TDCData);  // a std::map<ChannelKey,vector<TDCHit>>
  m_data->Stores["RecoEvent"]->Get("NRings",nrings);	// need to execute MCRecoEventLoader before this tool to load the relevant information into the store
  m_data->Stores["RecoEvent"]->Get("MRDStop",mrd_stop);	// need to execute EventSelector tool before this tool to load the relevant information
  m_data->Stores["RecoEvent"]->Get("EventFV",event_fv);
  std::cout <<"MCPropertiesToTree got EventFV = "<<event_fv<<" from RecoEvent store"<<std::endl;
  m_data->Stores["RecoEvent"]->Get("NoPiK",no_pik);
  m_data->Stores["RecoEvent"]->Get("EventPMTVol",event_pmtvol);
  std::cout <<"MCPropertiesToTree got EventPMTVol = "<<event_pmtvol<<" from RecoEvent store"<<std::endl;
  m_data->Stores.at("ANNIEEvent")->Get("NumMrdTimeClusters",mrdClusters);   // need to execute time clustering tool before accessing this variable

  particleTriggers = TriggerData->size();   	//there will be always 1 trigger entry for 1 WCSim event
  for (unsigned int i_trigger = 0; i_trigger< TriggerData->size(); i_trigger++){
    TriggerTime = TriggerData->at(i_trigger).GetTime();
    if (verbosity){
	std::cout <<"particleTriggers: "<<particleTriggers<<", ";
  	std::cout <<"MCTriggerNum: "<<MCTriggernum<<", ";
  	std::cout <<"EventNr: "<<evnum<<", ";
	std::cout <<"TriggerTime: "<<TriggerTime.GetNs()<<std::endl;
    }
    trigger_time = TriggerTime.GetNs();
  }

  std::cout <<"trigger_time: "<<trigger_time<<std::endl;
  std::cout <<"EventFV: "<<event_fv<<", EventPMTVol: "<<event_pmtvol<<", MRDStop: "<<mrd_stop<<std::endl;

  if (MCTriggernum==0) is_prompt = 1;
  else is_prompt = 0;

  num_primaries = 0;
  num_secondaries = 0;

  particleE->clear();
  particlePDG->clear();
  particleParentPDG->clear();
  particleFlag->clear();
  particlePDG_primaries->clear();
  particlePDG_secondaries->clear();
  particle_posX->clear();
  particle_posY->clear();
  particle_posZ->clear();
  particle_stopposX->clear();
  particle_stopposY->clear();
  particle_stopposZ->clear();
  particle_dirX->clear();
  particle_dirY->clear();
  particle_dirZ->clear();
  pmtQ->clear();
  pmtT->clear();
  lappdQ->clear();
  lappdT->clear();

  bool fill_tree = false;
  for (unsigned int i_particle = 0; i_particle < mcparticles->size(); i_particle++){

  	MCParticle aparticle = mcparticles->at(i_particle);
  	double particle_energy = aparticle.GetStopEnergy();
  	int particle_pdg = aparticle.GetPdgCode();
  	int particle_parentpdg = aparticle.GetParentPdg();
  	int particle_flag = aparticle.GetFlag();

    if (particle_parentpdg == 0) {
      num_primaries++;
      particlePDG_primaries->push_back(particle_pdg);
    }
    else {
      num_secondaries++;
      particlePDG_secondaries->push_back(particle_pdg);
    } 

  	if (particle_parentpdg!=0) continue;
  	if (particle_flag!=0) continue;
  	//if (particle_pdg!=11 && particle_pdg!=13) continue;

  	fill_tree=true;
  	
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
    	hNumPrimaries->Fill(num_primaries);
   	hNumSecondaries->Fill(num_secondaries);
    	for (unsigned int i_primary = 0; i_primary < particlePDG_primaries->size(); i_primary++){
     	 	hPDGPrimaries->Fill(particlePDG_primaries->at(i_primary));
    	}
    	for (unsigned int i_secondary = 0; i_secondary < particlePDG_secondaries->size(); i_secondary++){
      		hPDGSecondaries->Fill(particlePDG_secondaries->at(i_secondary));
    	}
	hRings->Fill(nrings);
	hNoPiK->Fill(no_pik);
	hMRDStop->Fill(mrd_stop);
	hFV->Fill(event_fv);
	hPMTVol->Fill(event_pmtvol);
	hMRDClusters->Fill(mrdClusters);
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

  if (fill_tree) {


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
        pmtT->push_back(t);
        pmtQ->push_back(q);
        hQ->Fill(q);
        hT->Fill(t);
        pmtHits++;
	pmtQtotal+=q;
      }
    }
  }
  hPMTHits->Fill(pmtHits);
  hQtotal->Fill(pmtQtotal);

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
        lappdQ->push_back(1.0);
        lappdT->push_back(ahit.GetTime());
        hT_LAPPD->Fill(ahit.GetTime());
        hQ_LAPPD->Fill(1.0);
	lappdQtotal++;
      }
      lappdHits++;
    }
  } else {
    Log("MCPropertiesToTree tool: No MCLAPPDHits!", v_warning, verbosity);
    num_lappds_hit = 0;
  }
  hLAPPDHits->Fill(lappdHits);
  hQtotal_LAPPD->Fill(lappdQtotal);



    if(!TDCData){
        std::cout<<"MCPropertiesToTree tool: No TDC data to process!"<<std::endl;
    } else {
        if(TDCData->size()==0){
          Log("MCPropertiesToTree tool: No TDC hits.",v_message,verbosity);
          hMRDPaddles->Fill(0);
          hMRDLayers->Fill(0);
          mrdPaddles = 0;
          mrdLayers = 0;
        } else {
          mrdPaddles=0;
          mrdLayers=0;
          bool layer_occupied[11] = {0};
          for(auto&& anmrdpmt : (*TDCData)){
              unsigned long chankey = anmrdpmt.first;
              Detector *thedetector = geom->ChannelToDetector(chankey);
              if(thedetector->GetDetectorElement()!="MRD") {
                    continue;                 // this is a veto hit, not an MRD hit.
              }
              mrdPaddles++;
              int detkey = thedetector->GetDetectorID();
              Paddle *apaddle = geom->GetDetectorPaddle(detkey);
              int layer = apaddle->GetLayer();
              layer_occupied[layer]=true;
          }
          hMRDPaddles->Fill(mrdPaddles);
          
          if (mrdPaddles > 0) {
            for (int i_layer=0;i_layer<11;i_layer++){
              if (layer_occupied[i_layer]==true) {
                mrdLayers++;
              }
            }
          }     
        }
      }
    hMRDLayers->Fill(mrdLayers);


    t->Fill();
  }

  return true;
}


bool MCPropertiesToTree::Finalise(){

  t->Write("",TObject::kOverwrite);
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

  f->Close();

  delete f;
  
  return true;
}
