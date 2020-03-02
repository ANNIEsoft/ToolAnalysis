#include "EnergyExtractor.h"


EnergyExtractor::EnergyExtractor():Tool(){}



duble TrueEnergy;
ofstream outfile_EnergyNeutrino, outfile_EnergyPion, outfile_EnergyKaon, outfile_EnergyMuon, outfile_EnergyElectron, outfile_NeutronNumber ,outfile_VisibleEnergy;
std::map<int,double> pdgcodetomass;
string cnn_outpath;
std::vector<MCParticle>* fMCParticles=nullptr;
std::map<unsigned long, std::vector<MCHit>>* MCHits = nullptr;
std::map<unsigned long, std::vector<MCLAPPDHit>>* MCLAPPDHits = nullptr;
Geometry *geom = nullptr;




bool EnergyExtractor::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();
  m_variables.Get("OutputFile",cnn_outpath);
  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  m_data->CStore.Get("PdgMassMap",pdgcodetomass);

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry",geom);

  std::string str_csv = ".csv";
  std::string str_EnergyNeutrino = "_EnergyNeutrino";
  std::string str_EnergyPion = "_EnergyPion";
  std::string str_EnergyKaon = "_EnergyKaon";
  std::string str_EnergyMuon = "_EnergyMuon";
  std::string str_EnergyElectron = "_EnergyElectron";
  std::string str_NeutronNumber = "_NeutronNumber";
  std::string str_VisibleEnergy = "_VisibleEnergy";

  std::string csvfile_EnergyNeutrino = cnn_outpath + str_EnergyNeutrino + str_csv;
  std::string csvfile_EnergyPion = cnn_outpath + str_EnergyPion + str_csv;
  std::string csvfile_EnergyKaon = cnn_outpath + str_EnergyKaon + str_csv;
  std::string csvfile_EnergyMuon = cnn_outpath + str_EnergyMuon + str_csv;
  std::string csvfile_EnergyElectron = cnn_outpath + str_EnergyElectron + str_csv;
  std::string csvfile_NeutronNumber = cnn_outpath + str_NeutronNumber + str_csv;
  std::string csvfile_VisibleEnergy = cnn_outpath + str_VisibleEnergy + str_csv;

  outfile_EnergyNeutrino.open(csvfile_EnergyNeutrino.c_str());
  outfile_EnergyPion.open(csvfile_EnergyPion.c_str());
  outfile_EnergyKaon.open(csvfile_EnergyKaon.c_str());
  outfile_EnergyMuon.open(csvfile_EnergyMuon.c_str());
  outfile_EnergyElectron.open(csvfile_EnergyElectron.c_str());
  outfile_NeutronNumber.open(csvfile_NeutronNumber.c_str());
  outfile_VisibleEnergy.open(csvfile_VisibleEnergy.c_str());

  return true;
}






