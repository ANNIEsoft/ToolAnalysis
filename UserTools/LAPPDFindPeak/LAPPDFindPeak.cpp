#include "LAPPDFindPeak.h"

LAPPDFindPeak::LAPPDFindPeak():Tool(){}


bool LAPPDFindPeak::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool LAPPDFindPeak::Execute(){

  //std::cout<<"in FindPeak"<<std::endl;
  Waveform<double> bwav;
  //m_data->Stores["ANNIEEvent"]->Print();

  bool testval =  m_data->Stores["ANNIEEvent"]->Get("LAPPDtrace",bwav);
  //std::cout<<"did i get from store "<<testval<<std::endl;
  //int tracesize = bwav.GetSamples()->size();
  //std::cout<<"trace size: "<<tracesize<<std::endl;

  double themax=0;
  int maxbin=0;

  double themin=55555.;
  int minbin = 0;

  // This function is defined in DataModel/ANNIEalgorithms.cpp
  FindPulseMax(bwav.GetSamples(),themax,maxbin,themin,minbin);


  std::cout<<"the max value is: "<<themax<<" at sample #"<<maxbin
  <<"   the min value is: "<<themin<<" at sample #"<<minbin<<std::endl;

  // add the measured peak values to the Store
  m_data->Stores["ANNIEEvent"]->Set("peakmax",themax);
  m_data->Stores["ANNIEEvent"]->Set("maxbin",maxbin);
  m_data->Stores["ANNIEEvent"]->Set("peakmin",themin);
  m_data->Stores["ANNIEEvent"]->Set("minbin",minbin);

  return true;
}


bool LAPPDFindPeak::Finalise(){

  return true;
}
