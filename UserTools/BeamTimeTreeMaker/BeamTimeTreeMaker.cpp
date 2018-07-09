#include "BeamTimeTreeMaker.h"

BeamTimeTreeMaker::BeamTimeTreeMaker():Tool(){}


bool BeamTimeTreeMaker::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  int rseed;

  m_variables.Get("outfile", OutFile);
  m_variables.Get("rseed", rseed);


  outtree = new TTree("ANNIEOutTree","ANNIEOutTree");

  outtree->Branch("nutype",&nutype);
  outtree->Branch("nuparent",&nuparent);
  outtree->Branch("nparticles",&nparticles);
  outtree->Branch("nuE",&nuE);
  outtree->Branch("nupx",&nupx);
  outtree->Branch("nupy",&nupy);
  outtree->Branch("nupz",&nupz);
  outtree->Branch("nupperp",&nupperp);
  outtree->Branch("nuang",&nuang);
  outtree->Branch("nustartx",&nustartx);
  outtree->Branch("nustarty",&nustarty);
  outtree->Branch("nustartz",&nustartz);
  outtree->Branch("nustartT",&nustartT);
  outtree->Branch("nuendx",&nuendx);
  outtree->Branch("nuendy",&nuendy);
  outtree->Branch("nuendz",&nuendz);
  outtree->Branch("nuendT",&nuendT);
  outtree->Branch("nuendTs1",&nuendTs1);
  outtree->Branch("nuendTs2",&nuendTs2);
  outtree->Branch("parentE",&parentE);
  outtree->Branch("parentpx",&parentpx);
  outtree->Branch("parentpy",&parentpy);
  outtree->Branch("parentpz",&parentpz);
  outtree->Branch("parentpperp",&parentpperp);
  outtree->Branch("parentang",&parentang);


  mrand1 = new TRandom3(rseed);
  mrand2 = new TRandom3(rseed+1);




  return true;
}


bool BeamTimeTreeMaker::Execute(){


  m_data->Stores["NeutrinoEvent"]->Get("nutype",nutype);
  m_data->Stores["NeutrinoEvent"]->Get("nuparent",nuparent);
  m_data->Stores["NeutrinoEvent"]->Get("nparticles",nparticles);
  m_data->Stores["NeutrinoEvent"]->Get("nuE",nuE);
  m_data->Stores["NeutrinoEvent"]->Get("nupx",nupx);
  m_data->Stores["NeutrinoEvent"]->Get("nupy",nupy);
  m_data->Stores["NeutrinoEvent"]->Get("nupz",nupz);
  m_data->Stores["NeutrinoEvent"]->Get("nupperp",nupperp);
  m_data->Stores["NeutrinoEvent"]->Get("nuang",nuang);
  m_data->Stores["NeutrinoEvent"]->Get("nustartx",nustartx);
  m_data->Stores["NeutrinoEvent"]->Get("nustarty",nustarty);
  m_data->Stores["NeutrinoEvent"]->Get("nustartz",nustartz);
  m_data->Stores["NeutrinoEvent"]->Get("nustartT",nustartT);
  m_data->Stores["NeutrinoEvent"]->Get("nuendx",nuendx);
  m_data->Stores["NeutrinoEvent"]->Get("nuendy",nuendy);
  m_data->Stores["NeutrinoEvent"]->Get("nuendz",nuendz);
  m_data->Stores["NeutrinoEvent"]->Get("nuendT",nuendT);
  m_data->Stores["NeutrinoEvent"]->Get("beamwgt",beamwgt);

  m_data->Stores["NeutrinoEvent"]->Get("parentE",parentE);
  m_data->Stores["NeutrinoEvent"]->Get("parentpx",parentpx);
  m_data->Stores["NeutrinoEvent"]->Get("parentpy",parentpy);
  m_data->Stores["NeutrinoEvent"]->Get("parentpz",parentpz);
  m_data->Stores["NeutrinoEvent"]->Get("parentpperp",parentpperp);
  m_data->Stores["NeutrinoEvent"]->Get("parentang",parentang);

  m_data->Stores["NeutrinoEvent"]->Get("passescut",passescut);

  double ts1 = mrand1->Gaus(0,1.2);
  double ts2 = mrand2->Gaus(0,1.8);

  nuendTs1 = nuendT + ts1;
  nuendTs2 = nuendT + ts2;

  if(passescut) outtree->Fill();



  return true;
}


bool BeamTimeTreeMaker::Finalise(){

  TFile* tout = new TFile(OutFile,"RECREATE");

  outtree->Write();
  tout->Close();

  return true;
}
