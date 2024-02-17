#include "SimpleReconstruction.h"

SimpleReconstruction::SimpleReconstruction():Tool(){}


bool SimpleReconstruction::Initialise(std::string configfile, DataModel &data){

  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  m_data= &data; //assigning transient data pointer

  m_variables.Get("verbosity",verbosity);

  return true;
}


bool SimpleReconstruction::Execute(){

  //Set default values for reconstruction variables
  this->SetDefaultValues();

  //Get ANNIEEvent variables which are necessary for reconstruction
  bool get_annie  = this->GetANNIEEventVariables();
  
  //Check which track id should be considered
  m_data->Stores.at("RecoEvent")->Get("PromptMuonTotalPE",max_pe); 

  std::vector<int> vector_clusterid;
  m_data->Stores["RecoEvent"]->Get("MRDCoincidenceCluster",vector_clusterid);
 
  bool reco_possible = this->RecoTankExitPoint(vector_clusterid); 

  //Only proceed with reconstructed if extrapolated MRD track passes the tank
  if (reco_possible){
    
    //Do simple energy reconstruction
    this->SimpleEnergyReconstruction();

    //Do simple vertex reconstruction
    this->SimpleVertexReconstruction();
  }

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

  //Fill Particles object with muon
  if (reco_possible){

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
    m_data->Stores["RecoEvent"]->Get("PromptMuonTime",muon_start_time);
    double muon_tracklength = muon_dir.Mag();
    double muon_stop_time = muon_start_time + muon_tracklength/3.E8*1.33;

    Particle muon(muon_pdg,muon_E_start,muon_E_stop,muon_vtx_start,muon_vtx_stop,muon_start_time,muon_stop_time,muon_start_dir,muon_tracklength,muon_tracktype);
    if (verbosity > 2){
      Log("SimpleReconstruction: Added muon with the following properties as a particle:",v_message,verbosity);
      muon.Print();
    }

    //Add muon particle to Particles collection
    std::vector<Particle> Particles;
    bool get_ok = m_data->Stores["ANNIEEvent"]->Get("Particles",Particles);
    if (!get_ok){
      Particles = {muon};
    } else {
      Particles.push_back(muon);
    }
    m_data->Stores["ANNIEEvent"]->Set("Particles",Particles);
  }

  return true;
}


bool SimpleReconstruction::Finalise(){

  return true;
}

void SimpleReconstruction::SetDefaultValues(){

  //Set Default values for reconstruction variables

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

  //Helper variables
  mrd_eloss = -9999;
  mrd_tracklength = -9999;
  dist_pmtvol_tank = -9999;
  max_pe = -9999;
  exitx = -9999;
  exity = -9999;
  exitz = -9999;
  dirx = -9999;
  diry = -9999;
  dirz = -9999;

  // Clear vectors
  fMRDTrackAngle.clear();
  fMRDTrackAngleError.clear();
  fMRDTrackLength.clear();
  fMRDPenetrationDepth.clear();
  fMRDEntryPointRadius.clear();
  fMRDEnergyLoss.clear();
  fMRDEnergyLossError.clear();
  fMRDTrackStartX.clear();
  fMRDTrackStartY.clear();
  fMRDTrackStartZ.clear();
  fMRDTrackStopX.clear();
  fMRDTrackStopY.clear();
  fMRDTrackStopZ.clear();
  fMRDStop.clear();
  fMRDSide.clear();
  fMRDThrough.clear();
  fMRDTrackEventID.clear();

}

