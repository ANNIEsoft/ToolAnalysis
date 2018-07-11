#include "ExampleOverTool.h"

ExampleOverTool::ExampleOverTool():Tool(){}


bool ExampleOverTool::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

 std::string subtoolchainconfig;
  m_variables.Get("SubToolChainConfig",subtoolchainconfig);

  Sub=new ToolChain(subtoolchainconfig);
  Sub->m_data.Stores["DataName"]= new BoostStore(false,0);

  Sub->Initialise();



  return true;
}


bool ExampleOverTool::Execute(){

  if(true){ //what ever logic you want

    int a=3;
    double b=7.34;
    std::string c="hello";
    Sub->m_data.Stores["DataName"]->Set("a",a);
    Sub->m_data.Stores["DataName"]->Set("b",b);
    Sub->m_data.Stores["DataName"]->Set("c",c);

    Sub->Execute();
    
     Sub->m_data.Stores["DataName"]->Get("a",a);
    std::cout<<"value of a is "<<a<<std::endl;

  }

  return true;
}


bool ExampleOverTool::Finalise(){

  Sub->Finalise();
  delete Sub;
  Sub=0;

  return true;
}
