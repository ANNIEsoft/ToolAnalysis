#include "NeutronCheck.h"

NeutronCheck::NeutronCheck():Tool(){}


bool NeutronCheck::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_variables.Get("verbosity",verbosity);
  m_variables.Get("IsData",fIsData);
  m_variables.Get("outfile",outfile);
  m_variables.Get("UseCleanEvent",useCleanEvent);
  m_variables.Get("UseCleanCluster",useCleanCluster);
  m_variables.Get("UseCleanDigit",useCleanDigit);
  m_variables.Get("FinderCompare",fFinderCompare);
  m_variables.Get("ParticleInfo",fParticleInfo);
  m_variables.Get("DelayThreshold",fDelayThreshold);
  m_variables.Get("VertexInfo",fVertexInfo);

  outfile+=".root";

  fOutput_tfile = new TFile(outfile.c_str(), "recreate");
  NeutCheckTree = new TTree("NeutCheckTree", "Neutron Check Tree");

  NeutCheckTree->Branch("eventNumber", &fMCEventNum, "eventNumber/I");
  
  NeutCheckTree->Branch("eventCutStatus", &fEventStatusFlagged,"eventStatusFlagged/I");
  NeutCheckTree->Branch("ClusterNumber",&fClusterNum);
  
  if(!fIsData){
  //NeutCheckTree->Branch("TrueEnu",&true_Enu,"true_Enu/D");
  NeutCheckTree->Branch("TrueEmu",&true_Emu,"true_Emu/D");
  //NeutCheckTree->Branch("TrueQ2",&TrueQ2,"true_Q2/D");
  NeutCheckTree->Branch("CalcQ2",&CalcQ2,"Calc_Q2/D");
  }

  NeutCheckTree->Branch("classicEmu",&classicEmu,"classic_Emu/D");
  NeutCheckTree->Branch("classicPt",&classicPt,"classic_Pt/D");

  NeutCheckTree->Branch("ClusterCount", &fClusterCount,"clusterCount/I");
  NeutCheckTree->Branch("RecoClusters",&fRecoClusters,"RecoClusters");

  NeutCheckTree->Branch("TrueNeutronMult",&fTrueNeutronMult,"trueNeutronMult/I");
  NeutCheckTree->Branch("TrueNeutronDelayed",&fTrueNeutronDelayed,"trueNeutronDel/I");
  NeutCheckTree->Branch("TrueNeutCapT",&fMCNeutCapTimes);
 

  NeutCheckTree->Branch("ClusterNeutronMult",&fNeutronMult,"neutronMult/I");

  NeutCheckTree->Branch("ClusterNDigits",&fClusterNDigits);
  NeutCheckTree->Branch("ClusterMode",&fClusterMode);
  NeutCheckTree->Branch("ClusterPDG",&fClusterPDG);
  NeutCheckTree->Branch("ClusterParentPDG",&fClusterParentPDG);
  NeutCheckTree->Branch("ClusterCoincPDG",&fClusterCoincPDG);
  NeutCheckTree->Branch("ClusterParticleEnergy",&fClusterParticleEnergy);
  NeutCheckTree->Branch("ClusterHits", &fClusterHits);
  NeutCheckTree->Branch("ClusterTime",&fClusterTime);
  NeutCheckTree->Branch("ClusterCharge",&fClusterCharge);
  NeutCheckTree->Branch("DigitCharges",&fDigitCharges);
  NeutCheckTree->Branch("DigitTimes",&fDigitTimes);
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
  NeutCheckTree->Branch("ClusterAW", &fClusterAW);
  NeutCheckTree->Branch("ClusterB1", &fClusterB1);
  NeutCheckTree->Branch("ClusterB2", &fClusterB2);
  NeutCheckTree->Branch("ClusterB3", &fClusterB3);
  NeutCheckTree->Branch("ClusterB4", &fClusterB4);
  NeutCheckTree->Branch("ClusterB5", &fClusterB5);
  NeutCheckTree->Branch("ClusterPlanarity", &fClusterPlanarity);
  NeutCheckTree->Branch("ClusterSphericity", &fClusterSphericity);
  NeutCheckTree->Branch("ClusterTRT", &fClusterTRT);
  NeutCheckTree->Branch("ClusterTRQ", &fClusterTRQ);
  NeutCheckTree->Branch("ClusterTRC", &fClusterTRC);
  NeutCheckTree->Branch("ClusterTRRTQ", &fClusterTRRTQ);
  NeutCheckTree->Branch("ClusterTRRTC", &fClusterTRRTC);
  NeutCheckTree->Branch("ClusterTRRQC", &fClusterTRRQC);

  if(fVertexInfo){
      if(!fIsData){
  NeutCheckTree->Branch("trueVtxX", &trueVtxX,"trueVtxX/D");
  NeutCheckTree->Branch("trueVtxY", &trueVtxY, "trueVtxY/D");
  NeutCheckTree->Branch("trueVtxZ", &trueVtxZ, "trueVtxZ/D");
      }

  NeutCheckTree->Branch("recoVtxX", &recoVtxX, "recoVtxX/D");
  NeutCheckTree->Branch("recoVtxY", &recoVtxY, "recoVtxY/D");
  NeutCheckTree->Branch("recoVtxZ", &recoVtxZ, "recoVtxZ/D");
  NeutCheckTree->Branch("recoVtxFOM",&recoVtxFOM, "recoVtxFOM/D");
  NeutCheckTree->Branch("vtxRecoStatus",&vtxRecoStatus, "vtxRecoStatus/I");

  if(!fIsData){
  NeutCheckTree->Branch("deltaVtxX", &deltaVtxX, "deltaVtxX/D");
  NeutCheckTree->Branch("deltaVtxY", &deltaVtxY, "deltaVtxY/D");
  NeutCheckTree->Branch("deltaVtxZ", &deltaVtxZ, "deltaVtxZ/D");
  NeutCheckTree->Branch("deltaVtxR", &deltaVtxR, "deltaVtxR/D");
  }
  }

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
      NeutCheckTree->Branch("ParticleNum",&fParticleNumber);
      NeutCheckTree->Branch("ParticlePDG",&fParticlePDG);
      NeutCheckTree->Branch("ParticleParent",&fParticleParent);
      NeutCheckTree->Branch("ParticleStartEnergy",&fParticleStartEnergy);
      NeutCheckTree->Branch("ParticleStartTime", &fParticleStartTime);
      NeutCheckTree->Branch("ParticleStopTime", &fParticleStopTime);
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

    if ((fEventStatusFlagged) != 0 && useCleanEvent) {
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
                
                if(fMCParticles->at(i).GetStopTime()>fDelayThreshold) fTrueNeutronDelayed++;
            }
            else if(fMCParticles->at(i).GetPdgCode()==2112 && fMCParticles->at(i).GetParentPdg()!=0){
                Log("Found non-primary neutron at particle "+to_string(i)+" with parent PDG "+ to_string(fMCParticles->at(i).GetParentPdg()), v_debug, verbosity);
            }
            if(fParticleInfo){
                fParticleNumber.push_back(i);
            fParticlePDG.push_back(fMCParticles->at(i).GetPdgCode());
            fParticleParent.push_back(fMCParticles->at(i).GetParentPdg());
            fParticleStartEnergy.push_back(fMCParticles->at(i).GetStartEnergy());
            fParticleStartTime.push_back(fMCParticles->at(i).GetStartTime());
            fParticleStopTime.push_back(fMCParticles->at(i).GetStopTime());


            /*if (fMCParticles->at(i).GetPdgCode() == 2112) {
                fMCNeutCapTimes.push_back(fMCParticles->at(i).GetStopTime());
                fMCNeutCapX.push_back(fMCParticles->at(i).GetStopVertex().X());
                fMCNeutCapY.push_back(fMCParticles->at(i).GetStopVertex().Y());
                fMCNeutCapZ.push_back(fMCParticles->at(i).GetStopVertex().Z());
            }*/
            }

            
        }

        if (fParticleInfo) {
            std::map<std::string, std::vector<double>> MCNeutCap;
            bool get_neutcap = m_data->Stores.at("ANNIEEvent")->Get("MCNeutCap", MCNeutCap);
            if (!get_neutcap) {
                Log("NeutCheck Tool: Did not find MCNeutCap in ANNIEEvent Store!", v_warning, verbosity);
            }
            fMCNeutCapTimes=MCNeutCap["CaptTime"];
            fMCNeutCapX=MCNeutCap["CaptVtxX"];
            fMCNeutCapY=MCNeutCap["CaptVtxY"];
            fMCNeutCapZ=MCNeutCap["CaptVtxZ"];
            fMCNeutCapNucleus = MCNeutCap["CaptNucleus"];
        }

        if (fVertexInfo) {
            RecoVertex* trueVertex=0;
            RecoVertex* recoVertex=0;
            bool get_truevertex=true;
            if(!fIsData) get_truevertex = m_data->Stores.at("RecoEvent")->Get("TrueVertex", trueVertex);
            bool get_recovertex = m_data->Stores.at("RecoEvent")->Get("ExtendedVertex", recoVertex);
            if (!(get_truevertex && get_recovertex)) {
                Log("NeutCheck Tool couldn't find a vertex!", v_error, verbosity);
            }
            else {
                if(!fIsData){
                trueVtxX=trueVertex->GetPosition().X();
                trueVtxY=trueVertex->GetPosition().Y();
                trueVtxZ=trueVertex->GetPosition().Z();
                trueDirX=trueVertex->GetDirection().X();
                trueDirY=trueVertex->GetDirection().Y();
                trueDirZ=trueVertex->GetDirection().Z();
                }

                recoVtxX=recoVertex->GetPosition().X();
                recoVtxY=recoVertex->GetPosition().Y();
                recoVtxZ=recoVertex->GetPosition().Z();
                recoDirX=recoVertex->GetDirection().X();
                recoDirY=recoVertex->GetDirection().Y();
                recoDirZ=recoVertex->GetDirection().Z();
                recoVtxFOM=recoVertex->GetFOM();
                vtxRecoStatus=recoVertex->GetStatus();

                
                if (!fIsData) {
                    deltaVtxX=trueVtxX-recoVtxX;
                    deltaVtxY=trueVtxY-recoVtxY;
                    deltaVtxZ=trueVtxZ-recoVtxZ;
                    deltaVtxR=pow(deltaVtxX*deltaVtxX + deltaVtxY*deltaVtxY + deltaVtxZ*deltaVtxZ,0.5);
                    Log("True, reco, delta VtxX: "+to_string(trueVtxX)+", "+to_string(recoVtxX)+", "+to_string(deltaVtxX),v_debug,verbosity);
                    Log("True, reco, delta VtxY: "+to_string(trueVtxY)+", "+to_string(recoVtxY)+", "+to_string(deltaVtxY),v_debug,verbosity);
                    Log("True, reco, delta VtxZ: "+to_string(trueVtxZ)+", "+to_string(recoVtxZ)+", "+to_string(deltaVtxZ),v_debug,verbosity);
                    Log("deltaVtxR: "+to_string(deltaVtxR),v_debug,verbosity);
                }
            }
        

        if(!fIsData){
        auto get_muonMCEnergy = m_data->Stores.at("RecoEvent")->Get("TrueMuonEnergy", true_Emu);
        //bool get_neutrino_energy = m_data->Stores["GenieInfo"]->Get("NeutrinoEnergy", true_Enu);

        //true_Emu*=1000;  //GeV->MeV to match other energies(unneeded, possibly)

        double theta = trueVertex->GetDirection().GetPhi();
        double p = sqrt(pow(true_Emu,2)-pow(105.7,2));
        //Log("True Enu, Emu, p, cos(theta): "+to_string(true_Enu)+", "+to_string(true_Emu)+", "+to_string(p)+", "+to_string(cos(theta)), v_debug, verbosity);
        //CalcQ2=2*true_Enu*(true_Emu-p*cos(theta))-pow(105.7,2);
        //CalcQ2/=1000000;
        //Log("NeutCheck CalculatedQ2: "+to_string(CalcQ2), v_debug, verbosity);

        //m_data->Stores["GenieInfo"]->Get("EventQ2", TrueQ2);
        //Log("NeutCheck TrueQ2: "+to_string(TrueQ2), v_debug, verbosity);
        }
        }

        m_data->Stores.at("RecoEvent")->Get("classicRecoEnergy",classicEmu);
        m_data->Stores.at("RecoEvent")->Get("classicTransverseP",classicPt);

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
            if (useCleanDigit && !digits->at(i).GetFilterStatus())continue;
            i_position = digits->at(i).GetPosition();
            for (int j = 0; j < digits->size(); j++) {
                if (j == i)continue;
                if (useCleanDigit && !digits->at(j).GetFilterStatus())continue;
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
                if(useCleanDigit && !digits->at(i).GetFilterStatus())continue;
                max_charge = digits->at(i).GetCalCharge();
                max_index = i;
                max_position = digits->at(i).GetPosition();
            }
        }


        for (int i = 0; i < digits->size(); i++) {
            if (i == max_index)continue;
            if (useCleanDigit && !digits->at(i).GetFilterStatus())continue;
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

    Log("Checka", v_debug, verbosity);
    for (int i = 0; i < fRecoClusters->size(); i++) {

        Log("Checkb", v_debug, verbosity);
        fRecoClusters->at(i).CheckFilter();
        Log("Checkb1", v_debug, verbosity);
        if(useCleanCluster && !(fRecoClusters->at(i).GetFilterStatus())) continue;

        Log("Checkc", v_debug, verbosity);
        fClusterNum.push_back(i);
        fClusterCount++;
        Log("Checkd",v_debug,verbosity);
        bestParent = fRecoClusters->at(i).calcBestParent();
        //bestParent = fRecoClusters->at(i).GetBestParent();
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
                        if (bestPDG == 2212 && fRecoClusters->at(i).GetTime()>fDelayThreshold) {

                        Log("Primary proton found ID " + to_string(bestParent) + " at time " +to_string(fRecoClusters->at(i).GetTime())+" but particle start, stop "+to_string(fMCParticles->at(bestParent).GetStartTime())+", "+to_string(fMCParticles->at(bestParent).GetStopTime()),v_debug,verbosity);
                        }
                        if (bestPDG == 13 && fRecoClusters->at(i).GetTime() > fDelayThreshold) {

                            Log("Primary muon found ID " + to_string(bestParent) + " at time " + to_string(fRecoClusters->at(i).GetTime()) + " but particle start, stop " + to_string(fMCParticles->at(bestParent).GetStartTime()) + ", " + to_string(fMCParticles->at(bestParent).GetStopTime()), v_debug, verbosity);
                        }
                        if (bestPDG == 2112 && fRecoClusters->at(i).GetTime() > fDelayThreshold) {

                            Log("Primary neutron found ID " + to_string(bestParent) + " at time " + to_string(fRecoClusters->at(i).GetTime()) + " but particle start, stop " + to_string(fMCParticles->at(bestParent).GetStartTime()) + ", " + to_string(fMCParticles->at(bestParent).GetStopTime()), v_debug, verbosity);
                        }
                    }
                    
                //}
                fRecoClusters->at(i).SetPDG(bestPDG);
                fClusterParticleEnergy.push_back(fMCParticles->at(bestParent).GetStartEnergy());
        
            //}
            
        //}
        /*if (matchCheck == false) {
            Log("NeutCheck: Failed to match Particle. IndexMap size: "+to_string(fMCParticleIndexMap->size()), v_error, verbosity);
        //continue;
        }*/
        

        //ClusterSearchCheck->Fill(fRecoClusters->at(i).GetClusterMode());
        //recoPDGHist->Fill(fRecoClusters->at(i).GetPDG());
        fClusterMode.push_back(fRecoClusters->at(i).GetClusterMode());
        fClusterPDG.push_back(fRecoClusters->at(i).GetPDG());
        if (fClusterPDG.at(fClusterPDG.size() - 1) == 2112) {
            fNeutronMult++;

        }

        fClusterNDigits.push_back(fRecoClusters->at(i).GetNDigits());
        fClusterCharge.push_back(fRecoClusters->at(i).GetCharge());
        fClusterPurity.push_back(fRecoClusters->at(i).Purity());
        fClusterTime.push_back(fRecoClusters->at(i).GetTime());
        fClusterCB.push_back(fRecoClusters->at(i).GetCB());
        for(int j=0; j<fRecoClusters->at(i).GetNDigits();j++){
       
            fDigitCharges.push_back(fRecoClusters->at(i).GetDigit(j).GetCalCharge());
            fDigitTimes.push_back(fRecoClusters->at(i).GetDigit(j).GetCalTime());
        }
        //if(fRecoClusters->at(i).GetTime()>fDelayThreshold) recoPDGHistDelayed->Fill(fRecoClusters->at(i).GetPDG());

        //AS0RecoCheck->Fill(fRecoClusters->at(i).GetAS(0));
        //AS1RecoCheck->Fill(fRecoClusters->at(i).GetAS(1));

        fClusterAS0.push_back(fRecoClusters->at(i).GetAS(0));
        fClusterAS1.push_back(fRecoClusters->at(i).GetAS(1));
        fClusterAMD.push_back(fRecoClusters->at(i).GetAMD());
        fClusterAW.push_back(fRecoClusters->at(i).GetAW());
        fClusterB1.push_back(fRecoClusters->at(i).CalcBeta(1));
        fClusterB2.push_back(fRecoClusters->at(i).CalcBeta(2));
        fClusterB3.push_back(fRecoClusters->at(i).CalcBeta(3));
        fClusterB4.push_back(fRecoClusters->at(i).CalcBeta(4));
        fClusterB5.push_back(fRecoClusters->at(i).CalcBeta(5));
        fClusterPlanarity.push_back(fRecoClusters->at(i).GetPlanarity());
        fClusterSphericity.push_back(fRecoClusters->at(i).GetSphericity());

        fClusterTRT.push_back(fRecoClusters->at(i).GetTimeRangeT());
        fClusterTRQ.push_back(fRecoClusters->at(i).GetTimeRangeQ());
        fClusterTRC.push_back(fRecoClusters->at(i).GetTimeRangeC());
        fClusterTRRTQ.push_back(fRecoClusters->at(i).GetTRRTQ());
        fClusterTRRTC.push_back(fRecoClusters->at(i).GetTRRTC());
        fClusterTRRQC.push_back(fRecoClusters->at(i).GetTRRQC());

        if(fRecoClusters->at(i).GetAS(0)>0){
            //AS2RecoCheck->Fill(fRecoClusters->at(i).GetAS(1)/fRecoClusters->at(i).GetAS(0));
            fClusterAS2.push_back(fClusterAS1.at(fClusterAS1.size()-1)/fClusterAS0.at(fClusterAS0.size()-1));
        }
        fClusterASC.push_back(fRecoClusters->at(i).GetASC());
        CVX= fRecoClusters->at(i).GetCV().X();
        CVY = fRecoClusters->at(i).GetCV().Y();
        CVZ = fRecoClusters->at(i).GetCV().Z();
        CVR = pow(pow(CVX,2)+pow(CVY,2)+pow(CVZ,2),0.5);
        fClusterCVX.push_back(CVX);
        fClusterCVY.push_back(CVY);
        fClusterCVZ.push_back(CVZ);
        fClusterCVR.push_back(CVR);
        //fClusterSA.push_back(fRecoClusters->at(i).GetSA());

        
    
    }

    return;
}

