#include "CCMCRecoEventLoader.h"

CCMCRecoEventLoader::CCMCRecoEventLoader():Tool(){}


bool CCMCRecoEventLoader::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(verbosity) cout<<"Initializing Tool CCMCRecoEventLoader"<<endl;
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  
  ///////////////////// Defaults for Config ///////////////
  fGetPiKInfo = 1;
  fGetNRings = 1;
  fParticleID = 13;
  fFollowerCheck = 0;
  xshift = 0.;
  yshift = 14.46469;
  zshift = -168.1;

  /// Get the Tool configuration variables
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("GetPionKaonInfo", fGetPiKInfo);
  m_variables.Get("GetNRings",fGetNRings);
  m_variables.Get("ParticleID", fParticleID);
  m_variables.Get("FollowerCheck", fFollowerCheck);
  m_variables.Get("xshift", xshift);
  m_variables.Get("yshift", yshift);
  m_variables.Get("zshift", zshift);

  /// Construct the other objects we'll be setting at event level,
  fMuonStartVertex = new RecoVertex();
  fMuonStopVertex = new RecoVertex();

  // Make the RecoDigit Store if it doesn't exist
  int recoeventexists = m_data->Stores.count("RecoEvent");
  if(recoeventexists==0) m_data->Stores["RecoEvent"] = new BoostStore(false,2);

  // Get particle masses map from CStore (populated by MCParticleProperties tool)
  m_data->CStore.Get("PdgMassMap",pdgcodetomass);

  std::map<int,double>::iterator it_map;
  for (it_map = pdgcodetomass.begin(); it_map != pdgcodetomass.end(); it_map++){
    int pdgcode = it_map->first;
    int pdgmass = it_map->second;
    double cherenkov_thr = GetCherenkovThresholdE(pdgcode);
    pdgcodetocherenkov.emplace(pdgcode,cherenkov_thr);
  }
  
  //std::cout <<"PdgCherenkovMap size (CCMCRecoEventLoader): "<<pdgcodetocherenkov.size()<<std::endl;
  // Set particle pdg - Cherenkov threshold map to CStore
  m_data->CStore.Set("PdgCherenkovMap",pdgcodetocherenkov);

  return true;
}
bool CCMCRecoEventLoader::Execute(){

  /// Reset everything
  this->Reset();

  // see if "ANNIEEvent" exists
  auto get_annieevent = m_data->Stores.count("ANNIEEvent");
  if(!get_annieevent){
    Log("DigitBuilder Tool: No ANNIEEvent store!",v_error,verbosity); 
    return false;
  };

  // Load MC Particles information for this event
  auto get_mcparticles = m_data->Stores.at("ANNIEEvent")->Get("MCParticles",
          fMCParticles);
  if(!get_mcparticles){
    Log("CCMCRecoEventLoader:: Tool: Error retrieving MCParticles from ANNIEEvent!",
            v_error,verbosity);
    return false;
  }

  ///Get MC Particle information
  this->FindTrueVertexFromMC();
  if (fGetPiKInfo) this->FindPionKaonCountFromMC();
  if (fFollowerCheck) this->FindFollowersFromMC();
  
  this->PushIBDInfo();

  return true;
}


bool CCMCRecoEventLoader::Finalise(){
  delete fMuonStartVertex;
  delete fMuonStopVertex;
  if(verbosity>0) cout<<"CCMCRecoEventLoader exitting"<<endl;
  return true;
}

void CCMCRecoEventLoader::Reset() {
  // Reset 
  fMuonStartVertex->Reset();
  fMuonStopVertex->Reset();
  TrueMuonEnergy = -9999.;
  WaterTrackLength = -9999.;
  MRDTrackLength = -9999.;
  projectedmrdhit = false;  
}

