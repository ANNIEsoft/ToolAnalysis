#include "MuonFitter.h"

/* *****************************************************************
 * tool name: MuonFitter
 * author: Julie He
 *
 * desc: This tool takes
 *
 * versions:
 * ...
 * 240331v1JH: fixed line of code defining reco_mu_e (prev defined 
 *              using mrdEnergyLoss)
 * 240401v2JH: removed unnecessary code, old algorithms
 * 240405v1JH: add option to use simple energy reco (just add eloss)
 * 240407v1JH: reco mode no longer includes finding muon, added 
 *              cluster_time to m_tank_track_fits
 * 240425v1JH: incorporated scattering effects into MRD track length
 * 240503v1JH: added SimpleReco* variables for NeutronMultiplicity 
 *              toolchain
 * 240506v1JH: adjusted nlyrs method (additional 0.5 iron layer)
 * 240508v1JH: I am once again, updating the dEdx
 * 240514v1JH: Removed FMV and spherical FV cut
 * 240515v1JH: Added in FMV cut from EventSelector
 * 240516v1JH: Reorganized script, added spherical FV cut back in
 * 240520v1JH: added original veto cut back in
 * 240520v2JH: removed original veto cut
 * 240520v3JH: added different FV cuts
 * 240524v1JH: fixed FV cut logic, added more plots to NM tool
 * *****************************************************************
 */

MuonFitter::MuonFitter():Tool(){}


bool MuonFitter::Initialise(std::string configfile, DataModel &data){

  ///////////////////////// Useful header /////////////////////////
  //-- Load config file
  if(configfile!="") m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  //-- Set default values
  verbosity = 3;
  isData = true;
  PMTMRDOffset = 745.;

  // -------------------------------------------------------------
  // --- Retrieve config variables -------------------------------
  // -------------------------------------------------------------
  m_variables.Get("verbosity", verbosity);
  m_variables.Get("IsData", isData);
  m_variables.Get("LuxArea", LUX_AREA);
  m_variables.Get("EtelArea", ETEL_AREA);
  m_variables.Get("HamamatsuArea", HAMAMATSU_AREA);
  m_variables.Get("WatchboyArea", WATCHBOY_AREA);
  m_variables.Get("WatchmanArea", WATCHMAN_AREA);
  m_variables.Get("PMTMRDOffset", PMTMRDOffset);
  m_variables.Get("Plot3D", plot3d);
  m_variables.Get("Draw3DFMV", draw3d_fmv);
  m_variables.Get("Draw3DMRD", draw3d_mrd);
  m_variables.Get("SaveHistograms", save_hists);
  m_variables.Get("StepSizeAi", step_size_ai);
  m_variables.Get("InsideAngle", insideAngle);
  m_variables.Get("OutsideAngle", outsideAngle);
  m_variables.Get("PMTChargeThreshold", PMTQCut);
  m_variables.Get("EtaThreshold", EtaThreshold);
  m_variables.Get("DisplayTruth", display_truth);
  m_variables.Get("RecoMode", reco_mode);
  m_variables.Get("AiEtaFile", aiEtaFile);
  m_variables.Get("TankTrackFitFile", tankTrackFitFile);
  m_variables.Get("UseNumLayers", use_nlyrs);
  m_variables.Get("UsePCA", use_pca);
  m_variables.Get("UseConnDots", use_conn_dots);
  m_variables.Get("UseELoss", use_eloss);
  m_variables.Get("UseSimpleEReco", use_simple_ereco);
  m_variables.Get("RecoEnergyShift", ERECO_SHIFT);

  Log("MuonFitter Tool: Version 240524v1", v_message, verbosity);
  if (use_nlyrs) Log("MuonFitter Tool: Using num layers to determine MRD track length", v_message, verbosity);
  if (use_pca) Log("MuonFitter Tool: Using PCA to determine MRD track angle", v_message, verbosity);
  if (use_conn_dots) Log("MuonFitter Tool: Using connect the dots method to determine MRD track angle", v_message, verbosity);
  if (use_eloss) Log("MuonFitter Tool: Using current ANNIE tools to determine MRD energy loss", v_message, verbosity);
  if (use_simple_ereco) Log("MuonFitter Tool: Just add tank and MRD energy depositions (don't update dEdx values)", v_message, verbosity);


  // Output ROOT file
  std::string outfile;
  m_variables.Get("OutputFile", outfile);
  Log("MuonFitter Tool: Saving output into " + outfile, v_message, verbosity);
  root_outp = new TFile(outfile.c_str(), "RECREATE");


  // -------------------------------------------------------------
  // --- Initialize canvases, graphs, histograms -----------------
  // -------------------------------------------------------------
  this->InitCanvases();
  this->InitHistograms();
  this->InitGraphs();


  // -------------------------------------------------------------
  // --- Retrieve Store variables --------------------------------
  // -------------------------------------------------------------
  m_data->CStore.Get("ChannelNumToTankPMTSPEChargeMap", ChannelKeyToSPEMap);  //same for data and mc


  // -------------------------------------------------------------
  // --- Get Geometry --------------------------------------------
  // -------------------------------------------------------------
  //-- Code based on UserTools/EventDisplay/EventDisplay.cpp
  auto get_geo = m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry", geom);
  if (!get_geo)
  {
    Log("MuonFitter Tool: Error retrieving Geometry from ANNIEEvent!", v_error, verbosity);
    return false;
  }
  tank_radius = geom->GetTankRadius();
  tank_height = geom->GetTankHalfheight();
  tank_height *= 2;
  detector_version = geom->GetVersion();
  Log("MuonFitter Tool: Using detector version " + std::to_string(detector_version), v_message, verbosity);
  double barrel_compression = 0.82;
  //-- Use compressed barrel radius for ANNIEp2v6 detector configuration (only MC)
  if (detector_config == "ANNIEp2v6" && !isData) { tank_height *= barrel_compression; }
  else if (isData) { tank_height = 1.2833; }
  //-- Set tank radius to standard value of old anniev2 configuration (v4/v6 seems to have very different radius?)
  if (tank_radius < 1. || isData) { tank_radius = 1.37504; }

  Position detector_center = geom->GetTankCentre();
  tank_center_x = 100.*detector_center.X();  //convert to cm
  tank_center_y = 100.*detector_center.Y();
  tank_center_z = 100.*detector_center.Z();
  //-- QA: Check tank center, radius, and height being used
  std::cout << " MuonFitter Tool: tank_center xyz [cm]: " << tank_center_x << "," << tank_center_y << "," << tank_center_z << std::endl;
  std::cout << " MuonFitter Tool: tank_radius [m]: " << tank_radius << std::endl;
  std::cout << " MuonFitter Tool: tank_height [m]: " << tank_height << std::endl;

  //-- QA: Check number of PMTs in each subdetector
  n_tank_pmts = geom->GetNumDetectorsInSet("Tank");
  n_mrd_pmts = geom->GetNumDetectorsInSet("MRD");
  n_veto_pmts = geom->GetNumDetectorsInSet("Veto");
  std::cout << " MuonFitter Tool: Number of Tank / MRD / Veto PMTs in this geometry: " << n_tank_pmts << " / " << n_mrd_pmts << " / " << n_veto_pmts << std::endl;


  // -------------------------------------------------------------
  // --- ANNIE in 3D ---------------------------------------------
  // -------------------------------------------------------------
  // --- Set up 3D geometry for viewing 
  Log("MuonFitter Tool: Creating 3D Geometry", v_debug, verbosity);
  ageom = new TGeoManager("ageom", "ANNIE in 3D");
  TGeoNode *node;

  //-- material
  vacuum = new TGeoMaterial("vacuum",0,0,0);
  Fe = new TGeoMaterial("Fe",55.845,26,7.87);

  //-- create media
  Air = new TGeoMedium("Vacuum",0,vacuum);
  Iron = new TGeoMedium("Iron",1,Fe);

  //-- create volume
  EXPH = ageom->MakeBox("EXPH",Air,300,300,300);
  ageom->SetTopVolume(EXPH);
  ageom->SetTopVisible(0);
  //-- If you want to see the boundary, input the number 1 instead of 0:
  // geom->SetTopVisible(1);

  //-- draw CANVAS/TOP CENTER
  bBlock = ageom->MakeSphere("EXPH_vol_center", Iron, 0,3,0,180,0,360);
  bBlock->SetLineColor(1);
  EXPH->AddNodeOverlap(bBlock,N++,new TGeoTranslation(0,0,0));

  //-- draw TANK
  TGeoVolume *annietank = ageom->MakeTubs("annietank", Iron, 0, tank_radius*100., tank_height*100., 0, 360);  //convert to cm
  annietank->SetLineColor(38);
  EXPH->AddNodeOverlap(annietank,N++,new TGeoCombiTrans(0,0,0,new TGeoRotation("annietank",0,90,0)));
  node = EXPH->GetNode(N-1);


  // -------------------------------------------------------------
  // --- Read in TANK PMTs ---------------------------------------
  // -------------------------------------------------------------
  //-- Code based on UserTools/EventDisplay/EventDisplay.cpp
  std::map<std::string, std::map<unsigned long, Detector *>> *Detectors = geom->GetDetectors();

  Log("MuonFitter Tool: Adding tank PMTs to 3D geometry", v_debug, verbosity);
  for (std::map<unsigned long, Detector *>::iterator it = Detectors->at("Tank").begin(); it != Detectors->at("Tank").end(); ++it)
  {
    Detector *apmt = it->second;
    unsigned long detkey = it->first;
    std::string det_type = apmt->GetDetectorType();
    Position position_PMT = apmt->GetDetectorPosition();  //in meters
    //-- PMT xyz corrected by tank center, in cm
    x_pmt.insert(std::pair<int,double>(detkey, 100.*position_PMT.X()-tank_center_x));
    y_pmt.insert(std::pair<int,double>(detkey, 100.*position_PMT.Y()-tank_center_y));
    z_pmt.insert(std::pair<int,double>(detkey, 100.*position_PMT.Z()-tank_center_z));
    //-- PMT orientation
    Direction direction_PMT = apmt->GetDetectorDirection();
    x_pmt_dir.insert(std::pair<unsigned long, double>(detkey, direction_PMT.X()));
    y_pmt_dir.insert(std::pair<unsigned long, double>(detkey, direction_PMT.Y()));
    z_pmt_dir.insert(std::pair<unsigned long, double>(detkey, direction_PMT.Z()));

    //-- Get PMT areas for ring area overlap later
    double det_area = 0.;
    if (det_type == "LUX" || det_type == "ANNIEp2v7-glassFaceWCPMT_R7081") { det_area = LUX_AREA; }
    else if (det_type == "ETEL" || det_type == "ANNIEp2v7-glassFaceWCPMT_D784KFLB") { det_area = ETEL_AREA; }
    else if (det_type == "Hamamatsu" || det_type == "ANNIEp2v7-glassFaceWCPMT_R5912HQE") { det_area = HAMAMATSU_AREA; }
    else if (det_type == "Watchboy" || det_type == "ANNIEp2v7-glassFaceWCPMT_R7081") { det_area = WATCHBOY_AREA; }
    else if (det_type == "Watchman" || det_type == "ANNIEp2v7-glassFaceWCPMT_R7081HQE") { det_area = WATCHMAN_AREA; }
    else { Log("MuonFitter Tool: Unrecognized detector type! Setting det_area to 0.", v_error, verbosity); }
    m_pmt_area.insert(std::pair<int, double>(detkey, det_area));

    //-- ANNIE in 3D: Drawing TANK PMTs
    sprintf(blockName,"tank_pmt%lu",detkey);
    bBlock = ageom->MakeSphere(blockName, Iron, 0,1.5,0,180,0,360);
    bBlock->SetLineColor(41);
    EXPH->AddNodeOverlap(bBlock,1,new TGeoTranslation(x_pmt[detkey], y_pmt[detkey], z_pmt[detkey]));
    detkey_to_node.insert(std::pair<unsigned long, int>(detkey, N++));
  }


  // -------------------------------------------------------------
  // --- Read in MRD PMTs ----------------------------------------
  // -------------------------------------------------------------
  //-- Code based on UserTools/EventDisplay/EventDisplay.cpp
  Log("MuonFitter Tool: Adding MRD PMTs to 3D geoemtry", v_debug, verbosity);
  for (std::map<unsigned long, Detector *>::iterator it = Detectors->at("MRD").begin(); it != Detectors->at("MRD").end(); ++it)
  {
    Detector *amrdpmt = it->second;
    unsigned long detkey = it->first;
    unsigned long chankey = amrdpmt->GetChannels()->begin()->first;
    Paddle *mrdpaddle = (Paddle *)geom->GetDetectorPaddle(detkey);

    //-- Retrieve xyz bounds of paddle, orientation, etc
    //-- NOTE: so far this has not been used
    double xmin = mrdpaddle->GetXmin();
    double xmax = mrdpaddle->GetXmax();
    double ymin = mrdpaddle->GetYmin();
    double ymax = mrdpaddle->GetYmax();
    double zmin = mrdpaddle->GetZmin();
    double zmax = mrdpaddle->GetZmax();
    int orientation = mrdpaddle->GetOrientation();    //0:horizontal,1:vertical
    int half = mrdpaddle->GetHalf();    //0 or 1
    int side = mrdpaddle->GetSide();

    std::vector<double> xdim{xmin,xmax};
    std::vector<double> ydim{ymin,ymax};
    std::vector<double> zdim{zmin,zmax};

    //-- Store the detkey and the bounds of the MRD paddle
    //-- NOTE: mrd_{xyz} is type std::vector<unsigned long, std::vector<double>>
    mrd_x.emplace(detkey,xdim);
    mrd_y.emplace(detkey,ydim);
    mrd_z.emplace(detkey,zdim);

    //-- Store detkey and paddle center xyz (tank center corrected)
    mrd_center_x.emplace(detkey,100.*(mrdpaddle->GetOrigin()).X()-tank_center_x);
    mrd_center_y.emplace(detkey,100.*(mrdpaddle->GetOrigin()).Y()-tank_center_y);
    mrd_center_z.emplace(detkey,100.*(mrdpaddle->GetOrigin()).Z()-tank_center_z);


    //-- ANNIE in 3D: drawing MRD
    if (draw3d_mrd)
    {
      Position position_MRD = mrdpaddle->GetOrigin();
      sprintf(blockName,"mrd_pmt%lu",detkey);
      bBlock = ageom->MakeBox(blockName, Iron, (xmax-xmin)*100., (ymax-ymin)*100., (zmax-zmin)*100.);
      bBlock->SetLineColor(16);
      EXPH->AddNodeOverlap(bBlock,detkey,new TGeoTranslation(100.*position_MRD.X()-tank_center_x, 100.*position_MRD.Y()-tank_center_y, 100.*position_MRD.Z()-tank_center_z));
      detkey_to_node.insert(std::pair<unsigned long, int>(detkey, N++));

      //-- QA: Check detkey_to_node map
      if (verbosity > v_debug)
      {
        std::cout << " [debug] blockName: " << blockName << std::endl;
        std::cout << " [debug] detkey_to_node[detkey]: " << detkey_to_node[detkey] << std::endl;
      }
    }
  }

  // ------------------------------------------------------------
  // --- Read in FMV PMTs ---------------------------------------
  // ------------------------------------------------------------
  //-- Code based on UserTools/EventDisplay/EventDisplay.cpp
  Log("MuonFitter Tool: Adding FMV PMTs to 3D geoemtry", v_debug, verbosity);
  for (std::map<unsigned long, Detector *>::iterator it = Detectors->at("Veto").begin(); it != Detectors->at("Veto").end(); ++it)
  {
    Detector *avetopmt = it->second;
    unsigned long detkey = it->first;
    unsigned long chankey = avetopmt->GetChannels()->begin()->first;
    Paddle *vetopaddle = (Paddle *)geom->GetDetectorPaddle(detkey);

    double xmin = vetopaddle->GetXmin();
    double xmax = vetopaddle->GetXmax();
    double ymin = vetopaddle->GetYmin();
    double ymax = vetopaddle->GetYmax();
    double zmin = vetopaddle->GetZmin();
    double zmax = vetopaddle->GetZmax();

    //-- ANNIE in 3D: drawing FMV
    if (draw3d_fmv)
    {
      Position position_FMV = vetopaddle->GetOrigin();
      sprintf(blockName,"fmv_pmt%lu",detkey);
      bBlock = ageom->MakeBox(blockName, Iron, (xmax-xmin)*100., (ymax-ymin)*100., (zmax-zmin)*100.);
      bBlock->SetLineColor(20);
      EXPH->AddNodeOverlap(bBlock,detkey,new TGeoTranslation(100.*position_FMV.X()-tank_center_x, 100.*position_FMV.Y()-tank_center_y, 100.*position_FMV.Z()-tank_center_z));
      detkey_to_node.insert(std::pair<unsigned long, int>(detkey, N++));

      //-- QA: Check detkey_to_node map
      if (verbosity > v_debug)
      {
        std::cout << " [debug] blockName: " << blockName << std::endl;
        std::cout << " [debug] detkey_to_node[detkey]: " << detkey_to_node[detkey] << std::endl;
      }
    }
  }

  //-- ANNIE in 3D: Set max number of nodes
  maxN = N;
  Log("MuonFitter Tool: Number of nodes in 3D geometry: " + std::to_string(maxN), v_debug, verbosity);


  //-- Save 3D plots
  ageom->CloseGeometry();   //close 3d geometry
  EXPH->SetVisibility(0);
  if (plot3d)
  {
    canvas_3d = new TCanvas("canvas_3d", "3D Event Display", 800, 600);
    canvas_3d->cd(1);
    EXPH->Draw();
    canvas_3d->Modified();
    canvas_3d->Update();
  }

  // ------------------------------------------------------------
  // --- Create txt files for separate analyses -----------------
  // ------------------------------------------------------------
  //-- Save start & stop vertices to file; Save (ai,eta) values
  //std::string pos_fname = "posFile.txt";
  //pos_file.open(pos_fname.c_str());
  //pos_file << "##evnum,startX,startY,startZ,stopX,stopY,stopZ" << std::endl;
  //std::string pos_fname = "ev_ai_eta.txt";
  std::string pos_fname = aiEtaFile.c_str();
  pos_file.open(pos_fname.c_str(), std::ostream::app);
  if (!pos_file)
  {
    std::cout << " [pos_file] File " << pos_fname << " does not exist! Creating now.." << std::endl;
    pos_file.open(pos_fname.c_str());
    pos_file << "##ev_id,cluster_time,ai,eta" << std::endl;
  }

  //-- Save avg eta to left and right of true tank track length
  if (!isData)
  {
    truetrack_file.open("true_track_len.txt", std::ostream::app);
    if (!truetrack_file)
    {
      std::cout << " [debug] true_track_len.txt does not exist! Creating now..." << std::endl;
      truetrack_file.open("true_track_len.txt");
      truetrack_file << "#event_id,true_track_len,left_avg,right_avg" << std::endl;
    }
  }


  //-- Save info about events with Ediff > 200 MeV
  if (!isData)
  {
    lg_ediff_file.open("lg_ediff.txt", std::ostream::app);
    if (!lg_ediff_file)
    {
      std::cout << "File lg_ediff.txt does not exist! Creating now..." << std::endl;
      lg_ediff_file.open("lg_ediff.txt");
      lg_ediff_file << "##event_id,ediff,pions,tanktrackF,tanktrackT,mrdtrackF,mrdtrackT,muonEF,muonET" << std::endl;
    }
  }

  // ------------------------------------------------------------
  // --- Load vertex fits (RECO MODE ONLY) ----------------------
  // ------------------------------------------------------------
  if (reco_mode)
  {
    if (this->FileExists(tankTrackFitFile))
    {
      //-- Load event part, number and track fit
      std::cout << "MuonFitter Tool: Loading tank track fits from " << tankTrackFitFile << std::endl;
      this->LoadTankTrackFits();
    }
    else
    {
      Log("MuonFitter Tool: File for tank track fit does not exist! Continuing tool NOT in RECO MODE.", v_error, verbosity);
      reco_mode = false;
    }
  }

  Log("MuonFitter Tool: Initialization complete", v_debug, verbosity);

  return true;
}


