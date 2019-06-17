#include "MCRecoEventLoader.h"

MCRecoEventLoader::MCRecoEventLoader():Tool(){}


bool MCRecoEventLoader::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(verbosity) cout<<"Initializing Tool MCRecoEventLoader"<<endl;
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  
  ///////////////////// Defaults for Config ///////////////
  fGetPiKInfo = 1;
  fParticleID = 13;
  xshift = 0.;
  yshift = 14.46469;
  zshift = -168.1;

  /// Get the Tool configuration variables
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("GetPionKaonInfo", fGetPiKInfo);
  m_variables.Get("ParticleID", fParticleID);
  m_variables.Get("xshift", xshift);
  m_variables.Get("yshift", yshift);
  m_variables.Get("zshift", zshift);

  /// Construct the other objects we'll be setting at event level,
  fMuonStartVertex = new RecoVertex();
  fMuonStopVertex = new RecoVertex();

  // Make the RecoDigit Store if it doesn't exist
  int recoeventexists = m_data->Stores.count("RecoEvent");
  if(recoeventexists==0) m_data->Stores["RecoEvent"] = new BoostStore(false,2);
  

  return true;
}


bool MCRecoEventLoader::Execute(){
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
    Log("MCRecoEventLoader:: Tool: Error retrieving MCParticles from ANNIEEvent!",
            v_error,verbosity);
    return false;
  }


  ///Get MC Particle information
  this->FindTrueVertexFromMC();
  if (fGetPiKInfo) this->FindPionKaonCountFromMC();

  this->PushTrueVertex(true);
  this->PushTrueStopVertex(true);
  this->PushTrueMuonEnergy(TrueMuonEnergy);
  this->PushTrueWaterTrackLength(WaterTrackLength);
  this->PushTrueMRDTrackLength(MRDTrackLength);

  return true;
}


bool MCRecoEventLoader::Finalise(){
  delete fMuonStartVertex;
  delete fMuonStopVertex;
  if(verbosity>0) cout<<"MCRecoEventLoader exitting"<<endl;
  return true;
}

void MCRecoEventLoader::Reset() {
  // Reset 
  fMuonStartVertex->Reset();
  fMuonStopVertex->Reset();
  TrueMuonEnergy = -9999.;
  WaterTrackLength = -9999.;
  MRDTrackLength = -9999.;
  
}

void MCRecoEventLoader::FindTrueVertexFromMC() {
  
  // loop over the MCParticles to find the highest enery primary muon
  // MCParticles is a std::vector<MCParticle>
  MCParticle primarymuon;  // primary muon
  bool mufound=false;
  if(fMCParticles){
    Log("MCRecoEventLoader::  Tool: Num MCParticles = "+to_string(fMCParticles->size()),v_message,verbosity);
    for(int particlei=0; particlei<fMCParticles->size(); particlei++){
      MCParticle aparticle = fMCParticles->at(particlei);
      //if(v_debug<verbosity) aparticle.Print();       // print if we're being *really* verbose
      if(aparticle.GetParentPdg()!=0) continue;      // not a primary particle
      if(aparticle.GetPdgCode()!=fParticleID) continue;       // not a muon
      primarymuon = aparticle;                       // note the particle
      mufound=true;                                  // note that we found it
      break;                                         // won't have more than one primary muon
    }
  } else {
    Log("MCRecoEventLoader::  Tool: No MCParticles in the event!",v_error,verbosity);
  }
  if(not mufound){
    Log("MCRecoEventLoader::  Tool: No muon in this event",v_warning,verbosity);
    return;
  }
  
  // retrieve desired information from the particle
  Position muonstartpos = primarymuon.GetStartVertex();    // only true if the muon is primary
  double muonstarttime = primarymuon.GetStartTime();
  Position muonstoppos = primarymuon.GetStopVertex();    // only true if the muon is primary
  double muonstoptime = primarymuon.GetStopTime();
  Direction muondirection = primarymuon.GetStartDirection();
  
  TrueMuonEnergy = primarymuon.GetStartEnergy();

  // MCParticleProperties tool fills in MRD track in m, but
  // Water track in cm...
  MRDTrackLength = primarymuon.GetTrackLengthInMrd()*100.;
  WaterTrackLength = primarymuon.GetTrackLengthInTank();

  // set true vertex
  // change unit
  muonstartpos.UnitToCentimeter(); // convert unit from meter to centimeter
  muonstoppos.UnitToCentimeter(); // convert unit from meter to centimeter
  // change coordinate for muon start vertex
  muonstartpos.SetY(muonstartpos.Y()+yshift);
  muonstartpos.SetZ(muonstartpos.Z()+zshift);
  fMuonStartVertex->SetVertex(muonstartpos, muonstarttime);
  fMuonStartVertex->SetDirection(muondirection);
  //  charge coordinate for muon stop vertex
  muonstoppos.SetY(muonstoppos.Y()+yshift);
  muonstoppos.SetZ(muonstoppos.Z()+zshift);
  fMuonStopVertex->SetVertex(muonstoppos, muonstoptime); 
  
  logmessage = "  trueVtx = (" +to_string(muonstartpos.X()) + ", " + to_string(muonstartpos.Y()) + ", " + to_string(muonstartpos.Z()) +", "+to_string(muonstarttime)+ "\n"
            + "           " +to_string(muondirection.X()) + ", " + to_string(muondirection.Y()) + ", " + to_string(muondirection.Z()) + ") " + "\n";
  
  Log(logmessage,v_debug,verbosity);
	logmessage = "  muonStop = ("+to_string(muonstoppos.X()) + ", " + to_string(muonstoppos.Y()) + ", " + to_string(muonstoppos.Z()) + ") "+ "\n";
	Log(logmessage,v_debug,verbosity);
}