bool EnergyExtractor::Execute()
{

  bool passed_eventselection;
  m_data->Stores["RecoEvent"]->Get("EventCutStatus",passed_eventselection);
  std::cout <<"passed_eventselection: "<<passed_eventselection<<std::endl;
  if (!passed_eventselection) {
    return true;
  }
  std::map<std::string, std::vector<double>> Energystore;
  std::std::vector<double> filler;
  Energystore.emplace("e-",filler);
  Energystore.emplace("e+",filler);
  Energystore.emplace("mu-",filler);
  Energystore.emplace("mu+",filler);
  Energystore.emplace("pi-",filler);
  Energystore.emplace("pi+",filler);
  Energystore.emplace("pi0",filler);
  Energystore.emplace("K+",filler);
  Energystore.emplace("K-",filler);
  Energystore.emplace("K0",filler);
  int pi0count = 0;
  int pipcount = 0;
  int pimcount = 0;
  int K0count = 0;
  int Kpcount = 0;
  int Kmcount = 0;
  int ecount = 0;
  int epcount = 0;
  int mcount = 0;
  int mpcount = 0;
  int ncount = 0;

  // see if "ANNIEEvent" exists
  auto get_annieevent = m_data->Stores.count("ANNIEEvent");
  if(!get_annieevent){
    Log("DigitBuilder Tool: No ANNIEEvent store!",v_error,verbosity);
    return false;
  }
  m_data->Stores["ANNIEEvent"]->Get("MCHits", MCHits);
  m_data->Stores["ANNIEEvent"]->Get("MCLAPPDHits", MCLAPPDHits);
  // Load MC Particles information for this event
  auto get_mcparticles = m_data->Stores.at("ANNIEEvent")->Get("MCParticles",
          fMCParticles);
  if(!get_mcparticles){
    Log("EnergyExtractor:: Tool: Error retrieving MCParticles from ANNIEEvent!",
            v_error,verbosity);
    return false;
  }


  int vectsize = MCHits->size();
  if (verbosity > 1) std::cout <<"Tool CNNImage: MCHits size: "<<vectsize<<std::endl;
  double total_charge_pmts=0;
  for(std::pair<unsigned long, std::vector<MCHit>>&& apair : *MCHits)
  {
    unsigned long chankey = apair.first;
    if (verbosity > 3) std::cout <<"chankey: "<<chankey<<std::endl;
    Detector* thistube = geom->ChannelToDetector(chankey);
    unsigned long detkey = thistube->GetDetectorID();
    if (verbosity > 3) std::cout <<"detkey: "<<detkey<<std::endl;
    if (thistube->GetDetectorElement()=="Tank")
    {
      if (thistube->GetTankLocation()=="OD") continue;

      std::vector<MCHit>& Hits = apair.second;

      for (MCHit &ahit : Hits)
      {
        if (ahit.GetTime()>-10. && ahit.GetTime()<40.)
        {
          total_charge_pmts += ahit.GetCharge();
        }
      }
    }
  }
  int vectsize = MCLAPPDHits->size();
  if (verbosity > 1) std::cout <<"Tool CNNImage: MCLAPPDHits size: "<<vectsize<<std::endl;
  double total_charge_lappd=0;
  for(std::pair<unsigned long, std::vector<MCLAPPDHit>>&& apair : *MCLAPPDHits)
  {
    unsigned long chankey = apair.first;
    if (verbosity > 3) std::cout <<"chankey: "<<chankey<<std::endl;

    std::vector<MCLAPPDHit>& Hits = apair.second;

    for (MCLAPPDHit &ahit : Hits)
    {
      if (ahit.GetTime()>-10. && ahit.GetTime()<40.)
      {
      total_charge_lappd += 1;
      }
    }

  }

  this->FindTrueEnergyFromMC();




  double neutrino_Energy;
  m_data->CStore.Get("NeutrinoEnergy",neutrino_Energy)
  outfile_EnergyNeutrino << neutrino_Energy << endl;



  outfile_EnergyPion << pimcount<<"," << pipcount << "," ;
  for(int i=0 ; i<Energystore["pi-"].size() ; i++)
  {
    outfile_EnergyPion << Energystore["pi-"][i] << ",";
  }
  for(int i=0 ; i<Energystore["pi+"].size() ; i++)
  {
    outfile_EnergyPion << Energystore["pi+"][i] << ",";
  }
  outfile_EnergyPion << endl;

  outfile_EnergyKaon << kmcount<<"," << kpcount << "," ;
  for(int i=0 ; i<Energystore["K-"].size() ; i++)
  {
    outfile_EnergyKaon << Energystore["K-"][i] << ",";
  }
  for(int i=0 ; i<Energystore["K+"].size() ; i++)
  {
    outfile_EnergyKaon << Energystore["K+"][i] << ",";
  }
  outfile_EnergyKaon << endl;


  outfile_EnergyMuon << mcount <<"," << mpcount << ",";
  for(int i=0 ; i<Energystore["mu-"].size() ; i++)
  {
    outfile_EnergyMuon << Energystore["mu-"][i] << ",";
  }
  for(int i=0 ; i<Energystore["mu+"].size() ; i++)
  {
    outfile_EnergyMuon << Energystore["mu+"][i] << ",";
  }
  outfile_EnergyMuon <<endl;


  outfile_EnergyElectron << ecount<<","<< epcount << ",";
  for(int i=0 ; i<Energystore["e-"].size() ; i++)
  {
    outfile_EnergyElectron << Energystore["e-"][i] << ",";
  }
  for(int i=0 ; i<Energystore["e+"].size() ; i++)
  {
    outfile_EnergyElectron << Energystore["e+"][i] << ",";
  }
  outfile_EnergyElectron << endl;


  outfile_NeutronNumber << ncount << endl;

  outfile_VisibleEnergy << total_charge_pmts << "," << total_charge_lappd <<endl;


  return true;
}