bool MuonFitter::Execute(){
  Log("MuonFitter Tool: Executing", v_debug, verbosity);

  //-- Retrieve ANNIEEvent
  int eventExists = m_data->Stores.count("ANNIEEvent");
  if (!eventExists)
  {
    Log("MuonFitter Tool: No ANNIEEvent store!", v_error, verbosity);
    return false;
  }

  // ------------------------------------------------------------
  // --- Reset variables ----------------------------------------
  // ------------------------------------------------------------
  this->ResetVariables();

  bool drawEvent = false;   //-- Make graphs for this event (e.g. ev displays, eta vs ai)

  gr_eta_ai->Set(0);
  gr_running_avg->Set(0);

  if (h_tzero) h_tzero->Delete();
  h_tzero = new TH1D("h_tzero", "t0", 60, -15, 15);
  h_tzero->GetXaxis()->SetTitle("[ns]");

  //-- Initialize tank track length and muon vertex to be saved in
  //-- CStore for downstream tools
  Position dummy_vtx(-888, -888, -888);
  m_data->CStore.Set("FittedTrackLengthInWater", -888.);
  m_data->CStore.Set("FittedMuonVertex", dummy_vtx);
  m_data->CStore.Set("RecoMuonKE", -888.);
  m_data->CStore.Set("NLyrs", -888);


  // -------------------------------------------------------------
  // --- Get event info (DATA) -----------------------------------
  // -------------------------------------------------------------
  int get_ok = false;
  if (isData)
  {
    get_ok = m_data->Stores["ANNIEEvent"]->Get("EventNumber", evnum);
    std::cout << "MuonFitter Tool: Working on event " << evnum << std::endl;
    if (not get_ok) { Log("MuonFitter Tool: Error retrieving EventNumber from ANNIEEvent!", v_error, verbosity); return false; }
    get_ok = m_data->Stores["ANNIEEvent"]->Get("RunNumber", runnumber);
    if (not get_ok) { Log("MuonFitter Tool: Error retrieving RunNumber from ANNIEEvent!", v_error, verbosity); return false; }
    get_ok = m_data->Stores["ANNIEEvent"]->Get("PartNumber", partnumber);
    if (not get_ok) { Log("MuonFitter Tool: Error retrieving PartNumber from ANNIEEvent!", v_error, verbosity); partnumber = -1; }
  }

  // -------------------------------------------------------------
  // --- Get event info (MC) -------------------------------------
  // -------------------------------------------------------------
  TVector3 trueTrackDir;
  RecoVertex *truevtx = 0;
  if (!isData)
  {
    get_ok = m_data->Stores["ANNIEEvent"]->Get("MCParticles", mcParticles);
    if (not get_ok) { Log("MuonFitter Tool: Error retrieving MCParticles from ANNIEEvent!", v_error, verbosity); return false; }  //<< needed to retrieve true vertex and direction
    get_ok = m_data->Stores["ANNIEEvent"]->Get("MCEventNum", mcevnum);
    std::cout << "MuonFitter Tool: Working on event " << mcevnum << std::endl;
    if (not get_ok) { Log("MuonFitter Tool: Error retrieving MCEventNum from ANNIEEvent!", v_error, verbosity); return false; }
    get_ok = m_data->Stores["ANNIEEvent"]->Get("MCTriggernum", mctrignum);
    if (not get_ok) { Log("MuonFitter Tool: Error retrieving MCTriggernum from ANNIEEvent!", v_error, verbosity); return false; }
    get_ok = m_data->Stores["ANNIEEvent"]->Get("MCFile", mcFile);
    if (not get_ok) { Log("MuonFitter Tool: Error retrieving MCFile from ANNIEEvent!", v_error, verbosity); mcFile = "-1"; }
 
    //-- Extract file part number
    //-- NOTE: event numbers weren't incrementing properly...
    std::string delim = ".";
    std::string end = ".root";
    std::string tmp_str = mcFile.erase(mcFile.rfind(delim), end.length());
    partnumber = stoi(tmp_str.substr(0, tmp_str.find(delim)));

    /*for(unsigned int particlei=0; particlei<mcParticles->size(); particlei++){

      MCParticle aparticle = mcParticles->at(particlei);
      std::string logmessage = "EventDisplay tool: Particle # "+std::to_string(particlei)+", parent ID = "+std::to_string(aparticle.GetParentPdg())+", pdg = "+std::to_string(aparticle.GetPdgCode())+", flag = "+std::to_string(aparticle.GetFlag());
      Log(logmessage,v_message,verbosity);
    }*/

    //-- Get RecoEvent variables
    get_ok = m_data->Stores["RecoEvent"]->Get("TrueVertex", truevtx);
    if (not get_ok) { Log("MuonFitter Tool: Error retrieving TrueVertex from RecoEvent Store!", v_error, verbosity); return false; }
    trueVtxX = truevtx->GetPosition().X();  //already in cm
    trueVtxY = truevtx->GetPosition().Y();  //and center corrected
    trueVtxZ = truevtx->GetPosition().Z();
    trueVtxTime = truevtx->GetTime();
    trueDirX = truevtx->GetDirection().X();
    trueDirY = truevtx->GetDirection().Y();
    trueDirZ = truevtx->GetDirection().Z();
    trueTrackDir = TVector3(trueDirX,trueDirY,trueDirZ).Unit();

    trueAngleRad = TMath::ACos(trueDirZ);  //calculated like this in other tools
    trueAngleDeg = trueAngleRad/(TMath::Pi()/180.);
    h_truevtx_angle->Fill(trueAngleDeg);

    RecoVertex *truestopvtx = 0;
    get_ok = m_data->Stores["RecoEvent"]->Get("TrueStopVertex", truestopvtx);
    if (not get_ok) { Log("MuonFitter Tool: Error retrieving TrueStopVertex from RecoEvent Store!", v_error, verbosity); return false; }
    trueStopVtxX = truestopvtx->GetPosition().X();  //already in cm
    trueStopVtxY = truestopvtx->GetPosition().Y();
    trueStopVtxZ = truestopvtx->GetPosition().Z();

    get_ok = m_data->Stores["RecoEvent"]->Get("TrueTrackLengthInWater", trueTrackLengthInWater);
    if (not get_ok) { Log("MuonFitter Tool: Error retrieving TrueTrackLengthInWater from RecoEvent Store!", v_error, verbosity); trueTrackLengthInWater = -1; }
    trueTrackLengthInWater = trueTrackLengthInWater*100.;   //convert to cm
    h_true_tanktrack_len->Fill(trueTrackLengthInWater);

    get_ok = m_data->Stores["RecoEvent"]->Get("TrueTrackLengthInMRD", trueTrackLengthInMRD);
    if (not get_ok) { Log("MuonFitter Tool: Error retrieving TrueTrackLengthInMRD from RecoEvent Store!", v_error, verbosity); trueTrackLengthInMRD = -1; }

    get_ok = m_data->Stores["RecoEvent"]->Get("TrueMuonEnergy", trueMuonEnergy);
    if (not get_ok) { Log("MuonFitter Tool: Error retrieving TrueMuonEnergy from RecoEvent Store!", v_error, verbosity); trueMuonEnergy = -1; }

    get_ok = m_data->Stores["RecoEvent"]->Get("NRings", nrings);
    if (not get_ok) { Log("MuonFitter Tool: Error retrieving NRings, true from RecoEvent!", v_error, verbosity); }
    get_ok = m_data->Stores["RecoEvent"]->Get("IndexParticlesRing", particles_ring);
    if (not get_ok) { Log("MuonFitter Tool: Error retrieving IndexParticlesRing, true from RecoEvent!", v_error, verbosity); }
  }

  //-- Get ev_id for matching
  std::stringstream ev_id;
  ev_id << "p" << partnumber << "_";
  if (isData) ev_id << evnum;
  else ev_id << mcevnum;
  std::cout << "MuonFitter Tool: Working on event " << ev_id.str() << std::endl;

  if (reco_mode)
  {
    //-- Skip events that weren't fitted to make processing faster
    std::map<std::string, std::vector<double>>::iterator it = m_tank_track_fits.find(ev_id.str());
    if (it == m_tank_track_fits.end()) return true;
  }

  // ------------------------------------------------------------
  // --- Get event info (both) ----------------------------------
  // ------------------------------------------------------------
  uint32_t trigword;
  get_ok = m_data->Stores.at("ANNIEEvent")->Get("TriggerWord",trigword);
  std::cout << " trigword: " << trigword << std::endl;


  // ------------------------------------------------------------
  // --- CUT: Select beam only events ---------------------------
  // ------------------------------------------------------------
  if (trigword != 5) return true;


  // ------------------------------------------------------------
  // --- Check for particles other than muon (MC ONLY) ----------
  // ------------------------------------------------------------
  bool hasPion = false;
  int n_rings = 0;
  if (!isData)
  {
    for (unsigned int mcp_i = 0; mcp_i < mcParticles->size(); mcp_i++)
    {
      MCParticle aparticle = mcParticles->at(mcp_i);
      //-- QA: Check MC evnum and particle PDG code
      //std::cout << " [debug] Ev " << mcevnum << ", particle pdg code: " << aparticle.GetPdgCode() << std::endl;
      if (std::find(particles_ring.begin(), particles_ring.end(), mcp_i) != particles_ring.end())
      {
        //-- Use number of rings to determine if there are particles other than muon
        ++n_rings;
      }
    }
    std::cout << " [debug] n_rings (my counter): " << n_rings << std::endl;
    if (n_rings > 1) hasPion = true;  //might include other particles
  }
  if (hasPion) std::cout << " [debug] has pion / other particle!" << std::endl;


  // ------------------------------------------------------------
  // --- CUT: Check for FMV hits --------------------------------
  // ------------------------------------------------------------
  get_ok = m_data->Stores["ANNIEEvent"]->Get("TDCData", tdcdata);   //'TDCData' for both data and MC
  if (!get_ok) { Log("MuonFitter Tool: No TDCData object in ANNIEEvent! Abort!", v_error, verbosity); return true; }

  bool passVetoCut = false;
	get_ok = m_data->Stores.at("RecoEvent")->Get("NoVeto", passVetoCut);
  if (!get_ok) { Log("MuonFitter Tool: Could not retrieve NoVeto variable", v_error, verbosity); }

  //-- Skip event if there is a veto hit
  //-- NOTE: Will let EventSelector tool determine whether FMV was hit
  //--        since it uses timing information
  if (!passVetoCut)
  {
    Log("MuonFitter Tool: Found FMV/Veto hit!", v_debug, verbosity);
    return true;
  }


  // ------------------------------------------------------------
  // --- CUT: Check for MRD tracks ------------------------------
  // ------------------------------------------------------------
  //get_ok = m_data->Stores["ANNIEEvent"]->Get("MRDTriggerType", mrdTriggerType);  //XXX:care about this? not defined yet
  get_ok = m_data->Stores["MRDTracks"]->Get("NumMrdTracks", numTracksInEv);
  get_ok = m_data->Stores["MRDTracks"]->Get("MRDTracks", mrdTracks);    //XXX:might need MC version

  if (!get_ok) { Log("MuonFitter Tool: Couldn't retrieve MRD tracks info. Did you run TimeClustering/FindMRDTracks first?", v_debug, verbosity); return false; }

  //-- Skip event if num tracks not equal to 1 in MRD
  if (numTracksInEv != 1)
  {
    Log("MuonFitter Tool: More than 1 reconstructed track found!", v_debug, verbosity);
    return true;
  }


  // ------------------------------------------------------------
  // --- Get MRD track params -----------------------------------
  // ------------------------------------------------------------
  double reco_mrd_track = 0.;
  double nlyrs_mrd_track = 0.;
  double conn_dots_mrd_track = 0.;
  bool isMrdStopped;
  bool isMrdPenetrating;
  bool isMrdSideExit;

  for(int track_i = 0; track_i < numTracksInEv; track_i++)
  {
    BoostStore* thisTrackAsBoostStore = &(mrdTracks->at(track_i));
    
    thisTrackAsBoostStore->Get("StartVertex", mrdStartVertex);  //m
    thisTrackAsBoostStore->Get("StopVertex", mrdStopVertex);    //m
    thisTrackAsBoostStore->Get("MrdEntryPoint", mrdEntryPoint); //m
    thisTrackAsBoostStore->Get("TankExitPoint", tankExitPoint); //m
    thisTrackAsBoostStore->Get("StartTime", mrdStartTime);
    thisTrackAsBoostStore->Get("TrackAngle", trackAngleRad);
    thisTrackAsBoostStore->Get("TrackAngleError", trackAngleError);
    thisTrackAsBoostStore->Get("PenetrationDepth", penetrationDepth);
    thisTrackAsBoostStore->Get("NumLayersHit", numLayersHit);
    thisTrackAsBoostStore->Get("LayersHit", LayersHit);
    thisTrackAsBoostStore->Get("PMTsHit", MrdPMTsHit);
    thisTrackAsBoostStore->Get("EnergyLoss", mrdEnergyLoss);
    thisTrackAsBoostStore->Get("EnergyLossError", mrdEnergyLossError);
    thisTrackAsBoostStore->Get("IsMrdStopped", isMrdStopped);
    thisTrackAsBoostStore->Get("IsMrdPenetrating", isMrdPenetrating);
    thisTrackAsBoostStore->Get("IsMrdSideExit", isMrdSideExit);

    //-- Calculate MRD track length using MRD track start and stop
    //-- NOTE: this was done in other tools
    reco_mrd_track = sqrt(pow((mrdStopVertex.X()-mrdStartVertex.X()),2)+pow(mrdStopVertex.Y()-mrdStartVertex.Y(),2)+pow(mrdStopVertex.Z()-mrdStartVertex.Z(),2));

    //-- Do some conversions
    reco_mrd_track = reco_mrd_track*100.;       //convert to cm
    penetrationDepth = penetrationDepth*100.;
    trackAngleDeg = trackAngleRad*180./TMath::Pi();

    //-- QA: Check MRD track variables
    std::cout << "  [debug] mrdStartVertex: " << mrdStartVertex.X() << "," << mrdStartVertex.Y() << "," << mrdStartVertex.Z() << std::endl;
    std::cout << "  [debug] mrdStopVertex: " << mrdStopVertex.X() << "," << mrdStopVertex.Y() << "," << mrdStopVertex.Z() << std::endl;
    std::cout << "  [debug] tankExitPoint: " << tankExitPoint.X() << "," << tankExitPoint.Y() << "," << tankExitPoint.Z() << std::endl;
    std::cout << "  [debug] mrdEntryPoint: " << mrdEntryPoint.X() << "," << mrdEntryPoint.Y() << "," << mrdEntryPoint.Z() << std::endl;
    std::cout << "  [debug] mrdStartTime: " << mrdStartTime << std::endl;
    std::cout << "  [debug] trackAngleRad: " << trackAngleRad << std::endl;
    std::cout << "  [debug] penetrationDepth: " << penetrationDepth << std::endl;
    std::cout << "  [debug] reco_mrd_track (ANNIE tools): " << reco_mrd_track << std::endl;
    std::cout << "  [debug] mrdEnergyLoss: " << mrdEnergyLoss << std::endl;
    std::cout << "  [debug] isMrdStopped: " << isMrdStopped << std::endl;
    std::cout << "  [debug] isMrdPenetrating: " << isMrdPenetrating << std::endl;
    std::cout << "  [debug] isMrdSideExit: " << isMrdSideExit << std::endl;
    std::cout << "  [debug] numLayersHit: " << numLayersHit << std::endl;
    std::cout << "  [debug] LayersHit.size(): " << LayersHit.size() << std::endl;
    std::cout << "  [debug] MrdPMTsHit.size(): " << MrdPMTsHit.size() << std::endl;

  } //-- Done retrieving MRD track params

  //-- QA: Check the values of LayersHit vector
  for (int i = 0; i < LayersHit.size(); ++i)
  {
    std::cout << " [debug] LayersHit at " << i << ": " << LayersHit.at(i) << std::endl;
  } //-- returns numbers from 0-11 (layer number ordered)

  //-- Histogram number of layers traversed
  h_num_mrd_layers->Fill(LayersHit.size());


  //-- QA: Check the values of MrdPMTsHit vector
  for (int i = 0; i < MrdPMTsHit.size(); ++i)
  {
    std::cout << " [debug] MrdPMTsHit at " << i << ": " << MrdPMTsHit.at(i) << std::endl;
  } //-- returns unsorted list of MRD detkeys


  //-- Use num layers to determine amount of track length in iron
  //-- NOTE: this is an "effective" track length
  //-- (5cm)*(num layers)/cos(theta) << 5cm is thickness of iron slab
  nlyrs_mrd_track = 5.*(LayersHit.size()+0.5)/abs(TMath::Cos(trackAngleRad));

  //-- Compare MRD tracks reconstructed w/ ANNIE methods and num layers
  double pcaAngleRad = this->PCATrackAngle(MrdPMTsHit);
  double pcaAngleDeg = pcaAngleRad*180./TMath::Pi();
  std::cout << " [debug] PCA angle (rad): " << pcaAngleRad << std::endl;
  std::cout << " [debug] PCA angle (deg): " << pcaAngleDeg << std::endl;
  h_pca_angle->Fill(pcaAngleDeg);

  //-- Update nlyrs_mrd_track so that it uses the PCA-reconstructed track angle
  double pca_mrd_track = 5.*(LayersHit.size()+0.5)/abs(TMath::Cos(pcaAngleRad));
  std::cout << " [debug] PCA nlyrs_mrd_track: " << pca_mrd_track << std::endl;
  if (use_pca) { nlyrs_mrd_track = pca_mrd_track; }


  //TODO:Move to separate function
  //-- Determine MRD track length by "connecting the dots"
  conn_dots_mrd_track = this->MRDConnectDots(MrdPMTsHit);
  std::cout << " [debug] FINAL conn_dots_mrd_track: " << conn_dots_mrd_track << std::endl;
  std::cout << " [debug] FINAL nlyrs_mrd_track: " << nlyrs_mrd_track << std::endl;


  //-- Select for tracks that end in MRD
  if (!isMrdStopped)
  {
    std::cout << " MuonFitter Tool: MUON DID NOT STOP IN MRD. Skipping..." << std::endl;
    return true;
  }

  if (isMrdSideExit)  //<< Not working
  {
    std::cout << " MuonFitter Tool: MUON EXITTED SIDE OF MRD. Skipping..." << std::endl;
    //return true;
  }


  // ------------------------------------------------------------
  // --- Now look at tank exit and MRD points -------------------
  // ------------------------------------------------------------
  double tankExitPointX = 100.*tankExitPoint.X()-tank_center_x;
  double tankExitPointY = 100.*tankExitPoint.Y()-tank_center_y;
  double tankExitPointZ = 100.*tankExitPoint.Z()-tank_center_z;
  TVector3 tankExit(tankExitPointX, tankExitPointY, tankExitPointZ);
  h_tankExitX->Fill(tankExitPointX);
  h_tankExitY->Fill(tankExitPointY);
  h_tankExitZ->Fill(tankExitPointZ);

  double mrdEntryPointX = 100.*mrdEntryPoint.X()-tank_center_x;
  double mrdEntryPointY = 100.*mrdEntryPoint.Y()-tank_center_y;
  double mrdEntryPointZ = 100.*mrdEntryPoint.Z()-tank_center_z;

  //-- Create vectors for MRD start/stop
  TVector3 mrdStart(100.*mrdStartVertex.X()-tank_center_x, 100.*mrdStartVertex.Y()-tank_center_y, 100.*mrdStartVertex.Z()-tank_center_z);   //cm
  TVector3 mrdStop(100.*mrdStopVertex.X()-tank_center_x, 100.*mrdStopVertex.Y()-tank_center_y, 100.*mrdStopVertex.Z()-tank_center_z);
  //-- Get track direction from MRD start/stop
  TVector3 mrdTrackDir = (mrdStop - mrdStart).Unit();
  //-- QA:Compare angle from mrdTrackDir to trackAngleRad
  std::cout << " [debug] mrdTrackDir, trackAngleRad: " << mrdTrackDir.Angle(TVector3(0,0,1)) << "," << trackAngleRad << std::endl;

  //-- ANNIE in 3D: Add mrdStart and mrdStop to 3D geo
  if (isData) sprintf(blockName,"mrdStart_%u",evnum);
  else sprintf(blockName,"mrdStart_%u",mcevnum);
  bBlock = ageom->MakeSphere(blockName, Iron, 0,1.5,0,180,0,360);
  bBlock->SetLineColor(8);
  EXPH->AddNodeOverlap(bBlock,69,new TGeoTranslation(mrdStart.X(), mrdStart.Y(), mrdStart.Z()));
  N++;

  if (isData) sprintf(blockName,"mrdStop_%u",evnum);
  else sprintf(blockName,"mrdStop_%u",mcevnum);
  bBlock = ageom->MakeSphere(blockName, Iron, 0,1.5,0,180,0,360);
  bBlock->SetLineColor(2);
  EXPH->AddNodeOverlap(bBlock,69,new TGeoTranslation(mrdStop.X(), mrdStop.Y(), mrdStop.Z()));
  N++;


  // ------------------------------------------------------------
  // --- Calculate several vertex candidates --------------------
  // ------------------------------------------------------------
  //-- This is kind of like an angle cut
  std::vector<TVector3> vtxCandidates;
  int c = 1;
  bool outsideTank = false;
  bool inFV = false;   //FV around tank center
  while (!outsideTank)
  {
    TVector3 v = mrdStart - c*10.*mrdTrackDir;  //cm
    if (c <= 5) { vtxCandidates.push_back(v); }   //get into tank first
    else if (c > 5)
    {
      //-- Check that vtx is contained in tank cylinder
      if ((pow(v.X(),2) + pow(v.Z(),2) >= pow(tank_radius*100.,2)) || (v.Y() > tank_height*100.) || (v.Y() < -tank_height*100.))
      {
        outsideTank = true;
      }
      else { vtxCandidates.push_back(v); }

      //-- Check if any vtx intersects FV around center
      if ((v.Y() > -100.)&&(v.Y() < 100.)&&(100. > std::sqrt(v.X()*v.X() + v.Z()*v.Z())))
      {
        inFV = true;
      }
    }
    c++;
  } //-- Done calculating vtx candidates

  //-- CUT: Make sure track originates in tank
  if (!inFV) 
  {
    std::cout << "MuonFitter Tool: Not in FV!" << std::endl;
    return true;
  }

  std::cout << "MuonFitter Tool: Num of vtxCandidates: " << vtxCandidates.size() << std::endl;

  //-- Check distance btwn vertices to make sure it's 10cm
/*  for (int i = 1; i < vtxCandidates.size(); ++i)
  {
    TVector3 v0(vtxCandidates.at(i-1));
    TVector3 v1(vtxCandidates.at(i));
    double d = TMath::Sqrt(pow(v1.X()-v0.X(),2) + pow(v1.Y()-v0.Y(),2) + pow(v1.Z()-v0.Z(),2));
    std::cout << "  [debug] distance btwn candidate vtx: " << d << std::endl;
  } //-- yes, it's 10cm */


  // ------------------------------------------------------------
  // --- Load Tank Clusters -------------------------------------
  // ------------------------------------------------------------
  bool has_clusters = false;
  if (isData) { has_clusters = m_data->CStore.Get("ClusterMap", m_all_clusters); }
  else { has_clusters = m_data->CStore.Get("ClusterMapMC", m_all_clusters_MC); }
  if (!has_clusters)
  {
    std::cout << " MuonFitter Tool: No clusters found in CStore! Did you run ClusterFinder tool before this?" << std::endl;
    return false;
  }

  has_clusters = m_data->CStore.Get("ClusterMapDetkey", m_all_clusters_detkeys);  //same for data and mc
  if (!has_clusters)
  {
    std::cout << " MuonFitter Tool: No cluster detkeys found in CStore! Did you run ClusterFinder tool before this?" << std::endl;
    return false;
  }

  Log("MuonFitter Tool: Accessing pairs in all_clusters map", v_debug, verbosity);
  int cluster_num = 0;
  int cluster_size = 0;
  if (isData) { cluster_size = (int)m_all_clusters->size(); }
  else { cluster_size = (int)m_all_clusters_MC->size(); }
  std::cout << " [debug] cluster_size (num clusters in event): " << cluster_size << std::endl;


  // ------------------------------------------------------------
  // --- Find main cluster when more than one cluster -----------
  // ------------------------------------------------------------
  //-- Max charge and in [0..2000ns] time window
  //-- Code based on UserTools/EventDisplay/EventDisplay.cpp
  bool found_muon = false;
  double main_cluster_time = 0;
  double main_cluster_charge = 0;
  double main_cluster_hits = 0;

  //bool found_muon = false;
  double earliest_hittime = 0;
  std::vector<double> v_cluster_times;
  if (isData)
  {
    for (std::pair<double, std::vector<Hit>>&& apair : *m_all_clusters)
    {
      std::vector<Hit>& cluster_hits = apair.second;
      double temp_time = 0;
      double temp_charge = 0;
      int temp_hits = 0;

      for (int ihit = 0; ihit < cluster_hits.size(); ihit++)
      {
        temp_hits++;
        temp_time += cluster_hits.at(ihit).GetTime();
        temp_charge += cluster_hits.at(ihit).GetCharge();

        //XXX: check single cluster hit positions
        int chankey = cluster_hits.at(ihit).GetTubeId();
        std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(chankey);
        if (it != ChannelKeyToSPEMap.end())
        {
          Detector* this_detector = geom->ChannelToDetector(chankey);
          unsigned long detkey = this_detector->GetDetectorID();  //chankey same as detkey
        }
      }
      if (temp_hits > 0) temp_time /= temp_hits;  //mean time
      std::cout << " [debug] temp_time [ns]: " << temp_time << std::endl;
      if (temp_time > 2000. || temp_time < 0.) continue;  // not in time window
      if (temp_charge > main_cluster_charge)
      {
        found_muon = true;
        main_cluster_charge = temp_charge;  //sum of all charges
        main_cluster_time = apair.first;    //same as mean time above
        main_cluster_hits = cluster_hits.size();
      }
    }
  }
  else  //is MC
  {
    for (std::pair<double, std::vector<MCHit>>&& apair : *m_all_clusters_MC)
    {
      std::vector<MCHit>& cluster_hits_MC = apair.second;
      double temp_time = 0;
      double temp_charge = 0;
      int temp_hits = 0;
      std::vector<double> v_cluster_hits;

      for (int ihit = 0; ihit < cluster_hits_MC.size(); ihit++)
      {
        temp_hits++;
        v_cluster_hits.push_back(cluster_hits_MC.at(ihit).GetTime());
        temp_time += cluster_hits_MC.at(ihit).GetTime();
        temp_charge += cluster_hits_MC.at(ihit).GetCharge();

        //XXX: check single cluster hit positions
        int chankey = cluster_hits_MC.at(ihit).GetTubeId();
        std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(chankey);
        if (it != ChannelKeyToSPEMap.end())
        {
          Detector* this_detector = geom->ChannelToDetector(chankey);
          unsigned long detkey = this_detector->GetDetectorID();  //chankey same as detkey

          //-- QA: Check if coordinates are the same. If so, just use existing map
          Position det_pos = this_detector->GetDetectorPosition();
        }
      }

      //-- Sort cluster hit times to determine diff btwn earliest and latest times
      sort(v_cluster_hits.begin(), v_cluster_hits.end());
      std::cout << " [debug] all MC cluster hit times (sorted): ";
      for (int ct = 0; ct < v_cluster_hits.size(); ++ct)
      {
        std::cout << v_cluster_hits.at(ct) << ",";
      }
      std::cout << std::endl;
      h_clusterhit_timespread->Fill(v_cluster_hits.at(v_cluster_hits.size()-1)-v_cluster_hits.at(0));

      if (temp_hits > 0) temp_time /= temp_hits;  //mean time
      v_cluster_times.push_back(temp_time);
      if (temp_time > 2000. || temp_time < 0.) continue;  // not in time window
      if (temp_charge > main_cluster_charge)
      {
        found_muon = true;
        main_cluster_charge = temp_charge;
        main_cluster_time = apair.first;    //same as mean time above
        main_cluster_hits = cluster_hits_MC.size();
        earliest_hittime = v_cluster_hits.at(0);
        std::cout << " [debug] earliest_hittime: " << earliest_hittime << std::endl;
      }
    }
  }
  //-- QA: Check cluster times, main cluster time, charge, num hits
  std::cout << " [debug] all cluster times: ";
  for (int ct = 0; ct < v_cluster_times.size(); ++ct)
  {
    std::cout << v_cluster_times.at(ct) << ",";
  }
  std::cout << std::endl;
  std::cout << " [debug] main_cluster_time [ns]: " << main_cluster_time << std::endl;
  std::cout << " [debug] main_cluster_charge [nC/pe]: " << main_cluster_charge << std::endl;
  std::cout << " [debug] main_cluster_hits [#]: " << main_cluster_hits << std::endl;


  // ------------------------------------------------------------
  // --- Check coincidence tank and MRD activity ----------------
  // ------------------------------------------------------------
  //-- Select events that are within +/- 50ns of PMTMRDOffset
  //-- in data: PMTMRDOffset ~745ns; in MC: PMTMRDOffset = 0
  //-- Code based on UserTools/EventSelector/EventSelector.cpp
  double tankmrd_tdiff = mrdStartTime - main_cluster_time;
  std::cout << " MuonFitter Tool: Time difference between tank and MRD activity [ns]: " << tankmrd_tdiff << std::endl;
  if ((tankmrd_tdiff > PMTMRDOffset-50) && (tankmrd_tdiff < PMTMRDOffset+50))
  {
    std::cout << "MuonFitter Tool: Main cluster is coincident with MRD cluster!" << std::endl;
  }
  else
  {
    found_muon = false;
    std::cout << "MuonFitter Tool: Main cluster is NOT coincident with MRD cluster! "
              << "Tank cluster and MRD cluster times are too different." << std::endl;
  }

  //-- Save num hits and charge of main/max cluster
  //-- NOTE: in MC, charge is given in PE
  //h_total_pe_hits->Fill(main_cluster_hits, main_cluster_charge);


  // ------------------------------------------------------------
  // --- FOUND MUON CANDIDATE -----------------------------------
  // ------------------------------------------------------------
  double max_eta = 0.;  //for plotting

  if (!reco_mode)
  {
  if (found_muon)
  {
    std::cout << "MuonFitter Tool: Found muon candidate! Event: p" << partnumber << "_";
    if (isData) std::cout << evnum;
    else std::cout << mcevnum;
    std::cout << std::endl;
    drawEvent = true;

    //-- Save nhits, cluster_charge to txt files
    //-- TODO: get rid of this
    /*pehits_file << "p" << partnumber << "_";
    if (isData) pehits_file << evnum;
    else pehits_file << mcevnum;
    pehits_file << "," << main_cluster_hits << "," << main_cluster_charge << std::endl;*/

    //-- Save main cluster charge
    h_clusterPE->Fill(main_cluster_charge);


    // ------------------------------------------------------------
    // --- Load MAIN cluster hits and detkeys ---------------------
    // ------------------------------------------------------------
    std::vector<Hit> cluster_hits;
    std::vector<MCHit> cluster_hits_MC;
    if (isData) { cluster_hits = m_all_clusters->at(main_cluster_time); }
    else { cluster_hits_MC = m_all_clusters_MC->at(main_cluster_time); }
    std::vector<unsigned long> cluster_detkeys = m_all_clusters_detkeys->at(main_cluster_time);
    std::vector<double> x_hits, y_hits, z_hits;

    // ------------------------------------------------------------
    // --- Ring Imaging: Calculate ai for each PMT ----------------
    // ------------------------------------------------------------
    std::map<unsigned long, double> charge;   //-- used for making charge cuts at PMT level
    std::map<unsigned long, std::vector<double>> hittime;   //-- used for t0 calc
    std::map<int, std::vector<double>> m_PE_ai;
    std::map<int, std::vector<double>> m_fpmt_ai;

    charge.clear();
    hittime.clear();
    for (unsigned long detkey = 332; detkey < 464; ++detkey)
    {
      charge.emplace(detkey, 0.);
      hittime.emplace(detkey, std::vector<double>{-99.});
    }

    //-- First collect all the charges seen by the PMT in the cluster
    if (isData)
    {
      for (int i = 0; i < (int)cluster_hits.size(); ++i)
      {
        //for each cluster hit
        int chankey = cluster_hits.at(i).GetTubeId();
        std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(chankey);
        if (it != ChannelKeyToSPEMap.end())
        {
          Detector* this_detector = geom->ChannelToDetector(chankey);
          unsigned long detkey = this_detector->GetDetectorID();  //chankey same as detkey
          double hit_charge = cluster_hits.at(i).GetCharge();
          double hit_PE = hit_charge / ChannelKeyToSPEMap.at(chankey);
          double hit_time = cluster_hits.at(i).GetTime();
          hittime[detkey].push_back(hit_time);

          // keep track of total charge seen by each PMT
          charge[detkey] += hit_PE;

        } //end if ChannelKeyToSPEMap
      } //end cluster_hits loop
    }
    else  //is MC
    {
      for (int i = 0; i < (int)cluster_hits_MC.size(); ++i)
      {
        //for each cluster hit
        int chankey = cluster_hits_MC.at(i).GetTubeId();
        std::map<int, double>::iterator it = ChannelKeyToSPEMap.find(chankey);
        if (it != ChannelKeyToSPEMap.end())
        {
          Detector* this_detector = geom->ChannelToDetector(chankey);
          unsigned long detkey = this_detector->GetDetectorID();  //chankey same as detkey
          double hit_PE = cluster_hits_MC.at(i).GetCharge();  //charge in MC is in PE
          double hit_charge = hit_PE * ChannelKeyToSPEMap.at(chankey);
          double hit_time = cluster_hits_MC.at(i).GetTime();
          hittime[detkey].push_back(hit_time);

          // keep track of total charge seen by each PMT
          charge[detkey] += hit_PE;

        } //end if ChannelKetToSPEMap
      } //end cluster_hits_MC loop
    }

    //-- Now go through charge, hittime maps
    int gi = 0;    //counter for TGraph
    int nhits = 0, nhits_incone = 0;
    double totalpe = 0, totalpe_incone = 0;
    std::vector<double> v_tzero;

    for (unsigned long detkey = 332; detkey < 464; ++detkey)
    {
      //-- Get the charge for this PMT
      double pmt_PE = charge[detkey];
      
      //-- NOTE: there are PMTs w/ 0pe from charge map initialization
      if (pmt_PE == 0) continue;

      std::vector<double> v_hittimes = hittime[detkey];
      double pmt_t = 0;
      int nhits_pmt = v_hittimes.size();
      std::cout << " [debug] all hit times for detkey " << detkey << ": ";
      for (int ht = 0; ht < nhits_pmt; ++ht) { std::cout << v_hittimes.at(ht) << ","; }
      std::cout << std::endl;

      //-- Get earliest hit time bc Cherenkov radiation is usu earliest
      sort(v_hittimes.begin(), v_hittimes.end());
      pmt_t = v_hittimes.at(0);
      if (pmt_t == -99.) pmt_t = v_hittimes.at(1);
      std::cout << " MuonFitter Tool: Earliest time for this PMT: " << pmt_t << std::endl;
      

      //-- CUT: Skip PMTs in this cluster that don't see enough light
      if (pmt_PE < PMTQCut)
      {
        std::cout << " MuonFitter Tool: SKIPPING charge[" << detkey << "] < " << PMTQCut << "pe: " << charge[detkey] << std::endl;
        pmt_PE = 0.;    //set PMT charge to 0
        continue;       //skip PMT entirely
      }

      //-- Keep track of total hits, PE that meet cut in this cluster
      nhits += 1;
      totalpe += pmt_PE;

      //-- Get position and direction of PMT
      double hitX = x_pmt[detkey];
      double hitY = y_pmt[detkey];
      double hitZ = z_pmt[detkey];
      double dirPMTX = x_pmt_dir[detkey];
      double dirPMTY = y_pmt_dir[detkey];
      double dirPMTZ = z_pmt_dir[detkey];
      
      TVector3 pmt_dir = TVector3(dirPMTX, dirPMTY, dirPMTZ).Unit();

      //-- Get vector from tankExitPoint to PMT (Ri)
      TVector3 vec_Ri = TVector3(hitX,hitY,hitZ) - tankExit;
      double Ri = vec_Ri.Mag();
      h_tankexit_to_pmt->Fill(Ri);
      //-- QA: Check that vector was calculated correctly
      std::cout << " [debug] hitXYZ: " << hitX << "," << hitY << "," << hitZ << std::endl;
      std::cout << " [debug] tankExitXYZ: " << tankExit.X() << "," << tankExit.Y() << "," << tankExit.Z() << std::endl;
      std::cout << " [debug] vec_Ri: " << vec_Ri.X() << "," << vec_Ri.Y() << "," << vec_Ri.Z() << std::endl;
      std::cout << " [debug] vec_Ri direction: " << vec_Ri.Unit().X() << "," << vec_Ri.Unit().Y() << "," << vec_Ri.Unit().Z() << std::endl;

      //-- Get angle btwn Ri and muon direction (ai)
      double ang_alpha = vec_Ri.Angle(-mrdTrackDir);  //rad
      double ai = Ri * TMath::Sin(ang_alpha) / TMath::Tan(CHER_ANGLE_RAD) + Ri * TMath::Cos(ang_alpha);
      h_tanktrack_ai->Fill(ai);

      //-- Get the vector from vertex of ai to the PMT (bi)
      TVector3 vec_ai = tankExit - ai*mrdTrackDir;
      TVector3 vec_bi = TVector3(hitX,hitY,hitZ) - vec_ai;
      double bi = TMath::Sqrt(pow(hitX-vec_ai.X(),2) + pow(hitY-vec_ai.Y(),2) + pow(hitZ-vec_ai.Z(),2));
      //-- QA: Check that vector was calculated correctly
      std::cout << " [debug] vec_ai: " << vec_ai.X() << "," << vec_ai.Y() << "," << vec_ai.Z() << std::endl;
      std::cout << " [debug] ai: " << ai << std::endl;
      std::cout << " [debug] vec_bi: " << vec_bi.X() << "," << vec_bi.Y() << "," << vec_bi.Z() << std::endl;
      std::cout << " [debug] bi: " << bi << std::endl;


      //-- Find angle btwn true vertex and hit (MC ONLY)
      //-- not sure what is being done w this rn
      if (!isData)
      {
        TVector3 vec_truevtx_hit = TVector3(hitX,hitY,hitZ) - TVector3(trueVtxX,trueVtxY,trueVtxZ);
        double anglePmtTrueVtx = vec_truevtx_hit.Angle(mrdTrackDir)*180./TMath::Pi();
      }

      //-- Get angle btwn vector bi and PMT direction
      double psi = vec_bi.Angle(-pmt_dir);    //used only to get angle; don't use for magnitude
      if (psi > TMath::Pi()/2.) psi = TMath::Pi()-psi;
      //-- QA: Check that angle was calculated correctly
      //std::cout << " [debug] psi(+), psi(-): " << psi*180./TMath::Pi() << ", " << vec_bi.Angle(pmt_dir)*180./TMath::Pi() << std::endl;

      //-- Calculate the area of PMT & frustum (ring)
      double eff_area_pmt = 0.5 * m_pmt_area[detkey] * (1. + TMath::Cos(psi));  // effective area seen by photons from emission point
      h_eff_area_pmt->Fill(eff_area_pmt);
      double area_frustum = 2. * TMath::Pi() * step_size_ai * Ri;
      double f_pmt = eff_area_pmt / area_frustum;   //fraction of photocathode of total frustrum (ring) area
      h_fpmt->Fill(f_pmt);


      //-- For each distance (ai) from tank exit point bin, collect the
      //-- charges associated w/ this distance
      for (int a = 55; a < 500; a+=step_size_ai)
      {
        if (ai >= a-step_size_ai/2 && ai < a+step_size_ai/2)
        {
          m_PE_ai[a].push_back(pmt_PE);
          m_fpmt_ai[a].push_back(f_pmt);
        }
      }

      ++gi;

    } //-- Done going through charge[detkey] map

    // ------------------------------------------------------------
    // -- Make eta vs ai (tank track segment) graphs for fitting --
    // ------------------------------------------------------------
    //-- eta represents photon density. In principle, it should be 
    //-- a high, constant value inside Cherenkov disc, and should 
    //-- drop off once outside the disc
    int j = 0;
    double running_avg = 0;

    //-- Go thru ai and PMT area fraction maps and add up fractions
    //-- and charges seen at each track segment (ai)
    for (auto const &pair: m_fpmt_ai)
    {
      double total_PE_ai = 0;
      double total_fpmt_ai = 0;
      for (int e = 0; e < pair.second.size(); ++e)
      {
        total_fpmt_ai += pair.second.at(e);
        total_PE_ai += m_PE_ai[pair.first].at(e);
      }
      //-- Calculate photon density at this track segment (ai)
      double total_eta = total_PE_ai / total_fpmt_ai;

      //-- Used to indicate where highest photon density occurs
      if (total_eta > max_eta) max_eta = total_eta;     //for plotting
      
      //-- Plot (ai,eta) pair
      gr_eta_ai->SetPoint(j, pair.first, total_eta);

      //-- Save (ev_id,cluster_time,ai,eta) data to txt file for ML scripts
      pos_file << "p" << partnumber << "_";
      if (isData) pos_file << evnum;
      else pos_file << mcevnum;
      pos_file << "," << main_cluster_time << "," << pair.first << "," << total_eta << std::endl;

      //-- Get the overall avg photon density (eta)
      avg_eta += total_eta;
      
      if (j == 0) running_avg = avg_eta;
      else
      {
        std::cout << " [debug] avg_eta (before dividing j, so right now it is total eta): " << avg_eta << std::endl;
        running_avg = avg_eta / (j+1);
      }

      std::cout << " [debug] ai, running_avg: " << pair.first << ", " << running_avg << std::endl;
      gr_running_avg->SetPoint(j, pair.first, running_avg);

      //-- Keep track of avg eta to the left and right of the trueTrackLegnthInWater:
      if (!isData)
      {
        if (pair.first < trueTrackLengthInWater)   //left of true track length
        {
          std::cout << " [debug] total_eta (left): " << total_eta << std::endl;
          num_left_eta++;
          left_avg_eta += total_eta;
        }
        if (pair.first > trueTrackLengthInWater)   //right of true track length
        {
          std::cout << " [debug] total_eta (right): " << total_eta << std::endl;
          num_right_eta++;
          right_avg_eta += total_eta;
        }
      }
      j+=1;
    }
    avg_eta /= j;
    h_avg_eta->Fill(avg_eta);
    std::cout << " [debug] avg eta: " << avg_eta << ", j: " << j << std::endl;

    //-- Keep track of avg eta to the left and right of the trueTrackLegnthInWater:
    if (!isData)
    {
      std::cout << " [debug] num pmts (left, right): " << num_left_eta << ", " << num_right_eta << std::endl;
      if (num_left_eta != 0) left_avg_eta /= num_left_eta;
      if (num_right_eta != 0) right_avg_eta /= num_right_eta;
      std::cout << " [debug] left avg: " << left_avg_eta << ", right avg: " << right_avg_eta << std::endl;
    }

    //-- Save truth info to txt files
    if (!isData)
    {
      truetrack_file << "p" << partnumber << "_" << mcevnum << ",";
      truetrack_file << trueTrackLengthInWater << "," << trueTrackLengthInMRD << "," << trueMuonEnergy << std::endl;
      //truetrack_file << trueTrackLengthInWater << "," << left_avg_eta << "," << right_avg_eta << std::endl;
    }
  } //-- End if found_muon
  }//-- End !reco_mode


  // ------------------------------------------------------------
  // --- RECO MODE: Fit tank track, muon energy, vertex ---------
  // ------------------------------------------------------------
  double fitted_tank_track = -999.;
  TVector3 fitted_vtx(-999,-999,-999);
  double reco_muon_ke = -999.;
  int num_mrd_lyrs = -999.;
  num_mrd_lyrs = LayersHit.size();
  if (reco_mode)
  {
    double t0 = -999.;
    bool save_t0 = false;

    //-- Get ev_id for matching << this has been moved further up in script
    /*std::stringstream ev_id;
    ev_id << "p" << partnumber << "_";
    if (isData) ev_id << evnum;
    else ev_id << mcevnum;*/

    if (abs(trackAngleRad*180./TMath::Pi()) > 5.)
    {
      std::cout << " [debug] angle more than 5 degrees! Event: p" << partnumber << "_";
      if (isData) std::cout << evnum;
      else std::cout << mcevnum;
      std::cout << std::endl;
    }

    //-- Get the fitted tank track length for this event from file
    std::map<std::string, std::vector<double>>::iterator it = m_tank_track_fits.find(ev_id.str());
    if (it != m_tank_track_fits.end())
    {
      std::vector<double> v_fit_ctime = m_tank_track_fits.at(ev_id.str());
      double fit_cluster_time = (double)v_fit_ctime.at(0);
      fitted_tank_track = (double)v_fit_ctime.at(1);
      std::cout << " MuonFitter Tool: Found track, cluster time for " << ev_id.str() << ": " << fitted_tank_track << ", " << fit_cluster_time << endl;
      h_fitted_tank_track->Fill(fitted_tank_track);

      //-- Skip the bad fits
      //-- NOTE: This may no longer be needed if using RNN to fit; might need different cuts
      if (fitted_tank_track < 0) return false;

      if (!isData)
      {
        //-- Check diff btwn reco and true info
      }

      //-- Load cluster
      //-- NOTE: Must use main_cluster_time from above since precision is lost when saving to file
      std::vector<Hit> cluster_hits;
      std::vector<MCHit> cluster_hits_MC;
      if (isData)
      {
        cluster_hits = m_all_clusters->at(main_cluster_time);
        //main_cluster_nhits = cluster_hits.size();
        for (int ihit = 0; ihit < cluster_hits.size(); ihit++)
        {
          main_cluster_charge += cluster_hits.at(ihit).GetCharge();
        }
      }
      else
      {
        cluster_hits_MC = m_all_clusters_MC->at(main_cluster_time);
        //main_cluster_nhits = cluster_hits_MC.size();
        for (int ihit = 0; ihit < cluster_hits.size(); ihit++)
        {
          main_cluster_charge += cluster_hits_MC.at(ihit).GetCharge();
        }
      }
      std::vector<unsigned long> cluster_detkeys = m_all_clusters_detkeys->at(main_cluster_time);

      //-- Save main cluster charge for fitted events
      h_clusterPE_fit->Fill(main_cluster_charge);
      if (hasPion) h_clusterPE_fit_haspion->Fill(main_cluster_charge);  //look at charge for events with pions

      // ------------------------------------------------------------
      // --- Fit muon vertex from fitted tank track length ----------
      // ------------------------------------------------------------
      //-- Start from tank exit point and move backwards into tank
      fitted_vtx = tankExit - fitted_tank_track * mrdTrackDir;
      std::cout << " MuonFitter Tool: Fitted vtx xyz: " << fitted_vtx.X() << "," << fitted_vtx.Y() << "," << fitted_vtx.Z() << std::endl;

      //-- Check if hit falls inside cone of FITTED vertex
      TVector3 vtx2tankExit = tankExit - fitted_vtx;   //similar to ai vector
/*      TVector3 vtx2pmt = TVector3(hitX,hitY,hitZ) - fitted_vtx;

      double ang = vtx2pmt.Angle(mrdTrackDir);

      if (!isData && display_truth)
      {
        std::cout << " [debug] vtx2tankExit (before truth): " << vtx2tankExit.X() << "," << vtx2tankExit.Y() << "," << vtx2tankExit.Z() << std::endl;
        vtx2tankExit = TVector3(trueStopVtxX,trueStopVtxY,trueStopVtxZ) - TVector3(trueVtxX,trueVtxY,trueVtxZ);
        vtx2pmt = TVector3(hitX,hitY,hitZ) - TVector3(trueVtxX,trueVtxY,trueVtxZ);
        std::cout << " [debug] vtx2tankExit (after truth): " << vtx2tankExit.X() << "," << vtx2tankExit.Y() << "," << vtx2tankExit.Z() << std::endl;
        ang = vtx2pmt.Angle(trueTrackDir);
        std::cout << " [debug] ang (w/ trueTrackDir, w/ vtx2tankExit): " << ang << "," << vtx2pmt.Angle(vtx2tankExit) << std::endl;
      }

      if (ang <= CHER_ANGLE_RAD)
      {
        //t0 = pmt_t - (TVector3(hitX,hitY,hitZ).Dot(mrdTrackDir) + ai) - TMath::Sqrt(1.33*1.33-1)*TVector3(hitX,hitY,hitZ).Cross(mrdTrackDir).Mag();
        t0 = pmt_t - (vtx2pmt.Dot(mrdTrackDir) + TMath::Sqrt(1.33*1.33-1)*vtx2pmt.Cross(mrdTrackDir).Mag()) / 30.;

        if (!isData && display_truth)
        {
          t0 = pmt_t - (vtx2pmt.Dot(trueTrackDir) + TMath::Sqrt(1.33*1.33-1)*vtx2pmt.Cross(trueTrackDir).Mag()) / 30.;
        }
        std::cout << " [debug] pmt_t made it inside cone! " << pmt_t << "," << t0 << std::endl;
        v_tzero.push_back(t0);
        nhits_incone += 1;
        totalpe_incone += pmt_PE;
      }
      else { std::cout << " [debug] pmt_t did not make it inside cone! " << pmt_t << std::endl; }
*/

      // ------------------------------------------------------------
      // --- Reconstruct Muon Energy --------------------------------
      // ------------------------------------------------------------
      //-- Calculate initial MIP energy loss in TANK from tank track length
      double tank_track = fitted_tank_track;
      if (!isData && display_truth)
      {
        // Check energy calc with trueTrackLengthInWater
        tank_track = trueTrackLengthInWater;
      }
      double tank_dEdx = 2.000;    //1.992 MeV/cm
      double tank_edep = tank_track * tank_dEdx;
      double tank_mrd_dist = TMath::Sqrt(pow(tankExitPointX-mrdEntryPointX,2) + pow(tankExitPointY-mrdEntryPointY,2) + pow(tankExitPointZ-mrdEntryPointZ,2));
      double outtank_edep = tank_mrd_dist * 1.05;  //this should be dE/dx in air..; currently not used
      std::cout << " [debug] tank_mrd_dist [cm]: " << tank_mrd_dist << std::endl;
      std::cout << " [debug] outtank_edep: " << outtank_edep << std::endl;

      //-- Calculate initial MIP energy loss in MRD from MRD track length
      //-- TODO: Use another dEdx value for iron?
      double mrd_track = reco_mrd_track;
      if (use_nlyrs) mrd_track = nlyrs_mrd_track;
      if (use_conn_dots) mrd_track = conn_dots_mrd_track;

      double mrd_dEdx = 11.5;     //MeV/cm; 10.1, 11.3
      double mrd_edep = mrd_track * mrd_dEdx;
      if (use_eloss) mrd_edep = mrdEnergyLoss;  //use official ANNIE tool reco MRD energy loss as the starting point
      std::cout << " [debug] mrd_edep: " << mrd_edep << std::endl;  //check which MRD energy is used

      if (use_nlyrs)
      {
        std::cout << " [debug] Adjust MRD track length based on initial mrd_edep" << std::endl;
        if (mrd_edep >= 400.)
        {
          mrd_track = mrd_track / 0.95;
        }
        else if (mrd_edep < 400.)
        {
          mrd_track = mrd_track / 0.80;
        }

        // Update with new track length
        mrd_edep = mrd_track * mrd_dEdx;
        std::cout << " [debug] nlyrs upd mrd_track: " << mrd_track << std::endl;
        std::cout << " [debug] nlyrs upd mrd_edep: " << mrd_edep << std::endl;
      }

      //-- Get initial reco muon energy by adding tank and MRD energy depositions
      double reco_mu_e = tank_edep + mrd_edep;


      //-- Fine-tune reconstructed muon energy by finding best dE/dx
      //-- and updating the muon energy until it doesn't change by 2%
      double T_before = reco_mu_e;
      bool eres_ok = false;
      int n_tries = 0;
      std::cout << " [debug] initial tank_dEdx: " << tank_dEdx << std::endl;

/*      while (!eres_ok)
      {
        double new_dEdx = CalcTankdEdx(T_before);
        std::cout << " [debug] new_dEdx: " << new_dEdx << std::endl;
        double T_after = tank_track*new_dEdx + mrdEnergyLoss + tank_mrd_dist*new_dEdx;
        double E_res = TMath::Abs(T_after-T_before)/T_before;
        std::cout << " [debug] T_before: " << T_before << std::endl;
        std::cout << " [debug] T_after: " << T_after << std::endl;
        std::cout << " [debug] Eres: " << E_res << std::endl;
        if (E_res <= 0.02)
        {
          reco_mu_e = T_after;  //set as new reco muon energy
          eres_ok = true;
        }
        else
        {
          T_before = T_after;
          n_tries++;
          if (n_tries > 10)
          {
            std::cout << " [debug] Did 10 tries!" << std::endl;
            eres_ok = true;
          }
        }
      } //end while eres_ok loop
*/

        // ------------------------------------------------------------
        // --- Fine-tune reco muon energy by finding dEdx for changing 
        // --- muon energy after traveling some distance dx -----------
        // ------------------------------------------------------------
        std::cout << " MuonFitter Tool: Calculating muon energy iteratively..." << std::endl;
        double dx = 10;   //cm; step size to update dE/dx
        double input_Emu = reco_mu_e;   //input energy for dE/dx calculation

        //-- Use true energy minus muon's rest mass as initial input energy
        //if (!isData && display_truth) { input_Emu = trueMuonEnergy-105.66; }

        //-- Start with energy deposition in tank
        double sum_tank_Emu = 0.;   //keep track of new reco muon energy in tank
        double d_tanktrack = fitted_tank_track;   //this will be used to keep track (haha get it) of remaining tank track length
        if (!isData && display_truth)
        {
          //-- Check energy calc with true track length in water
          d_tanktrack = trueTrackLengthInWater;
        }
        std::cout << " [debug] Initial d_tanktrack: " << d_tanktrack << std::endl;

        while (d_tanktrack > 0)
        {
          if (d_tanktrack < dx)
          {
            std::cout << " [debug] remaining d_tanktrack: " << d_tanktrack << std::endl;

            double deltaE = CalcTankdEdx(input_Emu) * d_tanktrack;
            sum_tank_Emu += deltaE;
            //-- Calculate new input energy for MRD energy calc
            input_Emu -= deltaE;
            d_tanktrack -= d_tanktrack;
            std::cout << " [debug] input_Emu: " << input_Emu << std::endl; 
            std::cout << " [debug] sum_tank_Emu: " << sum_tank_Emu << std::endl; 
            std::cout << " [debug] d_tanktrack: " << d_tanktrack << std::endl;  //should be 0
            std::cout << " [debug] final dEdx: " << deltaE/dx << std::endl; 
          }
          else
          {
            double deltaE = CalcTankdEdx(input_Emu) * dx;
            sum_tank_Emu += deltaE;
            //-- Calculate new input energy and remaining tank track length
            input_Emu -= deltaE;
            d_tanktrack -= dx;
            std::cout << " [debug] input_Emu: " << input_Emu << std::endl; 
            std::cout << " [debug] sum_tank_Emu: " << sum_tank_Emu << std::endl; 
            std::cout << " [debug] d_tanktrack: " << d_tanktrack << std::endl;
            std::cout << " [debug] dEdx: " << deltaE/dx << std::endl; 
          }
        }
        std::cout << " [debug] input_Emu (for MRD): " << input_Emu << "," << reco_mu_e-sum_tank_Emu << std::endl; //should be equal
        std::cout << " [debug] Initial reconstructed Emu: " << reco_mu_e << std::endl;
        //-- Update reco muon energy with the new tank energy (same initial MRD energy deposition)
        reco_mu_e = sum_tank_Emu + mrd_edep;
        std::cout << " [debug] New reconstructed Emu (updated tank energy): " << reco_mu_e << std::endl;

        //-- Now update energy deposition in MRD
        std::cout << " [debug] Now calculating muon energy in MRD..." << std::endl;
        double d_mrdtrack = mrd_track;
        double sum_mrd_Emu = 0.;
        //-- Use trueTrackLengthInMRD to check energy calculation
        //if (!isData && display_truth) { d_mrdtrack = trueTrackLengthInMRD; }

        std::cout << " [debug] Initial d_mrdtrack: " << d_mrdtrack << std::endl;
        while (d_mrdtrack > 0)
        {
          if (input_Emu < 0) break;

          //-- TODO: Need condition on remaining track length
          if (input_Emu < 20.)
          {
            std::cout << " [debug] input_Emu is LESS THAN 20 MeV: " << input_Emu << std::endl;
            std::cout << " [debug] Remaining d_mrdtrack: " << d_mrdtrack << std::endl;

            /*if (use_nlyrs && (d_mrdtrack > dx))
            {
              // Up the remaining energy since we still have a lot of track left
              input_Emu = d_mrdtrack * 10.1;
            }*/

            //-- If input energy < 20 MeV, just add remaining energy
            sum_mrd_Emu += input_Emu;

            //-- If input energy < 20, add (remaining track)*(previous dE/dx)
            //sum_mrd_Emu += d_mrdtrack * mrd_dEdx;

            std::cout << " [debug] sum_mrd_Emu: " << sum_mrd_Emu << std::endl; 
            input_Emu -= input_Emu;

            //-- hist: How much track length do we have left?
            h_remainder_track_last20MeV->Fill(d_mrdtrack);
            //-- hist: What is the diff btwn reco_mu_e now and true_mu_e?
            h_true_reco_Ediff_last20MeV->Fill((sum_tank_Emu+sum_mrd_Emu)-(trueMuonEnergy-105.66));
            h_remainder_track_Ediff_last20MeV->Fill((sum_tank_Emu+sum_mrd_Emu)-(trueMuonEnergy-105.66), d_mrdtrack);
            break;
          }

          if (d_mrdtrack < dx)
          {
            std::cout << " [debug] remaining d_mrdtrack: " << d_mrdtrack << std::endl;
            double deltaE = CalcMRDdEdx(input_Emu) * d_mrdtrack;
            sum_mrd_Emu += deltaE;
            input_Emu -= deltaE;
            d_mrdtrack -= d_mrdtrack;
            mrd_dEdx = deltaE/dx; //save for remaining track length calc

            std::cout << " [debug] input_Emu: " << input_Emu << std::endl;
            std::cout << " [debug] sum_mrd_Emu: " << sum_mrd_Emu << std::endl; 
            std::cout << " [debug] d_mrdtrack: " << d_mrdtrack << std::endl;
            std::cout << " [debug] final dEdx: " << deltaE/dx << std::endl; 
          }
          else
          {
            double deltaE = CalcMRDdEdx(input_Emu) * dx;
            sum_mrd_Emu += deltaE;
            input_Emu -= deltaE;
            d_mrdtrack -= dx;
            mrd_dEdx = deltaE/dx;

            std::cout << " [debug] input_Emu: " << input_Emu << std::endl;
            std::cout << " [debug] sum_mrd_Emu: " << sum_mrd_Emu << std::endl; 
            std::cout << " [debug] d_mrdtrack: " << d_mrdtrack << std::endl;
            std::cout << " [debug] dEdx: " << deltaE/dx << std::endl; 
          }
        } //-- Done updating MRD energy
        std::cout << " [debug] Updated reconstructed MRD energy (mrd_edep) vs mrdEnergyLoss: " << mrd_edep << "," << mrdEnergyLoss << std::endl;
        //-- Update reconstructed muon energy with new MRD energy deposition
        reco_mu_e = sum_tank_Emu + sum_mrd_Emu;
        std::cout << " [debug] New reconstructed Emu (after updating MRD): " << reco_mu_e << std::endl;

        if (use_simple_ereco) 
        {
          reco_mu_e = tank_edep + mrd_edep;     //-- mrd_edep diff if use_eloss is 1
          //reco_mu_e = sum_tank_Emu + mrd_edep;   //-- TODO:later do this where tank gets dynamically updated but MRD doesn't
          std::cout << " [debug] reconstructed Emu (Simple E Reco): " << reco_mu_e << std::endl;
        }

        reco_muon_ke = reco_mu_e;   //-- store for CStore

        h_mrd_eloss_diff->Fill(sum_mrd_Emu - mrdEnergyLoss);  //-- Diff btwn const dE/dx and ANNIE method

        //-- Calculated deltaR and other variables (MC ONLY -- requires truth info)
        if (!isData)
        {
          //-- Calculate spatial resolution (deltaR)
          double deltaR = TMath::Sqrt(pow(fitted_vtx.X()-trueVtxX,2) + pow(fitted_vtx.Y()-trueVtxY,2) + pow(fitted_vtx.Z()-trueVtxZ,2));
          h_deltaR->Fill(deltaR);

          TVector3 true_vtx = TVector3(trueVtxX,trueVtxY,trueVtxZ);
          double transverse = deltaR * TMath::Sin((true_vtx-fitted_vtx).Angle(mrdTrackDir));
          h_transverse->Fill(TMath::Abs(transverse));

          double parallel = deltaR * TMath::Cos((true_vtx-fitted_vtx).Angle(mrdTrackDir));
          h_parallel->Fill(TMath::Abs(parallel));

          //-- Compare with true muon energy
          double true_mu_e = trueMuonEnergy - 105.66;   //minus rest mass
          reco_mu_e = reco_mu_e + ERECO_SHIFT;
          double ediff = reco_mu_e - true_mu_e;
          h_true_reco_E->Fill(true_mu_e, reco_mu_e);
          h_true_reco_Ediff->Fill(ediff);

          //-- Use timing to select good vertex fits
          /*if (h_tzero->GetStdDev() < 2.) 
          {
            std::cout << " [debug] h_t0 width: " << h_tzero->GetStdDev() << std::endl;
            h_true_reco_E->Fill(true_mu_e, reco_mu_e);
            h_true_reco_Ediff->Fill(ediff);
            //if (main_cluster_charge < 3500.) h_true_reco_Ediff->Fill(ediff);  //make a charge cut
          }*/

          if (abs(ediff) > 100)
          {
            std::cout << " !!HEY! THERE IS A HUGE ENERGY DIFFERENCE!! Event: " << ev_id.str() <<", Ediff: " << ediff << std::endl;
            std::cout << " >> TANK track lengths (reco; true): " << tank_track << "; " << trueTrackLengthInWater << std::endl;
            std::cout << " >> MRD track lengths (reco; true): " << mrd_track << "; " << trueTrackLengthInMRD << std::endl;
            std::cout << " >> Muon energy (reco; true): " << reco_mu_e << "; " << true_mu_e << std::endl;
            std::cout << " >> Muon vertex (reco; true): " << fitted_vtx.X() << "," << fitted_vtx.Y() << "," << fitted_vtx.Z() << "; " << trueVtxX << "," << trueVtxY << "," << trueVtxZ << std::endl;

            h_tank_track_diff_large->Fill(tank_track-trueTrackLengthInWater);


            //-- Save info to file
            //save_t0 = true;
            lg_ediff_file << ev_id.str() << ","
                          << ediff << ","
                          << hasPion << ","
                          << tank_track << ","
                          << trueTrackLengthInWater << ","
                          << mrd_track << ","
                          << trueTrackLengthInMRD << ","
                          << reco_mu_e << ","
                          << true_mu_e << std::endl;

            //-- Save main_cluster_charge of events with ediff > 100 and/or has pion
            h_clusterPE_lrg_ediff->Fill(main_cluster_charge);
            if (hasPion) h_clusterPE_lrg_ediff_haspion->Fill(main_cluster_charge);
          }
          else
          {
            //-- Save info for events with ediff < 100
            h_tank_track_diff_small->Fill(tank_track-trueTrackLengthInWater);
          }


          //-- Look at fractions of the tank and MRD track lengths out of entire track length
          double f_tanktrack = (tank_track + tank_mrd_dist) / (tank_track + tank_mrd_dist + reco_mrd_track);
          double f_mrdtrack = (reco_mrd_track) / (tank_track + tank_mrd_dist + reco_mrd_track);

        }
        //-- histogram: Compare energy deposition in MRD and tank
        h_tank_mrd_E->Fill(mrdEnergyLoss, tank_edep);
      }
      else std::cout << " MuonFitter Tool: womp womp. No fitted track found for: " << ev_id.str() << std::endl;


    // Find mean t0 and subtract to center hist around 0
    //double mean_t0 = 0.;
/*    for (int t = 0; t < v_tzero.size(); ++t) { mean_t0 += v_tzero.at(t); }
    if (v_tzero.size() != 0) mean_t0 /= v_tzero.size();
    std::cout << " [debug] mean_t0: " << mean_t0 << std::endl;

    for (int t = 0; t < v_tzero.size(); ++t) { h_tzero->Fill(v_tzero.at(t)-mean_t0); }

    h_uber_t0widths->Fill(h_tzero->GetStdDev());
    if (!isData && reco_mode && save_t0)
    {
      lg_ediff_file << "," << h_tzero->GetStdDev() << std::endl;
    }
*/
    //save_t0 = false;  //reset


    // find which pmt charges are inside/outside cone of true track direction
/*    if (!isData)
    {
      for (unsigned long detkey = 332; detkey < 464; ++detkey)
      {
        if (charge[detkey] != 0.)
        {
          // find vector from true vertex to PMT
          TVector3 vec_pmt = TVector3(x_pmt[detkey],y_pmt[detkey],z_pmt[detkey]) - TVector3(trueVtxX,trueVtxY,trueVtxZ);
          std::cout << " [debug] xyz_pmt[detkey] " << detkey << ": " << x_pmt[detkey] << "," << y_pmt[detkey] << "," << z_pmt[detkey] << std::endl;
          // find angle between pmt vector (vec_pmt) and true track direction
          double pmt_angle = vec_pmt.Angle(trueTrackDir)*180./TMath::Pi();
          std::cout << " [debug] vec_pmt: " << vec_pmt.X() << "," << vec_pmt.Y() << "," << vec_pmt.Z() << std::endl;
          std::cout << " [debug] pmt_angle: " << pmt_angle << std::endl;
          std::cout << " [debug] trueTrackDir: " << trueTrackDir.X() << "," << trueTrackDir.Y() << "," << trueTrackDir.Z() << std::endl;
          std::cout << " [debug] trueVtx: " << trueVtxX << "," << trueVtxY << "," << trueVtxZ << std::endl;
          std::cout << " [debug] trueStopVtx: " << trueStopVtxX << "," << trueStopVtxY << "," << trueStopVtxZ << std::endl;

          if (pmt_angle < CHER_ANGLE_DEG+insideAngle) h_qincone_truevtx->Fill(charge[detkey]);
          if (pmt_angle > CHER_ANGLE_DEG+outsideAngle) h_qoutcone_truevtx->Fill(charge[detkey]);
        }
      }
    }*/

  } //-- End reco_mode


  // ------------------------------------------------------------
  // --- Store variables to CStore for downstream tools ---------
  // ------------------------------------------------------------
  std::cout << " MuonFitter Tool: Setting FittedTrackLengthInWater to CStore: " << fitted_tank_track << std::endl;
  m_data->CStore.Set("FittedTrackLengthInWater", fitted_tank_track);  //could be -888 or -999 if no fit
  std::cout << " MuonFitter Tool: Setting FittedMuonVertex to CStore" << std::endl;
  Position fitted_muon_vtx(fitted_vtx.X(), fitted_vtx.Y(), fitted_vtx.Z());   //could be -888 or -999 if no fit
  m_data->CStore.Set("FittedMuonVertex", fitted_muon_vtx);
  std::cout << " MuonFitter Tool: Setting RecoMuonKE to CStore: " << reco_muon_ke << std::endl;
  m_data->CStore.Set("RecoMuonKE", reco_muon_ke);   //could be -888 or -999 if no fit
  m_data->CStore.Set("NLyers", num_mrd_lyrs);


  // ------------------------------------------------------------
  // --- SimpleReconstruction variables -------------------------
  // ------------------------------------------------------------
  // -- Code based on SimpleReconstruction tool
  // if able to fit a vertex, set SimpleRecoFlag (int) to 1
  if (fitted_tank_track > 0) SimpleRecoFlag = 1;
  std::cout << " [debug] SimpleRecoFlag: " << SimpleRecoFlag << std::endl;

  SimpleRecoEnergy = reco_muon_ke;

  Position reco_vtx(fitted_vtx.X()/100., fitted_vtx.Y()/100. - 0.1446, fitted_vtx.Z()/100. + 1.681); //convert to [m] and set in same reference frame as Michael's SimpleRecoVtx
  SimpleRecoVtx = reco_vtx;
  SimpleRecoStopVtx = mrdStopVertex;  //already in [m]

  SimpleRecoCosTheta = TMath::Cos(trackAngleRad);   //diff from dirz in SimpleReco by factor of 2

  double SimpleRecoTotalEnergy = SimpleRecoEnergy + 105.66;
  SimpleRecoPt = sqrt((1-SimpleRecoCosTheta*SimpleRecoCosTheta)*(SimpleRecoTotalEnergy*SimpleRecoTotalEnergy-105.66*105.66)); 

  std::cout << " [debug] DownstreamFV (before): " << DownstreamFV << std::endl;
  std::cout << " [debug] FullCylFV (before): " << FullCylFV << std::endl;

  //if (sqrt(fitted_vtx.X()*fitted_vtx.X()+(fitted_vtx.Z()-1.681)*(fitted_vtx.Z()-1.681))<1.0 && fabs(fitted_vtx.Y())<0.5 && ((fitted_vtx.Z()-1.681) < 0.)) SimpleRecoFV = true;  //original FV cut with center correction
  // FV 1 - upstream half cylinder
  if (sqrt(reco_vtx.X()*reco_vtx.X()+reco_vtx.Z()*reco_vtx.Z())<1.0 && fabs(reco_vtx.Y())<0.5 && (reco_vtx.Z()<0.))
  { //no tank center correction needed
    std::cout << " MuonFitter Tool: Made FV cut!" << std::endl;
    SimpleRecoFV = true;
    std::cout << " MuonFitter Tool: Found in FV region 1 (upstream half of cylinder)! " << reco_vtx.X() << "," << reco_vtx.Y() << "," << reco_vtx.Z() << std::endl;
  }
  else { std::cout << " MuonFitter Tool: Did not make FV cut! reco_vtx(x,y,z): " << reco_vtx.X() << "," << reco_vtx.Y() << "," << reco_vtx.Z() << std::endl; }

  // FV 2 - downstream half cylinder
  if (sqrt(reco_vtx.X()*reco_vtx.X()+reco_vtx.Z()*reco_vtx.Z())<1.0 && fabs(reco_vtx.Y())<0.5 && (reco_vtx.Z()>0.) && (reco_vtx.Z()<1.))
  {
    DownstreamFV = 1;
    std::cout << " MuonFitter Tool: Found in FV region 2 (downstream half of cylinder)! " << reco_vtx.X() << "," << reco_vtx.Y() << "," << reco_vtx.Z() << std::endl;
  }
  // FV 3 - 1m full right cylinder
  if (sqrt(reco_vtx.X()*reco_vtx.X()+reco_vtx.Z()*reco_vtx.Z())<1.0 && fabs(reco_vtx.Y())<0.5)
  {
    FullCylFV = 1;
    std::cout << " MuonFitter Tool: Found in FV region 3 (full cylinder volume)! " << reco_vtx.X() << "," << reco_vtx.Y() << "," << reco_vtx.Z() << std::endl;
  }

  std::cout << " [debug] DownstreamFV (after): " << DownstreamFV << std::endl;
  std::cout << " [debug] FullCylFV (after): " << FullCylFV << std::endl;

  SimpleRecoMrdEnergyLoss = mrdEnergyLoss;  //placeholder; not used in NM tool
  SimpleRecoTrackLengthInMRD = reco_mrd_track/100.;  //placeholder; not used in NM tool
  SimpleRecoMRDStart = mrdStartVertex;
  SimpleRecoMRDStop = mrdStopVertex;

  SimpleRecoNeutrinoEnergy = this->CalcNeutrinoEnergy(SimpleRecoTotalEnergy, SimpleRecoCosTheta);
  SimpleRecoQ2 = this->CalcQ2(SimpleRecoTotalEnergy, SimpleRecoCosTheta, SimpleRecoNeutrinoEnergy);

  //Set variables in RecoEvent Store
  m_data->Stores["RecoEvent"]->Set("SimpleRecoFlag",SimpleRecoFlag);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoEnergy",SimpleRecoEnergy);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoVtx",SimpleRecoVtx);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoStopVtx",SimpleRecoStopVtx);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoCosTheta",SimpleRecoCosTheta);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoPt",SimpleRecoPt);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoFV",SimpleRecoFV);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoMrdEnergyLoss",SimpleRecoMrdEnergyLoss);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoTrackLengthInMRD",SimpleRecoTrackLengthInMRD);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoMRDStart",SimpleRecoMRDStart);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoMRDStop",SimpleRecoMRDStop);
  m_data->Stores["RecoEvent"]->Set("DownstreamFV",DownstreamFV);
  m_data->Stores["RecoEvent"]->Set("FullCylFV",FullCylFV);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoNeutrinoEnergy",SimpleRecoNeutrinoEnergy);
  m_data->Stores["RecoEvent"]->Set("SimpleRecoQ2",SimpleRecoQ2);

  int test_fv = -1;
  m_data->Stores["RecoEvent"]->Get("DownstreamFV", test_fv);
  std::cout << " [debug] test_fv (DownstreamFV): " << test_fv << std::endl;
  m_data->Stores["RecoEvent"]->Get("FullCylFV", test_fv);
  std::cout << " [debug] test_fv (FullCylFV): " << test_fv << std::endl;

  //Fill Particles object with muon
  if (SimpleRecoFlag){

    //Define muon particle
    int muon_pdg = 13;
    tracktype muon_tracktype = tracktype::CONTAINED;
    double muon_E_start = SimpleRecoEnergy;
    double muon_E_stop = 0;
    Position muon_vtx_start = SimpleRecoVtx;
    Position muon_vtx_stop = SimpleRecoStopVtx;
    Position muon_dir = SimpleRecoStopVtx - SimpleRecoVtx;
    Direction muon_start_dir = Direction(muon_dir.X(),muon_dir.Y(),muon_dir.Z());
    double muon_start_time;
    m_data->Stores["RecoEvent"]->Get("PromptMuonTime", muon_start_time);
    double muon_tracklength = muon_dir.Mag();
    double muon_stop_time = muon_start_time + muon_tracklength/3.E8*1.33;

    Particle muon(muon_pdg,muon_E_start,muon_E_stop,muon_vtx_start,muon_vtx_stop,muon_start_time,muon_stop_time,muon_start_dir,muon_tracklength,muon_tracktype);
    if (verbosity > 2){
      Log(" MuonFitter Tool: Added muon with the following properties as a particle:",v_message,verbosity);
      muon.Print();
    }

    //Add muon particle to Particles collection
    std::vector<Particle> Particles;
    get_ok = m_data->Stores["ANNIEEvent"]->Get("Particles",Particles);
    if (!get_ok){
      Particles = {muon};
    } else {
      Particles.push_back(muon);
    }
    m_data->Stores["ANNIEEvent"]->Set("Particles",Particles);
  }


  // ------------------------------------------------------------
  // --- Draw event ---------------------------------------------
  // ------------------------------------------------------------
  root_outp->cd();
  this->SaveHistograms();

  //-- ANNIE in 3D
  //-- Code based on UserTools/EventDisplay/EventDisplay.cpp
  if (plot3d && drawEvent)
  {
    if (verbosity > v_debug) { std::cout << "MuonFitter Tool: Saving 3D plots..." << std::endl; }

    std::stringstream ss_evdisplay3d_title, ss_evdisplay3d_name;
    ss_evdisplay3d_title << "evdisplay3d_ev";
    ss_evdisplay3d_name << "canvas_evdisplay3d_p" << partnumber << "_ev";
    if (!isData)
    {
      ss_evdisplay3d_title << mcevnum;
      ss_evdisplay3d_name << mcevnum;
    }
    else
    {
      ss_evdisplay3d_title << evnum;
      ss_evdisplay3d_name << evnum;
    }
    canvas_3d->SetTitle(ss_evdisplay3d_title.str().c_str());
    canvas_3d->SetName(ss_evdisplay3d_name.str().c_str());
    ageom->DrawTracks();
    canvas_3d->Modified();
    canvas_3d->Update();
    canvas_3d->Write();
  }

  //-- Save canvases/graphs for fitting
  if (!reco_mode && found_muon)
  {
    //------------------------------------------------------------
    //-- Photon density (eta) vs tank track segment (ai) ---------
    //------------------------------------------------------------

    c_eta_ai->cd();
    std::stringstream ss_eta_title, ss_eta_ai_name, ss_truetrack;
    ss_eta_title << "#eta: #PE Divided By f (tot PMT charge > " << PMTQCut << ") Ev_";
    ss_eta_ai_name << "c_eta_ai";
    if (isData) ss_eta_ai_name << "_r" << runnumber;
    else ss_eta_ai_name << "_wcsim";
    ss_eta_ai_name << "_p" << partnumber << "_ev";
    if (isData)
    {
      ss_eta_title << evnum;
      ss_eta_ai_name << evnum;
    }
    else
    {
      ss_eta_title << mcevnum;
      ss_eta_ai_name << mcevnum;
      ss_truetrack << "true track length in water: " << trueTrackLengthInWater << " cm";
      if (display_truth) gr_eta_ai->SetTitle(ss_truetrack.str().c_str());
    }
    gr_eta_ai->Draw("alp");
    gr_eta_ai->GetXaxis()->SetLimits(45., 505.);
    gr_eta_ai->GetHistogram()->SetMinimum(0.);
    //gr_running_avg->Draw("lp");
    c_eta_ai->Modified();
    c_eta_ai->Update();
    c_eta_ai->SetTitle(ss_eta_title.str().c_str());
    c_eta_ai->SetName(ss_eta_ai_name.str().c_str());

    TLegend *legend = new TLegend(0.55, 0.65, 0.89, 0.89);
    legend->AddEntry(gr_eta_ai, "#eta", "lp");
    //legend->AddEntry(gr_running_avg, "running avg #eta", "lp");

    if (!isData)
    { //indicate distance btwn trueVtx and tankExitPoint
      /*TLine *lTrueTankTrack = new TLine(trueTrackLengthInWater, 0, trueTrackLengthInWater, max_eta+1);
      lTrueTankTrack->SetLineColor(4);
      lTrueTankTrack->SetLineWidth(2);
      if (display_truth)
      {
        lTrueTankTrack->Draw();
        legend->AddEntry(lTrueTankTrack, "true tank track length", "l");
      }*/

      TLine *lLeftAvgEta = new TLine(50, left_avg_eta, trueTrackLengthInWater, left_avg_eta);
      lLeftAvgEta->SetLineColor(2);
      lLeftAvgEta->SetLineWidth(2);
      if (display_truth)
      {
        lLeftAvgEta->Draw();
        legend->AddEntry(lLeftAvgEta, "avg #eta to left of true track length", "l");
      }

      TLine *lRightAvgEta = new TLine(trueTrackLengthInWater, right_avg_eta, 455, right_avg_eta);
      lRightAvgEta->SetLineColor(8);
      lRightAvgEta->SetLineWidth(2);
      if (display_truth)
      {
        lRightAvgEta->Draw();
        legend->AddEntry(lRightAvgEta, "avg #eta to right of true track length", "l");
      }
    }
    TLine *lAvgEta = new TLine(55, avg_eta, 500-step_size_ai/2, avg_eta);
    lAvgEta->SetLineColor(46);
    lAvgEta->SetLineWidth(2);
    lAvgEta->Draw();
    legend->AddEntry(lAvgEta, "avg #eta", "l");
    legend->Draw();
    c_eta_ai->Write();
    if (!isData && display_truth) ss_eta_ai_name << "_truth";
    ss_eta_ai_name << ".png";
    c_eta_ai->SaveAs(ss_eta_ai_name.str().c_str());
  } //-- End !reco_mode

    
  if (reco_mode)
  {
    c_h_tzero->cd();
    std::stringstream ss_t0_title, ss_t0_name;
    ss_t0_title << "t0" << " p" << partnumber << "_ev";
    ss_t0_name << "c_h_tzero";
    if (isData) ss_t0_name << "_r" << runnumber;
    else ss_t0_name << "_wcsim";
    ss_t0_name << "_p" << partnumber << "_ev";
    if (isData)
    {
      ss_t0_title << evnum;
      ss_t0_name << evnum;
    }
    else
    {
      ss_t0_title << mcevnum;
      ss_t0_name << mcevnum;
    }
    h_tzero->Draw();
    c_h_tzero->Modified();
    c_h_tzero->Update();
    c_h_tzero->SetTitle(ss_t0_title.str().c_str());
    if (!isData && display_truth) ss_t0_name << "_truth";
    c_h_tzero->SetName(ss_t0_name.str().c_str());
    c_h_tzero->Write();
    ss_t0_name << ".png";
    c_h_tzero->SaveAs(ss_t0_name.str().c_str());
  }

  return true;
}