bool SimpleReconstruction::GetANNIEEventVariables(){

  //Get MRD track reco variables + PMT information from BoostStores

  //MRD reco variables
  Position StartVertex;
  Position StopVertex;
  double TrackLength = -9999;
  double TrackAngle = -9999;
  double TrackAngleError = -9999;
  double PenetrationDepth = -9999;
  Position MrdEntryPoint;
  double EnergyLoss = -9999; //in MeV
  double EnergyLossError = -9999;
  double EntryPointRadius = -9999;
  bool IsMrdPenetrating;
  bool IsMrdStopped;
  bool IsMrdSideExit;
  int numtracksinev;
  int TrackEventID = -1; 
  std::vector<BoostStore>* theMrdTracks;

  bool get_annie = true;

  std::vector<std::vector<int>> MrdTimeClusters;
  get_annie = m_data->CStore.Get("MrdTimeClusters",MrdTimeClusters);
  if (!get_annie) {
    Log("SimpleReconstruction tool: No MrdTimeClusters object in CStore! Did you run TimeClustering beforehand?",v_warning,verbosity);
    return false;
  } else if (MrdTimeClusters.size()==0){
    Log("SimpleReconstruction tool: MrdTimeClusters object is empty! Don't proceed with reconstruction...",vv_debug,verbosity);
    return false;
  }

  m_data->Stores["MRDTracks"]->Get("MRDTracks",theMrdTracks);
  m_data->Stores["MRDTracks"]->Get("NumMrdTracks",numtracksinev);

  int NumClusterTracks = 0;

  for(int tracki=0; tracki<numtracksinev; tracki++){
    BoostStore* thisTrackAsBoostStore = &(theMrdTracks->at(tracki));

    thisTrackAsBoostStore->Get("StartVertex",StartVertex);
    thisTrackAsBoostStore->Get("StopVertex",StopVertex);
    thisTrackAsBoostStore->Get("TrackAngle",TrackAngle);
    thisTrackAsBoostStore->Get("TrackAngleError",TrackAngleError);
    thisTrackAsBoostStore->Get("PenetrationDepth",PenetrationDepth);
    thisTrackAsBoostStore->Get("MrdEntryPoint",MrdEntryPoint);
    thisTrackAsBoostStore->Get("EnergyLoss",EnergyLoss);
    thisTrackAsBoostStore->Get("EnergyLossError",EnergyLossError);
    thisTrackAsBoostStore->Get("IsMrdPenetrating",IsMrdPenetrating);        // bool
    thisTrackAsBoostStore->Get("IsMrdStopped",IsMrdStopped);                // bool
    thisTrackAsBoostStore->Get("IsMrdSideExit",IsMrdSideExit);
    thisTrackAsBoostStore->Get("MrdSubEventID",TrackEventID);
    TrackLength = sqrt(pow((StopVertex.X()-StartVertex.X()),2)+pow(StopVertex.Y()-StartVertex.Y(),2)+pow(StopVertex.Z()-StartVertex.Z(),2)) * 100.0;
    EntryPointRadius = sqrt(pow(MrdEntryPoint.X(),2) + pow(MrdEntryPoint.Y(),2)) * 100.0; // convert to cm
    PenetrationDepth = PenetrationDepth*100.0;

    fMRDTrackAngle.push_back(TrackAngle);
    fMRDTrackAngleError.push_back(TrackAngleError);
    fMRDTrackLength.push_back(TrackLength);
    fMRDPenetrationDepth.push_back(PenetrationDepth);
    fMRDEntryPointRadius.push_back(EntryPointRadius);
    fMRDEnergyLoss.push_back(EnergyLoss);
    fMRDEnergyLossError.push_back(EnergyLossError);
    fMRDTrackStartX.push_back(StartVertex.X());
    fMRDTrackStartY.push_back(StartVertex.Y());
    fMRDTrackStartZ.push_back(StartVertex.Z());
    fMRDTrackStopX.push_back(StopVertex.X());
    fMRDTrackStopY.push_back(StopVertex.Y());
    fMRDTrackStopZ.push_back(StopVertex.Z());
    fMRDStop.push_back(IsMrdStopped);
    fMRDSide.push_back(IsMrdSideExit);
    fMRDThrough.push_back(IsMrdPenetrating);
    fMRDTrackEventID.push_back(TrackEventID);
    NumClusterTracks+=1;
  }

  return true;

}

