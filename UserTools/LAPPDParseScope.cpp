#include "LAPPDParseScope.h"
#include <stdlib.h>
#include <TF1.h>
#include <TRandom3.h>
LAPPDParseScope::LAPPDParseScope():Tool(){}


bool LAPPDParseScope::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  // here I would open the file

  std::string test = "hello";
  m_data->Stores["ANNIEEvent"]= new BoostStore(false,2);
  m_data->Stores["ANNIEEvent"]->Header->Set("test",test);

  return true;
}


bool LAPPDParseScope::Execute(){

  LAPPD data;

  TF1* aGauss = new TF1("aGauss","gaus",0,256);
  aGauss->SetParameter(0,50.);
  aGauss->SetParameter(1,60.);
  aGauss->SetParameter(2,10.);

  TRandom3* myTR = new TRandom3();
  double noise = 2.0*(myTR->Rndm()-0.5);


  for(int i=0; i<256; i++)
  {
    double signal  = aGauss->Eval(i,0,0,0);
    double thevoltage = signal + noise;
    data.waveform.push_back(thevoltage);

    //std::cout<<thevoltage<<std::endl;

    //double myrandnum = rand()%100;
    //data.waveform.push_back((double)myrandnum);
  }

  m_data->Stores["ANNIEEvent"]->Set("LAPPDData",data);

  return true;
}


bool LAPPDParseScope::Finalise(){

  return true;
}