bool MuonFitter::Finalise(){
  // Save output
  root_outp->cd();

  // Close 3D canvas
  if (plot3d)
  {
    canvas_3d->Clear();
    canvas_3d->Close();
    delete canvas_3d;
  }

  delete c_eta_ai;
  delete c_h_tzero;

  root_outp->Close();
  pos_file.close();
  if (!isData) truetrack_file.close();
  if (!isData) lg_ediff_file.close();

  Log("MuonFitter Tool: Exiting", v_message, verbosity);
  return true;
}


//------------------------------------------------------------
//--- Custom Functions ---------------------------------------
//------------------------------------------------------------

Position MuonFitter::Line3D(double x1, double y1, double z1, double x2, double y2, double z2, double C, std::string coord)
{
  // returns the XYZ coordinates of a new point in a 3D line
  // given two initial points and one X,Y,Z value
  double a = x2-x1;
  double b = y2-y1;
  double c = z2-z1;
  double x = -69., y = -69., z = -69., L = -69.;

  if (coord == "x" || coord == "X")
  {
    L = (C-x1)/a;
    x = C;
    y = b*L+y1;
    z = c*L+z1;
  }
  else if (coord == "y" || coord == "Y")
  {
    L = (C-y1)/b;
    x = a*L+x1;
    y = C;
    z = c*L+z1;
  }
  else if (coord == "z" || coord == "Z")
  {
    L = (C-z1)/c;
    x = a*L+x1;
    y = b*L+y1;
    z = C;
  }
  else { Log("MuonFitter Tool: Unable to recognize coordinate!", v_debug, verbosity); }

  return(Position(x,y,z));
}

