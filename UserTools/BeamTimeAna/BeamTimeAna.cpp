#include "BeamTimeAna.h"

BeamTimeAna::BeamTimeAna():Tool(){}


bool BeamTimeAna::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("infile", InFile);
  m_variables.Get("outfile", OutFile);
  m_variables.Get("TargetR", targetR);
  m_variables.Get("baseline", baseline);
  m_variables.Get("tc1", tc1);
  m_variables.Get("tc2", tc2);
  m_variables.Get("tc3", tc3);

  hbt = new TH1D("beamtime","beamtime",100,0.0,8.0);
  hbE0 = new TH1D("beamenergy0","beamenergy0",100.,0.,2.5);

  hbE_early = new TH1D("beamenergy_early","beamenergy_early",100.,0.,2.5);
  hbE_med = new TH1D("beamenergy_med","beamenergy_med",100.,0.,2.5);
  hbE_late = new TH1D("beamenergy_late","beamenergy_late",100.,0.,2.5);
  hbdvstimecorr = new TH2D("starttime","beamdist_vs_timecorr",100,0.,50.,100,0.,50.);
  hntp = new TH1D("neut_type","neut_type",4,-0.5,3.5);

  hbz0 = new TH1D("z0","z0",100,0.,5000.);
  hbbaseline = new TH1D("baseline","baseline",1000,-1.,15000.);

  ientry=0;

  return true;
}


bool BeamTimeAna::Execute(){

  double xslope,yslope;
  vector<double> vtransit;
  double transit=-1;

  bool passescut=false;

  int nutype,nuparent,nparticles;
  double nustartx,nustarty,nustartz,nustartT;
  double nupx,nupy,nupz,nuE;
  double beamwgt;

  double nuendx=0;
  double nuendy=0;
  double nuendz=0;
  double nuendT=0;

  m_data->Stores["NeutrinoEvent"]->Get("nutype",nutype);
  m_data->Stores["NeutrinoEvent"]->Get("nuparent",nuparent);
  m_data->Stores["NeutrinoEvent"]->Get("nparticles",nparticles);
  m_data->Stores["NeutrinoEvent"]->Get("nuE",nuE);
  m_data->Stores["NeutrinoEvent"]->Get("nupx",nupx);
  m_data->Stores["NeutrinoEvent"]->Get("nupy",nupy);
  m_data->Stores["NeutrinoEvent"]->Get("nupz",nupz);
  m_data->Stores["NeutrinoEvent"]->Get("nustartx",nustartx);
  m_data->Stores["NeutrinoEvent"]->Get("nustarty",nustarty);
  m_data->Stores["NeutrinoEvent"]->Get("nustartz",nustartz);
  m_data->Stores["NeutrinoEvent"]->Get("nustartT",nustartT);
  m_data->Stores["NeutrinoEvent"]->Get("beamwgt",beamwgt);

  xslope=0;
  yslope=0;


  if(nupz>0.){
    xslope = nupx/nupz;
    yslope = nupy/nupz;
    vtransit = Transit(nustartx, nustarty, nustartz, xslope, yslope, baseline, targetR);
  }

  double beamdist = sqrt(nustartx*nustartx + nustarty*nustarty + nustartz*nustartz);

  hbz0->Fill(nustartz);
  hntp->Fill(nutype);

  if(vtransit.size()>0){

    passescut=true;

    transit = vtransit.at(3);
    hbbaseline->Fill(transit);

    nuendT = nustartT + transit/30.;
    nuendx = nustartx + vtransit.at(0);
    nuendy = nustarty + vtransit.at(1);
    nuendz = nustartz + vtransit.at(2);

    double tshift = baseline/30.;
    double beamtime = nuendT-tshift;
    double timecorrection = beamdist/30.;

    hbt->Fill(beamtime,beamwgt);
    if(beamtime<tc1) hbE_early->Fill(nuE,beamwgt);
    if(beamtime>tc1&&beamtime<tc2) hbE_med->Fill(nuE,beamwgt);
    if(beamtime>tc2) hbE_late->Fill(nuE,beamwgt);
    hbdvstimecorr->Fill(nustartT,timecorrection,beamwgt);

  }

  m_data->Stores["NeutrinoEvent"]->Set("nuendx",nuendx);
  m_data->Stores["NeutrinoEvent"]->Set("nuendy",nuendy);
  m_data->Stores["NeutrinoEvent"]->Set("nuendz",nuendz);
  m_data->Stores["NeutrinoEvent"]->Set("nuendT",nuendT);
  m_data->Stores["NeutrinoEvent"]->Set("passescut",passescut);

  ientry++;

  return true;
}


vector<double> BeamTimeAna::Transit(double x0, double y0, double z0, double xslope, double yslope, double baseline, double radius){

  vector<double> vtransit;
  double transit;

  double deltaY = y0 + yslope*(baseline-z0);
  double deltaX = x0 + xslope*(baseline-z0);

  if(sqrt(deltaX*deltaX + deltaY*deltaY)<=radius){

    transit = sqrt( (baseline-z0)*(baseline-z0) + deltaY*deltaY + deltaX*deltaX );
    vtransit.push_back(deltaX);
    vtransit.push_back(deltaY);
    vtransit.push_back(baseline-z0);
    vtransit.push_back(transit);
  }


  return vtransit;
}


bool BeamTimeAna::Finalise(){

/*
  TFile* tout = new TFile(OutFile,"RECREATE");

  hntp->Write();
  hbt->Write();

  hbE0->Write();

  hbz0->Write();
  hbbaseline->Write();

  hbE_early->Write();
  hbE_med->Write();
  hbE_late->Write();
  hbdvstimecorr->Write();
*/

  return true;
}
