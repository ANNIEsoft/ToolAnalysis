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

  // here I would open the input file

  // here I would also store relevant geometry information

  std::string test = "hello";
  m_data->Stores["ANNIEEvent"]= new BoostStore(false,2);
  m_data->Stores["ANNIEEvent"]->Header->Set("test",test);

  // initialize the ROOT random number generator
  myTR = new TRandom3();

  return true;
}


bool LAPPDParseScope::Execute(){

  //create an instance of the Waveform class;
  Waveform<double> mwav;

  // choose a random amplitude and "time" for the pulse
  // myTR is declared in the .h file and initialized in Initialise()
  double theamp = myTR->Gaus(50.,20.); // 50 mV mean, 20 mV sigma
  double thetime = myTR->Gaus(60.,0.5); // in the 60th sample, with RMS 0.5

  TF1* aGauss = new TF1("aGauss","gaus",0,256);
  aGauss->SetParameter(0,theamp); // amplitude of the pulse
  aGauss->SetParameter(1,thetime); // peak location (in samples)
  aGauss->SetParameter(2,10.); // width (sigma) of the pulse

  // loop over 256 "psec" samples and fill the Waveform
  for(int i=0; i<256; i++)
  {
    double noise = myTR->Gaus(0.,2.0); //add in random baseline noise (2 mV sig)

    double signal  = aGauss->Eval(i,0,0,0);
    double thevoltage = signal + noise;

    mwav.PushSample(thevoltage);

    //std::cout<<thevoltage<<std::endl;
  }

  int thesize = mwav.GetSamples()->size();
  //std::cout<<"the size here  "<<thesize<<std::endl;

  //put the fake LAPPD pulse into the ANNIEEvent Store, call it "LAPPDtrace"
  m_data->Stores["ANNIEEvent"]->Set("LAPPDtrace",mwav);

  return true;
}


bool LAPPDParseScope::Finalise(){

  return true;
}