void MuonFitter::reset_3d()
{
  //reset TANK PMTs
  TGeoNode *node;
  TGeoSphere *sphere;
  for (unsigned long detkey=332; detkey<464; detkey++)
  {
    node = EXPH->GetNode(detkey_to_node[detkey]);
    sphere = (TGeoSphere *)(node->GetVolume()->GetShape());
    sphere->SetSphDimensions(0,1.5,0,180,0,180);
    node->GetVolume()->SetLineColor(41);
  }

  //remove extra node(s)
  if (N > maxN)
  {
    while (N > maxN)
    {
      node = EXPH->GetNode(maxN);
      EXPH->RemoveNode(node);
      --N;
    }
  }

  //remove tracks
  if(ageom->GetNtracks() > 0)
  {
    ageom->ClearTracks();
    if (verbosity > v_debug) { std::cout << "Clearing tracks... current number: " << ageom->GetNtracks() << std::endl; }
  }

  canvas_3d->Modified();
  canvas_3d->Update();

}

bool MuonFitter::FileExists(std::string fname)
{
  ifstream myfile(fname.c_str());
  return myfile.good();
}

void MuonFitter::LoadTankTrackFits()
{
  ifstream myfile(tankTrackFitFile.c_str());
  std::cout << "opened file" << std::endl;
  std::string line;
  if (myfile.is_open())
  {
    // Loop over lines, get event id, fit (should only be one line here)
    while (getline(myfile, line))
    {
      //if (verbosity > 3) std::cout << line << std::endl; //has our stuff
      if (line.find("#") != std::string::npos) continue;
      std::vector<std::string> fileLine;
      boost::split(fileLine, line, boost::is_any_of(","), boost::token_compress_on);
      std::string event_id = "";
      double tank_track_fit = -9999.;
      double cluster_time = -9999.;
      event_id = fileLine.at(0);
      cluster_time = std::stod(fileLine.at(1));
      tank_track_fit = std::stod(fileLine.at(2));
      std::vector<double> v_fit_ctime{cluster_time, tank_track_fit};
      if (verbosity > 5) std::cout << "Loading tank track fit: " << event_id << ", " << cluster_time << ", " << tank_track_fit << std::endl;
      m_tank_track_fits.insert(std::pair<std::string,std::vector<double>>(event_id, v_fit_ctime));
    }
  }
  return;
}