void NeutronCheck::ResetVariables() {
    fMCEventNum=-9999;
    fClusterNum.clear();
    fClusterCount=0;
    fClusterMode.clear();
    fClusterNDigits.clear();
    fClusterPDG.clear();
    fClusterParentPDG.clear();
    fClusterParticleEnergy.clear();
    fClusterCharge.clear();
    fDigitCharges.clear();
    fDigitTimes.clear();
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
    fClusterAW.clear();
    fClusterB1.clear();
    fClusterB2.clear();
    fClusterB3.clear();
    fClusterB4.clear();
    fClusterB5.clear();
    fClusterPlanarity.clear();
    fClusterSphericity.clear();

    fClusterTRT.clear();
    fClusterTRQ.clear();
    fClusterTRC.clear();
    fClusterTRRTQ.clear();
    fClusterTRRTC.clear();
    fClusterTRRQC.clear();

    fNeutronMult=0;
    fMCNeutCapTimes.clear();
    fMCNeutCapX.clear();
    fMCNeutCapY.clear();
    fMCNeutCapZ.clear();

    fParticleNumber.clear();
    fParticlePDG.clear();
    fParticleParent.clear();
    fParticleStartEnergy.clear();
    fParticleStartTime.clear();
    fParticleStopTime.clear();


    fFinderClusterNum.clear();
    fFinderClusterCount=0;
    fFinderPDG.clear();
    fFinderParentPDG.clear();
    fFinderCharge.clear();
    fFinderPurity.clear();
    fFinderCB.clear();
    fFinderTime.clear();

    classicEmu=-9999;
    classicPt=-9999;
}