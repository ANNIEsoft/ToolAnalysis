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

  LAPPD data;
  m_data->Stores["ANNIEEvent"]->Get("LAPPDData",data);

  int tracesize = data.waveform.size();
  double max=0;
  int maxbin=0;
  for(int i=0; i<tracesize; i++){

    if(max<data.waveform.at(i)){
      max =data.waveform.at(i);
      maxbin=i;
    }

    FindPulseMax();

    std::cout<<"the max value is: "<<max<<" at sample #"<<maxbin<<std::endl;
    m_data->Stores["ANNIEEvent"]->Set("peakmax",max);

  }

  return true;
}


bool LAPPDFindPeak::Finalise(){

  return true;
}