double MuonFitter::CalcTankdEdx(double input_E)
{
  double gamma = (input_E+105.66)/105.66;
  std::cout << " [debug] water dEdx: gamma: " << gamma << std::endl;
  double beta = TMath::Sqrt(gamma*gamma-1.)/gamma;
  std::cout << " [debug] water dEdx: beta: " << beta << std::endl;
  double Tmax = (2.*0.511*beta*beta*gamma*gamma)/(1.+2.*0.511*gamma/105.66 + pow(0.511/105.66,2));
  std::cout << " [debug] water dEdx: Tmax: " << Tmax << std::endl;
  double ox_ln = TMath::Log(2.*0.511*beta*beta*gamma*gamma*Tmax/(I_O*I_O));
  std::cout << " [debug] water dEdx: log factory oxy: " << ox_ln << std::endl;
  double ox_dEdx = (0.999)*(16./18.)*WATER_DENSITY*0.307*0.5*pow(1./beta,2)*(0.5*ox_ln-beta*beta-0.5);
  std::cout << " [debug] water dEdx: dE/dx oxy: " << ox_dEdx << std::endl;
  double h_ln = TMath::Log(2.*0.511*beta*beta*gamma*gamma*Tmax/(I_H*I_H));
  std::cout << " [debug] water dEdx: log factor H: " << h_ln << std::endl;
  double h_dEdx = (0.999)*(2./18.)*WATER_DENSITY*0.307*1.0*pow(1./beta,2)*(0.5*h_ln-beta*beta-0.5);
  std::cout << " [debug] water dEdx: dE/dx H: " << h_dEdx << std::endl;

  // including Gd2(SO4)3
  double gd_ln = TMath::Log(2.*0.511*beta*beta*gamma*gamma*Tmax/(I_GD*I_GD));
  double gd_dEdx = (0.001)*(314./602.)*1.0*0.307*0.5*pow(1./beta,2)*(0.5*gd_ln-beta*beta-0.5);
  //double s_ln = TMath::Log(2.*0.511*beta*beta*gamma*gamma*Tmax/(I_S*I_S));
  //double s_dEdx = (0.001)*(96./602.)*1.0*0.307*0.5*pow(1./beta,2)*(0.5*s_ln-beta*beta-0.5);
  //double gdox_dEdx = (0.001)*(192./602.)*1.0*0.307*0.5*pow(1./beta,2)*(0.5*ox_ln-beta*beta-0.5);

  //return (ox_dEdx + h_dEdx + gd_dEdx + s_dEdx + gdox_dEdx);
  return (ox_dEdx + h_dEdx + gd_dEdx);
}

