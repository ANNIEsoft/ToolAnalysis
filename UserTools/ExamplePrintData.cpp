#include "ExamplePrintData.h"

ExamplePrintData::ExamplePrintData():Tool(){}


bool ExamplePrintData::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////

  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer

  /////////////////////////////////////////////////////////////////


  return true;
}


bool ExamplePrintData::Execute(){
    
  m_data->Stores["DataName"]->Get("a",a);
  m_data->Stores["DataName"]->Get("b",b);
  m_data->Stores["DataName"]->Get("c",c);

  std::cout<<"a="<<a<<std::endl;
  std::cout<<"b="<<b<<std::endl;
  std::cout<<"c="<<c<<std::endl;

  return true;
}


bool ExamplePrintData::Finalise(){

  return true;
}