bool SimpleReconstruction::RecoTankExitPoint(std::vector<int> clusterid_vec){

  bool tank_exit = true;
  int trackid = -1;

  for (int i_clusterid=0; i_clusterid < (int)clusterid_vec.size(); i_clusterid++) {

    int clusterid = clusterid_vec.at(i_clusterid);

    for (int i = 0; i < (int) fMRDTrackEventID.size(); i++){
      if (fMRDTrackEventID.at(i) == clusterid) {
        Log("SimpleReconstruction:RecoTankExitPoint: Found corresponding track id >>> "+std::to_string(i)+"<<< for cluster id "+std::to_string(clusterid)+"! Proceeding with extracting MRD track information...",v_message,verbosity);
        trackid = i;
      }
    }
  }

  if (trackid == -1) {
    Log("SimpleReconstruction tool: Did not find corresponding track id for MRD clusterids. Abort reconstruction...",vv_debug,verbosity);
    return false;
  }

  //Check if track exited from the tank
  double startx = fMRDTrackStartX.at(trackid);
  double starty = fMRDTrackStartY.at(trackid);
  double startz = fMRDTrackStartZ.at(trackid);
  double stopx = fMRDTrackStopX.at(trackid);
  double stopy = fMRDTrackStopY.at(trackid);
  double stopz = fMRDTrackStopZ.at(trackid);

  SimpleRecoMRDStart.SetX(startx);
  SimpleRecoMRDStart.SetY(starty);
  SimpleRecoMRDStart.SetZ(startz);
  SimpleRecoMRDStop.SetX(stopx);
  SimpleRecoMRDStop.SetY(stopy);
  SimpleRecoMRDStop.SetZ(stopz);
  SimpleRecoTrackLengthInMRD = sqrt((stopx-startx)*(stopx-startx)+(stopy-starty)*(stopy-starty)+(stopz-startz)*(stopz-startz));


  //Calculate intersection point parameter t
  bool hit_tank = false;
  double diffx = stopx-startx;
  double diffy = stopy-starty;
  double diffz = stopz-startz;
  double startz_c = startz-1.681;
  double a = pow(diffx,2)+pow(diffz,2);
  double b = -2*diffx*startx-2*diffz*startz_c;
  double c = pow(startx,2)+pow(startz_c,2)-1.0*1.0;
  double t1 = (-b+sqrt(b*b-4*a*c))/(2*a);
  double t2 = (-b-sqrt(b*b-4*a*c))/(2*a);
  double t = 0;
  if (t1 < 0) t = t2;
  else if (t2 < 0) t = t1;
  else t = (t1 < t2)? t1 : t2;

  //Calculate exit point
  exitx = startx - t*diffx;
  exity = starty - t*diffy;
  exitz = startz - t*diffz;
  dirx = diffx/(sqrt(diffx*diffx+diffy*diffy+diffz*diffz));
  diry = diffy/(sqrt(diffx*diffx+diffy*diffy+diffz*diffz));
  dirz = diffz/(sqrt(diffx*diffx+diffy*diffy+diffz*diffz));
  SimpleRecoCosTheta = dirz;

  //Calculate different distances
  //dist_mrd: distance traveled in MRD
  //dist_air: distance traveled in air
                     
  double dist_mrd = sqrt(pow(startx-stopx,2)+pow(starty-stopy,2)+pow(startz-stopz,2));
  double dist_air = sqrt(pow(startx-exitx,2)+pow(starty-exity,2)+pow(startz-exitz,2));
  double dist_air_x = sqrt(pow(startx-exitx,2));
  double dist_air_y = sqrt(pow(starty-exity,2));
  double dist_air_z = sqrt(pow(startz-exitz,2));

  double ap = pow(diffx,2)+pow(diffz,2);
  double bp = -2*diffx*startx-2*diffz*startz_c;
  double cp = pow(startx,2)+pow(startz_c,2)-1.5*1.5;
  double t1p = (-bp+sqrt(bp*bp-4*ap*cp))/(2*ap);
  double t2p = (-bp-sqrt(bp*bp-4*ap*cp))/(2*ap);
  double tp = 0;
  if (t1p < 0) tp = t2p;
  else if (t2p < 0) tp = t1p;
  else tp = (t1p < t2p)? t1p : t2p;

  double exitxp = startx - tp*diffx;
  double exityp = starty - tp*diffy;
  double exitzp = startz - tp*diffz;

  double diff_exit_x = exitxp - exitx;
  double diff_exit_y = exityp - exity;
  double diff_exit_z = exitzp - exitz;

  //Cannot proceed with reconstruction for tracks which do not have a stopping track in the MRD -> set flag to false
  if (fMRDStop.at(trackid) == false){
    tank_exit = false;
    Log("SimpleReconstruction: Event does not have a stopping track in the MRD! Abort reconstruction",v_message,verbosity);
  }

  mrd_eloss = fMRDEnergyLoss.at(trackid);
  mrd_tracklength = fMRDTrackLength.at(trackid)/100;
  dist_pmtvol_tank = sqrt(pow(diff_exit_x,2)+pow(diff_exit_y,2)+pow(diff_exit_z,2));
 
  if (tank_exit == true){
    Log("SimpleReconstruction: Proceeding with reconstruction for event with stopping muon track in MRD. Eloss (MRD) = "+std::to_string(mrd_eloss)+" MeV, d (MRD) = "+std::to_string(mrd_tracklength)+" m, d (PMTVol - Tank) = "+std::to_string(dist_pmtvol_tank),v_message,verbosity);

    SimpleRecoStopVtx.SetX(stopx);
    SimpleRecoStopVtx.SetY(stopy);
    SimpleRecoStopVtx.SetZ(stopz);
    SimpleRecoFlag = 1;
  }

  return tank_exit;
}