double MuonFitter::CalcMRDdEdx(double input_E)
{
  // Using iterative dE/dx method
  double gamma = (input_E+105.66)/105.66;
  double beta = TMath::Sqrt(gamma*gamma-1.)/gamma;
  double Tmax = (2.*0.511*beta*beta*gamma*gamma)/(1.+2.*0.511*gamma/105.66 + pow(0.511/105.66,2));
  double fe_ln = TMath::Log(2.*0.511*beta*beta*gamma*gamma*Tmax/(I_FE*I_FE));
  double fe_dEdx = 7.874*0.307*0.464*pow(1./beta,2)*(0.5*fe_ln-beta*beta-0.5);
  double b_tot = 1.0338*TMath::Log(input_E/1000.) + 0.89287;  //fitted
  if (b_tot < 0.) b_tot = 0;
  //fe_dEdx = fe_dEdx + b_tot*input_E*7.874;

  return fe_dEdx;
}

void MuonFitter::ResetVariables()
{
  // photon density (eta) variables
  avg_eta = 0;
  left_avg_eta = 0.;
  right_avg_eta = 0.;
  num_left_eta = 0;
  num_right_eta = 0;

  // 3D plotting
  if (plot3d) { reset_3d(); }

  // Set default values for reconstruction variables
  SimpleRecoFlag = -9999;
  SimpleRecoEnergy = -9999;
  SimpleRecoVtx.SetX(-9999);
  SimpleRecoVtx.SetY(-9999);
  SimpleRecoVtx.SetZ(-9999);
  SimpleRecoCosTheta = -9999;
  SimpleRecoPt = -9999;
  SimpleRecoStopVtx.SetX(-9999);
  SimpleRecoStopVtx.SetY(-9999);
  SimpleRecoStopVtx.SetZ(-9999);
  SimpleRecoFV = false;
  SimpleRecoMrdEnergyLoss = -9999;
  SimpleRecoTrackLengthInMRD = -9999;
  SimpleRecoMRDStart.SetX(-9999);
  SimpleRecoMRDStart.SetY(-9999);
  SimpleRecoMRDStart.SetZ(-9999);
  SimpleRecoMRDStop.SetX(-9999);
  SimpleRecoMRDStop.SetY(-9999);
  SimpleRecoMRDStop.SetZ(-9999);
  DownstreamFV = -9999;
  FullCylFV = -9999;
  SimpleRecoNeutrinoEnergy = -9999;
  SimpleRecoQ2 = -9999;

}