void MCRecoEventLoader::FindPionKaonCountFromMC() {
  
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
  if(fMCParticles){
    Log("MCRecoEventLoader::  Tool: Num MCParticles = "+to_string(fMCParticles->size()),v_message,verbosity);
    for(int particlei=0; particlei<fMCParticles->size(); particlei++){
      MCParticle aparticle = fMCParticles->at(particlei);
      //if(v_debug<verbosity) aparticle.Print();       // print if we're being *really* verbose
      if(aparticle.GetParentPdg()!=0) continue;      // not a primary particle
      if(aparticle.GetPdgCode()==111){               // is a primary pi0
        pionfound = true;
        pi0count++;
      }
      if(aparticle.GetPdgCode()==211){               // is a primary pi+
        pionfound = true;
        pipcount++;
      }
      if(aparticle.GetPdgCode()==-211){               // is a primary pi-
        pionfound = true;
        pimcount++;
      }
      if(aparticle.GetParentPdg()!=0) continue;      // not a primary particle
      if(aparticle.GetPdgCode()==311){               // is a primary K0
        kaonfound = true;
        K0count++;
      }
      if(aparticle.GetPdgCode()==321){               // is a primary K+
        kaonfound = true;
        Kpcount++;
      }
      if(aparticle.GetPdgCode()==-321){               // is a primary K-
        kaonfound = true;
        Kmcount++;
      }
    }
  } else {
    Log("MCRecoEventLoader::  Tool: No MCParticles in the event!",v_error,verbosity);
  }
  if(not pionfound){
    Log("MCRecoEventLoader::  Tool: No primary pions in this event",v_warning,verbosity);
  }
  if(not kaonfound){
    Log("MCRecoEventLoader::  Tool: No kaons in this event",v_warning,verbosity);
  }
  //Fill in pion counts for this event
  m_data->Stores.at("RecoEvent")->Set("MCPi0Count", pi0count);
  m_data->Stores.at("RecoEvent")->Set("MCPiPlusCount", pipcount);
  m_data->Stores.at("RecoEvent")->Set("MCPiMinusCount", pimcount);
  m_data->Stores.at("RecoEvent")->Set("MCK0Count", K0count);
  m_data->Stores.at("RecoEvent")->Set("MCKPlusCount", Kpcount);
  m_data->Stores.at("RecoEvent")->Set("MCKMinusCount", Kmcount);
}


void MCRecoEventLoader::PushTrueVertex(bool savetodisk) {
  Log("MCRecoEventLoader Tool: Push true vertex to the RecoEvent store",v_message,verbosity);
  m_data->Stores.at("RecoEvent")->Set("TrueVertex", fMuonStartVertex, savetodisk); 
}


void MCRecoEventLoader::PushTrueStopVertex(bool savetodisk) {
  Log("MCRecoEventLoader Tool: Push true stop vertex to the RecoEvent store",v_message,verbosity);
  m_data->Stores.at("RecoEvent")->Set("TrueStopVertex", fMuonStopVertex, savetodisk); 
}

void MCRecoEventLoader::PushTrueMuonEnergy(double MuE) {
	Log("MCRecoEventLoader Tool: Push true muon energy to the RecoEvent store",v_message,verbosity);
	m_data->Stores.at("RecoEvent")->Set("TrueMuonEnergy", MuE);  ///> Add digits to RecoEvent
}

void MCRecoEventLoader::PushTrueWaterTrackLength(double WaterT) {
	Log("MCRecoEventLoader Tool: Push true track length in tank to the RecoEvent store",v_message,verbosity);
	m_data->Stores.at("RecoEvent")->Set("TrueTrackLengthInWater", WaterT);  ///> Add digits to RecoEvent
}

void MCRecoEventLoader::PushTrueMRDTrackLength(double MRDT) {
	Log("MCRecoEventLoader Tool: Push true track length in MRD to the RecoEvent store",v_message,verbosity);
	m_data->Stores.at("RecoEvent")->Set("TrueTrackLengthInMRD", MRDT);  ///> Add digits to RecoEvent
}



