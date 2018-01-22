#include "ExampleSaveRoot.h"

ExampleSaveRoot::ExampleSaveRoot():Tool(){}


bool ExampleSaveRoot::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////

  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer

  /////////////////////////////////////////////////////////////////

  m_variables.Get("verbose",verbose);
  m_variables.Get("OutFile",outfile);

  file=new TFile(outfile.c_str(),"RECREATE");
  tree=new TTree("Data","Data");
  tree->Branch("a",&a,"a/I");
  tree->Branch("b",&b,"b/D");
  tree->Branch("c",&c);

  return true;
}


bool ExampleSaveRoot::Execute(){

  m_data->Stores["DataName"]->Get("name",c);
  m_data->Stores["DataName"]->Get("a",a);
  m_data->Stores["DataName"]->Get("b",b);

  tree->Fill();


  return true;
}


bool ExampleSaveRoot::Finalise(){

  tree->Write();
  file->Close();

  return true;
}
