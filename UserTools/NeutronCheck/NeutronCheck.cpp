#include "NeutronCheck.h"

NeutronCheck::NeutronCheck():Tool(){}


bool NeutronCheck::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_variables.Get("verbosity",verbosity);
  m_variables.Get("outfile",outfile);
  m_variables.Get("UseClean",useClean);
  m_variables.Get("FinderCompare",fFinderCompare);
  m_variables.Get("ParticleInfo",fParticleInfo);

  outfile+=".root";

  fOutput_tfile = new TFile(outfile.c_str(), "recreate");
  NeutCheckTree = new TTree("NeutCheckTree", "Neutron Check Tree");

  NeutCheckTree->Branch("eventNumber", &fMCEventNum, "eventNumber/I");
  
  NeutCheckTree->Branch("eventCutStatus", &fEventStatusFlagged,"eventStatusFlagged/I");
  NeutCheckTree->Branch("ClusterNumber",&fClusterNum);
  NeutCheckTree->Branch("TrueEnu",&true_Enu,"true_Enu/D");
  NeutCheckTree->Branch("TrueEmu",&true_Emu,"true_Emu/D");
  NeutCheckTree->Branch("TrueQ2",&TrueQ2,"true_Q2/D");
  NeutCheckTree->Branch("ClusterCount", &fClusterCount,"clusterCount/I");
  NeutCheckTree->Branch("RecoClusters",&fRecoClusters,"RecoClusters");

  NeutCheckTree->Branch("TrueNeutronMult",&fTrueNeutronMult,"trueNeutronMult/I");
  NeutCheckTree->Branch("TrueNeutronDelayed",&fTrueNeutronDelayed,"trueNeutronDel/I");
  NeutCheckTree->Branch("TrueNeutCapT",&fMCNeutCapTimes);
 

  NeutCheckTree->Branch("ClusterNeutronMult",&fNeutronMult,"neutronMult/I");

  NeutCheckTree->Branch("ClusterMode",&fClusterMode);
  NeutCheckTree->Branch("ClusterPDG",&fClusterPDG);
  NeutCheckTree->Branch("ClusterParentPDG",&fClusterParentPDG);
  NeutCheckTree->Branch("ClusterParticleEnergy",&fClusterParticleEnergy);
  NeutCheckTree->Branch("ClusterHits", &fClusterHits);
  NeutCheckTree->Branch("ClusterTime",&fClusterTime);
  NeutCheckTree->Branch("ClusterCharge",&fClusterCharge);
  NeutCheckTree->Branch("ClusterPurity",&fClusterPurity);
  NeutCheckTree->Branch("ClusterCB",&fClusterCB);
  NeutCheckTree->Branch("ClusterAS0",&fClusterAS0);
  NeutCheckTree->Branch("ClusterAS1",&fClusterAS1);
  NeutCheckTree->Branch("ClusterAS2",&fClusterAS2);
  //NeutCheckTree->Branch("ClusterSA",&fClusterSA);
  NeutCheckTree->Branch("ClusterCVX",&fClusterCVX);
  NeutCheckTree->Branch("ClusterCVY", &fClusterCVY);
  NeutCheckTree->Branch("ClusterCVZ", &fClusterCVZ);
  NeutCheckTree->Branch("ClusterCVR", &fClusterCVR);
  NeutCheckTree->Branch("ClusterAMD", &fClusterAMD);
  NeutCheckTree->Branch("ClusterCA", &fClusterCA);


  if(fFinderCompare){
  NeutCheckTree->Branch("FinderClusterNumber",&fFinderClusterNum);
  NeutCheckTree->Branch("FinderClusterCount",&fFinderClusterCount,"FinderCount/I");
  NeutCheckTree->Branch("FinderPDG",&fFinderPDG);
  NeutCheckTree->Branch("FinderParentPDG",&fFinderParentPDG);
  NeutCheckTree->Branch("FinderCharge",&fFinderCharge);
  NeutCheckTree->Branch("FinderPurity",&fFinderPurity);
  NeutCheckTree->Branch("FinderTime",&fFinderTime);
  }

  if (fParticleInfo) {
      //Log("Particle Information to be added.",v_debug,verbosity);
      NeutCheckTree->Branch("ParticlePDG",&fParticlePDG);
      NeutCheckTree->Branch("ParticleParent",&fParticleParent);
      NeutCheckTree->Branch("ParticleStartEnergy",&fParticleStartEnergy);
      NeutCheckTree->Branch("ParticleStartTime", &fParticleStartTime);
      NeutCheckTree->Branch("TrueNeutCapX", &fMCNeutCapX);
      NeutCheckTree->Branch("TrueNeutCapY", &fMCNeutCapY);
      NeutCheckTree->Branch("TrueNeutCapZ", &fMCNeutCapZ);
  }

 
  

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool NeutronCheck::Execute(){

    Log("NeutronCheck Tool Executing:",v_message,verbosity);

    ResetVariables();

    // MC entry number
    m_data->Stores.at("ANNIEEvent")->Get("MCEventNum", fMCEventNum);

    // MC trigger number
    m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum", fMCTriggerNum);

    // ANNIE Event number
    m_data->Stores.at("ANNIEEvent")->Get("EventNumber", fEventNumber);

    auto get_flags = m_data->Stores.at("RecoEvent")->Get("EventFlagged", fEventStatusFlagged);

    if ((fEventStatusFlagged) != 0 && useClean) {
        //  if (!fEventCutStatus){
        Log("NeutronCheck Tool: Event was flagged with one of the active cuts.", v_debug, verbosity);
        return true;
    }
 

  

    

    //if(EventStore=="ANNIEEvent"){

        GetClusterInformation();

        m_data->Stores.at("ANNIEEvent")->Get("ClusterToBestParticleID", fClusterToBestParticleID);
        m_data->Stores.at("ANNIEEvent")->Get("ClusterToBestParticlePDG", fClusterToBestParticlePDG);
        m_data->Stores.at("ANNIEEvent")->Get("ClusterEfficiency", fClusterEfficiency);
        m_data->Stores.at("ANNIEEvent")->Get("ClusterPurity", fClusterPurityMap);
        m_data->Stores.at("ANNIEEvent")->Get("ClusterTotalCharge", fClusterTotalCharge);
        //bool get_q2 = m_data->Stores["GenieInfo"]->Get("EventQ2", TrueQ2);

        //if(get_q2) Log("True Q2 from GENIE: "+to_string(TrueQ2),v_debug,verbosity);
        //else Log("Error finding Q2 info from GENIE",v_debug,verbosity);
        //m_data->CStore.Get("ClausterMapMC", m_all_clusters);

        int bestPartId;
        for (int i=0;i<cluster_times.size();i++) {
            fFinderClusterNum.push_back(i);
            fFinderTime.push_back(cluster_times.at(i));
            fFinderCharge.push_back(cluster_charges.at(i));
            fFinderCB.push_back(cluster_cb.at(i));

           bestPartId=fClusterToBestParticleID->at(cluster_times.at(i));
           fFinderParentPDG.push_back(fMCParticles->at(bestPartId).GetParentPdg());
           fFinderPDG.push_back(fClusterToBestParticlePDG->at(cluster_times.at(i)));
           fFinderPurity.push_back(fClusterPurityMap->at(cluster_times.at(i)));
           
        }
        fFinderClusterCount=fFinderClusterNum.size()+1;

        fTrueNeutronMult=0;
        fTrueNeutronDelayed=0;
        Log("NeutCheck Tool: Scanning "+to_string(fMCParticles->size())+" MCParticles",v_debug,verbosity);
        for (int i = 0; i < fMCParticles->size(); i++) {
            if(fMCParticles->at(i).GetPdgCode()==2112 && fMCParticles->at(i).GetParentPdg()==0) {
                fTrueNeutronMult++;
                
                if(fMCParticles->at(i).GetStopTime()>10000) fTrueNeutronDelayed++;
            }
            else if(fMCParticles->at(i).GetPdgCode()==2112 && fMCParticles->at(i).GetParentPdg()!=0){
                Log("Found non-primary neutron at particle "+to_string(i)+" with parent PDG "+ to_string(fMCParticles->at(i).GetParentPdg()), v_debug, verbosity);
            }
            if(fParticleInfo){
            fParticlePDG.push_back(fMCParticles->at(i).GetPdgCode());
            fParticleParent.push_back(fMCParticles->at(i).GetParentPdg());
            fParticleStartEnergy.push_back(fMCParticles->at(i).GetStartEnergy());
            fParticleStartTime.push_back(fMCParticles->at(i).GetStartTime());
            if (fMCParticles->at(i).GetPdgCode() == 2112) {
                fMCNeutCapTimes.push_back(fMCParticles->at(i).GetStopTime());
                fMCNeutCapX.push_back(fMCParticles->at(i).GetStopVertex().X());
                fMCNeutCapY.push_back(fMCParticles->at(i).GetStopVertex().Y());
                fMCNeutCapZ.push_back(fMCParticles->at(i).GetStopVertex().Z());
            }
            }
        }

        auto get_muonMCEnergy = m_data->Stores.at("RecoEvent")->Get("TrueMuonEnergy", true_Emu);
        bool get_neutrino_energy = m_data->Stores["GenieInfo"]->Get("NeutrinoEnergy", true_Enu);

        RecoVertex* truevtx = 0;
        auto get_muonMC = m_data->Stores.at("RecoEvent")->Get("TrueVertex", truevtx);

        //true_Emu*=1000;  //GeV->MeV to match other energies(unneeded, possibly)

        double theta = truevtx->GetDirection().GetTheta();
        double p = sqrt(pow(true_Emu,2)-pow(105.7,2));
        TrueQ2=2*true_Enu*(true_Emu-p*cos(theta))-pow(105.7,2);
        Log("NeutCheck TrueQ2: "+to_string(TrueQ2), v_debug, verbosity);

    CSCheck();

    //MapRecoCheck->Fill(fClusterToBestParticleID->size(), fRecoClusters->size());
    //}

   NeutCheckTree->Fill();
  return true;
}


