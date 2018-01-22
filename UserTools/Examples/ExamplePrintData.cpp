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

  //Print outs in ToolDAQ can be done either mnormally with cout and prinf or by using the inbuilt log
  //e.g.

  std::cout<<"a="<<a<<std::endl;
  printf("b=%f\n",b);
  logmessage<<"c="<<c;
  m_data->Log->Log(logmessage.str());
  logmessage.str("");

  return true;
}


bool ExamplePrintData::Finalise(){

  return true;
}
