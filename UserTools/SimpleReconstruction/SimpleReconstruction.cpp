#include "SimpleReconstruction.h"

SimpleReconstruction::SimpleReconstruction():Tool(){}


bool SimpleReconstruction::Initialise(std::string configfile, DataModel &data){

  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  m_data= &data; //assigning transient data pointer

  m_data->vars.Get("verbosity",verbosity);

  return true;
}


bool SimpleReconstruction::Execute(){

  //Set default values for reconstruction variables
  this->SetDefaultValues();

  //Get ANNIEEvent variables which are necessary for reconstruction
  bool get_annie  = this->GetANNIEEventVariables();
  
  //Check which track id should be considered
 

 
  bool reco_possible = this->RecoTankExitPoint(trackid); 

  //Only proceed with reconstructed if extrapolated MRD track passes the tank
  if (reco_possible){
    
    //Do simple energy reconstruction
    this->SimpleEnergyReconstruction();

    //Do simple vertex reconstruction
    this->SimpleVertexReconstruction();
  }

  //Set variables in ANNIEEvent Store
  m_data->Stores["ANNIEEvent"]->Set("SimpleRecoFlag",SimpleRecoFlag);
  m_data->Stores["ANNIEEvent"]->Set("SimpleRecoEnergy",SimpleRecoEnergy);
  m_data->Stores["ANNIEEvent"]->Set("SimpleRecoVtx",SimpleRecoVtx);

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
  XXXX theMrdTracks;

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
    TrackLength = sqrt(pow((StopVertex.X()-StartVertex.X()),2)+pow(StopVertex.Y()-StartVertex.Y(),2)+pow(StopVertex.Z()-StartVertex.Z(),2)) * 100.0;
    EntryPointRadius = sqrt(pow(MrdEntryPoint.X(),2) + pow(MrdEntryPoint.Y(),2)) * 100.0; // convert to cm
    PenetrationDepth = PenetrationDepth*100.0;

    MRDTrackAngle.push_back(TrackAngle);
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
    NumClusterTracks+=1;
  }

  return reco_tank_exit;

}

bool SimpleReconstruction::RecoTankExitPoint(int trackid){

  bool tank_exit = false;

  //Check if track exited from the tank
  double startx = fMRDTrackStartX.at(trackid);
  double starty = fMRDTrackStartY.at(trackid);
  double startz = fMRDTrackStartZ.at(trackid);
  double stopx = fMRDTrackStopX.at(trackid);
  double stopy = fMRDTrackStopY.at(trackid);
  double stopz = fMRDTrackStopZ.at(trackid);

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
  double exitx = startx - t*diffx;
  double exity = starty - t*diffy;
  double exitz = startz - t*diffz;
  dirx = diffx/(sqrt(diffx*diffx+diffy*diffy+diffz*diffz));
  diry = diffy/(sqrt(diffx*diffx+diffy*diffy+diffz*diffz));
  dirz = diffz/(sqrt(diffx*diffx+diffy*diffy+diffz*diffz));
  cosTheta = dirz;

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

  mrd_eloss = fMRDEnergyLoss.at(trackid);
  mrd_tracklength = fMRDTrackLength.at(trackid);
  max_pe = ;
  dist_pmtvol_tank = ;
 
  return tank_exit;
}

bool SimpleReconstruction::SimpleEnergyReconstruction(){

  Log("SimpleReconstruction tool: SimpleEnergyReconstruction",v_message,verbosity);
  Log("SimpleReconstruction tool: Conditions: dist (pmtvol - tank) = "+std::to_string(dist_pmtvol_tank)+" m, MRD Eloss = "+std::to_string(mrd_eloss)+" MeV, Qmax = "+std::to_string(max_pe)+" p.e.",v_message,verbosity);

  SimpleRecoEnergy = 87.3 + 200*(dist_pmtvol_tank) + mrd_eloss + max_pe*0.08534;

  Log("SimpleEnergyReconstruction: Reconstructed muon energy: "+std::to_string(SimpleRecoEnergy),v_message,verbosity);

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

  return true;

}