bool NeutronCheck::Finalise(){

    Log("NeutronCheck Tool: Finalizing",v_message,verbosity);
    fOutput_tfile->cd();
    

    NeutCheckTree->Write();

    

    fOutput_tfile->Close();

    
  return true;
}


bool NeutronCheck::GetClusterInformation() {

    bool return_value = true;

    bool get_cluster_idxN = m_data->Stores["RecoEvent"]->Get("ClusterIndicesNeutron", cluster_neutron);
    if (!get_cluster_idxN) Log("NeutronCheck tool: No ClusterIndicesNeutron In RecoEvent!", v_error, verbosity);
    bool get_cluster_tN = m_data->Stores["RecoEvent"]->Get("ClusterTimesNeutron", cluster_times_neutron);
    if (!get_cluster_tN) Log("NeutronCheck tool: No ClusterTimesNeutron In RecoEvent!", v_error, verbosity);
    bool get_cluster_qN = m_data->Stores["RecoEvent"]->Get("ClusterChargesNeutron", cluster_charges_neutron);
    if (!get_cluster_qN) Log("NeutronCheck tool: No ClusterChargesNeutron In RecoEvent!", v_error, verbosity);
    bool get_cluster_cbN = m_data->Stores["RecoEvent"]->Get("ClusterCBNeutron", cluster_cb_neutron);
    if (!get_cluster_cbN) Log("NeutronCheck tool: No ClusterCBNeutron In RecoEvent!", v_error, verbosity);
    bool get_cluster_t = m_data->Stores["RecoEvent"]->Get("ClusterTimes", cluster_times);
    if (!get_cluster_t) Log("NeutronCheck tool: No ClusterTimes In RecoEvent!", v_error, verbosity);
    bool get_cluster_q = m_data->Stores["RecoEvent"]->Get("ClusterCharges", cluster_charges);
    if (!get_cluster_q) Log("NeutronCheck tool: No ClusterCharges In RecoEvent!", v_error, verbosity);
    bool get_cluster_cb = m_data->Stores["RecoEvent"]->Get("ClusterCB", cluster_cb);
    if (!get_cluster_cb) Log("NeutronCheck tool: No ClusterCB In RecoEvent!", v_error, verbosity);

    bool goodMCParticles = m_data->Stores.at("ANNIEEvent")->Get("MCParticles", fMCParticles);
    if (!goodMCParticles) {
        std::cerr << "BackTracker: no MCParticles in the ANNIEEvent!" << endl;
        return false;
    }

    return_value = (get_cluster_idxN && get_cluster_tN && get_cluster_qN && get_cluster_cbN && get_cluster_t && get_cluster_q && get_cluster_cb);

    
   
    reco_Clusters = cluster_cb.size();



    return return_value;

}

