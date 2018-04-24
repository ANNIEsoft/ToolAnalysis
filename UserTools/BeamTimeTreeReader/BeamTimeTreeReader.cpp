#include "BeamTimeTreeReader.h"

BeamTimeTreeReader::BeamTimeTreeReader():Tool(){}


bool BeamTimeTreeReader::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_data->Stores["NeutrinoEvent"]= new BoostStore(false,2);


  m_variables.Get("infile", InFile);
  m_variables.Get("outfile", OutFile);
  m_variables.Get("TargetR", targetR);
  m_variables.Get("baseline", baseline);
  m_variables.Get("tc1", tc1);
  m_variables.Get("tc2", tc2);
  m_variables.Get("tc3", tc3);

  tf = new TFile(InFile,"READ");
  fChain = (TTree*) tf->Get("h201");

  cout<<"Tree, # entries: "<<fChain->GetEntries()<<endl;

  fChain->SetBranchAddress("beamwgt", &beamwgt, &b_beamwgt);
  fChain->SetBranchAddress("ntp", &ntp, &b_ntp);
  fChain->SetBranchAddress("npart", &npart, &b_npart);
  fChain->SetBranchAddress("id", id, &b_id);
  fChain->SetBranchAddress("ini_pos", ini_pos, &b_ini_pos);
  fChain->SetBranchAddress("ini_mom", ini_mom, &b_ini_mom);
  fChain->SetBranchAddress("ini_eng", ini_eng, &b_ini_eng);
  fChain->SetBranchAddress("ini_t", ini_t, &b_ini_t);
  fChain->SetBranchAddress("fin_mom", fin_mom, &b_fin_mom);
  fChain->SetBranchAddress("fin_pol", fin_pol, &b_fin_pol);
  fChain->SetBranchAddress("mul_weight", mul_weight, &b_mul_weight);


  ientry=0;

  return true;
}


bool BeamTimeTreeReader::Execute(){

  if(ientry%500==0) cout<<"READING TREE, entry="<<ientry<<endl;


  fChain->GetEntry(ientry);

  double xslope,yslope;
  vector<double> vtransit;
  double transit=-1;

  bool passescut=false;

  int nutype=ntp;
  int nuparent=id[1];
  int nparticles=npart;

  double nustartx = ini_pos[0][0];
  double nustarty = ini_pos[0][1];
  double nustartz = ini_pos[0][2];
  double nustartT = ini_t[0];

  double nupx = ini_mom[0][0];
  double nupy = ini_mom[0][1];
  double nupz = ini_mom[0][2];
  double nupperp = sqrt(nupx*nupx+nupy*nupy);
  double nuang = TMath::ATan(nupperp/nupz);

  double parentpx = ini_mom[1][0];
  double parentpy = ini_mom[1][1];
  double parentpz = ini_mom[1][2];
  double parentpperp = sqrt(parentpx*parentpx+parentpy*parentpy);
  double parentE = ini_eng[1];
  double parentang = TMath::ATan(parentpperp/parentpz);

  double nuE = ini_eng[0];

  double nuendx = ini_pos[0][0];
  double nuendy = ini_pos[0][1];
  double nuendz = ini_pos[0][2];

  double bwt = (double)beamwgt;

  m_data->Stores["NeutrinoEvent"]->Set("nutype",nutype);
  m_data->Stores["NeutrinoEvent"]->Set("nuparent",nuparent);
  m_data->Stores["NeutrinoEvent"]->Set("nparticles",nparticles);
  m_data->Stores["NeutrinoEvent"]->Set("nuE",nuE);
  m_data->Stores["NeutrinoEvent"]->Set("nupx",nupx);
  m_data->Stores["NeutrinoEvent"]->Set("nupy",nupy);
  m_data->Stores["NeutrinoEvent"]->Set("nupz",nupz);
  m_data->Stores["NeutrinoEvent"]->Set("nupperp",nupperp);
  m_data->Stores["NeutrinoEvent"]->Set("nuang",nuang);
  m_data->Stores["NeutrinoEvent"]->Set("nustartx",nustartx);
  m_data->Stores["NeutrinoEvent"]->Set("nustarty",nustarty);
  m_data->Stores["NeutrinoEvent"]->Set("nustartz",nustartz);
  m_data->Stores["NeutrinoEvent"]->Set("nustartT",nustartT);
  m_data->Stores["NeutrinoEvent"]->Set("parentE",parentE);
  m_data->Stores["NeutrinoEvent"]->Set("parentpx",parentpx);
  m_data->Stores["NeutrinoEvent"]->Set("parentpy",parentpy);
  m_data->Stores["NeutrinoEvent"]->Set("parentpz",parentpz);
  m_data->Stores["NeutrinoEvent"]->Set("parentpperp",parentpperp);
  m_data->Stores["NeutrinoEvent"]->Set("parentang",parentang);


  m_data->Stores["NeutrinoEvent"]->Set("beamwgt",bwt);

  ientry++;

  return true;
}


bool BeamTimeTreeReader::Finalise(){

  return true;
}
