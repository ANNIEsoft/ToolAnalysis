#include "SimpleReconstruction.h"

SimpleReconstruction::SimpleReconstruction():Tool(){}


bool SimpleReconstruction::Initialise(std::string configfile, DataModel &data){

  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  m_data= &data; //assigning transient data pointer



  return true;
}


bool SimpleReconstruction::Execute(){

  //Set default values for reconstruction variables
  this->SetDefaultValues();

  //Get ANNIEEvent variables which are necessary for reconstruction
  bool reco_possible  = this->GetANNIEEventVariables();

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

  SimpleRecoFlag = -9999;
  SimpleRecoEnergy = -9999;
  SimpleRecoVtx.SetX(-9999);
  SimpleRecoVtx.SetY(-9999);
  SimpleRecoVtx.SetZ(-9999);

}

bool SimpleReconstruction::GetANNIEEventVariables(){

  double mrd_eloss = eloss->at(0);
  double mrd_tracklength = tracklength->at(0);
  
  bool reco_tank_exit = this->RecoTankExitPoint(); 
  return reco_tank_exit;

}

bool SimpleReconstruction::RecoTankExitPoint(){

  bool tank_exit = false;

double startx = mrdstartx->at(0);
                                double starty = mrdstarty->at(0);
                                double startz = mrdstartz->at(0);
                                double stopx = mrdstopx->at(0);
                                double stopy = mrdstopy->at(0);
                                double stopz = mrdstopz->at(0);
                                double diffx = stopx-startx;
                                double diffy = stopy-starty;
                                double diffz = stopz-startz;
                                double startz_c = startz-1.681;
                                bool hit_tank = false;
                                double a = pow(diffx,2)+pow(diffz,2);
                                double b = -2*diffx*startx-2*diffz*startz_c;
                                double c = pow(startx,2)+pow(startz_c,2)-1.0*1.0;
                                double t1 = (-b+sqrt(b*b-4*a*c))/(2*a);
                                double t2 = (-b-sqrt(b*b-4*a*c))/(2*a);
                                double t = 0;
                                if (t1 < 0) t = t2;
                                else if (t2 < 0) t = t1;
 else t = (t1 < t2)? t1 : t2;
                                double exitx = startx - t*diffx;
                                double exity = starty - t*diffy;
                                double exitz = startz - t*diffz;
                                double dirx = diffx/(sqrt(diffx*diffx+diffy*diffy+diffz*diffz));
                                double diry = diffy/(sqrt(diffx*diffx+diffy*diffy+diffz*diffz));
                                double dirz = diffz/(sqrt(diffx*diffx+diffy*diffy+diffz*diffz));
                                cosTheta = dirz;

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
                                std::cout <<"t1, t2: "<<t1p<<","<<t2p<<std::endl;
                                double tp = 0;
                                if (t1p < 0) tp = t2p;
                                else if (t2p < 0) tp = t1p;
                                else tp = (t1p < t2p)? t1p : t2p;
                                std::cout <<"t: "<<tp<<std::endl;

                                double exitxp = startx - tp*diffx;
                                double exityp = starty - tp*diffy;
                                double exitzp = startz - tp*diffz;

                                double diff_exit_x = exitxp - exitx;
                                double diff_exit_y = exityp - exity;
                                double diff_exit_z = exitzp - exitz;


  return tank_exit;
}

bool SimpleReconstruction::SimpleEnergyReconstruction(){

  SimpleRecoEnergy = 87.3 + 200*(dist_pmtvol_tank) + mrd_eloss + max_pe*0.08534;
  Log("SimpleEnergyReconstruction: Reconstructed muon energy: "+std::to_string(SimpleRecoEnergy),v_message,verbosity);
  return true;

}

bool SimpleReconstruction::SimpleVertexReconstruction(){

  double reco_vtx_x = exitx - max_pe/9/2./100.*dirx;
  double reco_vtx_y = exitx - max_pe/9/2./100.*dirx;
  double reco_vtx_z = exitx - max_pe/9/2./100.*dirx;
  SimpleRecoVtx.SetX(reco_vtx_x);
  SimpleRecoVtx.SetY(reco_vtx_y);
  SimpleRecoVtx.SetZ(reco_vtx_z);
  Log("SimpleEnergyReconstruction: Reconstructed interaction vertex: ("+std::to_string(SimpleRecoVtx.X())+","+std::to_string(SimpleRecoVtx.Y())+","+std::to_string(SimpleRecoVtx.Z())+")",v_message,verbosity);
  return true;

}
