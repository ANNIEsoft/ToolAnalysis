#include "ExampleLoadRoot.h"

ExampleLoadRoot::ExampleLoadRoot():Tool(){}


bool ExampleLoadRoot::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////

  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer

  /////////////////////////////////////////////////////////////////

  m_variables.Get("verbose",verbose);
  m_variables.Get("InputFile",infile);

  file= new TFile(infile.c_str(),"READ");
  tree= (TTree*) file->Get("Data");
  Data= new ExampleRoot(tree);
  m_data->CStore.SetPtr("Data",Data,false);
  m_data->Stores["DataName"]=new BoostStore(false,2);

  currententry=1;

  return true;
}


bool ExampleLoadRoot::Execute(){

  std::cout<<"getentry="<<Data->GetEntry(currententry)<<std::endl;;
  std::cout<<"currententry="<<currententry<<std::endl;
  std::cout<<"a="<<Data->a<<std::endl;
  std::cout<<"b="<<Data->b<<std::endl;
  /// saving to store for the sake of printing
  m_data->Stores["DataName"]->Set("a",Data->a);
  m_data->Stores["DataName"]->Set("b",Data->b);

  currententry++;

  return true;
}


bool ExampleLoadRoot::Finalise(){



  return true;
}