double NeutronCheck::ASCheck(vector<RecoDigit>*& digits, int mode) {
    //mode config.  0 is maximal distance; 1 is max distance from highest charge
    
    double max_angle = 0;
    double max_angle2 = 0;
    if (mode == 0||mode==2) {
        
        double angle;
        Position i_position,j_position;
        for (int i = 0; i < digits->size(); i++) {
            if (useClean && !digits->at(i).GetFilterStatus())continue;
            i_position = digits->at(i).GetPosition();
            for (int j = 0; j < digits->size(); j++) {
                if (j == i)continue;
                if (useClean && !digits->at(j).GetFilterStatus())continue;
                j_position = digits->at(j).GetPosition();

                angle = j_position.Angle(i_position);
                if (angle > max_angle)max_angle = angle;

            }
        }
        cout<<"Neutcheck max_angle: "<<max_angle<<endl;
        if(mode==0)
        return max_angle;
    }

    if (mode == 1||mode==2) {
        double max_charge = 0;
        double angle;
        
        int max_index = 0;
        Position max_position;
        Position i_position;
        for (int i = 0; i < digits->size(); i++) {
            if (digits->at(i).GetCalCharge() > max_charge) {
                if(useClean && !digits->at(i).GetFilterStatus())continue;
                max_charge = digits->at(i).GetCalCharge();
                max_index = i;
                max_position = digits->at(i).GetPosition();
            }
        }


        for (int i = 0; i < digits->size(); i++) {
            if (i == max_index)continue;
            if (useClean && !digits->at(i).GetFilterStatus())continue;
            i_position = digits->at(i).GetPosition();
            angle = i_position.Angle(max_position);
            if (angle > max_angle2)max_angle2 = angle;
        }
        cout<<"Neutcheck max_angle2: "<<max_angle2<<endl;
        if(mode==1)
        return max_angle2;
    }
    double ratio = 0;
    if(max_angle>0) ratio = max_angle2/max_angle;
    if(mode==2)return ratio;
    
    return 0;
}