bool EnergyExtractor::Finalise(){
  outfile_EnergyNeutrino.close();
  outfile_EnergyPion.close();
  outfile_EnergyKaon.close();
  outfile_EnergyMuon.close();
  outfile_EnergyElectron.close();
  outfile_NeutronNumber.close();
  outfile_VisibleEnergy.close();



  return true;
}









void EnergyExtractor::FindTrueEnergyFromMC() {

  // loop over the MCParticles to find the highest enery primary muon
  // MCParticles is a std::vector<MCParticle>
  //MCParticle primarymuon;  // primary muon
  //bool mufound=false;
  if(fMCParticles)
  {
    Log("EnergyExtractor::  Tool: Num MCParticles = "+to_string(fMCParticles->size()),v_message,verbosity);
    for(int particlei=0; particlei<fMCParticles->size(); particlei++)
    {

      MCParticle aparticle = fMCParticles->at(particlei);

      if(aparticle.GetParentPdg()!=0) continue;      // not a primary particle

      TrueEnergy = aparticle.GetStartEnergy();


      if(aparticle.GetPdgCode()==111){               // is a primary pi0
        pionfound = true;
        pi0count++;
        if (aparticle.GetStartEnergy() > GetCherenkovThresholdE(111)) {Energystore["pi0"].push_back(TrueEnergy);}

      }
      if(aparticle.GetPdgCode()==211){               // is a primary pi+

        pipcount++;

        if (aparticle.GetStartEnergy() > GetCherenkovThresholdE(211)) {Energystore["pi+"].push_back(TrueEnergy);}
      }
      if(aparticle.GetPdgCode()==-211){               // is a primary pi-

        pimcount++;

        if (aparticle.GetStartEnergy() > GetCherenkovThresholdE(-211)) {Energystore["pi-"].push_back(TrueEnergy);}
      }
      if(aparticle.GetParentPdg()!=0) continue;

      if(aparticle.GetPdgCode()==311){               // is a primary K0
        K0count++;

        if (aparticle.GetStartEnergy() > GetCherenkovThresholdE(311)) {Energystore["K0"].push_back(TrueEnergy);}
      }
      if(aparticle.GetPdgCode()==321){               // is a primary K+
        Kpcount++;

        if (aparticle.GetStartEnergy() > GetCherenkovThresholdE(321)) {Energystore["K+"].push_back(TrueEnergy);}
      }
      if(aparticle.GetPdgCode()==-321){               // is a primary K-
        Kmcount++;

        if (aparticle.GetStartEnergy() > GetCherenkovThresholdE(-321)) {Energystore["K-"].push_back(TrueEnergy);}
      }
      if(aparticle.GetPdgCode()==11){               // is a primary e-
        ecount++;

        if (aparticle.GetStartEnergy() > GetCherenkovThresholdE(11)) {Energystore["e-"].push_back(TrueEnergy);}
      }
      if(aparticle.GetPdgCode()==-11){               // is a primary e+
        epcount++;

        if (aparticle.GetStartEnergy() > GetCherenkovThresholdE(-11)) {Energystore["e+"].push_back(TrueEnergy);}
      }
      if(aparticle.GetPdgCode()==13){               // is a primary mu-
        mcount++;

        if (aparticle.GetStartEnergy() > GetCherenkovThresholdE(13)) {Energystore["mu-"].push_back(TrueEnergy);}
      }
      if(aparticle.GetPdgCode()==-13){               // is a primary mu+
        mpcount++;
        if (aparticle.GetStartEnergy() > GetCherenkovThresholdE(-13)) {Energystore["mu+"].push_back(TrueEnergy);}
      }
      if(aparticle.GetPdgCode()==2112){               // is a primary mu+
        ncount++;
      }

    }

  } else {
    Log("EnergyExtractor::  Tool: No MCParticles in the event!",v_error,verbosity);
  }


}

double EnergyExtractor::GetCherenkovThresholdE(int pdg_code) {
  Log("EnergyExtractor Tool: GetCherenkovThresholdE",v_message,verbosity);            ///> Calculate Cherenkov threshold energies depending on particle pdg
  double Ethr = pdgcodetomass[pdg_code]*sqrt(1+1./sqrt(n*n-1));
  return Ethr;
}
