#include "NeutronStudyReadSandbox.h"

NeutronStudyReadSandbox::NeutronStudyReadSandbox():Tool(){}


bool NeutronStudyReadSandbox::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_data->Stores["NeutrinoEvent"]= new BoostStore(false,2);


  primneut=0;
  totneut=0;
  ispi=0;

  nuE=0.;
  muE=0.;
  muAngle=0.;
  mupx=0.;
  mupy=0.;
  mupz=0.;
  piE=0.;
  piAngle=0.;
  q2=0.;
  recoE=0.;

  iterationNum=0;

  tf = new TFile("/ANNIEcode/neutronsout.root","READ");
  neutT = (TTree*) tf->Get("ANNIEToyEventTree");

  neutT->SetBranchAddress("nu_E",&nuE);
  neutT->SetBranchAddress("mu_E",&muE);
  neutT->SetBranchAddress("mu_angle",&muAngle);
  neutT->SetBranchAddress("isPi",&ispi);
  neutT->SetBranchAddress("pi_E",&piE);
  neutT->SetBranchAddress("pi_angle",&piAngle);
  neutT->SetBranchAddress("q2",&q2);
  neutT->SetBranchAddress("Nprimaryneutrons",&primneut);
  neutT->SetBranchAddress("Ntotneutrons",&totneut);
  neutT->SetBranchAddress("recoE",&recoE);
  neutT->SetBranchAddress("mu_px",&mupx);
  neutT->SetBranchAddress("mu_py",&mupy);
  neutT->SetBranchAddress("mu_pz",&mupz);

  nentries = neutT->GetEntries();

  return true;
}


bool NeutronStudyReadSandbox::Execute(){

  neutT->GetEntry(iterationNum);

  if(iterationNum%1000==0) std::cout<<"Now on iteration number: "<<iterationNum<<"  out of "<<nentries<<std::endl;

  m_data->Stores["NeutrinoEvent"]->Set("Nprimaryneutrons",primneut);
  m_data->Stores["NeutrinoEvent"]->Set("Ntotalneutrons",totneut);
  m_data->Stores["NeutrinoEvent"]->Set("isPion",ispi);
  m_data->Stores["NeutrinoEvent"]->Set("trueNeutrinoEnergy",nuE);
  m_data->Stores["NeutrinoEvent"]->Set("trueMuonEnergy",muE);
  m_data->Stores["NeutrinoEvent"]->Set("trueMuonAngle",muAngle);
  m_data->Stores["NeutrinoEvent"]->Set("trueMuonPx",mupx);
  m_data->Stores["NeutrinoEvent"]->Set("trueMuonPy",mupy);
  m_data->Stores["NeutrinoEvent"]->Set("trueMuonPz",mupz);
  m_data->Stores["NeutrinoEvent"]->Set("truePionEnergy",piE);
  m_data->Stores["NeutrinoEvent"]->Set("truePionAngle",piAngle);
  m_data->Stores["NeutrinoEvent"]->Set("trueQ2",q2);
  m_data->Stores["NeutrinoEvent"]->Set("recoNeutrinoEnergy",recoE);

  iterationNum++;

  return true;
}


bool NeutronStudyReadSandbox::Finalise(){

  return true;
}