void NeutronCheck::CSCheck() {

    std::map<int,int>* fMCParticleIndexMap;
    
    Log("Cluster Check!",v_debug,verbosity);
    bool goodMCParticleIndexMap = m_data->Stores.at("ANNIEEvent")->Get("TrackId_to_MCParticleIndex", fMCParticleIndexMap);
    if (!goodMCParticleIndexMap) {
        std::cerr << "NeutCheck: no TrackId_to_MCParticleIndex in the ANNIEEvent!" << endl;
        return;
    }
    

    
    bool cluster_status = m_data->Stores.at("RecoEvent")->Get("RecoClusters", fRecoClusters);
    if (!cluster_status) {
        Log("Neutcheck tool found no recoclusters.",v_debug,verbosity);
        return;
    }
 
    int cluster_size = fRecoClusters->size();
    if (cluster_size == 0) {
        Log("Neutcheck tool found empty recoclusterlist.",v_debug,verbosity);
        return;
    }
    Log("Neutcheck tool found this many clusters: "+to_string(cluster_size),v_debug,verbosity);

    int bestParent;
    int bestParticleID=-5;
    int bestPDG=-5;
    double CVX,CVY,CVZ,CVR;

    for (int i = 0; i < fRecoClusters->size(); i++) {
        fClusterNum.push_back(i);
        fClusterCount++;
        bestParent = fRecoClusters->at(i)->calcBestParent();
        //bestParent = fRecoClusters->at(i)->GetBestParent();
        Log("Check 1: Particle ID "+to_string(bestParent),v_debug,verbosity);
        if (bestParent >= fMCParticles->size()) {
            Log("Invalid particle ID " + to_string(bestParent) + " vs number of particles: " + to_string(fMCParticles->size()) + "; skipping.",v_error,verbosity);
            continue;
        }
        
        //for (std::pair<int, int> apair : *fMCParticleIndexMap) {
            //Log("Testing match for particle: "+to_string(apair.first) + " corresponding to ID " + to_string(apair.second), v_debug, verbosity);
            //if(apair.second==bestParent){
                //bestParticleID=apair.first;
                bestPDG=fMCParticles->at(bestParent).GetPdgCode();
                //if (bestPDG == 22) {

                if (fMCParticles->at(bestParent).GetFlag() != 0) {
                    Log("Flagged particle!  PDG "+to_string(bestPDG)+" excluding.",v_message,verbosity);
                    bestPDG=-5;
                }
                    if (fMCParticles->at(bestParent).GetParentPdg() != 0) {
                        
                    Log("NeutCheck: PDG "+to_string(bestPDG)+" Finding parent.",v_message,verbosity);
                    fClusterParentPDG.push_back(fMCParticles->at(bestParent).GetParentPdg());
                    Log("Parent PDG "+to_string(fClusterParentPDG.at(fClusterParentPDG.size()-1)),v_message,verbosity);
                    }
                    else {
                        Log("NeutCheck tool: no parent found.  Treating as primary. PDG "+to_string(bestPDG), v_message, verbosity);
                        fClusterParentPDG.push_back(0);
                        if (bestPDG == 2212 && fRecoClusters->at(i)->GetTime()>10000) {

                        Log("Primary proton found ID " + to_string(bestParent) + " at time " +to_string(fRecoClusters->at(i)->GetTime())+" but particle start, stop "+to_string(fMCParticles->at(bestParent).GetStartTime())+", "+to_string(fMCParticles->at(bestParent).GetStopTime()),v_debug,verbosity);
                        }
                        if (bestPDG == 13 && fRecoClusters->at(i)->GetTime() > 10000) {

                            Log("Primary muon found ID " + to_string(bestParent) + " at time " + to_string(fRecoClusters->at(i)->GetTime()) + " but particle start, stop " + to_string(fMCParticles->at(bestParent).GetStartTime()) + ", " + to_string(fMCParticles->at(bestParent).GetStopTime()), v_debug, verbosity);
                        }
                        if (bestPDG == 2112 && fRecoClusters->at(i)->GetTime() > 10000) {

                            Log("Primary neutron found ID " + to_string(bestParent) + " at time " + to_string(fRecoClusters->at(i)->GetTime()) + " but particle start, stop " + to_string(fMCParticles->at(bestParent).GetStartTime()) + ", " + to_string(fMCParticles->at(bestParent).GetStopTime()), v_debug, verbosity);
                        }
                    }
                    
                //}
                fRecoClusters->at(i)->SetPDG(bestPDG);
                fClusterParticleEnergy.push_back(fMCParticles->at(bestParent).GetStartEnergy());
        
            //}
            
        //}
        /*if (matchCheck == false) {
            Log("NeutCheck: Failed to match Particle. IndexMap size: "+to_string(fMCParticleIndexMap->size()), v_error, verbosity);
        //continue;
        }*/
        

        //ClusterSearchCheck->Fill(fRecoClusters->at(i)->GetClusterMode());
        //recoPDGHist->Fill(fRecoClusters->at(i)->GetPDG());
        fClusterMode.push_back(fRecoClusters->at(i)->GetClusterMode());
        fClusterPDG.push_back(fRecoClusters->at(i)->GetPDG());
        if (fClusterPDG.at(fClusterPDG.size() - 1) == 2112) {
            fNeutronMult++;

        }

        fClusterCharge.push_back(fRecoClusters->at(i)->GetCharge());
        fClusterPurity.push_back(fRecoClusters->at(i)->Purity());
        fClusterTime.push_back(fRecoClusters->at(i)->GetTime());
        fClusterCB.push_back(fRecoClusters->at(i)->GetCB());
        fClusterHits.push_back(fRecoClusters->at(i)->GetNDigits());
        //if(fRecoClusters->at(i)->GetTime()>10000) recoPDGHistDelayed->Fill(fRecoClusters->at(i)->GetPDG());

        //AS0RecoCheck->Fill(fRecoClusters->at(i)->GetAS(0));
        //AS1RecoCheck->Fill(fRecoClusters->at(i)->GetAS(1));

        fClusterAS0.push_back(fRecoClusters->at(i)->GetAS(0));
        fClusterAS1.push_back(fRecoClusters->at(i)->GetAS(1));
        fClusterAMD.push_back(fRecoClusters->at(i)->GetAMD());
        fClusterCA.push_back(fRecoClusters->at(i)->GetCA());
        if(fRecoClusters->at(i)->GetAS(0)>0){
            //AS2RecoCheck->Fill(fRecoClusters->at(i)->GetAS(1)/fRecoClusters->at(i)->GetAS(0));
            fClusterAS2.push_back(fClusterAS1.at(fClusterAS1.size()-1)/fClusterAS0.at(fClusterAS0.size()-1));
        }
        fClusterASC.push_back(fRecoClusters->at(i)->GetASC());
        CVX= fRecoClusters->at(i)->GetCV().X();
        CVY = fRecoClusters->at(i)->GetCV().Y();
        CVZ = fRecoClusters->at(i)->GetCV().Z();
        CVR = pow(pow(CVX,2)+pow(CVY,2)+pow(CVZ,2),0.5);
        fClusterCVX.push_back(CVX);
        fClusterCVY.push_back(CVY);
        fClusterCVZ.push_back(CVZ);
        fClusterCVR.push_back(CVR);
        //fClusterSA.push_back(fRecoClusters->at(i)->GetSA());
        
    
    }

    return;
}