void CCMCRecoEventLoader::FindTrueVertexFromMC() {
  
  // loop over the MCParticles to find the highest enery primary muon
  // MCParticles is a std::vector<MCParticle>
  MCParticle primarylepton;  // primary lepton
  bool mufound=false;
  if(fMCParticles){
    Log("CCMCRecoEventLoader::  Tool: Num MCParticles = "+to_string(fMCParticles->size()),v_message,verbosity);
    for(unsigned int particlei=0; particlei<fMCParticles->size(); particlei++){
      MCParticle aparticle = fMCParticles->at(particlei);
      if(aparticle.GetFlag()!=0) continue;             //excludes target 
      //if(v_debug<verbosity) aparticle.Print();       // print if we're being *really* verbose
      if(aparticle.GetParentPdg()!=0) continue;      // not a primary particle
      if(aparticle.GetPdgCode()==1000080160) continue;     // skip target nuclei
      //Accept both electrons and muons as primary particles, if no selection is specified
      if( fabs(aparticle.GetPdgCode())!=11 && fabs(aparticle.GetPdgCode())!=13 && fabs(aparticle.GetPdgCode())!=12 && fabs(aparticle.GetPdgCode())!=14) continue;
      primarylepton = aparticle;
      if( fabs(aparticle.GetPdgCode())==13) mufound=true;
      m_data->Stores.at("RecoEvent")->Set("PdgPrimary",aparticle.GetPdgCode());
      break;
    }
  } else {
    Log("CCMCRecoEventLoader::  Tool: No MCParticles in the event!",v_error,verbosity);
  }
  
  // retrieve desired information from the particle
  Position muonstartpos = primarylepton.GetStartVertex();    // only true if the muon is primary
  double muonstarttime = primarylepton.GetStartTime();
  Position muonstoppos = primarylepton.GetStopVertex();    // only true if the muon is primary
  double muonstoptime = primarylepton.GetStopTime();
  Direction muondirection = primarylepton.GetStartDirection();

  if(mufound){ 
    TrueMuonEnergy = primarylepton.GetStartEnergy();
    // MCParticleProperties tool fills in MRD track in m, but
    // Water track in cm...
    MRDTrackLength = primarylepton.GetTrackLengthInMrd()*100.;
    WaterTrackLength = primarylepton.GetTrackLengthInTank();
  }

  m_data->Stores.at("RecoEvent")->Set("TrueMuonEnergy", TrueMuonEnergy);  ///> Add digits to RecoEvent
  m_data->Stores.at("RecoEvent")->Set("TrueTrackLengthInWater", WaterTrackLength);  ///> Add digits to RecoEvent
  m_data->Stores.at("RecoEvent")->Set("TrueTrackLengthInMRD", MRDTrackLength);  ///> Add digits to RecoEvent

  //std::cout <<"CCMCRecoEventLoader: Muon start position: ("<<muonstartpos.X()<<","<<muonstartpos.Y()<<","<<muonstartpos.Z()<<")"<<std::endl;
  // set true vertex
  // change unit
  muonstartpos.UnitToCentimeter(); // convert unit from meter to centimeter
  muonstoppos.UnitToCentimeter(); // convert unit from meter to centimeter
  // change coordinate for muon start vertex
  muonstartpos.SetY(muonstartpos.Y()+yshift);
  muonstartpos.SetZ(muonstartpos.Z()+zshift);
  //std::cout <<"CCMCRecoEventLoader: NEW Muon start position: ("<<muonstartpos.X()<<","<<muonstartpos.Y()<<","<<muonstartpos.Z()<<")"<<std::endl;
  fMuonStartVertex->SetVertex(muonstartpos, muonstarttime);
  fMuonStartVertex->SetDirection(muondirection);
  //  charge coordinate for muon stop vertex
  muonstoppos.SetY(muonstoppos.Y()+yshift);
  muonstoppos.SetZ(muonstoppos.Z()+zshift);
  fMuonStopVertex->SetVertex(muonstoppos, muonstoptime); 

  Log("MCRecoEventLoader Tool: Push true vertex to the RecoEvent store",v_message,verbosity);
  m_data->Stores.at("RecoEvent")->Set("TrueVertex", fMuonStartVertex, true); 

  Log("MCRecoEventLoader Tool: Push true stop vertex to the RecoEvent store",v_message,verbosity);
  m_data->Stores.at("RecoEvent")->Set("TrueStopVertex", fMuonStopVertex, true); 
  
  logmessage = "  trueVtx = (" +to_string(muonstartpos.X()) + ", " + to_string(muonstartpos.Y()) + ", " + to_string(muonstartpos.Z()) +", "+to_string(muonstarttime)+ "\n"
            + "           " +to_string(muondirection.X()) + ", " + to_string(muondirection.Y()) + ", " + to_string(muondirection.Z()) + ") " + "\n";
  
  Log(logmessage,v_debug,verbosity);

  //get information whether the extended particle trajectory were to hit the MRD
  projectedmrdhit = primarylepton.GetProjectedHitMrd();
  m_data->Stores.at("RecoEvent")->Set("ProjectedMRDHit", projectedmrdhit);  ///> Add digits to RecoEvent 
 
}