bool SimpleReconstruction::SimpleEnergyReconstruction(){

  Log("SimpleReconstruction tool: SimpleEnergyReconstruction",v_message,verbosity);
  Log("SimpleReconstruction tool: Conditions: dist (pmtvol - tank) = "+std::to_string(dist_pmtvol_tank)+" m, MRD Eloss = "+std::to_string(mrd_eloss)+" MeV, Qmax = "+std::to_string(max_pe)+" p.e.",v_message,verbosity);

  SimpleRecoEnergy = 87.3 + 200*(dist_pmtvol_tank) + mrd_eloss + max_pe*0.08534;

  Log("SimpleEnergyReconstruction: Reconstructed muon energy: "+std::to_string(SimpleRecoEnergy),v_message,verbosity);

  SimpleRecoMrdEnergyLoss = mrd_eloss;

  //Also calculate transverse muon momentum
  double SimpleRecoTotalEnergy = SimpleRecoEnergy + 105.6;
  SimpleRecoPt = sqrt((1-SimpleRecoCosTheta*SimpleRecoCosTheta)*(SimpleRecoTotalEnergy*SimpleRecoTotalEnergy-105.6*105.6)); 

  return true;

}

bool SimpleReconstruction::SimpleVertexReconstruction(){

  Log("SimpleReconstruction tool: SimpleVertexReconstruction",v_message,verbosity);
  Log("Conditions: Exit Position = ("+std::to_string(exitx)+","+std::to_string(exity)+","+std::to_string(exitz)+"), Direction = ("+std::to_string(dirx)+","+std::to_string(diry)+","+std::to_string(dirz)+", Qmax = "+std::to_string(max_pe)+" p.e.",v_message,verbosity);

  double reco_vtx_x = exitx - max_pe/9/2./100.*dirx;
  double reco_vtx_y = exity - max_pe/9/2./100.*diry;
  double reco_vtx_z = exitz - max_pe/9/2./100.*dirz;
  SimpleRecoVtx.SetX(reco_vtx_x);
  SimpleRecoVtx.SetY(reco_vtx_y);
  SimpleRecoVtx.SetZ(reco_vtx_z);

  Log("SimpleEnergyReconstruction: Reconstructed interaction vertex: ("+std::to_string(SimpleRecoVtx.X())+","+std::to_string(SimpleRecoVtx.Y())+","+std::to_string(SimpleRecoVtx.Z())+")",v_message,verbosity);

  //Check if vertex is in Fiducial Volume
  if (sqrt(reco_vtx_x*reco_vtx_x+(reco_vtx_z-1.681)*(reco_vtx_z-1.681))<1.0 && fabs(reco_vtx_y)<0.5 && ((reco_vtx_z-1.681) < 0.)) SimpleRecoFV = true;

  return true;

}