void NeutronCheck::ResetVariables() {
    fMCEventNum=-9999;
    fClusterNum.clear();
    fClusterCount=0;
    fClusterMode.clear();
    fClusterPDG.clear();
    fClusterParentPDG.clear();
    fClusterParticleEnergy.clear();
    fClusterCharge.clear();
    fClusterPurity.clear();
    fClusterCB.clear();
    fClusterTime.clear();
    fClusterAS0.clear();
    fClusterAS1.clear();
    fClusterAS2.clear();
    fClusterASC.clear();
    //fClusterSA.clear();
    fClusterCVX.clear();
    fClusterCVY.clear();
    fClusterCVZ.clear();
    fClusterCVR.clear();
    fClusterAMD.clear();
    fClusterCA.clear();
    fNeutronMult=0;
    fMCNeutCapTimes.clear();
    fMCNeutCapX.clear();
    fMCNeutCapY.clear();
    fMCNeutCapZ.clear();

    fParticlePDG.clear();
    fParticleParent.clear();
    fParticleStartEnergy.clear();
    fParticleStartTime.clear();


    fFinderClusterNum.clear();
    fFinderClusterCount=0;
    fFinderPDG.clear();
    fFinderParentPDG.clear();
    fFinderCharge.clear();
    fFinderPurity.clear();
    fFinderCB.clear();
    fFinderTime.clear();
}