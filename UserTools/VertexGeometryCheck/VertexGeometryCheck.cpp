#include "VertexGeometryCheck.h"

VertexGeometryCheck::VertexGeometryCheck():Tool(){}


bool VertexGeometryCheck::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();
  std::string output_filename;
  m_variables.Get("verbosity", verbosity);
  m_variables.Get("OutputFile", output_filename);
  m_variables.Get("ShowEvent", fShowEvent);
  m_variables.Get("Theta", vertheta);
  m_variables.Get("Phi", verphi);
  m_variables.Get("StripTimePlot", StripTimePlot);
  m_variables.Get("CleanHitsOnly", cleanHitsOnly);
  m_variables.Get("RecoCluster", fRecoCluster);
  fOutput_tfile = new TFile(output_filename.c_str(), "recreate");
  
  // Histograms
  flappdextendedtres = new TH1D("lappdextendedtres","lappd Extended time residual", 1000,-10000,10000);
  fpmtextendedtres = new TH1D("pmtextendedtres","pmt Extended time residual",1000, -10, 30); 
  fpointtres = new TH1D("pointres","Point time residual",1000,-10,30);
  fdelta = new TH1D("delta", "delta", 1000, -10, 30); 
  fmeanres = new TH1D("meanres","Mean time residual",1000, -10, 30);
  fltrack = new TH1D("ltrack","track path length",1000,0,1000);
  flphoton = new TH1D("lphoton","photon path length",1000,0,1000);
  fzenith = new TH1D("zenith","zenith angle",180,0,180);
  fazimuth = new TH1D("azimuth","azimuth angle",180,0,360);
  fconeangle = new TH1D("coneangle","cone angle",90,0,90);
  fdigitcharge = new TH1D("digitcharge","digit charge", 500,0,500);
  fdigittime = new TH1D("digittime", "digit time", 1000, -10000, 10000);
  flappdtimesmear = new TH1D("lappdtimesmear","lappdtimesmear", 100, 0, 0.1);
  fpmttimesmear = new TH1D("pmttimesmear","pmttimesmear",100, 0, 1.0);   
  fYvsDigitTheta_all = new TH2D("YvsDigitTheta_all", "Y vs DigitTheta", 400, -200, 200, 400, -200, 200);
  fYvsDigitTheta_all->GetXaxis()->SetTitle("DigitTheta [deg]");                                             
  fYvsDigitTheta_all->GetYaxis()->SetTitle("Digit Y [cm]");   
  if (StripTimePlot > 0) {
      StripHits1 = new TH2D("LAPPD Strip Time Residual", "LAPPD Strip Time Residual", 300, 0, 15, 400, -200, 200);
      StripHits1->SetTitle("LAPPD Hit Time vs. strip");
      StripHits1->GetYaxis()->SetTitle("Hit Time");
      StripHits1->GetXaxis()->SetTitle("Strip Location(x)");
  }
  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool VertexGeometryCheck::Execute() {
    Log("===========================================================================================", v_debug, verbosity);

    Log("VertexGeometryCheck Tool: Executing", v_debug, verbosity);

    // Get a pointer to the ANNIEEvent Store
    auto* reco_event = m_data->Stores["RecoEvent"];
    if (!reco_event) {
        Log("Error: The PhaseITreeMaker tool could not find the ANNIEEvent Store",
            0, verbosity);
        return false;
    }

    // MC entry number
    m_data->Stores.at("ANNIEEvent")->Get("MCEventNum", fMCEventNum);

    // MC trigger number
    m_data->Stores.at("ANNIEEvent")->Get("MCTriggernum", fMCTriggerNum);

    // ANNIE Event number
    m_data->Stores.at("ANNIEEvent")->Get("EventNumber", fEventNumber);

    // Only check this event
    if (fShowEvent > 0 && (int)fEventNumber != fShowEvent) return true;

    // check if event passes the cut
    bool EventCutstatus = false;
    auto get_evtstatus = m_data->Stores.at("RecoEvent")->Get("EventCutStatus", EventCutstatus);
    if (!get_evtstatus) {
        Log("Error: The VertexGeometryCheck tool could not find the Event selection status", v_error, verbosity);
        return false;
    }
    if (!EventCutstatus) {
        Log("Message: This event doesn't pass the event selection. ", v_message, verbosity);
        return true;
    }

    // Read True Vertex   
    RecoVertex* truevtx = 0;
    auto get_vtx = m_data->Stores.at("RecoEvent")->Get("TrueVertex", fTrueVertex);  ///> Get digits from "RecoEvent" 
    if (!get_vtx) {
        Log("VertexGeometryCheck  Tool: Error retrieving TrueVertex! ", v_error, verbosity);
        return true;
    }

    // Retrive digits from RecoEvent
    /*auto get_digit = m_data->Stores.at("RecoEvent")->Get("RecoDigit", fDigitList);  ///> Get digits from "RecoEvent" 
    if (!get_digit) {
        Log("VertexGeometryCheck  Tool: Error retrieving RecoDigits,no digit from the RecoEvent!", v_error, verbosity);
        return true;
    }*/

    if (!fRecoCluster) {
        // Retrive digits from RecoEvent
        get_ok = m_data->Stores.at("RecoEvent")->Get("RecoDigit", fDigitList);  ///> Get digits from "RecoEvent" 
        if (not get_ok) {
            Log("VtxExtendedVertexFinder  Tool: Error retrieving RecoDigits,no digit from the RecoEvent!", v_error, verbosity);
            return false;
        }
    }
    else {
        //Get focused digits from RecoCluster list.  Note that 'ClusterMode 1' must refer to dense, directional, hit-cleaned clusters for this to work.
        get_ok = m_data->Stores.at("RecoEvent")->Get("RecoClusters", fClusterList);
        if (not get_ok) {
            Log("VtxExtendedVertexFinder Tool: Error retrieving RecoClusters, no clusters from the RecoEvent!", v_error, verbosity);
            return false;
        }
        vector<RecoDigit> tempDigitList;
        for (int i = 0; i < fClusterList->size(); i++) {
            cout << "check1" << fClusterList->at(i).GetNDigits() << ", " << fClusterList->at(i).GetDigitList().size() << endl;
            fClusterList->at(i).Print();
            if (fClusterList->at(i).GetClusterMode() == 1 && fClusterList->at(i).GetTime() < 10000 && fClusterList->at(i).GetNDigits() > 0) {  //Todo: Set Time Window in config
                tempDigitList = fClusterList->at(i).GetDigitList();
                fDigitList = &tempDigitList;

                Log("VtxGeoCheck Tool: DigitCount, number, time: " + to_string(fDigitList->size()) + ", " + to_string(i)/* + ", "+to_string(fDigitList->at(i).GetCalTime())*/, v_debug, verbosity);
                break;
            }
        }
        if (fDigitList == nullptr) {
            Log("VtxExtendedVertexFinder Tool: No primary Cherenkov Cluster.  Aborting", v_error, verbosity);
            return false;
        }
    }

    double recoVtxX, recoVtxY, recoVtxZ, recoVtxT, recoDirX, recoDirY, recoDirZ;
    double trueVtxX, trueVtxY, trueVtxZ, trueVtxT, trueDirX, trueDirY, trueDirZ;
    double digitX, digitY, digitZ, digitT;
    double dx, dy, dz, px, py, pz, ds, cosphi, sinphi, phi, phideg;

    Position vtxPos = fTrueVertex->GetPosition();
    Direction vtxDir = fTrueVertex->GetDirection();
    trueVtxX = vtxPos.X();
    trueVtxY = vtxPos.Y();
    trueVtxZ = vtxPos.Z();
    trueVtxT = fTrueVertex->GetTime();
    trueDirX = vtxDir.X();
    trueDirY = vtxDir.Y();
    trueDirZ = vtxDir.Z();
    if (vertheta != -999 && verphi != -999) {
        Log("overriding direction", v_debug, verbosity);
        trueDirX = cos(vertheta) * sin(verphi);
        trueDirY = sin(vertheta) * sin(verphi);
        trueDirZ = cos(verphi);
    }

    double ConeAngle = Parameters::CherenkovAngle();

    FoMCalculator* myFoMCalculator = new FoMCalculator();
    VertexGeometry* myvtxgeo = VertexGeometry::Instance();
    myvtxgeo->LoadDigits(fDigitList);
    myFoMCalculator->LoadVertexGeometry(myvtxgeo); //Load vertex geometry
    int nhits = myvtxgeo->GetNDigits();
    myvtxgeo->CalcExtendedResiduals(trueVtxX, trueVtxY, trueVtxZ, trueVtxT, trueDirX, trueDirY, trueDirZ);
    double meantime = myFoMCalculator->FindSimpleTimeProperties(ConeAngle);
    fmeanres->Fill(meantime);
    double fom = -999.999 * 100;
    myFoMCalculator->TimePropertiesLnL(meantime, fom);
    std::string stripPlotName;
    int currentLAPPD = 0;
    
    int iStripHit = 0;
    std::cout << "VGCheck entering for loop\n";
    for (int n = 0; n < nhits; n++) {
        if (cleanHitsOnly && !(fDigitList->at(n).GetFilterStatus())) continue;
        digitX = fDigitList->at(n).GetPosition().X();
        digitY = fDigitList->at(n).GetPosition().Y();
        digitZ = fDigitList->at(n).GetPosition().Z();
        digitT = fDigitList->at(n).GetCalTime();
        dx = digitX - trueVtxX;
        dy = digitY - trueVtxY;
        dz = digitZ - trueVtxZ;
        ds = sqrt(dx * dx + dy * dy + dz * dz);
        px = dx / ds;
        py = dy / ds;
        pz = dz / ds;
        cosphi = 1.0;
        sinphi = 1.0;
        phi = 0.0;
        phideg = 0.0;
        // zenith angle relative to muon track direction                                           
        if (trueDirX * trueDirX + trueDirY * trueDirY + trueDirZ * trueDirZ > 0.0) {
            // zenith angle
            cosphi = px * trueDirX + py * trueDirY + pz * trueDirZ;
            phi = acos(cosphi); // radians
            phideg = phi / (TMath::Pi() / 180.0); // radians->degrees
        }
        // Y vs theta
        double theta = 0.0;
        double thetadeg = 0.0;
        if (digitZ != 0.0) {
            theta = atan(digitX / digitZ);
        }
        if (digitZ <= 0.0) {
            if (digitX > 0.0) theta += TMath::Pi();
            if (digitX < 0.0) theta -= TMath::Pi();
        }
        thetadeg = theta / (TMath::Pi() / 180.0); // radians->degrees   
        if(fDigitList->at(n).GetDigitType() == RecoDigit::PMT8inch)fYvsDigitTheta_all->Fill(thetadeg, digitY, myvtxgeo->GetDigitQ(n));
        if(fDigitList->at(n).GetDigitType() == RecoDigit::lappd_v0)fYvsDigitTheta_all->Fill(thetadeg, digitY, myvtxgeo->GetDigitQ(n));
        fdelta->Fill(myvtxgeo->GetDelta(n));
        fpointtres->Fill(myvtxgeo->GetPointResidual(n));
        if (myvtxgeo->GetDigitType(n) == RecoDigit::lappd_v0) flappdextendedtres->Fill(myvtxgeo->GetExtendedResidual(n));
        if (myvtxgeo->GetDigitType(n) == RecoDigit::lappd_v0) Log("LAPPD extres: "+to_string(myvtxgeo->GetExtendedResidual(n)),v_debug,verbosity);
        if (myvtxgeo->GetDigitType(n) == RecoDigit::PMT8inch) fpmtextendedtres->Fill(myvtxgeo->GetExtendedResidual(n));
        fltrack->Fill(myvtxgeo->GetDistTrack(n)); //cm
        flphoton->Fill(myvtxgeo->GetDistPhoton(n)); //cm
        fzenith->Fill(phideg/*myvtxgeo->GetZenith(n)*/); //
        fazimuth->Fill(myvtxgeo->GetAzimuth(n)); //
        fconeangle->Fill(myvtxgeo->GetConeAngle(n)); //
        fdigitcharge->Fill(myvtxgeo->GetDigitQ(n));
        fdigittime->Fill(digitT);
        Log("VGCheck: Digit's time, delta, ltrack, lphoton: "+to_string(digitT)+", "+to_string(myvtxgeo->GetDelta(n))+", "+to_string(myvtxgeo->GetDistTrack(n))+", "+to_string(myvtxgeo->GetDistPhoton(n)),v_debug,verbosity);
        if (myvtxgeo->GetDigitType(n) == RecoDigit::lappd_v0) flappdtimesmear->Fill(Parameters::TimeResolution(RecoDigit::lappd_v0, myvtxgeo->GetDigitQ(n)));
        if (myvtxgeo->GetDigitType(n) == RecoDigit::PMT8inch) fpmttimesmear->Fill(Parameters::TimeResolution(RecoDigit::PMT8inch, myvtxgeo->GetDigitQ(n)));
        std::cout << "VGCheck " << n << endl;
        if (StripTimePlot > 0) {
            if (fDigitList->at(n).GetDigitType() == RecoDigit::lappd_v0 && fDigitList->at(n).GetCalTime()>0 /* && fDigitList->at(n).GetDetectorID() == StripTimePlot*/) {
                if (fDigitList->at(n).GetDetectorID() != currentLAPPD)
                {

                    if (currentLAPPD > 0 && iStripHit > 0) {
                        stripPlotName = "StripPlot_e" + std::to_string(fEventNumber) + "lappd" + std::to_string(currentLAPPD);
                        fOutput_tfile->cd();
                        StripHits1->Write(stripPlotName.c_str());
                    }
                    StripHits1->Reset();
                    currentLAPPD = fDigitList->at(n).GetDetectorID();
                    iStripHit = 0;
                }
                StripHits1->Fill(fDigitList->at(n).GetPosition().X(), fDigitList->at(n).GetCalTime() * 1000/*, fDigitList->at(n).GetCalCharge()*/);
                iStripHit++;
                std::cout << "VGCheck striphit";
                
            }
        }
    }

    delete myFoMCalculator;

  return true;
}

bool VertexGeometryCheck::Finalise(){
  fOutput_tfile->cd();
  fOutput_tfile->Write();
  if (StripTimePlot > 0) StripHits1->Write("lappdtimegradient");
  fOutput_tfile->Close();
  Log("VertexGeometryCheck exitting", v_debug,verbosity);
  return true;
}