void CCMCRecoEventLoader::FindPionKaonCountFromMC() {

  Log("CCMCRecoEventLoader: Find PionKaonCountFromMC",v_message,verbosity);
  
  // loop over the MCParticles to find the highest enery primary muon
  // MCParticles is a std::vector<MCParticle>
  bool pionfound=false;
  bool kaonfound=false;
  int pi0count = 0;
  int pipcount = 0;
  int pimcount = 0;
  int K0count = 0;
  int Kpcount = 0;
  int Kmcount = 0;

  //set up number of rings to 0 before counting
  int nprimary = 0;
  int nsecondary = 0;
  int nrings = 0;
  std::vector<unsigned int> index_particles_ring;
  std::vector<int> primary_pdgs;
  std::vector<double> tank_tracks;
  std::vector<double> mrd_tracks;
  std::vector<bool> contained_tracks;
  std::vector<double> mrd_angle;
  std::vector<double> energies;
  std::vector<double> start_t;
  std::vector<double> stop_t;
  std::vector<double> dirx;
  std::vector<double> diry;
  std::vector<double> dirz;

  if(fMCParticles){
    Log("CCMCRecoEventLoader::  Tool: Num MCParticles = "+to_string(fMCParticles->size()),v_message,verbosity);
    for(unsigned int particlei=0; particlei<fMCParticles->size(); particlei++){
      MCParticle aparticle = fMCParticles->at(particlei);
      if(aparticle.GetFlag()!=0) continue;             //excludes target 
      //if(v_debug<verbosity) aparticle.Print();       // print if we're being *really* verbose
      if(aparticle.GetParentPdg()==0) {                //primary particle
        nprimary++;
        //General track/particle info
        primary_pdgs.push_back( aparticle.GetPdgCode() );
	tank_tracks.push_back( aparticle.GetTrackLengthInTank() );
        mrd_tracks.push_back( aparticle.GetTrackLengthInMrd()*100. );
        energies.push_back( aparticle.GetStartEnergy() );
        start_t.push_back( aparticle.GetStartTime() );
        stop_t.push_back( aparticle.GetStopTime() );
        dirx.push_back( aparticle.GetStartDirection().X() );
        diry.push_back( aparticle.GetStartDirection().Y() );
        dirz.push_back( aparticle.GetStartDirection().Z() );
        if(!aparticle.GetExitsTank()){
          contained_tracks.push_back(true);
          mrd_angle.push_back(-9999);
        } else if(aparticle.GetEntersMrd()){
            Position end_point = aparticle.GetStopVertex();
            Position start_point = aparticle.GetMrdEntryPoint();
            Position mrd_point = aparticle.GetMrdExitPoint();
            double x_length = end_point.X() - start_point.X();
            double y_length = end_point.Y() - start_point.Y();
            double z_length = end_point.Z() - start_point.Z();
            double hyp = sqrt(x_length*x_length + y_length*y_length + z_length*z_length);
            mrd_angle.push_back(acos(z_length / hyp));
            if(!aparticle.GetExitsMrd()){
              contained_tracks.push_back(true);
            } else {
              contained_tracks.push_back(false);
            }
        } else {
          contained_tracks.push_back(false);
          mrd_angle.push_back(-9999);
        }

        //PDG counting
        if (TMath::Abs(aparticle.GetPdgCode())==11){
          if (aparticle.GetStartEnergy() > GetCherenkovThresholdE(11)) {nrings++; index_particles_ring.push_back(particlei);}
        } 
        if (TMath::Abs(aparticle.GetPdgCode())==13){
          if (aparticle.GetStartEnergy() > GetCherenkovThresholdE(13)) {nrings++; index_particles_ring.push_back(particlei);}
        }
        if(aparticle.GetPdgCode()==111){               // is a primary pi0
          pionfound = true;
          pi0count++;
          nrings+=2; 
          index_particles_ring.push_back(particlei);}
        if(aparticle.GetPdgCode()==211){               // is a primary pi+
          pionfound = true;
          pipcount++;
          if (aparticle.GetStartEnergy() > GetCherenkovThresholdE(211)) {nrings++; index_particles_ring.push_back(particlei);}
        }
        if(aparticle.GetPdgCode()==-211){               // is a primary pi-
          pionfound = true;
          pimcount++;
          if (aparticle.GetStartEnergy() > GetCherenkovThresholdE(-211)) {nrings++; index_particles_ring.push_back(particlei);}
        }
        if(aparticle.GetPdgCode()==311){               // is a primary K0
          kaonfound = true;
          K0count++;
        }
        if(aparticle.GetPdgCode()==321){               // is a primary K+
          kaonfound = true;
          Kpcount++;
          if (aparticle.GetStartEnergy() > GetCherenkovThresholdE(321)) {nrings++; index_particles_ring.push_back(particlei);}
        }
        if(aparticle.GetPdgCode()==-321){               // is a primary K-
          kaonfound = true;
          Kmcount++;
          if (aparticle.GetStartEnergy() > GetCherenkovThresholdE(-321)) {nrings++; index_particles_ring.push_back(particlei);}
        }
      } else {                                          // not a primary particle
        nsecondary++;
        Log("CCMCRecoEventLoader: Secondary particle with pdg "+std::to_string(aparticle.GetPdgCode()),v_debug,verbosity);
 	//don't count rings from secondary particles for now (should we?)
      }
    }
  } else {
    Log("CCMCRecoEventLoader::  Tool: No MCParticles in the event!",v_error,verbosity);
  }
  if(not pionfound){
    Log("CCMCRecoEventLoader::  Tool: No primary pions in this event",v_warning,verbosity);
  }
  if(not kaonfound){
    Log("CCMCRecoEventLoader::  Tool: No kaons in this event",v_warning,verbosity);
  }
  if (fGetNRings){
    Log("CCMCRecoEventLoader: Found "+std::to_string(nrings)+" rings in this event, from "+std::to_string(nprimary)+" primary particles and "+std::to_string(nsecondary)+" secondary particles.",2,verbosity);
  }
  //Fill in pion counts for this event
  m_data->Stores.at("RecoEvent")->Set("MCPi0Count", pi0count);
  m_data->Stores.at("RecoEvent")->Set("MCPiPlusCount", pipcount);
  m_data->Stores.at("RecoEvent")->Set("MCPiMinusCount", pimcount);
  m_data->Stores.at("RecoEvent")->Set("MCK0Count", K0count);
  m_data->Stores.at("RecoEvent")->Set("MCKPlusCount", Kpcount);
  m_data->Stores.at("RecoEvent")->Set("MCKMinusCount", Kmcount);
  m_data->Stores.at("RecoEvent")->Set("PrimaryPdgs",primary_pdgs);
  m_data->Stores.at("RecoEvent")->Set("FSPTankTrackLengths",tank_tracks);
  m_data->Stores.at("RecoEvent")->Set("FSPMrdTrackLengths",mrd_tracks);
  m_data->Stores.at("RecoEvent")->Set("FSPContained",contained_tracks);
  m_data->Stores.at("RecoEvent")->Set("FSPMrdAngles",mrd_angle);
  m_data->Stores.at("RecoEvent")->Set("FSPEnergies",energies);
  m_data->Stores.at("RecoEvent")->Set("FSPStartT",start_t);
  m_data->Stores.at("RecoEvent")->Set("FSPStopT",stop_t);
  m_data->Stores.at("RecoEvent")->Set("FSPDirX",dirx);
  m_data->Stores.at("RecoEvent")->Set("FSPDirY",diry);
  m_data->Stores.at("RecoEvent")->Set("FSPDirZ",dirz);
  if (fGetNRings) {
    m_data->Stores.at("RecoEvent")->Set("NRings",nrings);
    m_data->Stores.at("RecoEvent")->Set("IndexParticlesRing",index_particles_ring);
  }

}