void MuonFitter::InitHistograms()
{
  //-- Initialize Histograms
  h_num_mrd_layers = new TH1D("h_num_mrd_layers", "Num MRD Layers Hit", 50, 0, 50.);
  h_num_mrd_layers->GetXaxis()->SetTitle("# layers");
  
  h_truevtx_angle = new TH1D("h_truevtx_angle", "Angle of True Muon Vertex", 360, -180., 180.);
  h_truevtx_angle->GetXaxis()->SetTitle("angle [deg]");

  h_tankexit_to_pmt = new TH1D("h_tankexit_to_pmt", "Distance from Tank Exit Point to PMT Position", 350, 0., 350.);
  h_tankexit_to_pmt->GetXaxis()->SetTitle("Ri [cm]");

  h_tanktrack_ai = new TH1D("h_tanktrack_ai", "Segment of Tank Track (a_{i})", 250, 0., 500.);
  h_tanktrack_ai->GetXaxis()->SetTitle("a_{i} [cm]");

  h_eff_area_pmt = new TH1D("h_eff_area_pmt", "Effective Area of PMT Seen by Photons", 650, 0., 650.);
  h_eff_area_pmt->GetXaxis()->SetTitle("effective area [cm^2]");

  h_fpmt = new TH1D("h_fpmt", "area fraction f", 100, 0., 1.);
  h_fpmt->GetXaxis()->SetTitle("f");

  h_clusterhit_timespread = new TH1D("h_clusterhit_timespread", "Time Spread of Clusters (Latest - Earliest)", 200, 0, 200);
  h_clusterhit_timespread->GetXaxis()->SetTitle("latest t - earliest t [ns]");

  h_avg_eta = new TH1D("h_avg_eta", "Overall Average Eta", 100, 0, 5000);
  h_avg_eta->GetXaxis()->SetTitle("eta [PE/cm]");

  h_tdiff = new TH1D("h_tdiff", "Time Residuals", 1000, -1000., 1000.);
  h_tdiff->GetXaxis()->SetTitle("time residual [ns]");

  h_uber_t0widths = new TH1D("h_uber_t0widths", "Distribution of Timing Distribution Widths", 31, -1., 30.);
  h_uber_t0widths->GetXaxis()->SetTitle("std devs [ns]");

  h_true_tanktrack_len = new TH1D("h_true_tanktrack_len", "Distribution of True Track Lengths", 400, 0, 2000);
  h_true_tanktrack_len->GetXaxis()->SetTitle("track length [cm]");

  h_fitted_tank_track = new TH1D("h_fitted_tank_track", "Distribution of Fitted Track Length", 200, 0, 400);
  h_fitted_tank_track->GetXaxis()->SetTitle("track length [cm]");

  h_deltaR = new TH1D("h_deltaR", "#Deltar", 75, 0., 150.);
  h_deltaR->GetXaxis()->SetTitle("#Deltar [cm]");

  h_transverse = new TH1D("h_transverse", "transverse distance", 75, 0., 150.);
  h_transverse->GetXaxis()->SetTitle("[cm]");

  h_parallel = new TH1D("h_parallel", "parallel distance", 75, 0., 150.);
  h_parallel->GetXaxis()->SetTitle("[cm]");

  h_true_reco_E = new TH2D("h_true_reco_E", "Reco vs True #mu Energy", 250, 0., 5000., 250, 0., 5000.);
  h_true_reco_E->GetXaxis()->SetTitle("TrueE [MeV]");
  h_true_reco_E->GetYaxis()->SetTitle("RecoE (tank+mrd) [MeV]");

  h_true_reco_Ediff = new TH1D("h_true_reco_Ediff", "Diff in Reco and True #mu Energy", 100, -500, 500);
  h_true_reco_Ediff->GetXaxis()->SetTitle("recoE - trueE [MeV]");

  h_tank_mrd_E = new TH2D("h_tank_mrd_E", "Reco #mu Energy in Tank vs in MRD", 250, 0., 5000., 250, 0., 5000.);
  h_tank_mrd_E->GetXaxis()->SetTitle("Reco MrdE [MeV]");
  h_tank_mrd_E->GetYaxis()->SetTitle("Reco TankE [MeV]");

  h_mrd_eloss_diff = new TH1D("h_mrd_eloss_diff", "diff in MRD energy loss", 200, -500, 1500);
  h_mrd_eloss_diff->GetXaxis()->SetTitle("[MeV]");

  h_tank_track_diff_small = new TH1D("h_tank_track_diff_small", "Difference in TANK track lengths (Reco-True) When E_{diff} < 200 MeV", 150, -150, 150);
  h_tank_track_diff_small->GetXaxis()->SetTitle("reco-truth [cm]");

  h_tank_track_diff_large = new TH1D("h_tank_track_diff_large", "Difference in TANK track lengths (Reco-True) When E_{diff} > 200 MeV", 150, -150, 150);
  h_tank_track_diff_large->GetXaxis()->SetTitle("reco-truth [cm]");

  h_clusterPE = new TH1D("h_clusterPE", "Cluster PE of Events with Found Muon", 100, 0, 10000);
  h_clusterPE->GetXaxis()->SetTitle("cluster PE [p.e.]");

  h_clusterPE_fit = new TH1D("h_clusterPE_fit", "Cluster PE of Events with Tank Track Fits", 100, 0, 10000);
  h_clusterPE_fit->GetXaxis()->SetTitle("cluster PE [p.e.]");

  h_clusterPE_fit_haspion = new TH1D("h_clusterPE_fit_haspion", "Cluster PE of Events with Tank Track Fits", 100, 0, 10000);
  h_clusterPE_fit_haspion->GetXaxis()->SetTitle("cluster PE [p.e.]");

  h_clusterPE_lrg_ediff = new TH1D("h_clusterPE_lrg_ediff", "Cluster PE of Events with Large E Diff", 100, 0, 10000);
  h_clusterPE_lrg_ediff->GetXaxis()->SetTitle("cluster PE [p.e.]");

  h_clusterPE_lrg_ediff_haspion = new TH1D("h_clusterPE_lrg_ediff_haspion", "Cluster PE of Events with Large E Diff and Has Pion", 100, 0, 10000);
  h_clusterPE_lrg_ediff_haspion->GetXaxis()->SetTitle("cluster PE [p.e.]");

  h_pca_angle = new TH1D("h_pca_angle", "Reconstructed Muon Direction Angle Using PCA", 200, -50, 50);
  h_pca_angle->GetXaxis()->SetTitle("pca angle [deg]");

  h_remainder_track_last20MeV = new TH1D("h_remainder_track_last20MeV", "Remaining Track Length in MRD When Input E_{#mu} < 20 MeV", 25, 0, 50);
  h_remainder_track_last20MeV->GetXaxis()->SetTitle("remaining MRD track [cm]");

  h_true_reco_Ediff_last20MeV = new TH1D("h_true_reco_Ediff_last20MeV", "Energy Difference Btwn True and Reco Energy When Input E_{#mu} < 20 MeV", 100, -500, 500);
  h_true_reco_Ediff_last20MeV->GetXaxis()->SetTitle("E diff (reco-true) [MeV]");

  h_remainder_track_Ediff_last20MeV = new TH2D("h_remainder_track_Ediff_last20MeV", "Remaining MRD Track Length vs True/Reco Energy Diff When Input E_{#mu} < 20 MeV", 100, -500, 500, 25, 0, 50);
  h_remainder_track_Ediff_last20MeV->GetXaxis()->SetTitle("E diff (reco-true) [MeV]");
  h_remainder_track_Ediff_last20MeV->GetYaxis()->SetTitle("remaining MRD track [cm]");

  h_tankExitX = new TH1D("h_tankExitX", "Tank Exit Point X", 200, -500, 500);
  h_tankExitX->GetXaxis()->SetTitle("x [cm]");

  h_tankExitY = new TH1D("h_tankExitY", "Tank Exit Point Y", 200, -500, 500);
  h_tankExitY->GetXaxis()->SetTitle("y [cm]");

  h_tankExitZ = new TH1D("h_tankExitZ", "Tank Exit Point Z", 200, -500, 500);
  h_tankExitZ->GetXaxis()->SetTitle("z [cm]");

}

