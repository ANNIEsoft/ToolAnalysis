#include "LAPPDParseScope.h"
#include <stdlib.h>
#include <TF1.h>

LAPPDParseScope::LAPPDParseScope():Tool(){}


bool LAPPDParseScope::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////


  return true;
}


bool LAPPDParseScope::Execute(){

  std::map<int,vector<Waveform<double>>> RawLAPPDData;

  //loop over N channels, populate


  m_data->Stores["ANNIEEvent"]->Set("RawLAPPDData",RawLAPPDData);

  return true;
}

bool LAPPDParseScope::Finalise(){

  return true;
}