double CCMCRecoEventLoader::GetCherenkovThresholdE(int pdg_code) {
  Log("CCMCRecoEventLoader Tool: GetCherenkovThresholdE",v_message,verbosity);            ///> Calculate Cherenkov threshold energies depending on particle pdg
  double Ethr = pdgcodetomass[pdg_code]*sqrt(1/(1-1/(n*n)));
  return Ethr;
}

void CCMCRecoEventLoader::PushIBDInfo(){

  Log("CCMCRecoEventLoader Tool: PushIBDInfo",v_message,verbosity);

  int n_neutrons = 0;
  int n_gammas = 0;
  int n_positrons = 0;

  if(fMCParticles){
    Log("CCMCRecoEventLoader::  Tool: Num MCParticles = "+to_string(fMCParticles->size()),v_message,verbosity);
    for(unsigned int particlei=0; particlei<fMCParticles->size(); particlei++){
      MCParticle aparticle = fMCParticles->at(particlei);
      if(aparticle.GetParentPdg()==0) {                //primary particle
        int pdg = aparticle.GetPdgCode();
        double energy = aparticle.GetStartEnergy();
        if (pdg == 2112) n_neutrons++;
        if (pdg == -11 && energy < 100) n_positrons++;
        if (pdg == 22 && energy < 100) n_gammas++;
      }
    }
  }

  m_data->Stores.at("RecoEvent")->Set("NeutronCount",n_neutrons);
  m_data->Stores.at("RecoEvent")->Set("PositronCount",n_positrons);
  m_data->Stores.at("RecoEvent")->Set("GammaCount",n_gammas); 
  
}