void MuonFitter::SaveHistograms()
{
  h_num_mrd_layers->Write();
  //h_clusterhit_timespread->Write();
  if (!isData)
  {
    //h_truevtx_angle->Write();
    h_true_tanktrack_len->Write();
    h_clusterPE->Write();
    h_clusterPE_fit->Write();
    h_clusterPE_fit_haspion->Write();
    h_clusterPE_lrg_ediff->Write();
    h_clusterPE_lrg_ediff_haspion->Write();
    h_pca_angle->Write();
    h_remainder_track_last20MeV->Write();
    h_true_reco_Ediff_last20MeV->Write();
    h_remainder_track_Ediff_last20MeV->Write();
  }
  h_tankexit_to_pmt->Write();
  h_tanktrack_ai->Write();
  h_eff_area_pmt->Write();
  h_fpmt->Write();
  //h_tdiff->Write();
  if (reco_mode)
  {
    h_uber_t0widths->Write();
    h_fitted_tank_track->Write();
    h_tank_mrd_E->Write();
    if (!isData)
    {
      h_deltaR->Write();
      h_transverse->Write();
      h_parallel->Write();
      h_true_reco_E->Write();
      h_true_reco_Ediff->Write();
      h_mrd_eloss_diff->Write();
      h_tank_track_diff_small->Write();
      h_tank_track_diff_large->Write();
    }
  }
}

void MuonFitter::InitCanvases()
{
  //-- Canvases
  c_eta_ai = new TCanvas("c_eta_ai", "Charge per cm (#eta)", 800, 600);
  c_h_tzero = new TCanvas("c_h_tzero", "t0", 800, 600);
}

void MuonFitter::SaveCanvases()
{
  
}

void MuonFitter::InitGraphs()
{
  //-- Graphs
  // photon density (eta) vs tank track segment (ai) plot
  gr_eta_ai = new TGraph();
  gr_eta_ai->SetLineWidth(0);
  gr_eta_ai->SetMarkerStyle(25);
  gr_eta_ai->SetMarkerColor(46);
  gr_eta_ai->SetFillColor(0);
  gr_eta_ai->GetXaxis()->SetTitle("a_{i} [cm]");
  gr_eta_ai->GetYaxis()->SetTitle("#eta (n_{PE}/f)");

  // running avg of photon density (eta)
  gr_running_avg = new TGraph();
  gr_running_avg->SetLineWidth(0);
  gr_running_avg->SetMarkerStyle(8);
  gr_running_avg->SetMarkerColor(38);
  gr_running_avg->SetFillColor(0);
  gr_running_avg->GetXaxis()->SetTitle("a_{i} [cm]");
  gr_running_avg->GetYaxis()->SetTitle("#eta (n_{PE}/f)");

}

double MuonFitter::PCATrackAngle(std::vector<int> v_MrdPMTsHit)
{
  // ------------------------------------------------------------
  // --- Use PCA to determine track angle in MRD ----------------
  // ------------------------------------------------------------
  //-- Code based on https://github.com/JaydipSingh/EnergyStudies/blob/main/trackAna_module.cc#L468
  //-- First define vectors to store xyz of each hit
  std::vector<double> fRec_SpacePoint_X;
  std::vector<double> fRec_SpacePoint_Y;
  std::vector<double> fRec_SpacePoint_Z;

  for (int i = 0; i < v_MrdPMTsHit.size(); ++i)
  {
    unsigned long detkey = (unsigned long)v_MrdPMTsHit.at(i);
    //std::cout << " [debug] PCA algorithm - detkey: " << detkey << std::endl;
    if (detkey < 26) continue;  //skip FMV hits
    Paddle *mrdpaddle = (Paddle *)geom->GetDetectorPaddle(detkey);
    Position position_MRD = mrdpaddle->GetOrigin();
    
    fRec_SpacePoint_X.push_back(mrd_center_x[detkey]);
    fRec_SpacePoint_Y.push_back(mrd_center_y[detkey]);
    fRec_SpacePoint_Z.push_back(mrd_center_z[detkey]);
  }
    
  //-- Put all the xyz info into one array
  double *xyz[3];
  TVector3 pos(0,0,0);
  TVector3 dir;
  xyz[0] = fRec_SpacePoint_X.data();
  xyz[1] = fRec_SpacePoint_Y.data();
  xyz[2] = fRec_SpacePoint_Z.data();
  size_t npts = fRec_SpacePoint_X.size();

  if (npts < 2)
  {
    //throw std::exception("tpcvechitfinder2_module.cc: too few TPCClusters to fit a line in linefit");
    std::cout << " MuonFitter Tool: PCA Method: Too few TPCClusters to fit a line in linefit" << std::endl;
  }

  TMatrixDSym covmat(3);  //covariance matrix (use symmetric version)

  // position is just the average of the coordinates
  double psum[3] = {0,0,0};
  for (size_t ipoint=0; ipoint<npts; ++ipoint)
  {
    for(size_t j=0; j<3; j++)
    {
      psum[j] += xyz[j][ipoint];
    }
  }
  for (size_t j=0; j<3; ++j)
  {
    psum[j] /= npts;
  }
  pos.SetXYZ(psum[0],psum[1],psum[2]);

  for(size_t i=0; i<3; ++i)
  {
    for (size_t j=0; j<= i; ++j)
    {
      double csum=0;
      for (size_t ipoint=0; ipoint<npts; ++ipoint)
      {
        csum += (xyz[i][ipoint] - psum[i]) * (xyz[j][ipoint] - psum[j]);
      }
      csum /= (npts-1);
      covmat[i][j] = csum;
      covmat[j][i] = csum;
    }
  }
  TVectorD eigenvalues(3);
  TMatrixD eigenvectors = covmat.EigenVectors(eigenvalues);

  double dirv[3] = {0,0,0};
  for (size_t i=0; i<3; ++i)
  {
    dirv[i] = eigenvectors[i][0];
  }
  dir.SetXYZ(dirv[0],dirv[1],dirv[2]);

  // return angle wrt to beam axis (z-axis) [radians]
  return (dir.Angle(TVector3(0,0,1)));
}

double MuonFitter::MRDConnectDots(std::vector<int> v_MrdPMTsHit)
{
  //-- Calculate MRD track by "connecting the dots (hits)"

  double conn_dots_mrd_track = 0;

  //-- First sort MRD PMTs IDs; MrdPMTsHit is type std::vector<int>
  std::sort(v_MrdPMTsHit.begin(), v_MrdPMTsHit.end());

  //-- QA: Check that MRD pmts have been sorted
  std::cout << " [debug] sorted MRD PMTs: ";
  for (int i = 0; i < v_MrdPMTsHit.size(); ++i) { std::cout << v_MrdPMTsHit.at(i) << ","; }
  std::cout << std::endl;

  //-- Now connect the dots
  for (int i = 1; i < v_MrdPMTsHit.size(); ++i)
  {
    unsigned long detkey1 = (unsigned long)v_MrdPMTsHit.at(i-1);
    unsigned long detkey2 = (unsigned long)v_MrdPMTsHit.at(i);

    //-- XXX:For some reason, there are FMV PMTs in these clusters
    if (detkey1 < 26) continue;

    double dist_btwn_lyrs = TMath::Sqrt(pow((mrd_center_x[detkey2]-mrd_center_x[detkey1]),2) + pow((mrd_center_y[detkey2]-mrd_center_y[detkey1]),2) + pow((mrd_center_z[detkey2]-mrd_center_z[detkey1]),2));

    conn_dots_mrd_track += dist_btwn_lyrs;

    //-- QA: Check calculations
    std::cout << " [debug] detkey 1,2: " << detkey1 << "," << detkey2 << std::endl;
    std::cout << " [debug] dist_btwn_lyrs: " << dist_btwn_lyrs << std::endl;
    std::cout << " [debug] conn_dots_mrd_track: " << conn_dots_mrd_track << std::endl;
  } //-- Done connecting the dots

  return conn_dots_mrd_track;
}

double MuonFitter::CalcNeutrinoEnergy(double Emu, double cosT)
{
  //-- Calculate the neutrino energy from reconstructed muon energy
  //-- and track angle
  //-- Emu: SimpleRecoTotalEnergy
  //-- cosT: SimpleRecoCosTheta

  double Ev = 0;
  double Mp = 938.272;  //MeV
  double Mn = 939.565;  //MeV
  double Eb = 26;
  double Mmu = 105.658; //MeV
  double pmu = sqrt(Emu*Emu-Mmu*Mmu);
  Ev = ((Mp*Mp-pow((Mn-Eb),2)-Mmu*Mmu+2*(Mn-Eb)*Emu)/(2*(Mn-Eb-Emu+pmu*cosT)));
  std::cout <<"Emu: "<<Emu<<", cosT: "<<cosT<<", Ev: "<<Ev<<std::endl;
  return Ev;
}

double MuonFitter::CalcQ2(double Emu, double cosT, double Ev)
{
  //-- Calculated the Q^2 value from reconstructed muon, neutrino
  //-- energy, and track angle
  //-- Emu: SimpleRecoTotalEnergy
  //-- cosT: SimpleRecoCosTheta
  //-- Ev: SimpleRecoNeutrinoEnergy
 
  double Qsq = 0;
  double Mmu = 105.658; //MeV
  double pmu = sqrt(Emu*Emu-Mmu*Mmu);
  Qsq = 2.*Ev*(Emu-pmu*cosT)-Mmu*Mmu;
  std::cout <<"Q2: "<<Qsq<<std::endl;
  return Qsq;
}
