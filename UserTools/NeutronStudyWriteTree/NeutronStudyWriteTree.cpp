#include "NeutronStudyWriteTree.h"

NeutronStudyWriteTree::NeutronStudyWriteTree():Tool(){}


bool NeutronStudyWriteTree::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  tf = new TFile ("savem.root","RECREATE");
  outtree = new TTree("ANNIEOutTree","ANNIEOutTree");

  outtree->Branch("nu_E",&nuE);
  outtree->Branch("mu_angle",&unsmearedMuangle);
  outtree->Branch("mu_E",&muE);
  outtree->Branch("mu_px",&mupx);
  outtree->Branch("mu_py",&mupy);
  outtree->Branch("mu_pz",&mupz);
  outtree->Branch("isPi",&ispi);
  outtree->Branch("pi_angle",&piAngle);
  outtree->Branch("pi_E",&piE);
  outtree->Branch("q2",&q2);
  outtree->Branch("Nprimaryneutrons",&primneut);
  outtree->Branch("Ntotneutrons",&totneut);
  outtree->Branch("mu_angle_smeared",&smearedMuangle);
  outtree->Branch("mu_E_smeared",&smearedMuE);
  outtree->Branch("myrecoE",&myRecoE);
  outtree->Branch("myrecoE_unsmeared",&myRecoE_unsmeared);
  outtree->Branch("recoE_orig",&origRecoE);
  outtree->Branch("goodmuon",&isgoodmuon);
  outtree->Branch("isPismeared",&isPismeared);
  outtree->Branch("Ntotneutsmeared",&Ntotneutsmeared);
  outtree->Branch("Nprimneutsmeared",&Nprimneutsmeared);
  outtree->Branch("Nbkgdneutrons",&Nbkgdneutrons);
  outtree->Branch("Nbkgdneutrons_high",&Nbkgdneutrons_high);
  outtree->Branch("PassedSelection",&passedselection);
  outtree->Branch("MuonEffic",&muonefficiency);

  return true;
}


bool NeutronStudyWriteTree::Execute(){

  m_data->Stores["NeutrinoEvent"]->Get("Nprimaryneutrons",primneut);
  m_data->Stores["NeutrinoEvent"]->Get("Ntotalneutrons",totneut);
  m_data->Stores["NeutrinoEvent"]->Get("isPion",ispi);
  m_data->Stores["NeutrinoEvent"]->Get("trueNeutrinoEnergy",nuE);
  m_data->Stores["NeutrinoEvent"]->Get("trueMuonEnergy",muE);
  m_data->Stores["NeutrinoEvent"]->Get("trueMuonAngle",muAngle);
  m_data->Stores["NeutrinoEvent"]->Get("trueMuonPx",mupx);
  m_data->Stores["NeutrinoEvent"]->Get("trueMuonPy",mupy);
  m_data->Stores["NeutrinoEvent"]->Get("trueMuonPz",mupz);
  m_data->Stores["NeutrinoEvent"]->Get("truePionEnergy",piE);
  m_data->Stores["NeutrinoEvent"]->Get("truePionAngle",piAngle);
  m_data->Stores["NeutrinoEvent"]->Get("trueQ2",q2);
  m_data->Stores["NeutrinoEvent"]->Get("originalrecoNeutrinoEnergy",origRecoE);
  m_data->Stores["NeutrinoEvent"]->Get("myrecoNeutrinoEnergy",myRecoE);
  m_data->Stores["NeutrinoEvent"]->Get("myrecoNuEunsmeared",myRecoE_unsmeared);
  m_data->Stores["NeutrinoEvent"]->Get("smearedMuonAngle",smearedMuangle);
  m_data->Stores["NeutrinoEvent"]->Get("smearedMuonEnergy",smearedMuE);
  m_data->Stores["NeutrinoEvent"]->Get("isGoodMuon",isgoodmuon);
  m_data->Stores["NeutrinoEvent"]->Get("smearedIsPion",isPismeared);
  m_data->Stores["NeutrinoEvent"]->Get("smearedNtotalNeutrons",Ntotneutsmeared);
  m_data->Stores["NeutrinoEvent"]->Get("smearedNprimaryNeutrons",Nprimneutsmeared);
  m_data->Stores["NeutrinoEvent"]->Get("NbackgroundNeutrons",Nbkgdneutrons);
  m_data->Stores["NeutrinoEvent"]->Get("NbackgroundNeutrons_high",Nbkgdneutrons_high);
  m_data->Stores["NeutrinoEvent"]->Get("PassedSelectionCuts",passedselection);
  m_data->Stores["NeutrinoEvent"]->Get("MuonEfficiency",muonefficiency);



  outtree->Fill();

  return true;
}


bool NeutronStudyWriteTree::Finalise(){

  outtree->Write();
  tf->Close();

  return true;
}