void MCRecoEventLoader::FindFollowersFromMC(){
  vector<double> followerE;
  vector<double> followerX;
  vector<double> followerY;
  vector<double> followerZ;
  vector<double> followerStartT;
  vector<double> followerStopT;
  vector<int> followerPDG;
  vector<int> followerParentPDG;

  if(fMCParticles){
    Log("MCRecoEventLoader::  Tool: Num MCParticles = "+to_string(fMCParticles->size()),v_message,verbosity);
    for(unsigned int particlei=0; particlei<fMCParticles->size(); particlei++){
      MCParticle aparticle = fMCParticles->at(particlei);
      //if(v_debug<verbosity) aparticle.Print();       // print if we're being *really* verbose
      if(aparticle.GetParentPdg()==0) continue;     //not primary particle
      if(TMath::Abs(aparticle.GetPdgCode()) == 11 || TMath::Abs(aparticle.GetPdgCode()) == 13 || (aparticle.GetPdgCode() == 22 && aparticle.GetParentPdg() == 111)){
        followerE.push_back(aparticle.GetStartEnergy());
        followerPDG.push_back(aparticle.GetPdgCode());
        followerParentPDG.push_back(aparticle.GetParentPdg());
        followerStartT.push_back(aparticle.GetStartTime());
        followerStopT.push_back(aparticle.GetStopTime());
        followerX.push_back(aparticle.GetStartDirection().X());
        followerY.push_back(aparticle.GetStartDirection().Y());
        followerZ.push_back(aparticle.GetStartDirection().Z());
      }
    }
  } 

  m_data->Stores.at("RecoEvent")->Set("FollowerE",followerE);
  m_data->Stores.at("RecoEvent")->Set("FollowerPDG",followerPDG);
  m_data->Stores.at("RecoEvent")->Set("FollowerParentPDG",followerParentPDG);
  m_data->Stores.at("RecoEvent")->Set("FollowerStartT",followerStartT);
  m_data->Stores.at("RecoEvent")->Set("FollowerStopT",followerStopT);
  m_data->Stores.at("RecoEvent")->Set("FollowerDirX",followerX);
  m_data->Stores.at("RecoEvent")->Set("FollowerDirY",followerY);
  m_data->Stores.at("RecoEvent")->Set("FollowerDirZ",followerZ);
 
}

